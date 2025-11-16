#include "repo.hpp"
#include "util.hpp"
#include <crow.h>

namespace {
    void bind_text(sqlite3_stmt* s, int idx, const std::string& v) {
        sqlite3_bind_text(s, idx, v.c_str(), -1, SQLITE_TRANSIENT);
    }

    void bind_int(sqlite3_stmt* s, int idx, int v) {
        sqlite3_bind_int(s, idx, v);
    }

    std::string trim_ws(std::string s) {
        size_t a = s.find_first_not_of(" \t");
        if (a == std::string::npos) return {};
        size_t b = s.find_last_not_of(" \t");
        return s.substr(a, b - a + 1);
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

// Schema: only room_schedule, suction_state, suction_log
void Repo::init_schema() {
    // room_schedule is now the canonical room table (id, room_number, occupancy, last_changed)
    const char* create_room_schedule = R"(
        CREATE TABLE IF NOT EXISTS room_schedule (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            room_number TEXT UNIQUE NOT NULL,
            occupancy INTEGER NOT NULL DEFAULT 0,
            last_changed TEXT
        );
    )";

    const char* create_suction_state = R"(
        CREATE TABLE IF NOT EXISTS suction_state (
            room_id INTEGER PRIMARY KEY,
            suction_on INTEGER NOT NULL DEFAULT 0,
            last_updated TEXT,
            FOREIGN KEY (room_id) REFERENCES room_schedule(id) ON DELETE CASCADE
        );
    )";

    const char* create_suction_log = R"(
        CREATE TABLE IF NOT EXISTS suction_log (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            room_id INTEGER,
            timestamp TEXT,
            suction_on INTEGER,
            FOREIGN KEY (room_id) REFERENCES room_schedule(id) ON DELETE CASCADE
        );
    )";

    // create tables
    exec_ddl(create_room_schedule);
    exec_ddl(create_suction_state);
    exec_ddl(create_suction_log);
    CROW_LOG_INFO << "Database schema ready.";
}

// Initialize the DB with mock OR data
void Repo::seed_if_empty() {
    const char* count_sql = "SELECT COUNT(*) FROM room_schedule";
    sqlite3_stmt* s = nullptr;

    if (sqlite3_prepare_v2(db_, count_sql, -1, &s, nullptr) != SQLITE_OK)
        return;

    sqlite3_step(s);
    int count = sqlite3_column_int(s, 0);
    sqlite3_finalize(s);

    if (count != 0)
        return;

    struct SeedRoom {
        std::string number;
        bool occupancy;       // boolean now
        std::string schedule; // kept for informational purposes in original seed
        bool suction_on;
    };

    std::vector<SeedRoom> seed = {
        {"OR 1",   true,  "08:00 - 10:30", true},
        {"OR 2",   false, "07:30 - 11:00", false},
        {"OR 3",   false, "09:00 - 14:00", false},
        {"OR 4",   false, "08:30 - 12:00", false},
        {"OR 5",   false, "10:00 - 11:30", false},
        {"OR 6",   true,  "01:00 - 23:30", true},
        {"OR-DEV", false, "01:00 - 23:30", false}
    };

    for (auto& r : seed) {
        // Ensure room row exists (room_number only)
        insert_room(OperatingRoom{0, r.number, r.occupancy, "", r.suction_on});

        // Resolve room id from room_schedule
        int room_id = 0;
        sqlite3_stmt* t = nullptr;
        if (sqlite3_prepare_v2(db_, "SELECT id FROM room_schedule WHERE room_number = ? LIMIT 1",
                               -1, &t, nullptr) == SQLITE_OK) {
            bind_text(t, 1, r.number);
            if (sqlite3_step(t) == SQLITE_ROW) {
                room_id = sqlite3_column_int(t, 0);
            }
        }
        sqlite3_finalize(t);
        if (room_id <= 0) continue;

        // Update occupancy + last_changed timestamp on the room row
        const char* upd_sql = "UPDATE room_schedule SET occupancy = ?, last_changed = ? WHERE id = ?";
        sqlite3_stmt* ust = nullptr;
        if (sqlite3_prepare_v2(db_, upd_sql, -1, &ust, nullptr) == SQLITE_OK) {
            bind_int(ust, 1, r.occupancy ? 1 : 0);
            bind_text(ust, 2, format_timestamp());
            bind_int(ust, 3, room_id);
            sqlite3_step(ust);
        }
        sqlite3_finalize(ust);

        // Log initial suction state (also upserts suction_state)
        log_suction_status(room_id, r.suction_on);
    }

    CROW_LOG_INFO << "Seeded initial room data.";
}

