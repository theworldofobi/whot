#include "TestHelpers.hpp"
#include <chrono>

namespace whot {
namespace test {

std::unique_ptr<core::Card> makeCard(core::Suit suit, core::CardValue value) {
    return std::make_unique<core::Card>(suit, value);
}

std::unique_ptr<core::Hand> makeHandWithCards(
    const std::vector<std::pair<core::Suit, core::CardValue>>& cards) {
    auto hand = std::make_unique<core::Hand>(static_cast<int>(cards.size()));
    for (const auto& p : cards)
        hand->addCard(std::make_unique<core::Card>(p.first, p.second));
    return hand;
}

std::unique_ptr<game::GameState> makeGameStateWithPlayers(int playerCount) {
    game::GameConfig config;
    config.minPlayers = 2;
    config.maxPlayers = 8;
    auto state = std::make_unique<game::GameState>(config);
    state->initialize();
    for (int i = 0; i < playerCount; ++i) {
        std::string id = "player-" + std::to_string(i);
        std::string name = "Player" + std::to_string(i);
        state->addPlayer(std::make_unique<core::Player>(
            id, name, core::PlayerType::HUMAN));
    }
    return state;
}

network::Message makeMessage(network::MessageType type, const std::string& payload) {
    network::Message m;
    m.type = type;
    m.payload = payload;
    m.timestamp = static_cast<uint64_t>(
        std::chrono::system_clock::now().time_since_epoch().count());
    return m;
}

std::string malformedJson() {
    return "{ invalid }";
}

std::string emptyString() {
    return "";
}

std::string wrongTypeJson(const std::string& key, const std::string& type) {
    if (type == "number")
        return "{\"" + key + "\": 42}";
    if (type == "array")
        return "{\"" + key + "\": []}";
    if (type == "null")
        return "{\"" + key + "\": null}";
    return "{\"" + key + "\": true}";
}

persistence::DatabaseConfig makeInMemoryDbConfig() {
    persistence::DatabaseConfig config;
    config.type = persistence::DatabaseType::SQLITE;
    config.filepath = ":memory:";
    return config;
}

std::unique_ptr<persistence::Database> createInMemoryDatabase() {
    auto config = makeInMemoryDbConfig();
    auto db = persistence::DatabaseFactory::create(config);
    if (db && db->connect())
        db->initializeSchema();
    return db;
}

} // namespace test
} // namespace whot
