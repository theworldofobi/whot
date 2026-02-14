#ifndef WHOT_CORE_GAME_CONSTANTS_HPP
#define WHOT_CORE_GAME_CONSTANTS_HPP

#include <cstdint>
#include <string>

namespace whot::core {

enum class Suit : uint8_t {
    CIRCLE,
    TRIANGLE,
    CROSS,
    BLOCK,
    STAR,
    WHOT
};

enum class CardValue : uint8_t {
    ONE = 1,
    TWO = 2,
    THREE = 3,
    FOUR = 4,
    FIVE = 5,
    SEVEN = 7,
    EIGHT = 8,
    TEN = 10,
    ELEVEN = 11,
    TWELVE = 12,
    THIRTEEN = 13,
    FOURTEEN = 14,
    TWENTY = 20
};

constexpr int STANDARD_DECK_SIZE = 54;
constexpr int WHOT_CARDS_COUNT = 5;
constexpr int DEFAULT_STARTING_CARDS = 6;
constexpr int NIGERIAN_MIN_CARDS = 3;
constexpr int NIGERIAN_MAX_CARDS = 6;
constexpr int DEFAULT_TURN_TIME_SECONDS = 10;
constexpr int ELIMINATION_SCORE = 100;

enum class SpecialAbility : uint8_t {
    NONE,
    HOLD_ON,           // 1 - skip next player
    PICK_TWO,          // 2 - next player picks 2
    GENERAL_MARKET,    // 14 - all players pick 1
    WHOT_CARD          // 20 - change suit
};

std::string suitToString(Suit suit);
std::string cardValueToString(CardValue value);
Suit stringToSuit(const std::string& str);
CardValue stringToCardValue(const std::string& str);

} // namespace whot::core

#endif // WHOT_CORE_GAME_CONSTANTS_HPP
