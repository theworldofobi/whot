#include "../../include/Persistence/GameRepository.hpp"
#include "../../include/Game/GameState.hpp"
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
} // namespace

GameRepository::GameRepository(Database* database) : database_(database) {}

bool GameRepository::saveGame(const game::GameState& state) {
    if (!database_ || !database_->isConnected()) return false;
    std::string json = state.toJson();
    std::string gid = escapeSql(state.getGameId());
    std::string jesc = escapeSql(json);
    int64_t now = toUnixTime(std::chrono::system_clock::now());
    std::string status = "active";
    if (state.getPhase() == game::GamePhase::GAME_ENDED) status = "ended";
    else if (state.getPhase() == game::GamePhase::ROUND_ENDED) status = "round_ended";

    std::ostringstream sql;
    sql << "INSERT OR REPLACE INTO games (game_id, game_state, rule_variant, created_at, updated_at, status) "
        << "VALUES ('" << gid << "','" << jesc << "','nigerian'," << now << "," << now << ",'" << status << "')";
    if (!database_->execute(sql.str())) return false;

    database_->execute("DELETE FROM game_players WHERE game_id='" + gid + "'");
    try {
        auto jo = nlohmann::json::parse(json);
        if (jo.contains("players") && jo["players"].is_array())
            for (const auto& pj : jo["players"])
                if (pj.contains("id")) {
                    std::string pid = escapeSql(pj["id"].get<std::string>());
                    database_->execute("INSERT OR IGNORE INTO game_players (game_id, player_id) VALUES ('" + gid + "','" + pid + "')");
                }
    } catch (...) {}
    return true;
}

std::optional<game::GameState> GameRepository::loadGame(const std::string& gameId) {
    if (!database_ || !database_->isConnected()) return std::nullopt;
    std::string gid = escapeSql(gameId);
    auto json = database_->queryOne("SELECT game_state FROM games WHERE game_id='" + gid + "' LIMIT 1");
    if (!json) return std::nullopt;
    auto ptr = game::GameState::fromJson(*json);
    if (!ptr) return std::nullopt;
    return std::optional<game::GameState>(std::move(*ptr));
}

bool GameRepository::updateGame(const game::GameState& state) {
    return saveGame(state);
}

bool GameRepository::deleteGame(const std::string& gameId) {
    if (!database_) return false;
    std::string gid = escapeSql(gameId);
    database_->execute("DELETE FROM game_players WHERE game_id='" + gid + "'");
    return database_->execute("DELETE FROM games WHERE game_id='" + gid + "'");
}

std::vector<GameRecord> GameRepository::getActiveGames() {
    std::vector<GameRecord> out;
    if (!database_) return out;
    auto rows = database_->queryMany("SELECT game_id FROM games WHERE status='active' OR status='in_progress'");
    for (const auto& gid : rows) {
        auto j = database_->queryOne("SELECT game_state FROM games WHERE game_id='" + escapeSql(gid) + "' LIMIT 1");
        if (j) {
            GameRecord rec;
            rec.gameId = gid;
            rec.gameStateJson = *j;
            try {
                auto jo = nlohmann::json::parse(*j);
                rec.status = jo.value("phase", "active");
                if (jo.contains("gameId")) rec.gameId = jo["gameId"].get<std::string>();
            } catch (...) {}
            out.push_back(rec);
        }
    }
    return out;
}

std::vector<GameRecord> GameRepository::getGamesByPlayer(const std::string& playerId) {
    std::vector<GameRecord> out;
    if (!database_) return out;
    std::string pid = escapeSql(playerId);
    auto gids = database_->queryMany("SELECT game_id FROM game_players WHERE player_id='" + pid + "'");
    for (const auto& gid : gids) {
        auto j = database_->queryOne("SELECT game_state FROM games WHERE game_id='" + escapeSql(gid) + "' LIMIT 1");
        if (j) {
            GameRecord rec;
            rec.gameId = gid;
            rec.gameStateJson = *j;
            rec.playerIds.push_back(playerId);
            out.push_back(rec);
        }
    }
    return out;
}

std::vector<GameRecord> GameRepository::getCompletedGames(int limit) {
    std::vector<GameRecord> out;
    if (!database_) return out;
    auto rows = database_->queryMany("SELECT game_id FROM games WHERE status='ended' ORDER BY updated_at DESC LIMIT " + std::to_string(limit));
    for (const auto& gid : rows) {
        auto j = database_->queryOne("SELECT game_state FROM games WHERE game_id='" + escapeSql(gid) + "' LIMIT 1");
        if (j) {
            GameRecord rec;
            rec.gameId = gid;
            rec.gameStateJson = *j;
            rec.status = "ended";
            out.push_back(rec);
        }
    }
    return out;
}

std::optional<GameRecord> GameRepository::findGameByPlayer(const std::string& playerId) {
    auto list = getGamesByPlayer(playerId);
    for (auto& rec : list) {
        try {
            auto jo = nlohmann::json::parse(rec.gameStateJson);
            std::string status = jo.value("phase", "");
            if (status != "ended" && status != "GAME_ENDED")
                return rec;
        } catch (...) {}
    }
    return std::nullopt;
}

std::vector<std::string> GameRepository::getAvailableGames(const std::string& ruleVariant) {
    (void)ruleVariant;
    auto recs = getActiveGames();
    std::vector<std::string> ids;
    for (const auto& r : recs) ids.push_back(r.gameId);
    return ids;
}

void GameRepository::deleteOldGames(int daysOld) {
    if (!database_) return;
    int64_t cutoff = toUnixTime(std::chrono::system_clock::now()) - static_cast<int64_t>(daysOld) * 86400;
    database_->execute("DELETE FROM game_players WHERE game_id IN (SELECT game_id FROM games WHERE updated_at < " + std::to_string(cutoff) + ")");
    database_->execute("DELETE FROM games WHERE updated_at < " + std::to_string(cutoff));
}

void GameRepository::archiveCompletedGames() {
    if (!database_) return;
    database_->execute("UPDATE games SET status='archived' WHERE status='ended'");
}

GameRecord GameRepository::resultToRecord(const std::string& queryResult) {
    GameRecord r;
    r.gameStateJson = queryResult;
    return r;
}

std::string GameRepository::recordToJson(const GameRecord& record) {
    return record.gameStateJson;
}

} // namespace whot::persistence
