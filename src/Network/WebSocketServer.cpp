#include "../../include/Network/WebSocketServer.hpp"
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <mutex>
#include <map>
#include <set>
#include <vector>
#include <string>

namespace whot::network {

// asio_no_tls.hpp defines struct websocketpp::config::asio (no-TLS variant)
using WsServer = websocketpp::server<websocketpp::config::asio>;
using connection_hdl = websocketpp::connection_hdl;

struct WebSocketServer::WsServerImpl {
    explicit WsServerImpl(WebSocketServer* owner)
        : owner_(owner)
    {
        server_.init_asio();
        server_.set_reuse_addr(true);
        // Handle non-WebSocket HTTP requests (e.g. OPTIONS preflight or wrong port):
        // respond with 204 for OPTIONS (CORS), 426 with Sec-WebSocket-Version for others.
        server_.set_http_handler([this](connection_hdl hdl) {
            on_http_request(hdl);
        });
        server_.set_validate_handler([this](connection_hdl) {
            return true; // accept all WebSocket handshakes that pass library checks
        });
        server_.set_open_handler([this](connection_hdl hdl) {
            on_open(hdl);
        });
        server_.set_close_handler([this](connection_hdl hdl) {
            on_close(hdl);
        });
        server_.set_message_handler([this](connection_hdl hdl, WsServer::message_ptr msg) {
            on_message(hdl, std::move(msg));
        });
        server_.set_fail_handler([this](connection_hdl hdl) {
            on_close(hdl);
        });
    }

    void on_http_request(connection_hdl hdl) {
        WsServer::connection_ptr con = server_.get_con_from_hdl(hdl);
        if (!con) return;
        std::string method = con->get_request().get_method();
        if (method == "OPTIONS") {
            con->set_status(websocketpp::http::status_code::no_content);
            con->set_body("");
            con->replace_header("Access-Control-Allow-Origin", "*");
            con->replace_header("Access-Control-Allow-Methods", "GET");
            con->replace_header("Access-Control-Allow-Headers", "Content-Type");
            return;
        }
        con->set_status(websocketpp::http::status_code::upgrade_required);
        con->replace_header("Sec-WebSocket-Version", "13");
        con->set_body("");
    }

    void run(uint16_t port) {
        try {
            server_.listen(port);
            server_.start_accept();
            server_.run();
        } catch (const std::exception& e) {
            (void)e;
        }
    }

    void stop() {
        try {
            server_.get_io_service().post([this]() {
                try {
                    server_.stop_listening();
                    std::vector<connection_hdl> to_close;
                    {
                        std::lock_guard<std::mutex> lock(mutex_);
                        for (const auto& [hdl, sid] : hdl_to_session_)
                            to_close.push_back(hdl);
                    }
                    for (const connection_hdl& hdl : to_close) {
                        try {
                            server_.close(hdl, websocketpp::close::status::going_away, "");
                        } catch (...) {}
                    }
                    server_.stop();
                } catch (...) {}
            });
        } catch (...) {}
    }

    void send(const std::string& sessionId, const std::string& data) {
        connection_hdl hdl;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = session_to_hdl_.find(sessionId);
            if (it == session_to_hdl_.end()) return;
            hdl = it->second;
        }
        try {
            server_.send(hdl, data, websocketpp::frame::opcode::text);
        } catch (...) {}
    }

    std::vector<std::string> getSessionIds() const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<std::string> out;
        for (const auto& p : session_to_hdl_) out.push_back(p.first);
        return out;
    }

