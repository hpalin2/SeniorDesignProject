#include <crow.h>
#include <sqlite3.h>

#include <chrono>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

// ───────────────────────────────────────────────
// Struct representing an operating room
// ───────────────────────────────────────────────
struct OperatingRoom {
    int id;
    std::string room_number;
    std::string procedure;
    std::string schedule;
    bool suction_on;
};

// ───────────────────────────────────────────────
// Utility: format timestamp as string
// ───────────────────────────────────────────────
std::string format_timestamp()
{
    const auto now = std::chrono::system_clock::now();
    const std::time_t tt = std::chrono::system_clock::to_time_t(now);
    std::tm local_tm{};
#ifdef _WIN32
    localtime_s(&local_tm, &tt);
#else
    localtime_r(&tt, &local_tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&local_tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

// ───────────────────────────────────────────────
// SQLite: initialize database and tables
// ───────────────────────────────────────────────
sqlite3* init_database(const std::string& db_name)
{
    sqlite3* db;
    if (sqlite3_open(db_name.c_str(), &db) != SQLITE_OK) {
        CROW_LOG_ERROR << "Cannot open database: " << sqlite3_errmsg(db);
        return nullptr;
    }

    const char* create_rooms_table = R"(
        CREATE TABLE IF NOT EXISTS operating_rooms (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            room_number TEXT NOT NULL,
            procedure TEXT,
            schedule TEXT
        );
    )";

    const char* create_history_table = R"(
        CREATE TABLE IF NOT EXISTS suction_history (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            room_id INTEGER,
            timestamp TEXT,
            suction_on INTEGER,
            FOREIGN KEY (room_id) REFERENCES operating_rooms(id)
        );
    )";

    char* errMsg = nullptr;
    if (sqlite3_exec(db, create_rooms_table, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        CROW_LOG_ERROR << "Failed to create operating_rooms: " << errMsg;
        sqlite3_free(errMsg);
    }
    if (sqlite3_exec(db, create_history_table, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        CROW_LOG_ERROR << "Failed to create suction_history: " << errMsg;
        sqlite3_free(errMsg);
    }

    CROW_LOG_INFO << "Database initialized successfully.";
    return db;
}

// ───────────────────────────────────────────────
// Insert a new operating room record
// ───────────────────────────────────────────────
void insert_room(sqlite3* db, const OperatingRoom& room)
{
    const char* sql = "INSERT INTO operating_rooms (room_number, procedure, schedule) VALUES (?, ?, ?)";
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, room.room_number.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, room.procedure.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, room.schedule.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

// ───────────────────────────────────────────────
// Log suction status change for a room
// ───────────────────────────────────────────────
void log_suction_status(sqlite3* db, int room_id, bool suction_on)
{
    const char* sql = "INSERT INTO suction_history (room_id, timestamp, suction_on) VALUES (?, ?, ?)";
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, room_id);
    sqlite3_bind_text(stmt, 2, format_timestamp().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, suction_on ? 1 : 0);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

// ───────────────────────────────────────────────
// Query: get most recent suction status for a room
// ───────────────────────────────────────────────
bool get_latest_suction_status(sqlite3* db, int room_id)
{
    const char* sql = "SELECT suction_on FROM suction_history WHERE room_id = ? ORDER BY id DESC LIMIT 1";
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, room_id);

    bool suction_on = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        suction_on = sqlite3_column_int(stmt, 0) != 0;
    }

    sqlite3_finalize(stmt);
    return suction_on;
}

// ───────────────────────────────────────────────
// Query: load all operating rooms
// ───────────────────────────────────────────────
std::vector<OperatingRoom> load_rooms(sqlite3* db)
{
    std::vector<OperatingRoom> rooms;
    const char* sql = "SELECT id, room_number, procedure, schedule FROM operating_rooms";
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        OperatingRoom room;
        room.id = sqlite3_column_int(stmt, 0);
        room.room_number = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        room.procedure = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        room.schedule = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        room.suction_on = get_latest_suction_status(db, room.id);
        rooms.push_back(room);
    }

    sqlite3_finalize(stmt);
    return rooms;
}

