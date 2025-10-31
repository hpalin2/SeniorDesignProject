#include <crow.h>
#include "repo.hpp"
#include "api.hpp"
#include <iostream>
#include <cstdlib>

int main() {
    std::cerr << ">>> ENTER MAIN <<<\n";

    Repo repo("suction_sense.db");
    if (!repo.ok()) { std::cerr << "[FATAL] DB not OK\n"; return 1; }
    repo.seed_if_empty();
    // std::cerr << ">>> AFTER SEED <<<\n";

    crow::SimpleApp app;
    app.loglevel(crow::LogLevel::Debug);

    std::cerr << ">>> BEFORE register_routes <<<\n";
    register_routes(app, repo);
    std::cerr << ">>> AFTER register_routes <<<\n";

    uint16_t port = 18080;
    if (const char* p = std::getenv("PORT")) {
        try { port = static_cast<uint16_t>(std::stoi(p)); }
        catch (...) { std::cerr << "[WARN] Bad PORT='" << p << "'; using 18080\n"; }
    }

    std::cerr << ">>> STARTING HTTP on http://127.0.0.1:" << port << " <<<\n";
    try {
        app.port(port).bindaddr("127.0.0.1").multithreaded().run();
        std::cerr << ">>> run() returned (should not print) <<<\n";
    } catch (const std::exception& ex) {
        std::cerr << "[FATAL] Crow failed to start: " << ex.what() << "\n";
        return 1;
    }
    return 0;
}
