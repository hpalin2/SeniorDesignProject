#include <crow.h>

#include <chrono>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

struct OperatingRoom {
    int id;
    std::string room_number;
    std::string procedure;
    std::string schedule;
    bool suction_on;
};

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
    oss << std::put_time(&local_tm, "%I:%M:%S %p");
    return oss.str();
}

std::string render_dashboard(const std::vector<OperatingRoom>& rooms)
{
    static const std::string css = R"(body{margin:0;font-family:'Inter',system-ui,-apple-system,BlinkMacSystemFont,'Segoe UI',sans-serif;background:#0f172a;color:#e2e8f0;}a{color:#38bdf8;}main{max-width:1200px;margin:0 auto;padding:3rem 1.5rem;}header{text-align:center;margin-bottom:3rem;}header .logo{display:inline-flex;align-items:center;gap:0.75rem;margin-bottom:1rem;}header .logo-symbol{height:2.75rem;width:2.75rem;border-radius:0.9rem;background:#38bdf8;display:flex;align-items:center;justify-content:center;font-size:1.35rem;font-weight:700;color:#0f172a;}header .logo-text{font-size:1.5rem;font-weight:600;color:#e2e8f0;}h1{font-size:clamp(2.5rem,5vw,3.5rem);margin:0;color:#38bdf8;}p.subtitle{margin-top:0.5rem;color:#94a3b8;}section.cards{display:grid;grid-template-columns:repeat(auto-fit,minmax(260px,1fr));gap:1.5rem;}article.room-card{padding:1.5rem;border-radius:1.25rem;position:relative;overflow:hidden;box-shadow:0 15px 35px rgba(15,23,42,0.25);transition:transform 0.2s ease, box-shadow 0.2s ease;border:1px solid rgba(148,163,184,0.15);}article.room-card:hover{transform:translateY(-6px);box-shadow:0 20px 45px rgba(15,23,42,0.35);}article.room-card.room-card--ok{background:linear-gradient(135deg,rgba(22,163,74,0.95),rgba(21,128,61,0.9));color:#dcfce7;border-color:rgba(134,239,172,0.5);}article.room-card.room-card--warn{background:linear-gradient(135deg,rgba(234,179,8,0.95),rgba(202,138,4,0.9));color:#1f2937;border-color:rgba(234,179,8,0.55);}article.room-card .card-header{display:flex;align-items:center;justify-content:space-between;margin-bottom:0.5rem;}article.room-card h3{font-size:2rem;margin:0;font-weight:800;}article.room-card .meta{margin-top:0.25rem;font-weight:600;opacity:0.9;}article.room-card .time{font-size:0.85rem;opacity:0.8;}article.room-card .status{margin-top:1rem;font-size:1rem;font-weight:700;display:flex;align-items:center;gap:0.5rem;}article.room-card .status .icon{font-size:1.4rem;}article.room-card .status-icon{font-size:1.5rem;}footer{text-align:center;margin-top:3rem;color:#64748b;font-size:0.85rem;}footer span{font-weight:600;color:#38bdf8;}@media (prefers-color-scheme: light){body{background:#f8fafc;color:#0f172a;}article.room-card{box-shadow:0 10px 25px rgba(15,23,42,0.12);}})";

    std::ostringstream cards;
    for (const auto& room : rooms) {
        const bool ok = room.suction_on;
        cards << "<article class='room-card " << (ok ? "room-card--ok" : "room-card--warn") << "'>";
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
        cards << "Suction: " << (ok ? "ON" : "Off");
        cards << "</div>";
        cards << "</article>";
    }

    std::ostringstream page;
    page << "<!DOCTYPE html><html lang='en'>";
    page << "<head><meta charset='utf-8'/><meta name='viewport' content='width=device-width,initial-scale=1'/>";
    page << "<title>SuctionSense Dashboard</title>";
    page << "<link rel='preconnect' href='https://fonts.googleapis.com'><link rel='preconnect' href='https://fonts.gstatic.com' crossorigin>";
    page << "<link href='https://fonts.googleapis.com/css2?family=Inter:wght@400;500;600;700;800&display=swap' rel='stylesheet'>";
    page << "<style>" << css << "</style></head>";
    page << "<body><main>";
    page << "<header><div class='logo'><div class='logo-symbol'>S</div><span class='logo-text'>SuctionSense</span></div>";
    page << "<h1>Operating Room Suction Status</h1><p class='subtitle'>Real-time monitoring dashboard</p></header>";
    page << "<section class='cards'>" << cards.str() << "</section>";
    page << "<footer><p>Last updated: <span>" << format_timestamp() << "</span></p></footer>";
    page << "</main></body></html>";
    return page.str();
}

crow::json::wvalue rooms_to_json(const std::vector<OperatingRoom>& rooms)
{
    crow::json::wvalue::list list;
    list.reserve(rooms.size());
    for (const auto& room : rooms) {
        crow::json::wvalue item;
        item["id"] = room.id;
        item["roomNumber"] = room.room_number;
        item["procedure"] = room.procedure;
        item["time"] = room.schedule;
        item["suctionOn"] = room.suction_on;
        list.push_back(std::move(item));
    }

    crow::json::wvalue payload;
    payload["rooms"] = std::move(list);
    payload["generatedAt"] = format_timestamp();
    return payload;
}

std::string render_not_found()
{
    std::ostringstream html;
    html << "<!DOCTYPE html><html lang='en'><head><meta charset='utf-8'/><meta name='viewport' content='width=device-width,initial-scale=1'/>";
    html << "<title>404 Not Found</title>";
    html << "<style>body{margin:0;font-family:'Inter',system-ui,sans-serif;background:#f1f5f9;color:#0f172a;display:flex;align-items:center;justify-content:center;min-height:100vh;}main{text-align:center;padding:2rem;}h1{font-size:4rem;margin-bottom:0.5rem;color:#0ea5e9;}a{color:#2563eb;text-decoration:none;font-weight:600;}a:hover{text-decoration:underline;}</style></head>";
    html << "<body><main><h1>404</h1><p>Oops! Page not found.</p><a href='/'>Return to Home</a></main></body></html>";
    return html.str();
}

int main()
{
    std::vector<OperatingRoom> rooms = {
        {1, "OR 1", "General Surgery", "08:00 - 10:30", true},
        {2, "OR 2", "Orthopedic", "07:30 - 11:00", false},
        {3, "OR 3", "Neurosurgery", "09:00 - 14:00", true},
        {4, "OR 4", "Cardiac Surgery", "08:30 - 12:00", true},
        {5, "OR 5", "ENT Procedure", "10:00 - 11:30", true},
        {6, "OR 6", "Plastic Surgery", "11:00 - 13:30", false},
    };

    crow::SimpleApp app;

    CROW_ROUTE(app, "/")([&rooms] {
        crow::response res;
        res.code = crow::status::OK;
        res.set_header("Content-Type", "text/html; charset=UTF-8");
        res.body = render_dashboard(rooms);
        return res;
    });

    CROW_ROUTE(app, "/api/rooms")([&rooms] {
        auto json = rooms_to_json(rooms);
        crow::response res{json};
        res.set_header("Cache-Control", "no-store");
        return res;
    });

    CROW_ROUTE(app, "/health")([] {
        return "ok";
    });


    const uint16_t port = static_cast<uint16_t>(std::getenv("PORT") ? std::stoi(std::getenv("PORT")) : 18080);
    CROW_LOG_INFO << "Starting SuctionSense dashboard on port " << port;

    app.port(port).multithreaded().run();
    return 0;
}
