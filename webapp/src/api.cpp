#include "api.hpp"
#include "views.hpp"
#include "util.hpp"
#include <crow.h>

static crow::json::wvalue rooms_to_json(const std::vector<OperatingRoom>& rooms) {
    crow::json::wvalue::list list;
    list.reserve(rooms.size());
    for (const auto& room : rooms) {
        crow::json::wvalue item;
        item["id"]         = room.id;
        item["roomNumber"] = room.room_number;
        item["occupancy"]  = room.occupancy;
        item["schedule"]   = room.schedule;
        item["suctionOn"]  = room.suction_on;
        list.push_back(std::move(item));
    }
    crow::json::wvalue payload;
    payload["rooms"] = std::move(list);
    payload["generatedAt"] = format_timestamp();
    return payload;
}

void register_routes(crow::SimpleApp& app, Repo& repo) {
    // HTML dashboard
    CROW_ROUTE(app, "/")([&repo]{
        auto rooms = repo.load_rooms();
        crow::response res;
        res.code = crow::status::OK;
        res.set_header("Content-Type", "text/html; charset=UTF-8");
        res.body = render_dashboard(rooms);
        return res;
    });

    // JSON API for current room data
    CROW_ROUTE(app, "/api/rooms")([&repo]{
        auto rooms = repo.load_rooms();
        auto json = rooms_to_json(rooms);
        crow::response res{json};
        res.set_header("Cache-Control", "no-store");
        return res;
    });

    // Update suction status (log event)
    CROW_ROUTE(app, "/api/rooms/<int>/suction/<int>")
    ([&repo](int id, int status){
        bool suction_on = (status != 0);
        repo.update_suction(id, suction_on);
        crow::json::wvalue res;
        res["success"]   = true;
        res["roomId"]    = id;
        res["suctionOn"] = suction_on;
        res["timestamp"] = format_timestamp();
        return res;
    });

    // Health check
    CROW_ROUTE(app, "/health")([]{ return "ok"; });
}
