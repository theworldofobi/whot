#ifndef WHOT_NETWORK_HTTP_SERVER_HPP
#define WHOT_NETWORK_HTTP_SERVER_HPP

#include <string>
#include <map>
#include <functional>
#include <memory>
#include <vector>
#include <tuple>
#include <atomic>
#include <thread>

namespace whot::network {

enum class HttpMethod {
    GET,
    POST,
    PUT,
    DELETE,
    OPTIONS
};

struct HttpRequest {
    HttpMethod method;
    std::string path;
    std::map<std::string, std::string> headers;
    std::map<std::string, std::string> queryParams;
    std::map<std::string, std::string> pathParams;  // e.g. :id from /api/games/:id
    std::string body;
};

struct HttpResponse {
    int statusCode;
    std::map<std::string, std::string> headers;
    std::string body;
    
    static HttpResponse ok(const std::string& body);
    static HttpResponse created(const std::string& body);
    static HttpResponse badRequest(const std::string& message);
    static HttpResponse notFound(const std::string& message);
    static HttpResponse serverError(const std::string& message);
    static HttpResponse json(int statusCode, const std::string& jsonBody);
};

using RouteHandler = std::function<HttpResponse(const HttpRequest&)>;

class HttpServer {
public:
    explicit HttpServer(uint16_t port);
    ~HttpServer();
    
    // Server lifecycle
    void start();
    void stop();
    bool isRunning() const;
    
    // Route registration
    void addRoute(HttpMethod method, const std::string& path, RouteHandler handler);
    void addPatternRoute(HttpMethod method, const std::string& pattern, RouteHandler handler);
    void addStaticDirectory(const std::string& urlPath, const std::string& fsPath);
    void setStaticRoot(const std::string& fsPath);
    
    // Middleware
    void addCorsHeaders();
    void enableCompression();
    void setMaxBodySize(size_t bytes);
    
    // REST API helpers
    void setupGameApi();  // Sets up standard game API endpoints
    
private:
    uint16_t port_;
    std::atomic<bool> running_;
    size_t maxBodySize_;
    int listenFd_;
    std::thread serverThread_;

    std::map<std::string, std::map<HttpMethod, RouteHandler>> routes_;
    std::vector<std::tuple<std::string, HttpMethod, RouteHandler>> patternRoutes_;
    std::map<std::string, std::string> staticDirectories_;
    std::string staticRoot_;
    
    HttpResponse handleRequest(const HttpRequest& request);
    HttpResponse serveStaticFile(const std::string& urlPath);
    bool matchRoute(const std::string& pattern, const std::string& path,
                    std::map<std::string, std::string>& params);
};

} // namespace whot::network

#endif // WHOT_NETWORK_HTTP_SERVER_HPP
