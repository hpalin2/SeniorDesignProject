#include "repo.hpp"
#include "util.hpp"
#include <crow.h>

namespace {
    void bind_text(sqlite3_stmt* s, int idx, const std::string& v) {
        sqlite3_bind_text(s, idx, v.c_str(), -1, SQLITE_TRANSIENT);
    }
}

Repo::Repo(const std::string& db_path) {
    if (sqlite3_open(db_path.c_str(), &db_) != SQLITE_OK) {
        CROW_LOG_ERROR << "Cannot open database: " << sqlite3_errmsg(db_);
        if (db_) { sqlite3_close(db_); db_ = nullptr; }
        return;
    }
    // Pragmas for sane defaults
    char* err = nullptr;
    sqlite3_exec(db_, "PRAGMA foreign_keys = ON;", nullptr, nullptr, &err);
    if (err) { CROW_LOG_WARNING << "PRAGMA foreign_keys: " << err; sqlite3_free(err); }
    sqlite3_exec(db_, "PRAGMA journal_mode=WAL;", nullptr, nullptr, &err);
    if (err) { CROW_LOG_WARNING << "PRAGMA journal_mode: " << err; sqlite3_free(err); }
    sqlite3_exec(db_, "PRAGMA synchronous=NORMAL;", nullptr, nullptr, &err);
    if (err) { CROW_LOG_WARNING << "PRAGMA synchronous: " << err; sqlite3_free(err); }

    init_schema();
}

Repo::~Repo() {
    if (db_) sqlite3_close(db_);
}

void Repo::exec_ddl(const char* sql) {
    char* err = nullptr;
    if (sqlite3_exec(db_, sql, nullptr, nullptr, &err) != SQLITE_OK) {
        CROW_LOG_ERROR << "DDL failed: " << (err ? err : "(unknown)");
        if (err) sqlite3_free(err);
    }
}

void Repo::init_schema() {
    const char* create_rooms = R"(
        CREATE TABLE IF NOT EXISTS rooms (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            room_number TEXT UNIQUE NOT NULL
        );
    )";
    const char* create_room_schedule = R"(
        CREATE TABLE IF NOT EXISTS room_schedule (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            room_id INTEGER NOT NULL,
            procedure TEXT,
            start_time TEXT,
            end_time  TEXT,
            date      TEXT,
            FOREIGN KEY (room_id) REFERENCES rooms(id) ON DELETE CASCADE
        );
    )";
    const char* create_suction_state = R"(
        CREATE TABLE IF NOT EXISTS suction_state (
            room_id INTEGER PRIMARY KEY,
            suction_on INTEGER NOT NULL DEFAULT 0,
            last_updated TEXT,
            FOREIGN KEY (room_id) REFERENCES rooms(id) ON DELETE CASCADE
        );
    )";
    const char* create_suction_log = R"(
        CREATE TABLE IF NOT EXISTS suction_log (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            room_id INTEGER,
            timestamp TEXT,
            suction_on INTEGER,
            FOREIGN KEY (room_id) REFERENCES rooms(id) ON DELETE CASCADE
        );
    )";
    exec_ddl(create_rooms);
    exec_ddl(create_room_schedule);
    exec_ddl(create_suction_state);
    exec_ddl(create_suction_log);
    CROW_LOG_INFO << "Database schema ready.";
}

void Repo::seed_if_empty() {
    const char* count_sql = "SELECT COUNT(*) FROM rooms";
    sqlite3_stmt* s = nullptr;
    if (sqlite3_prepare_v2(db_, count_sql, -1, &s, nullptr) != SQLITE_OK) return;
    sqlite3_step(s);
    int count = sqlite3_column_int(s, 0);
    sqlite3_finalize(s);
    if (count != 0) return;

    std::vector<OperatingRoom> seed = {
        {0, "OR 1", "General Surgery", "08:00 - 10:30", true},
        {0, "OR 2", "Orthopedic",       "07:30 - 11:00", false},
        {0, "OR 3", "Neurosurgery",     "09:00 - 14:00", true},
        {0, "OR 4", "Cardiac Surgery",  "08:30 - 12:00", true},
        {0, "OR 5", "ENT Procedure",    "10:00 - 11:30", true},
        {0, "OR 6", "Plastic Surgery",  "11:00 - 13:30", false},
    };

    for (auto& r : seed) {
        insert_room(r);

        // Get assigned id
        sqlite3_stmt* t = nullptr;
        if (sqlite3_prepare_v2(db_, "SELECT id FROM rooms WHERE room_number = ? LIMIT 1", -1, &t, nullptr) == SQLITE_OK) {
            bind_text(t, 1, r.room_number);
            int id = 0;
            if (sqlite3_step(t) == SQLITE_ROW) id = sqlite3_column_int(t, 0);
            sqlite3_finalize(t);
            if (id > 0) log_suction_status(id, r.suction_on);
        }
    }
    CROW_LOG_INFO << "Seeded initial room data.";
}

