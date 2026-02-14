#include "../../include/Network/MessageProtocol.hpp"
#include "../../include/Core/GameConstants.hpp"
#include <nlohmann/json.hpp>
#include <chrono>

namespace whot::network {

using json = nlohmann::json;

std::string Message::serialize() const {
    json j;
    j["type"] = static_cast<uint16_t>(type);
    j["playerId"] = playerId;
    j["gameId"] = gameId;
    j["payload"] = payload;
    j["timestamp"] = timestamp;
    return j.dump();
}

Message Message::deserialize(const std::string& data) {
    Message m;
    m.type = MessageType::ERROR;
    m.timestamp = static_cast<uint64_t>(std::chrono::system_clock::now().time_since_epoch().count());
    try {
        json j = json::parse(data);
        if (j.contains("type")) m.type = static_cast<MessageType>(j["type"].get<uint16_t>());
        if (j.contains("playerId")) m.playerId = j["playerId"].get<std::string>();
        if (j.contains("gameId")) m.gameId = j["gameId"].get<std::string>();
        if (j.contains("payload")) m.payload = j["payload"].is_string() ? j["payload"].get<std::string>() : j["payload"].dump();
        if (j.contains("timestamp")) m.timestamp = j["timestamp"].get<uint64_t>();
    } catch (...) {}
    return m;
}

std::string JoinGamePayload::toJson() const {
    json j;
    j["playerName"] = playerName;
    j["gameId"] = gameId;
    if (password.has_value()) j["password"] = password.value();
    return j.dump();
}

JoinGamePayload JoinGamePayload::fromJson(const std::string& jsonStr) {
    JoinGamePayload p;
    try {
        json j = json::parse(jsonStr);
        if (j.contains("playerName")) p.playerName = j["playerName"].get<std::string>();
        if (j.contains("gameId")) p.gameId = j["gameId"].get<std::string>();
        if (j.contains("gameCode")) p.gameCode = j["gameCode"].get<std::string>();
        if (j.contains("password")) p.password = j["password"].get<std::string>();
    } catch (...) {}
    return p;
}

std::string PlayCardPayload::toJson() const {
    json j;
    j["cardIndex"] = cardIndex;
    if (chosenSuit.has_value()) j["chosenSuit"] = core::suitToString(chosenSuit.value());
    if (reverseDirection.has_value()) j["reverseDirection"] = reverseDirection.value();
    j["additionalCards"] = additionalCards;
    return j.dump();
}

PlayCardPayload PlayCardPayload::fromJson(const std::string& jsonStr) {
    PlayCardPayload p;
    p.cardIndex = 0;
    try {
        json j = json::parse(jsonStr);
        if (j.contains("cardIndex")) p.cardIndex = j["cardIndex"].get<size_t>();
        if (j.contains("chosenSuit")) p.chosenSuit = core::stringToSuit(j["chosenSuit"].get<std::string>());
        if (j.contains("reverseDirection")) p.reverseDirection = j["reverseDirection"].get<bool>();
        if (j.contains("additionalCards") && j["additionalCards"].is_array())
            for (const auto& x : j["additionalCards"]) p.additionalCards.push_back(x.get<size_t>());
    } catch (...) {}
    return p;
}

std::string GameStateUpdatePayload::toJson() const {
    json j;
    j["gameStateJson"] = gameStateJson;
    j["visibleTo"] = visibleTo;
    return j.dump();
}

GameStateUpdatePayload GameStateUpdatePayload::fromJson(const std::string& jsonStr) {
    GameStateUpdatePayload p;
    try {
        json j = json::parse(jsonStr);
        if (j.contains("gameStateJson")) p.gameStateJson = j["gameStateJson"].get<std::string>();
        if (j.contains("visibleTo") && j["visibleTo"].is_array())
            for (const auto& x : j["visibleTo"]) p.visibleTo.push_back(x.get<std::string>());
    } catch (...) {}
    return p;
}

std::string CardPlayedPayload::toJson() const {
    json j;
    j["playerId"] = playerId;
    j["cardJson"] = cardJson;
    j["cardsRemaining"] = cardsRemaining;
    j["saidLastCard"] = saidLastCard;
    return j.dump();
}

CardPlayedPayload CardPlayedPayload::fromJson(const std::string& jsonStr) {
    CardPlayedPayload p;
    try {
        json j = json::parse(jsonStr);
        if (j.contains("playerId")) p.playerId = j["playerId"].get<std::string>();
        if (j.contains("cardJson")) p.cardJson = j["cardJson"].get<std::string>();
        if (j.contains("cardsRemaining")) p.cardsRemaining = j["cardsRemaining"].get<int>();
        if (j.contains("saidLastCard")) p.saidLastCard = j["saidLastCard"].get<bool>();
    } catch (...) {}
    return p;
}

std::string RoundEndedPayload::toJson() const {
    json j;
    j["winnerId"] = winnerId;
    j["scores"] = scores;
    j["eliminatedPlayers"] = eliminatedPlayers;
    return j.dump();
}

RoundEndedPayload RoundEndedPayload::fromJson(const std::string& jsonStr) {
    RoundEndedPayload p;
    try {
        json j = json::parse(jsonStr);
        if (j.contains("winnerId")) p.winnerId = j["winnerId"].get<std::string>();
        if (j.contains("scores") && j["scores"].is_object())
            for (auto it = j["scores"].begin(); it != j["scores"].end(); ++it)
                p.scores[it.key()] = it.value().get<int>();
        if (j.contains("eliminatedPlayers") && j["eliminatedPlayers"].is_array())
            for (const auto& x : j["eliminatedPlayers"]) p.eliminatedPlayers.push_back(x.get<std::string>());
    } catch (...) {}
    return p;
}

std::string ErrorPayload::toJson() const {
    json j;
    j["errorCode"] = errorCode;
    j["message"] = message;
    if (field.has_value()) j["field"] = field.value();
    return j.dump();
}

ErrorPayload ErrorPayload::fromJson(const std::string& jsonStr) {
    ErrorPayload p;
    try {
        json j = json::parse(jsonStr);
        if (j.contains("errorCode")) p.errorCode = j["errorCode"].get<std::string>();
        if (j.contains("message")) p.message = j["message"].get<std::string>();
        if (j.contains("field")) p.field = j["field"].get<std::string>();
    } catch (...) {}
    return p;
}

} // namespace whot::network
