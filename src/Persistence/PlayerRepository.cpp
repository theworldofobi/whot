#include "../../include/Persistence/PlayerRepository.hpp"
#include "../../include/Core/Player.hpp"
#include <nlohmann/json.hpp>
#include <chrono>
#include <sstream>

namespace whot::persistence {

namespace {
int64_t toUnixTime(std::chrono::system_clock::time_point tp) {
    return std::chrono::duration_cast<std::chrono::seconds>(
               tp.time_since_epoch())
        .count();
}
std::chrono::system_clock::time_point fromUnixTime(int64_t t) {
    return std::chrono::system_clock::time_point(std::chrono::seconds(t));
}
} // namespace

PlayerRepository::PlayerRepository(Database* database) : database_(database) {}

bool PlayerRepository::savePlayer(const core::Player& player) {
    if (!database_ || !database_->isConnected()) return false;
    int64_t now = toUnixTime(std::chrono::system_clock::now());

    if (!database_->executeBound(
            "INSERT OR REPLACE INTO players (player_id, player_name, created_at)"
            " VALUES (?, ?, ?)",
            {player.getId(), player.getName(), now}))
        return false;

    database_->executeBound(
        "INSERT OR REPLACE INTO player_stats"
        " (player_id, total_games, games_won, total_score, last_played)"
        " VALUES (?, ?, ?, ?, ?)",
        {player.getId(),
         static_cast<int64_t>(player.getGamesPlayed()),
         static_cast<int64_t>(player.getGamesWon()),
         static_cast<int64_t>(player.getCumulativeScore()),
         now});
    return true;
}

std::optional<core::Player> PlayerRepository::loadPlayer(const std::string& playerId) {
    if (!database_ || !database_->isConnected()) return std::nullopt;
    auto name = database_->queryOneBound(
        "SELECT player_name FROM players WHERE player_id = ? LIMIT 1",
        {playerId});
    if (!name) return std::nullopt;
    core::Player p(playerId, *name, core::PlayerType::HUMAN);
    auto totalGames = database_->queryOneBound(
        "SELECT total_games FROM player_stats WHERE player_id = ? LIMIT 1",
        {playerId});
    auto gamesWon = database_->queryOneBound(
        "SELECT games_won FROM player_stats WHERE player_id = ? LIMIT 1",
        {playerId});
    auto totalScore = database_->queryOneBound(
        "SELECT total_score FROM player_stats WHERE player_id = ? LIMIT 1",
        {playerId});
    try {
        if (totalGames) p.setGamesPlayed(std::stoi(*totalGames));
        if (gamesWon)   p.setGamesWon(std::stoi(*gamesWon));
        if (totalScore) p.setCumulativeScore(std::stoi(*totalScore));
    } catch (...) {}
    return p;
}

bool PlayerRepository::updatePlayer(const core::Player& player) {
    return savePlayer(player);
}

bool PlayerRepository::deletePlayer(const std::string& playerId) {
    if (!database_) return false;
    database_->executeBound(
        "DELETE FROM player_stats WHERE player_id = ?", {playerId});
    return database_->executeBound(
        "DELETE FROM players WHERE player_id = ?", {playerId});
}

PlayerStats PlayerRepository::getPlayerStats(const std::string& playerId) {
    PlayerStats s;
    s.playerId = playerId;
    s.totalGames = 0;
    s.gamesWon = 0;
    s.totalScore = 0;
    s.winRate = 0.0;
    if (!database_) return s;

    auto name = database_->queryOneBound(
        "SELECT player_name FROM players WHERE player_id = ?", {playerId});
    if (name) s.playerName = *name;

    auto totalGames = database_->queryOneBound(
        "SELECT total_games FROM player_stats WHERE player_id = ? LIMIT 1",
        {playerId});
    auto gamesWon = database_->queryOneBound(
        "SELECT games_won FROM player_stats WHERE player_id = ? LIMIT 1",
        {playerId});
    auto totalScore = database_->queryOneBound(
        "SELECT total_score FROM player_stats WHERE player_id = ? LIMIT 1",
        {playerId});
    auto lastPlayed = database_->queryOneBound(
        "SELECT last_played FROM player_stats WHERE player_id = ? LIMIT 1",
        {playerId});
    try {
        if (totalGames) s.totalGames = std::stoi(*totalGames);
        if (gamesWon)   s.gamesWon   = std::stoi(*gamesWon);
        if (totalScore) s.totalScore  = std::stoi(*totalScore);
        if (s.totalGames > 0)
            s.winRate = static_cast<double>(s.gamesWon) / s.totalGames;
        if (lastPlayed)
            s.lastPlayed = fromUnixTime(std::stoll(*lastPlayed));
    } catch (...) {}
    return s;
}

void PlayerRepository::updateStats(const std::string& playerId, bool won, int score) {
    if (!database_) return;
    auto s = getPlayerStats(playerId);
    s.totalGames++;
    if (won) s.gamesWon++;
    s.totalScore += score;
    s.lastPlayed = std::chrono::system_clock::now();
    int64_t lp = toUnixTime(s.lastPlayed);
    database_->executeBound(
        "INSERT OR REPLACE INTO player_stats"
        " (player_id, total_games, games_won, total_score, last_played)"
        " VALUES (?, ?, ?, ?, ?)",
        {playerId,
         static_cast<int64_t>(s.totalGames),
         static_cast<int64_t>(s.gamesWon),
         static_cast<int64_t>(s.totalScore),
         lp});
}

std::vector<PlayerStats> PlayerRepository::getLeaderboard(int limit) {
    std::vector<PlayerStats> out;
    if (!database_) return out;
    auto ids = database_->queryManyBound(
        "SELECT player_id FROM player_stats"
        " ORDER BY games_won DESC, total_score DESC LIMIT ?",
        {static_cast<int64_t>(limit)});
    for (const auto& pid : ids)
        out.push_back(getPlayerStats(pid));
    return out;
}

std::optional<std::string> PlayerRepository::findPlayerByName(const std::string& name) {
    if (!database_) return std::nullopt;
    return database_->queryOneBound(
        "SELECT player_id FROM players WHERE player_name = ? LIMIT 1", {name});
}

std::vector<PlayerStats> PlayerRepository::searchPlayers(const std::string& query) {
    std::vector<PlayerStats> out;
    if (!database_) return out;
    std::string pattern = "%" + query + "%";
    auto ids = database_->queryManyBound(
        "SELECT player_id FROM players WHERE player_name LIKE ? LIMIT 50",
        {pattern});
    for (const auto& id : ids)
        out.push_back(getPlayerStats(id));
    return out;
}

PlayerStats PlayerRepository::resultToStats(const std::string& queryResult) {
    PlayerStats s;
    s.totalGames = 0;
    s.gamesWon = 0;
    s.totalScore = 0;
    s.winRate = 0.0;
    try {
        auto j = nlohmann::json::parse(queryResult);
        s.playerId   = j.value("playerId", "");
        s.playerName = j.value("playerName", "");
        s.totalGames = j.value("totalGames", 0);
        s.gamesWon   = j.value("gamesWon", 0);
        s.totalScore = j.value("totalScore", 0);
        if (s.totalGames > 0)
            s.winRate = static_cast<double>(s.gamesWon) / s.totalGames;
    } catch (...) {}
    return s;
}

} // namespace whot::persistence