private:
    void on_open(connection_hdl hdl) {
        std::string ip = "0.0.0.0";
        try {
            auto con = server_.get_con_from_hdl(hdl);
            if (con) {
                ip = con->get_remote_endpoint();
            }
        } catch (...) {}
        std::string sessionId;
        if (owner_ && owner_->sessionManager_)
            sessionId = owner_->sessionManager_->createSession(ip);
        if (sessionId.empty()) return;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            hdl_to_session_[hdl] = sessionId;
            session_to_hdl_[sessionId] = hdl;
        }
        if (owner_) owner_->handleConnection(sessionId);
    }

    void on_close(connection_hdl hdl) {
        std::string sessionId;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = hdl_to_session_.find(hdl);
            if (it != hdl_to_session_.end()) {
                sessionId = it->second;
                hdl_to_session_.erase(it);
                session_to_hdl_.erase(sessionId);
            }
        }
        if (!sessionId.empty()) {
            if (owner_) {
                owner_->handleDisconnection(sessionId);
                if (owner_->sessionManager_)
                    owner_->sessionManager_->destroySession(sessionId);
            }
        }
    }

    void on_message(connection_hdl hdl, WsServer::message_ptr msg) {
        if (!msg) return;
        std::string sessionId;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = hdl_to_session_.find(hdl);
            if (it != hdl_to_session_.end()) sessionId = it->second;
        }
        if (sessionId.empty()) return;
        if (owner_)
            owner_->handleIncomingMessage(sessionId, msg->get_payload());
    }

    WebSocketServer* owner_;
    WsServer server_;
    mutable std::mutex mutex_;
    std::map<connection_hdl, std::string, std::owner_less<connection_hdl>> hdl_to_session_;
    std::map<std::string, connection_hdl> session_to_hdl_;
};

WebSocketServer::WebSocketServer(uint16_t port)
    : port_(port)
    , running_(false)
    , sessionManager_(std::make_unique<SessionManager>())
    , maxConnections_(1000)
    , heartbeatInterval_(30)
    , timeout_(60)
    , messagesSent_(0)
    , messagesReceived_(0)
    , impl_(nullptr)
{}

WebSocketServer::~WebSocketServer() {
    stop();
    if (serverThread_.joinable()) serverThread_.join();
}

void WebSocketServer::start() {
    if (running_) return;
    running_ = true;
    impl_ = std::make_unique<WsServerImpl>(this);
    serverThread_ = std::thread([this]() {
        impl_->run(port_);
    });
}

void WebSocketServer::stop() {
    running_ = false;
    if (impl_) {
        impl_->stop();
    }
    if (serverThread_.joinable()) serverThread_.join();
    impl_.reset();
}

bool WebSocketServer::isRunning() const { return running_; }

void WebSocketServer::setMessageHandler(MessageHandler handler) { messageHandler_ = std::move(handler); }

void WebSocketServer::broadcastMessage(const Message& message) {
    if (!impl_) return;
    for (const auto& sid : impl_->getSessionIds())
        sendMessage(sid, message);
}

void WebSocketServer::sendMessage(const std::string& sessionId, const Message& message) {
    if (!impl_) return;
    impl_->send(sessionId, message.serialize());
    messagesSent_++;
}

void WebSocketServer::sendToGame(const std::string& gameId, const Message& message) {
    if (!sessionManager_) return;
    auto sessionIds = sessionManager_->getSessionsForGame(gameId);
    for (const auto& sid : sessionIds)
        sendMessage(sid, message);
}

SessionManager* WebSocketServer::getSessionManager() { return sessionManager_.get(); }
void WebSocketServer::setMaxConnections(size_t max) { maxConnections_ = max; }
void WebSocketServer::setHeartbeatInterval(int seconds) { heartbeatInterval_ = seconds; }
void WebSocketServer::setTimeout(int seconds) { timeout_ = seconds; }

size_t WebSocketServer::getActiveConnectionCount() const {
    return sessionManager_ ? sessionManager_->getActiveSessionCount() : 0;
}

size_t WebSocketServer::getTotalMessagesSent() const { return messagesSent_; }
size_t WebSocketServer::getTotalMessagesReceived() const { return messagesReceived_; }

void WebSocketServer::handleConnection(const std::string& sessionId) {
    (void)sessionId;
}

void WebSocketServer::handleDisconnection(const std::string& sessionId) {
    (void)sessionId;
}

void WebSocketServer::handleIncomingMessage(const std::string& sessionId,
                                            const std::string& data) {
    messagesReceived_++;
    if (messageHandler_) {
        Message msg = Message::deserialize(data);
        messageHandler_(sessionId, msg);
    }
}

void WebSocketServer::runHeartbeat() {}
void WebSocketServer::checkTimeouts() {}

} // namespace whot::network