std::vector<OperatingRoom> Repo::load_rooms() {
    std::vector<OperatingRoom> out;
    const char* sql = "SELECT id, room_number, occupancy, last_changed FROM room_schedule ORDER BY id";
    sqlite3_stmt* s = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &s, nullptr) == SQLITE_OK) {
        while (sqlite3_step(s) == SQLITE_ROW) {
            int id = sqlite3_column_int(s, 0);
            auto p = sqlite3_column_text(s, 1);
            std::string roomno = p ? reinterpret_cast<const char*>(p) : "";
            int occ = sqlite3_column_int(s, 2);
            auto t = sqlite3_column_text(s, 3);
            std::string last_changed = t ? reinterpret_cast<const char*>(t) : "";

            // Build the OperatingRoom: occupency is the stored occupancy,
            // schedule holds the last_changed timestamp for display,
            // suctionOn from latest suction state/log.
            bool suction = get_latest_suction_status(id);

            out.push_back(OperatingRoom{
                id,
                roomno,
                occ != 0,
                last_changed,
                suction
            });
        }
    }
    if (s) sqlite3_finalize(s);
    return out;
}

// Returns occupancy state + last_changed for a room
RoomEvent Repo::get_current_event_for_room(int room_id) {
    RoomEvent event{"Idle", "", "", false};

    const char* sql = R"(
        SELECT occupancy, last_changed
        FROM room_schedule
        WHERE id = ?
        LIMIT 1;
    )";

    sqlite3_stmt* s = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &s, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(s, 1, room_id);
        if (sqlite3_step(s) == SQLITE_ROW) {
            int occ = sqlite3_column_int(s, 0);
            auto p = sqlite3_column_text(s, 1);
            std::string last_changed = p ? reinterpret_cast<const char*>(p) : "";

            bool active = (occ != 0);
            std::string label = active ? "Occupied" : "Idle";
            event = {label, last_changed, "", active};
        }
    }
    if (s) sqlite3_finalize(s);
    return event;
}

// reads suction_state; if missing, falls back to the latest suction_log
bool Repo::get_latest_suction_status(int room_id) {
    const char* sql = "SELECT suction_on FROM suction_state WHERE room_id = ?";
    sqlite3_stmt* s = nullptr;
    bool result = false;
    bool has_row = false;

    if (sqlite3_prepare_v2(db_, sql, -1, &s, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(s, 1, room_id);
        if (sqlite3_step(s) == SQLITE_ROW) {
            result = sqlite3_column_int(s, 0) != 0;
            has_row = true;
        }
    }
    if (s) sqlite3_finalize(s);

    // Only fall back to log if there is no state row at all
    if (!has_row) {
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

// Reads existing state; if changed or missing, appends to suction_log with current timestamp.
// Updates suction_state with the new value and last_updated.
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
        prev = sqlite3_column_int(s, 0) != 0;
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
            suction_on = excluded.suction_on,
            last_updated = excluded.last_updated;
    )";
    sqlite3_prepare_v2(db_, upsert_sql, -1, &s, nullptr);
    sqlite3_bind_int(s, 1, room_id);
    sqlite3_bind_int(s, 2, suction_on ? 1 : 0);
    bind_text(s, 3, format_timestamp());
    sqlite3_step(s);
    sqlite3_finalize(s);
}

// Insert a new room into the DB (no schedule here anymore)
void Repo::insert_room(const OperatingRoom& r) {
    std::lock_guard<std::mutex> lk(mtx_);
    const char* sql = "INSERT OR IGNORE INTO room_schedule (room_number) VALUES (?)";
    sqlite3_stmt* s = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &s, nullptr) == SQLITE_OK) {
        bind_text(s, 1, r.room_number);
        sqlite3_step(s);
    }
    if (s) sqlite3_finalize(s);
}

void Repo::log_suction_status(int room_id, bool suction_on) {
    update_suction(room_id, suction_on); // already logs + upserts
}

// Resolve room id
int Repo::ensure_room_id(const std::string& room_number) {
    int room_id = 0;
    {
        std::lock_guard<std::mutex> lk(mtx_);
        // Create if missing
        const char* insert_sql = "INSERT OR IGNORE INTO room_schedule (room_number) VALUES (?)";
        sqlite3_stmt* s = nullptr;
        if (sqlite3_prepare_v2(db_, insert_sql, -1, &s, nullptr) == SQLITE_OK) {
            bind_text(s, 1, room_number);
            sqlite3_step(s);
        }
        if (s) sqlite3_finalize(s);

        // Fetch id
        const char* sel = "SELECT id FROM room_schedule WHERE room_number = ? LIMIT 1";
        s = nullptr;
        if (sqlite3_prepare_v2(db_, sel, -1, &s, nullptr) == SQLITE_OK) {
            bind_text(s, 1, room_number);
            if (sqlite3_step(s) == SQLITE_ROW) {
                room_id = sqlite3_column_int(s, 0);
            }
        }
        if (s) sqlite3_finalize(s);
    }
    return room_id;
}