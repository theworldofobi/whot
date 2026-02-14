#include "../../include/Persistence/PlayerRepository.hpp"
#include "../../include/Core/Player.hpp"
#include <nlohmann/json.hpp>
#include <chrono>
#include <sstream>

namespace whot::persistence {

namespace {
std::string escapeSql(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 16);
    for (char c : s) {
        if (c == '\'') out += "''";
        else out += c;
    }
    return out;
}
int64_t toUnixTime(std::chrono::system_clock::time_point tp) {
    return std::chrono::duration_cast<std::chrono::seconds>(tp.time_since_epoch()).count();
}
std::chrono::system_clock::time_point fromUnixTime(int64_t t) {
    return std::chrono::system_clock::time_point(std::chrono::seconds(t));
}
} // namespace

PlayerRepository::PlayerRepository(Database* database) : database_(database) {}

bool PlayerRepository::savePlayer(const core::Player& player) {
    if (!database_ || !database_->isConnected()) return false;
    std::string id = escapeSql(player.getId());
    std::string name = escapeSql(player.getName());
    int64_t now = toUnixTime(std::chrono::system_clock::now());

    if (!database_->execute("INSERT OR REPLACE INTO players (player_id, player_name, created_at) VALUES ('" + id + "','" + name + "'," + std::to_string(now) + ")"))
        return false;

    database_->execute(
        "INSERT OR REPLACE INTO player_stats (player_id, total_games, games_won, total_score, last_played) "
        "VALUES ('" + id + "'," + std::to_string(player.getGamesPlayed()) + "," + std::to_string(player.getGamesWon()) + ","
        + std::to_string(player.getCumulativeScore()) + "," + std::to_string(now) + ")");
    return true;
}

std::optional<core::Player> PlayerRepository::loadPlayer(const std::string& playerId) {
    if (!database_ || !database_->isConnected()) return std::nullopt;
    std::string id = escapeSql(playerId);
    auto name = database_->queryOne("SELECT player_name FROM players WHERE player_id='" + id + "' LIMIT 1");
    if (!name) return std::nullopt;
    auto p = std::make_unique<core::Player>(playerId, *name, core::PlayerType::HUMAN);
    auto stats = database_->queryMany("SELECT total_games, games_won, total_score FROM player_stats WHERE player_id='" + id + "' LIMIT 1");
    return std::optional<core::Player>(std::move(*p));
}

bool PlayerRepository::updatePlayer(const core::Player& player) {
    return savePlayer(player);
}

bool PlayerRepository::deletePlayer(const std::string& playerId) {
    if (!database_) return false;
    std::string id = escapeSql(playerId);
    database_->execute("DELETE FROM player_stats WHERE player_id='" + id + "'");
    return database_->execute("DELETE FROM players WHERE player_id='" + id + "'");
}

PlayerStats PlayerRepository::getPlayerStats(const std::string& playerId) {
    PlayerStats s;
    s.playerId = playerId;
    s.totalGames = 0;
    s.gamesWon = 0;
    s.totalScore = 0;
    s.winRate = 0.0;
    if (!database_) return s;

    std::string id = escapeSql(playerId);
    auto name = database_->queryOne("SELECT player_name FROM players WHERE player_id='" + id + "'");
    if (name) s.playerName = *name;

    auto totalGames = database_->queryOne("SELECT total_games FROM player_stats WHERE player_id='" + id + "' LIMIT 1");
    auto gamesWon = database_->queryOne("SELECT games_won FROM player_stats WHERE player_id='" + id + "' LIMIT 1");
    auto totalScore = database_->queryOne("SELECT total_score FROM player_stats WHERE player_id='" + id + "' LIMIT 1");
    auto lastPlayed = database_->queryOne("SELECT last_played FROM player_stats WHERE player_id='" + id + "' LIMIT 1");
    try {
        if (totalGames) s.totalGames = std::stoi(*totalGames);
        if (gamesWon) s.gamesWon = std::stoi(*gamesWon);
        if (totalScore) s.totalScore = std::stoi(*totalScore);
        if (s.totalGames > 0) s.winRate = static_cast<double>(s.gamesWon) / s.totalGames;
        if (lastPlayed) s.lastPlayed = fromUnixTime(std::stoll(*lastPlayed));
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
    std::string id = escapeSql(playerId);
    int64_t lp = toUnixTime(s.lastPlayed);
    database_->execute(
        "INSERT OR REPLACE INTO player_stats (player_id, total_games, games_won, total_score, last_played) "
        "VALUES ('" + id + "'," + std::to_string(s.totalGames) + "," + std::to_string(s.gamesWon) + ","
        + std::to_string(s.totalScore) + "," + std::to_string(lp) + ")");
}

std::vector<PlayerStats> PlayerRepository::getLeaderboard(int limit) {
    std::vector<PlayerStats> out;
    if (!database_) return out;
    auto rows = database_->queryMany("SELECT player_id FROM player_stats ORDER BY games_won DESC, total_score DESC LIMIT " + std::to_string(limit));
    for (const auto& pid : rows) {
        out.push_back(getPlayerStats(pid));
    }
    return out;
}

std::optional<std::string> PlayerRepository::findPlayerByName(const std::string& name) {
    if (!database_) return std::nullopt;
    std::string n = escapeSql(name);
    return database_->queryOne("SELECT player_id FROM players WHERE player_name='" + n + "' LIMIT 1");
}

std::vector<PlayerStats> PlayerRepository::searchPlayers(const std::string& query) {
    std::vector<PlayerStats> out;
    if (!database_) return out;
    std::string q = escapeSql(query);
    auto ids = database_->queryMany("SELECT player_id FROM players WHERE player_name LIKE '%" + q + "%' LIMIT 50");
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
        s.playerId = j.value("playerId", "");
        s.playerName = j.value("playerName", "");
        s.totalGames = j.value("totalGames", 0);
        s.gamesWon = j.value("gamesWon", 0);
        s.totalScore = j.value("totalScore", 0);
        if (s.totalGames > 0) s.winRate = static_cast<double>(s.gamesWon) / s.totalGames;
    } catch (...) {}
    return s;
}

} // namespace whot::persistence
