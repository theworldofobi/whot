#ifndef WHOT_PERSISTENCE_DATABASE_HPP
#define WHOT_PERSISTENCE_DATABASE_HPP

#include <string>
#include <variant>
#include <vector>
#include <memory>
#include <optional>

namespace whot::persistence {

// A typed parameter for parameterised queries.  Only text and 64-bit
// integer variants are needed; NULLs are represented by an empty text.
using SqlParam = std::variant<std::string, int64_t>;

enum class DatabaseType {
    SQLITE,
    POSTGRESQL,
    MYSQL,
    MEMORY  // For testing
};

struct DatabaseConfig {
    DatabaseType type;
    std::string host;
    int port;
    std::string database;
    std::string username;
    std::string password;
    std::string filepath;  // For SQLite
    int poolSize = 10;
};

class Database {
public:
    explicit Database(const DatabaseConfig& config);
    virtual ~Database() = default;
    
    // Connection management
    virtual bool connect() = 0;
    virtual void disconnect() = 0;
    virtual bool isConnected() const = 0;
    
    // Query execution
    virtual bool execute(const std::string& query) = 0;
    virtual std::optional<std::string> queryOne(const std::string& query) = 0;
    virtual std::vector<std::string> queryMany(const std::string& query) = 0;

    // Parameterised variants — prefer these for any query that incorporates
    // external input.  Placeholders in sql are positional '?' markers.
    virtual bool executeBound(
        const std::string& sql, const std::vector<SqlParam>& params) = 0;
    virtual std::optional<std::string> queryOneBound(
        const std::string& sql, const std::vector<SqlParam>& params) = 0;
    virtual std::vector<std::string> queryManyBound(
        const std::string& sql, const std::vector<SqlParam>& params) = 0;
    
    // Transactions
    virtual void beginTransaction() = 0;
    virtual void commit() = 0;
    virtual void rollback() = 0;
    
    // Schema management
    virtual void initializeSchema() = 0;
    virtual void migrate(int toVersion) = 0;
    virtual int getCurrentSchemaVersion() = 0;
    
protected:
    DatabaseConfig config_;
};

class DatabaseFactory {
public:
    static std::unique_ptr<Database> create(const DatabaseConfig& config);
};

} // namespace whot::persistence

#endif // WHOT_PERSISTENCE_DATABASE_HPP
