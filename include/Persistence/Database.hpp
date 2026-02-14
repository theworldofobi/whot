#ifndef WHOT_PERSISTENCE_DATABASE_HPP
#define WHOT_PERSISTENCE_DATABASE_HPP

#include <string>
#include <vector>
#include <memory>
#include <optional>

namespace whot::persistence {

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
