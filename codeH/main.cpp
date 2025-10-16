#include <crow.h>

//https://crowcpp.org/master/getting_started/a_simple_webpage/ -> crow framework documentation

int main() {
    crow::SimpleApp app;

    CROW_ROUTE(app, "/")([]{
        auto page = crow::mustache::load_text("fancypage.html");
        return page;
    });

    CROW_ROUTE(app, "/hello/<string>")
    ([](const std::string& name){
        return "Hello, " + name + "!";
    });

    // Listen on http://localhost:18080
    app.port(18080).multithreaded().run();
}
