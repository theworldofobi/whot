#include "../../include/Persistence/Database.hpp"
#include <sqlite3.h>
#include <chrono>
#include <stdexcept>

namespace whot::persistence {

static const int SCHEMA_VERSION = 1;

class SQLiteDatabase : public Database {
public:
    explicit SQLiteDatabase(const DatabaseConfig& config) : Database(config), db_(nullptr) {}

    ~SQLiteDatabase() override { disconnect(); }

    bool connect() override {
        if (db_) return true;
        if (config_.filepath.empty()) config_.filepath = ":memory:";
        int rc = sqlite3_open(config_.filepath.c_str(), &db_);
        if (rc != SQLITE_OK) {
            if (db_) { sqlite3_close(db_); db_ = nullptr; }
            return false;
        }
        return true;
    }

    void disconnect() override {
        if (db_) {
            sqlite3_close(db_);
            db_ = nullptr;
        }
    }

    bool isConnected() const override { return db_ != nullptr; }

    bool execute(const std::string& query) override {
        if (!db_) return false;
        char* err = nullptr;
        int rc = sqlite3_exec(db_, query.c_str(), nullptr, nullptr, &err);
        if (err) { sqlite3_free(err); }
        return rc == SQLITE_OK;
    }

    std::optional<std::string> queryOne(const std::string& query) override {
        if (!db_) return std::nullopt;
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db_, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
            return std::nullopt;
        std::optional<std::string> result;
        if (sqlite3_step(stmt) == SQLITE_ROW && sqlite3_column_count(stmt) > 0) {
            const char* t = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            if (t) result = t;
        }
        sqlite3_finalize(stmt);
        return result;
    }

    std::vector<std::string> queryMany(const std::string& query) override {
        std::vector<std::string> out;
        if (!db_) return out;
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db_, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
            return out;
        while (sqlite3_step(stmt) == SQLITE_ROW && sqlite3_column_count(stmt) > 0) {
            const char* t = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            out.push_back(t ? t : "");
        }
        sqlite3_finalize(stmt);
        return out;
    }

    void beginTransaction() override { execute("BEGIN TRANSACTION"); }
    void commit() override { execute("COMMIT"); }
    void rollback() override { execute("ROLLBACK"); }

    void initializeSchema() override {
        execute(R"(
            CREATE TABLE IF NOT EXISTS schema_version (version INTEGER PRIMARY KEY);
            INSERT OR IGNORE INTO schema_version (version) VALUES (1);
        )");
        execute(R"(
            CREATE TABLE IF NOT EXISTS games (
                game_id TEXT PRIMARY KEY,
                game_state TEXT,
                rule_variant TEXT,
                created_at INTEGER,
                updated_at INTEGER,
                status TEXT
            )
        )");
        execute(R"(
            CREATE TABLE IF NOT EXISTS players (
                player_id TEXT PRIMARY KEY,
                player_name TEXT,
                created_at INTEGER
            )
        )");
        execute(R"(
            CREATE TABLE IF NOT EXISTS player_stats (
                player_id TEXT PRIMARY KEY,
                total_games INTEGER DEFAULT 0,
                games_won INTEGER DEFAULT 0,
                total_score INTEGER DEFAULT 0,
                last_played INTEGER,
                FOREIGN KEY(player_id) REFERENCES players(player_id)
            )
        )");
        execute(R"(
            CREATE TABLE IF NOT EXISTS game_players (
                game_id TEXT,
                player_id TEXT,
                PRIMARY KEY(game_id, player_id),
                FOREIGN KEY(game_id) REFERENCES games(game_id),
                FOREIGN KEY(player_id) REFERENCES players(player_id)
            )
        )");
    }

    void migrate(int toVersion) override {
        (void)toVersion;
        if (getCurrentSchemaVersion() < toVersion)
            initializeSchema();
    }

    int getCurrentSchemaVersion() override {
        auto r = queryOne("SELECT version FROM schema_version LIMIT 1");
        if (r) return std::stoi(*r);
        return 0;
    }

private:
    sqlite3* db_;
};

Database::Database(const DatabaseConfig& config) : config_(config) {}

std::unique_ptr<Database> DatabaseFactory::create(const DatabaseConfig& config) {
    if (config.type == DatabaseType::SQLITE)
        return std::make_unique<SQLiteDatabase>(config);
    return nullptr;
}

} // namespace whot::persistence