std::vector<OperatingRoom> Repo::load_rooms() {
    std::lock_guard<std::mutex> lk(mtx_);
    std::vector<OperatingRoom> rooms;
    const char* sql = "SELECT id, room_number FROM rooms ORDER BY id";
    sqlite3_stmt* s = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &s, nullptr) != SQLITE_OK) return rooms;

    while (sqlite3_step(s) == SQLITE_ROW) {
        OperatingRoom room{};
        room.id = sqlite3_column_int(s, 0);
        auto txt = sqlite3_column_text(s, 1);
        room.room_number = txt ? reinterpret_cast<const char*>(txt) : "";

        RoomEvent current = get_current_event_for_room(room.id);
        if (current.active) {
            room.procedure = current.procedure;
            room.schedule  = current.start_time + " - " + current.end_time;
        } else {
            room.procedure = "Idle / Unscheduled";
            room.schedule  = "—";
        }
        room.suction_on = get_latest_suction_status(room.id);
        rooms.push_back(room);
    }
    sqlite3_finalize(s);
    return rooms;
}

RoomEvent Repo::get_current_event_for_room(int room_id) {
    RoomEvent event{"Idle", "", "", false};

    // current date/time
    const auto now = std::chrono::system_clock::now();
    const std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm local_tm{};
#if defined(_WIN32)
    localtime_s(&local_tm, &t);
#else
    localtime_r(&t, &local_tm);
#endif
    char date_buf[11];
    char time_buf[6];
    std::strftime(date_buf, sizeof(date_buf), "%Y-%m-%d", &local_tm);
    std::strftime(time_buf, sizeof(time_buf), "%H:%M", &local_tm);

    const char* sql = R"(
        SELECT procedure, start_time, end_time
        FROM room_schedule
        WHERE room_id = ? AND date = ?
        ORDER BY start_time;
    )";

    sqlite3_stmt* s = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &s, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(s, 1, room_id);
        bind_text(s, 2, date_buf);

        while (sqlite3_step(s) == SQLITE_ROW) {
            auto p0 = sqlite3_column_text(s, 0);
            auto p1 = sqlite3_column_text(s, 1);
            auto p2 = sqlite3_column_text(s, 2);
            std::string proc  = p0 ? reinterpret_cast<const char*>(p0) : "";
            std::string start = p1 ? reinterpret_cast<const char*>(p1) : "";
            std::string end   = p2 ? reinterpret_cast<const char*>(p2) : "";

            if (!start.empty() && !end.empty()
                && std::string(time_buf) >= start
                && std::string(time_buf) <= end) {
                event = {proc, start, end, true};
                break;
            }
        }
    }
    if (s) sqlite3_finalize(s);
    return event;
}

bool Repo::get_latest_suction_status(int room_id) {
    const char* sql = "SELECT suction_on FROM suction_state WHERE room_id = ?";
    sqlite3_stmt* s = nullptr;
    bool result = false;
    if (sqlite3_prepare_v2(db_, sql, -1, &s, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(s, 1, room_id);
        if (sqlite3_step(s) == SQLITE_ROW) {
            result = sqlite3_column_int(s, 0) != 0;
        }
    }
    if (s) sqlite3_finalize(s);

    if (!result) {
        const char* log_sql =
            "SELECT suction_on FROM suction_log WHERE room_id = ? ORDER BY id DESC LIMIT 1";
        if (sqlite3_prepare_v2(db_, log_sql, -1, &s, nullptr) == SQLITE_OK) {
            sqlite3_bind_int(s, 1, room_id);
            if (sqlite3_step(s) == SQLITE_ROW) {
                result = sqlite3_column_int(s, 0) != 0;
            }
        }
        if (s) sqlite3_finalize(s);
    }
    return result;
}

void Repo::update_suction(int room_id, bool suction_on) {
    std::lock_guard<std::mutex> lk(mtx_);

    // Read current
    const char* select_sql = "SELECT suction_on FROM suction_state WHERE room_id = ?";
    sqlite3_stmt* s = nullptr;
    sqlite3_prepare_v2(db_, select_sql, -1, &s, nullptr);
    sqlite3_bind_int(s, 1, room_id);
    bool prev = false;
    bool exists = false;
    if (sqlite3_step(s) == SQLITE_ROW) {
        prev = sqlite3_column_int(s, 0);
        exists = true;
    }
    sqlite3_finalize(s);

    if (!exists || prev != suction_on) {
        const char* log_sql =
            "INSERT INTO suction_log (room_id, timestamp, suction_on) VALUES (?, ?, ?)";
        sqlite3_prepare_v2(db_, log_sql, -1, &s, nullptr);
        sqlite3_bind_int(s, 1, room_id);
        bind_text(s, 2, format_timestamp());
        sqlite3_bind_int(s, 3, suction_on ? 1 : 0);
        sqlite3_step(s);
        sqlite3_finalize(s);
    }

    const char* upsert_sql = R"(
        INSERT INTO suction_state (room_id, suction_on, last_updated)
        VALUES (?, ?, ?)
        ON CONFLICT(room_id) DO UPDATE SET
            suction_on=excluded.suction_on,
            last_updated=excluded.last_updated;
    )";
    sqlite3_prepare_v2(db_, upsert_sql, -1, &s, nullptr);
    sqlite3_bind_int(s, 1, room_id);
    sqlite3_bind_int(s, 2, suction_on ? 1 : 0);
    bind_text(s, 3, format_timestamp());
    sqlite3_step(s);
    sqlite3_finalize(s);
}

