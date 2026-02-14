#include "../../include/Network/SessionManager.hpp"
#include "../../include/Utils/Random.hpp"
#include <algorithm>

namespace whot::network {

SessionManager::SessionManager() = default;

std::string SessionManager::generateSessionId() const {
    return whot::utils::Random::getInstance().generateId(24);
}

std::string SessionManager::createSession(const std::string& ipAddress) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string id = generateSessionId();
    while (sessions_.find(id) != sessions_.end())
        id = generateSessionId();
    auto s = std::make_unique<Session>();
    s->sessionId = id;
    s->ipAddress = ipAddress;
    s->lastActivity = std::chrono::steady_clock::now();
    s->authenticated = false;
    sessions_[id] = std::move(s);
    return id;
}

void SessionManager::destroySession(const std::string& sessionId) {
    std::lock_guard<std::mutex> lock(mutex_);
    sessions_.erase(sessionId);
}

bool SessionManager::sessionExists(const std::string& sessionId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return sessions_.find(sessionId) != sessions_.end();
}

Session* SessionManager::getSession(const std::string& sessionId) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = sessions_.find(sessionId);
    return it != sessions_.end() ? it->second.get() : nullptr;
}

const Session* SessionManager::getSession(const std::string& sessionId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = sessions_.find(sessionId);
    return it != sessions_.end() ? it->second.get() : nullptr;
}

void SessionManager::setPlayerId(const std::string& sessionId, const std::string& playerId) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = sessions_.find(sessionId);
    if (it != sessions_.end()) it->second->playerId = playerId;
}

void SessionManager::setGameId(const std::string& sessionId, const std::string& gameId) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = sessions_.find(sessionId);
    if (it != sessions_.end()) it->second->gameId = gameId;
}

void SessionManager::updateActivity(const std::string& sessionId) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = sessions_.find(sessionId);
    if (it != sessions_.end())
        it->second->lastActivity = std::chrono::steady_clock::now();
}

std::vector<std::string> SessionManager::getSessionsForGame(const std::string& gameId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> out;
    for (const auto& [id, s] : sessions_)
        if (s && s->gameId == gameId) out.push_back(id);
    return out;
}

std::vector<std::string> SessionManager::getSessionsForPlayer(const std::string& playerId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> out;
    for (const auto& [id, s] : sessions_)
        if (s && s->playerId == playerId) out.push_back(id);
    return out;
}

std::string SessionManager::getSessionIdForPlayer(const std::string& playerId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& [id, s] : sessions_)
        if (s && s->playerId == playerId) return id;
    return {};
}

void SessionManager::removeExpiredSessions(int timeoutSeconds) {
    if (timeoutSeconds <= 0) return;
    std::lock_guard<std::mutex> lock(mutex_);
    auto now = std::chrono::steady_clock::now();
    auto threshold = now - std::chrono::seconds(timeoutSeconds);
    for (auto it = sessions_.begin(); it != sessions_.end(); ) {
        if (it->second->lastActivity < threshold)
            it = sessions_.erase(it);
        else
            ++it;
    }
}

void SessionManager::removeAllSessionsForGame(const std::string& gameId) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto it = sessions_.begin(); it != sessions_.end(); ) {
        if (it->second->gameId == gameId)
            it = sessions_.erase(it);
        else
            ++it;
    }
}

size_t SessionManager::getActiveSessionCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return sessions_.size();
}

} // namespace whot::network
