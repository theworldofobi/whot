#ifndef WHOT_GAME_ACTION_TYPES_HPP
#define WHOT_GAME_ACTION_TYPES_HPP

#include "Game/GameState.hpp"
#include "Core/Card.hpp"
#include <string>
#include <optional>

namespace whot::game {

enum class ActionType : uint8_t {
    PLAY_CARD,
    DRAW_CARD,
    DECLARE_LAST_CARD,
    DECLARE_CHECK_UP,
    CHOOSE_SUIT,
    CHOOSE_DIRECTION,
    FORFEIT_TURN
};

struct GameAction {
    ActionType type;
    std::string playerId;
    std::optional<size_t> cardIndex;
    std::optional<core::Suit> chosenSuit;
    std::optional<PlayDirection> chosenDirection;
    std::string metadata;
};

struct ActionResult {
    bool success;
    std::string message;
    std::vector<std::string> affectedPlayerIds;
    std::string newStateJson;
};

} // namespace whot::game

#endif // WHOT_GAME_ACTION_TYPES_HPP
