#ifndef WHOT_PERSISTENCE_GAME_REPOSITORY_HPP
#define WHOT_PERSISTENCE_GAME_REPOSITORY_HPP

#include "Game/GameState.hpp"
#include "Persistence/Database.hpp"
#include <memory>
#include <optional>
#include <vector>
#include <chrono>

namespace whot::persistence {

struct GameRecord {
    std::string gameId;
    std::string gameStateJson;
    std::string ruleVariant;
    std::chrono::system_clock::time_point createdAt;
    std::chrono::system_clock::time_point updatedAt;
    std::string status;
    std::vector<std::string> playerIds;
};

class GameRepository {
public:
    explicit GameRepository(Database* database);
    
    // CRUD operations
    bool saveGame(const game::GameState& state);
    std::optional<game::GameState> loadGame(const std::string& gameId);
    bool updateGame(const game::GameState& state);
    bool deleteGame(const std::string& gameId);
    
    // Queries
    std::vector<GameRecord> getActiveGames();
    std::vector<GameRecord> getGamesByPlayer(const std::string& playerId);
    std::vector<GameRecord> getCompletedGames(int limit = 100);
    
    // Game search
    std::optional<GameRecord> findGameByPlayer(const std::string& playerId);
    std::vector<std::string> getAvailableGames(const std::string& ruleVariant);
    
    // Cleanup
    void deleteOldGames(int daysOld);
    void archiveCompletedGames();
    
private:
    Database* database_;
    
    GameRecord resultToRecord(const std::string& queryResult);
    std::string recordToJson(const GameRecord& record);
};

} // namespace whot::persistence

#endif // WHOT_PERSISTENCE_GAME_REPOSITORY_HPP
