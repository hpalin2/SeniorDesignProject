#include <crow.h>
#include "repo.hpp"
#include "api.hpp"
#include "mqtt_ingestor.hpp"
#include <iostream>
#include <cstdlib>

int main() {
    std::cerr << ">>> ENTER MAIN <<<\n";

    Repo repo("suction_sense.db"); //Create the DB
    repo.seed_if_empty(); //init DB

    //Mqtt subscriber
    MqttIngestor ingestor(repo, "localhost", 1883, "suction/+/state", 1);
    if (!ingestor.start()) {
        CROW_LOG_ERROR << "MQTT ingestor failed to start";
    }

    crow::SimpleApp app;
    app.loglevel(crow::LogLevel::Debug);

    register_routes(app, repo);

    uint16_t port = 18080;
    if (const char* p = std::getenv("PORT")) {
        try { port = static_cast<uint16_t>(std::stoi(p)); }
        catch (...) { std::cerr << "[WARN] Bad PORT='" << p << "'; using 18080\n"; }
    }

    std::cerr << ">>> STARTING HTTP on http://127.0.0.1:" << port << " <<<\n";
    try {
        app.port(port).bindaddr("127.0.0.1").multithreaded().run();
    } catch (const std::exception& ex) {
        std::cerr << "[FATAL] Crow failed to start: " << ex.what() << "\n";
        return 1;
    }
    return 0;
}
