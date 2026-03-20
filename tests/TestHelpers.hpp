#ifndef WHOT_TESTS_TEST_HELPERS_HPP
#define WHOT_TESTS_TEST_HELPERS_HPP

#include "Core/Card.hpp"
#include "Core/Hand.hpp"
#include "Core/Player.hpp"
#include "Core/GameConstants.hpp"
#include "Game/GameState.hpp"
#include "Network/MessageProtocol.hpp"
#include "Persistence/Database.hpp"
#include <memory>
#include <string>
#include <vector>

namespace whot {
namespace test {

// --- Factory helpers ---
std::unique_ptr<core::Card> makeCard(core::Suit suit, core::CardValue value);

std::unique_ptr<core::Hand> makeHandWithCards(
    const std::vector<std::pair<core::Suit, core::CardValue>>& cards);

std::unique_ptr<game::GameState> makeGameStateWithPlayers(int playerCount);

network::Message makeMessage(network::MessageType type, const std::string& payload = "{}");

// --- JSON fault injection ---
std::string malformedJson();
std::string emptyString();
std::string wrongTypeJson(const std::string& key, const std::string& type);

// --- In-memory database ---
persistence::DatabaseConfig makeInMemoryDbConfig();
std::unique_ptr<persistence::Database> createInMemoryDatabase();

} // namespace test
} // namespace whot

#endif // WHOT_TESTS_TEST_HELPERS_HPP
