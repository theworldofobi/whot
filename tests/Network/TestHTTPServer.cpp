#include <gtest/gtest.h>
#include "Network/HTTPServer.hpp"

namespace whot::network {

TEST(TestHTTPServer, HttpResponse_Ok) {
    HttpResponse r = HttpResponse::ok("body");
    EXPECT_EQ(r.statusCode, 200);
    EXPECT_EQ(r.body, "body");
}

TEST(TestHTTPServer, HttpResponse_Created) {
    HttpResponse r = HttpResponse::created("{\"id\":\"1\"}");
    EXPECT_EQ(r.statusCode, 201);
}

TEST(TestHTTPServer, HttpResponse_BadRequest) {
    HttpResponse r = HttpResponse::badRequest("Invalid");
    EXPECT_EQ(r.statusCode, 400);
    EXPECT_FALSE(r.body.empty());
}

TEST(TestHTTPServer, HttpResponse_NotFound) {
    HttpResponse r = HttpResponse::notFound("Not found");
    EXPECT_EQ(r.statusCode, 404);
}

TEST(TestHTTPServer, HttpResponse_ServerError) {
    HttpResponse r = HttpResponse::serverError("Error");
    EXPECT_EQ(r.statusCode, 500);
}

TEST(TestHTTPServer, HttpResponse_Json) {
    HttpResponse r = HttpResponse::json(200, "{}");
    EXPECT_EQ(r.statusCode, 200);
    EXPECT_EQ(r.body, "{}");
    EXPECT_EQ(r.headers["Content-Type"], "application/json");
}

TEST(TestHTTPServer, AddRoute_ThenStop) {
    HttpServer server(0);
    server.addRoute(HttpMethod::GET, "/health", [](const HttpRequest&) {
        return HttpResponse::ok("ok");
    });
    EXPECT_FALSE(server.isRunning());
    server.stop();
}

TEST(TestHTTPServer, SetStaticRoot) {
    HttpServer server(0);
    server.setStaticRoot("/tmp");
    server.stop();
}

} // namespace whot::network