// ───────────────────────────────────────────────
// HTML rendering
// ───────────────────────────────────────────────
std::string render_dashboard(const std::vector<OperatingRoom>& rooms)
{
    static const std::string css = R"(body{margin:0;font-family:'Inter',system-ui,-apple-system,BlinkMacSystemFont,'Segoe UI',sans-serif;background:#0f172a;color:#e2e8f0;}a{color:#38bdf8;}main{max-width:1200px;margin:0 auto;padding:3rem 1.5rem;}header{text-align:center;margin-bottom:3rem;}header .logo{display:inline-flex;align-items:center;gap:0.75rem;margin-bottom:1rem;}header .logo-symbol{height:2.75rem;width:2.75rem;border-radius:0.9rem;background:#38bdf8;display:flex;align-items:center;justify-content:center;font-size:1.35rem;font-weight:700;color:#0f172a;}header .logo-text{font-size:1.5rem;font-weight:600;color:#e2e8f0;}h1{font-size:clamp(2.5rem,5vw,3.5rem);margin:0;color:#38bdf8;}p.subtitle{margin-top:0.5rem;color:#94a3b8;}section.cards{display:grid;grid-template-columns:repeat(auto-fit,minmax(260px,1fr));gap:1.5rem;}article.room-card{padding:1.5rem;border-radius:1.25rem;position:relative;overflow:hidden;box-shadow:0 15px 35px rgba(15,23,42,0.25);transition:transform 0.2s ease, box-shadow 0.2s ease;border:1px solid rgba(148,163,184,0.15);}article.room-card:hover{transform:translateY(-6px);box-shadow:0 20px 45px rgba(15,23,42,0.35);}article.room-card.room-card--ok{background:linear-gradient(135deg,rgba(22,163,74,0.95),rgba(21,128,61,0.9));color:#dcfce7;border-color:rgba(134,239,172,0.5);}article.room-card.room-card--warn{background:linear-gradient(135deg,rgba(234,179,8,0.95),rgba(202,138,4,0.9));color:#1f2937;border-color:rgba(234,179,8,0.55);}article.room-card .card-header{display:flex;align-items:center;justify-content:space-between;margin-bottom:0.5rem;}article.room-card h3{font-size:2rem;margin:0;font-weight:800;}article.room-card .meta{margin-top:0.25rem;font-weight:600;opacity:0.9;}article.room-card .time{font-size:0.85rem;opacity:0.8;}article.room-card .status{margin-top:1rem;font-size:1rem;font-weight:700;display:flex;align-items:center;gap:0.5rem;}article.room-card .status .icon{font-size:1.4rem;}article.room-card .status-icon{font-size:1.5rem;}footer{text-align:center;margin-top:3rem;color:#64748b;font-size:0.85rem;}footer span{font-weight:600;color:#38bdf8;}@media (prefers-color-scheme: light){body{background:#f8fafc;color:#0f172a;}article.room-card{box-shadow:0 10px 25px rgba(15,23,42,0.12);}})";

    std::ostringstream cards;
    for (const auto& room : rooms) {
        const bool ok = room.suction_on;
        // Add data-room-id for JavaScript to target each card
        cards << "<article class='room-card " << (ok ? "room-card--ok" : "room-card--warn")
              << "' data-room-id='" << room.id << "'>";
        cards << "<div class='card-header'>";
        cards << "<h3>" << room.room_number << "</h3>";
        if (!ok) {
            cards << "<div class='status-icon' aria-hidden='true'>⚠️</div>";
        }
        cards << "</div>";
        cards << "<p class='meta'>" << room.procedure << "</p>";
        cards << "<p class='time'>" << room.schedule << "</p>";
        cards << "<div class='status'>";
        cards << "<span class='icon'>" << (ok ? "🟢" : "🔴") << "</span>";
        cards << "Suction: " << (ok ? "ON" : "OFF");
        cards << "</div>";
        cards << "</article>";
    }

    std::ostringstream page;
    page << "<!DOCTYPE html><html lang='en'><head><meta charset='utf-8'/><meta name='viewport' content='width=device-width,initial-scale=1'/>";
    page << "<title>SuctionSense Dashboard</title>";
    page << "<link rel='preconnect' href='https://fonts.googleapis.com'><link rel='preconnect' href='https://fonts.gstatic.com' crossorigin>";
    page << "<link href='https://fonts.googleapis.com/css2?family=Inter:wght@400;500;600;700;800&display=swap' rel='stylesheet'>";
    page << "<style>" << css << "</style></head><body><main>";
    page << "<header><div class='logo'><div class='logo-symbol'>S</div><span class='logo-text'>SuctionSense</span></div>";
    page << "<h1>Operating Room Suction Status</h1><p class='subtitle'>Real-time monitoring dashboard</p></header>";
    page << "<section class='cards'>" << cards.str() << "</section>";

    // Footer and auto-refreshing script
    page << "<footer><p>Last updated: <span id='last-updated'>" << format_timestamp() << "</span></p></footer>";
    page << "</main>";

    // JavaScript for live updates
    page << R"(<script>
async function fetchData() {
    try {
        const res = await fetch('/api/rooms');
        const data = await res.json();
        const rooms = data.rooms;
        document.getElementById('last-updated').textContent = data.generatedAt;

        rooms.forEach(room => {
            const card = document.querySelector(`[data-room-id='${room.id}']`);
            if (!card) return;
            const status = card.querySelector('.status');
            const cardClass = room.suctionOn ? 'room-card--ok' : 'room-card--warn';
            card.classList.remove('room-card--ok', 'room-card--warn');
            card.classList.add(cardClass);
            status.innerHTML = `<span class='icon'>${room.suctionOn ? '🟢' : '🔴'}</span> Suction: ${room.suctionOn ? 'ON' : 'OFF'}`;
        });
    } catch (err) {
        console.error('Error updating data:', err);
    }
}
setInterval(fetchData, 5000);
</script>)";

    page << "</body></html>";
    return page.str();
}

