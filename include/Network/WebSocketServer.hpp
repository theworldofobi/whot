#ifndef WHOT_NETWORK_WEBSOCKET_SERVER_HPP
#define WHOT_NETWORK_WEBSOCKET_SERVER_HPP

#include "Network/MessageProtocol.hpp"
#include "Network/SessionManager.hpp"
#include <memory>
#include <functional>
#include <thread>
#include <atomic>

namespace whot::network {

using MessageHandler = std::function<void(const std::string& sessionId,
                                          const Message& message)>;

class WebSocketServer {
public:
    explicit WebSocketServer(uint16_t port);
    ~WebSocketServer();
    
    // Server lifecycle
    void start();
    void stop();
    bool isRunning() const;
    
    // Message handling
    void setMessageHandler(MessageHandler handler);
    void broadcastMessage(const Message& message);
    void sendMessage(const std::string& sessionId, const Message& message);
    void sendToGame(const std::string& gameId, const Message& message);
    
    // Session management
    SessionManager* getSessionManager();
    
    // Configuration
    void setMaxConnections(size_t max);
    void setHeartbeatInterval(int seconds);
    void setTimeout(int seconds);
    
    // Statistics
    size_t getActiveConnectionCount() const;
    size_t getTotalMessagesSent() const;
    size_t getTotalMessagesReceived() const;
    
private:
    uint16_t port_;
    std::atomic<bool> running_;

    std::unique_ptr<SessionManager> sessionManager_;
    MessageHandler messageHandler_;

    std::thread serverThread_;

    // Connection tracking
    size_t maxConnections_;
    int heartbeatInterval_;
    int timeout_;

    // Statistics
    std::atomic<size_t> messagesSent_;
    std::atomic<size_t> messagesReceived_;

    void handleConnection(const std::string& sessionId);
    void handleDisconnection(const std::string& sessionId);
    void handleIncomingMessage(const std::string& sessionId,
                               const std::string& data);

    void runHeartbeat();
    void checkTimeouts();

    struct WsServerImpl;
    std::unique_ptr<WsServerImpl> impl_;
};

} // namespace whot::network

#endif // WHOT_NETWORK_WEBSOCKET_SERVER_HPP
