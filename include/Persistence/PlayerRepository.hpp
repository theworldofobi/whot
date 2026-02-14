#ifndef WHOT_PERSISTENCE_PLAYER_REPOSITORY_HPP
#define WHOT_PERSISTENCE_PLAYER_REPOSITORY_HPP

#include "Core/Player.hpp"
#include "Persistence/Database.hpp"
#include <optional>
#include <vector>
#include <chrono>

namespace whot::persistence {

struct PlayerStats {
    std::string playerId;
    std::string playerName;
    int totalGames;
    int gamesWon;
    int totalScore;
    double winRate;
    std::chrono::system_clock::time_point lastPlayed;
    std::chrono::system_clock::time_point createdAt;
};

class PlayerRepository {
public:
    explicit PlayerRepository(Database* database);
    
    // Player management
    bool savePlayer(const core::Player& player);
    std::optional<core::Player> loadPlayer(const std::string& playerId);
    bool updatePlayer(const core::Player& player);
    bool deletePlayer(const std::string& playerId);
    
    // Statistics
    PlayerStats getPlayerStats(const std::string& playerId);
    void updateStats(const std::string& playerId, bool won, int score);
    std::vector<PlayerStats> getLeaderboard(int limit = 100);
    
    // Queries
    std::optional<std::string> findPlayerByName(const std::string& name);
    std::vector<PlayerStats> searchPlayers(const std::string& query);
    
private:
    Database* database_;
    
    PlayerStats resultToStats(const std::string& queryResult);
};

} // namespace whot::persistence

#endif // WHOT_PERSISTENCE_PLAYER_REPOSITORY_HPP
