#ifndef WHOT_NETWORK_SESSION_MANAGER_HPP
#define WHOT_NETWORK_SESSION_MANAGER_HPP

#include <string>
#include <map>
#include <memory>
#include <chrono>
#include <mutex>
#include <vector>

namespace whot::network {

struct Session {
    std::string sessionId;
    std::string playerId;
    std::string gameId;
    std::chrono::steady_clock::time_point lastActivity;
    std::string ipAddress;
    std::map<std::string, std::string> metadata;
    bool authenticated;
};

class SessionManager {
public:
    SessionManager();
    
    // Session lifecycle
    std::string createSession(const std::string& ipAddress);
    void destroySession(const std::string& sessionId);
    bool sessionExists(const std::string& sessionId) const;
    
    // Session data
    Session* getSession(const std::string& sessionId);
    const Session* getSession(const std::string& sessionId) const;
    
    void setPlayerId(const std::string& sessionId, const std::string& playerId);
    void setGameId(const std::string& sessionId, const std::string& gameId);
    void updateActivity(const std::string& sessionId);
    
    // Queries
    std::vector<std::string> getSessionsForGame(const std::string& gameId) const;
    std::vector<std::string> getSessionsForPlayer(const std::string& playerId) const;
    std::string getSessionIdForPlayer(const std::string& playerId) const;
    
    // Cleanup
    void removeExpiredSessions(int timeoutSeconds);
    void removeAllSessionsForGame(const std::string& gameId);
    
    // Statistics
    size_t getActiveSessionCount() const;
    
private:
    mutable std::mutex mutex_;
    std::map<std::string, std::unique_ptr<Session>> sessions_;
    
    std::string generateSessionId() const;
};

} // namespace whot::network

#endif // WHOT_NETWORK_SESSION_MANAGER_HPP
