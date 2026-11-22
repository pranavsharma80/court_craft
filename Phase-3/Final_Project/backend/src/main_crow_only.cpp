#include <crow.h>
#include <iostream>

int main()
{
    std::cerr << "Starting CourtCraft backend...\n";
    std::cerr.flush();

    crow::SimpleApp app;

    CROW_ROUTE(app, "/")
    ([] {
        return "CourtCraft API is running!";
    });

    CROW_ROUTE(app, "/api/health")
    ([] {
        crow::json::wvalue response;
        response["status"] = "ok";
        response["message"] = "CourtCraft backend is healthy";
        return response;
    });

    std::cerr << "Routes registered, starting server on port 8080...\n";
    std::cerr.flush();

    app.port(8080).multithreaded().run();

    return 0;
}