// ───────────────────────────────────────────────
// JSON serialization for API
// ───────────────────────────────────────────────
crow::json::wvalue rooms_to_json(const std::vector<OperatingRoom>& rooms)
{
    crow::json::wvalue::list list;
    list.reserve(rooms.size());
    for (const auto& room : rooms) {
        crow::json::wvalue item;
        item["id"] = room.id;
        item["roomNumber"] = room.room_number;
        item["procedure"] = room.procedure;
        item["schedule"] = room.schedule;
        item["suctionOn"] = room.suction_on;
        list.push_back(std::move(item));
    }
    crow::json::wvalue payload;
    payload["rooms"] = std::move(list);
    payload["generatedAt"] = format_timestamp();
    return payload;
}

// ───────────────────────────────────────────────
// MAIN APPLICATION
// ───────────────────────────────────────────────
int main()
{
    sqlite3* db = init_database("suction_sense.db");
    if (!db) return 1;

    // Seed database if empty
    const char* count_sql = "SELECT COUNT(*) FROM operating_rooms";
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db, count_sql, -1, &stmt, nullptr);
    sqlite3_step(stmt);
    int count = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);

    if (count == 0) {
        std::vector<OperatingRoom> seed = {
            {0, "OR 1", "General Surgery", "08:00 - 10:30", true},
            {0, "OR 2", "Orthopedic", "07:30 - 11:00", false},
            {0, "OR 3", "Neurosurgery", "09:00 - 14:00", true},
            {0, "OR 4", "Cardiac Surgery", "08:30 - 12:00", true},
            {0, "OR 5", "ENT Procedure", "10:00 - 11:30", true},
            {0, "OR 6", "Plastic Surgery", "11:00 - 13:30", false},
        };
        for (auto& r : seed) {
            insert_room(db, r);
            // Log initial suction state
            int id = (int)sqlite3_last_insert_rowid(db);
            log_suction_status(db, id, r.suction_on);
        }
        CROW_LOG_INFO << "Seeded initial room data.";
    }

    crow::SimpleApp app;

    // Root route → HTML dashboard
    CROW_ROUTE(app, "/")([db] {
        auto rooms = load_rooms(db);
        crow::response res;
        res.code = crow::status::OK;
        res.set_header("Content-Type", "text/html; charset=UTF-8");
        res.body = render_dashboard(rooms);
        return res;
    });

    // JSON API for current room data
    CROW_ROUTE(app, "/api/rooms")([db] {
        auto rooms = load_rooms(db);
        auto json = rooms_to_json(rooms);
        crow::response res{json};
        res.set_header("Cache-Control", "no-store");
        return res;
    });

    // Update suction status (log event)
    CROW_ROUTE(app, "/api/rooms/<int>/suction/<int>")
    ([db](int id, int status) {
        bool suction_on = (status != 0);
        log_suction_status(db, id, suction_on);
        crow::json::wvalue res;
        res["success"] = true;
        res["roomId"] = id;
        res["suctionOn"] = suction_on;
        res["timestamp"] = format_timestamp();
        return res;
    });

    // Health check
    CROW_ROUTE(app, "/health")([] {
        return "ok";
    });

    const uint16_t port = static_cast<uint16_t>(
        std::getenv("PORT") ? std::stoi(std::getenv("PORT")) : 18080);
    CROW_LOG_INFO << "Starting SuctionSense dashboard on port " << port;

    app.port(port).multithreaded().run();

    sqlite3_close(db);
    return 0;
}