void Repo::insert_room(const OperatingRoom& r) {
    // Insert (ignore if exists)
    {
        std::lock_guard<std::mutex> lk(mtx_);
        const char* sql = "INSERT OR IGNORE INTO rooms (room_number) VALUES (?)";
        sqlite3_stmt* s = nullptr;
        if (sqlite3_prepare_v2(db_, sql, -1, &s, nullptr) == SQLITE_OK) {
            bind_text(s, 1, r.room_number);
            sqlite3_step(s);
        }
        if (s) sqlite3_finalize(s);
    }

    // Get room id
    int room_id = 0;
    {
        std::lock_guard<std::mutex> lk(mtx_);
        const char* sel = "SELECT id FROM rooms WHERE room_number = ? LIMIT 1";
        sqlite3_stmt* s = nullptr;
        if (sqlite3_prepare_v2(db_, sel, -1, &s, nullptr) == SQLITE_OK) {
            bind_text(s, 1, r.room_number);
            if (sqlite3_step(s) == SQLITE_ROW) room_id = sqlite3_column_int(s, 0);
        }
        if (s) sqlite3_finalize(s);
    }
    if (room_id <= 0) return;

    // If schedule is provided in "HH:MM - HH:MM", insert today's entry
    std::string start, end;
    if (!r.schedule.empty()) {
        size_t dash = r.schedule.find('-');
        if (dash != std::string::npos) {
            auto trim = [](std::string s) {
                size_t a = s.find_first_not_of(" \t");
                size_t b = s.find_last_not_of(" \t");
                if (a == std::string::npos) return std::string();
                return s.substr(a, b - a + 1);
            };
            start = trim(r.schedule.substr(0, dash));
            end   = trim(r.schedule.substr(dash + 1));
        }
    }

    // today's date
    const auto now = std::chrono::system_clock::now();
    const std::time_t tt = std::chrono::system_clock::to_time_t(now);
    std::tm local_tm{};
#if defined(_WIN32)
    localtime_s(&local_tm, &tt);
#else
    localtime_r(&tt, &local_tm);
#endif
    char date_buf[11];
    std::strftime(date_buf, sizeof(date_buf), "%Y-%m-%d", &local_tm);

    std::lock_guard<std::mutex> lk(mtx_);
    const char* insert_schedule_sql = R"(
        INSERT INTO room_schedule (room_id, procedure, start_time, end_time, date)
        VALUES (?, ?, ?, ?, ?)
    )";
    sqlite3_stmt* s = nullptr;
    if (sqlite3_prepare_v2(db_, insert_schedule_sql, -1, &s, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(s, 1, room_id);
        bind_text(s, 2, r.procedure);
        if (!start.empty()) bind_text(s, 3, start); else sqlite3_bind_null(s, 3);
        if (!end.empty())   bind_text(s, 4, end);   else sqlite3_bind_null(s, 4);
        bind_text(s, 5, date_buf);
        sqlite3_step(s);
    }
    if (s) sqlite3_finalize(s);
}

void Repo::log_suction_status(int room_id, bool suction_on) {
    update_suction(room_id, suction_on); // already logs + upserts
}
