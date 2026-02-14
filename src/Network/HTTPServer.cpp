#include "../../include/Network/HTTPServer.hpp"
#include <thread>
#include <sstream>
#include <fstream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>

namespace whot::network {

HttpResponse HttpResponse::ok(const std::string& body) {
    HttpResponse r; r.statusCode = 200; r.body = body; return r;
}
HttpResponse HttpResponse::created(const std::string& body) {
    HttpResponse r; r.statusCode = 201; r.body = body; return r;
}
HttpResponse HttpResponse::badRequest(const std::string& message) {
    HttpResponse r; r.statusCode = 400; r.body = message; return r;
}
HttpResponse HttpResponse::notFound(const std::string& message) {
    HttpResponse r; r.statusCode = 404; r.body = message; return r;
}
HttpResponse HttpResponse::serverError(const std::string& message) {
    HttpResponse r; r.statusCode = 500; r.body = message; return r;
}
HttpResponse HttpResponse::json(int statusCode, const std::string& jsonBody) {
    HttpResponse r; r.statusCode = statusCode; r.body = jsonBody; r.headers["Content-Type"] = "application/json"; return r;
}

static HttpMethod parseMethod(const std::string& s) {
    if (s == "GET") return HttpMethod::GET;
    if (s == "POST") return HttpMethod::POST;
    if (s == "PUT") return HttpMethod::PUT;
    if (s == "DELETE") return HttpMethod::DELETE;
    if (s == "OPTIONS") return HttpMethod::OPTIONS;
    return HttpMethod::GET;
}

static std::string statusLine(int code) {
    if (code == 200) return "200 OK";
    if (code == 201) return "201 Created";
    if (code == 204) return "204 No Content";
    if (code == 400) return "400 Bad Request";
    if (code == 404) return "404 Not Found";
    if (code == 500) return "500 Internal Server Error";
    return "200 OK";
}

HttpServer::HttpServer(uint16_t port)
    : port_(port)
    , running_(false)
    , maxBodySize_(1024 * 1024)
    , listenFd_(-1)
    , serverThread_()
{}

HttpServer::~HttpServer() { stop(); }

void HttpServer::start() {
    if (running_) return;
    listenFd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (listenFd_ < 0) return;
    int opt = 1;
    setsockopt(listenFd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    struct sockaddr_in addr {};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port_);
    if (bind(listenFd_, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0) {
        close(listenFd_); listenFd_ = -1; return;
    }
    if (listen(listenFd_, 64) < 0) {
        close(listenFd_); listenFd_ = -1; return;
    }
    running_ = true;
    serverThread_ = std::thread([this]() {
        while (running_ && listenFd_ >= 0) {
            struct sockaddr_in clientAddr {};
            socklen_t len = sizeof(clientAddr);
            int fd = accept(listenFd_, reinterpret_cast<struct sockaddr*>(&clientAddr), &len);
            if (fd < 0) continue;
            std::thread([this, fd]() {
                char buf[8192];
                ssize_t n = recv(fd, buf, sizeof(buf) - 1, 0);
                if (n > 0) {
                    buf[n] = '\0';
                    std::istringstream iss(buf);
                    std::string methodStr, path, proto;
                    iss >> methodStr >> path >> proto;
                    HttpRequest req;
                    req.method = parseMethod(methodStr);
                    size_t q = path.find('?');
                    if (q != std::string::npos) {
                        req.path = path.substr(0, q);
                        std::string qs = path.substr(q + 1);
                        for (size_t i = 0; i < qs.size(); ) {
                            size_t amp = qs.find('&', i);
                            std::string pair = (amp == std::string::npos) ? qs.substr(i) : qs.substr(i, amp - i);
                            size_t eq = pair.find('=');
                            if (eq != std::string::npos)
                                req.queryParams[pair.substr(0, eq)] = pair.substr(eq + 1);
                            i = (amp == std::string::npos) ? qs.size() : amp + 1;
                        }
                    } else req.path = path;
                    std::string line;
                    while (std::getline(iss, line) && line != "\r" && line != "") {
                        size_t colon = line.find(':');
                        if (colon != std::string::npos)
                            req.headers[line.substr(0, colon)] = line.substr(colon + 1);
                    }
                    const char* bodyStart = std::strstr(buf, "\r\n\r\n");
                    if (bodyStart && bodyStart + 4 <= buf + n)
                        req.body.assign(bodyStart + 4, static_cast<size_t>((buf + n) - (bodyStart + 4)));
                    HttpResponse resp = handleRequest(req);
                    resp.headers["Access-Control-Allow-Origin"] = "*";
                    resp.headers["Access-Control-Allow-Methods"] = "GET, POST, PUT, DELETE, OPTIONS";
                    resp.headers["Access-Control-Allow-Headers"] = "Content-Type";
                    std::ostringstream oss;
                    oss << "HTTP/1.1 " << statusLine(resp.statusCode) << "\r\n";
                    for (const auto& [k, v] : resp.headers) oss << k << ": " << v << "\r\n";
                    if (resp.headers.find("Content-Length") == resp.headers.end())
                        oss << "Content-Length: " << resp.body.size() << "\r\n";
                    oss << "Connection: close\r\n\r\n" << resp.body;
                    std::string out = oss.str();
                    send(fd, out.data(), out.size(), 0);
                }
                close(fd);
            }).detach();
        }
    });
}

void HttpServer::stop() {
    running_ = false;
    if (listenFd_ >= 0) {
        close(listenFd_);
        listenFd_ = -1;
    }
    if (serverThread_.joinable()) serverThread_.join();
}

bool HttpServer::isRunning() const { return running_; }

void HttpServer::addRoute(HttpMethod method, const std::string& path, RouteHandler handler) {
    routes_[path][method] = std::move(handler);
}

void HttpServer::addPatternRoute(HttpMethod method, const std::string& pattern, RouteHandler handler) {
    patternRoutes_.emplace_back(pattern, method, std::move(handler));
}

void HttpServer::addStaticDirectory(const std::string& urlPath, const std::string& fsPath) {
    staticDirectories_[urlPath] = fsPath;
}

void HttpServer::setStaticRoot(const std::string& fsPath) {
    staticRoot_ = fsPath;
}

void HttpServer::addCorsHeaders() {}
void HttpServer::enableCompression() {}
void HttpServer::setMaxBodySize(size_t bytes) { maxBodySize_ = bytes; }

void HttpServer::setupGameApi() {
    addRoute(HttpMethod::GET, "/api/games", [](const HttpRequest&) {
        return HttpResponse::json(200, "[]");
    });
    addRoute(HttpMethod::POST, "/api/games", [](const HttpRequest&) {
        return HttpResponse::json(201, "{\"gameId\":\"\",\"message\":\"Use WebSocket to create game\"}");
    });
    addRoute(HttpMethod::GET, "/api/health", [](const HttpRequest&) {
        return HttpResponse::json(200, "{\"status\":\"ok\"}");
    });
}

HttpResponse HttpServer::handleRequest(const HttpRequest& request) {
    if (request.method == HttpMethod::OPTIONS) {
        HttpResponse preflight;
        preflight.statusCode = 204;
        preflight.headers["Access-Control-Allow-Origin"] = "*";
        preflight.headers["Access-Control-Allow-Methods"] = "GET, POST, PUT, DELETE, OPTIONS";
        preflight.headers["Access-Control-Allow-Headers"] = "Content-Type";
        preflight.headers["Access-Control-Max-Age"] = "86400";
        return preflight;
    }
    auto it = routes_.find(request.path);
    if (it != routes_.end()) {
        auto jt = it->second.find(request.method);
        if (jt != it->second.end())
            return jt->second(request);
    }
    for (const auto& [pattern, method, handler] : patternRoutes_) {
        if (method != request.method) continue;
        std::map<std::string, std::string> params;
        if (matchRoute(pattern, request.path, params)) {
            HttpRequest req = request;
            req.pathParams = std::move(params);
            return handler(req);
        }
    }
    if (request.method == HttpMethod::GET && !staticRoot_.empty())
        return serveStaticFile(request.path);
    return HttpResponse::notFound("Not Found: " + request.path);
}

HttpResponse HttpServer::serveStaticFile(const std::string& urlPath) {
    std::string path = urlPath;
    if (path.empty() || path == "/") path = "/index.html";
    if (path.front() == '/') path = path.substr(1);
    std::string fullPath = staticRoot_;
    if (fullPath.empty() || fullPath.back() != '/') fullPath += '/';
    fullPath += path;
    std::ifstream f(fullPath, std::ios::binary);
    if (!f) return HttpResponse::notFound("");
    std::string body((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    f.close();
    HttpResponse r;
    r.statusCode = 200;
    r.body = std::move(body);
    size_t dot = urlPath.rfind('.');
    if (dot != std::string::npos) {
        std::string ext = urlPath.substr(dot + 1);
        if (ext == "html") r.headers["Content-Type"] = "text/html; charset=utf-8";
        else if (ext == "css") r.headers["Content-Type"] = "text/css; charset=utf-8";
        else if (ext == "js") r.headers["Content-Type"] = "application/javascript; charset=utf-8";
        else if (ext == "json") r.headers["Content-Type"] = "application/json";
        else if (ext == "png") r.headers["Content-Type"] = "image/png";
        else if (ext == "ico") r.headers["Content-Type"] = "image/x-icon";
    } else {
        r.headers["Content-Type"] = "text/html; charset=utf-8";
    }
    return r;
}

bool HttpServer::matchRoute(const std::string& pattern, const std::string& path,
                            std::map<std::string, std::string>& params) {
    params.clear();
    size_t pi = 0, qi = 0;
    while (pi < pattern.size() && qi < path.size()) {
        if (pattern[pi] == ':') {
            size_t nameStart = pi + 1;
            size_t nameEnd = pattern.find('/', nameStart);
            if (nameEnd == std::string::npos) nameEnd = pattern.size();
            std::string name = pattern.substr(nameStart, nameEnd - nameStart);
            size_t valueEnd = path.find('/', qi);
            if (valueEnd == std::string::npos) valueEnd = path.size();
            params[name] = path.substr(qi, valueEnd - qi);
            pi = nameEnd;
            qi = valueEnd;
            if (qi < path.size() && path[qi] == '/') ++qi;
            continue;
        }
        if (pattern[pi] != path[qi]) return false;
        ++pi; ++qi;
    }
    if (pi != pattern.size() || qi != path.size()) return false;
    return true;
}

} // namespace whot::network
