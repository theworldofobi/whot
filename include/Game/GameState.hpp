#ifndef WHOT_GAME_GAME_STATE_HPP
#define WHOT_GAME_GAME_STATE_HPP

#include "Core/Player.hpp"
#include "Core/Deck.hpp"
#include "Core/Card.hpp"
#include <vector>
#include <memory>
#include <optional>
#include <string>
#include <chrono>

namespace whot::game {

enum class GamePhase : uint8_t {
    LOBBY,
    STARTING,
    IN_PROGRESS,
    ROUND_ENDED,
    GAME_ENDED
};

enum class PlayDirection : uint8_t {
    CLOCKWISE,
    COUNTER_CLOCKWISE
};

struct GameConfig {
    int minPlayers = 2;
    int maxPlayers = 8;
    int startingCards = 6;
    int turnTimeSeconds = 10;
    int eliminationScore = 100;
    bool allowDoubleDecking = false;
    bool allowDirectionChange = true;
    bool enforceTurnTimer = false;
};

class GameState {
public:
    explicit GameState(const GameConfig& config);
    
    // Game lifecycle
    void initialize();
    void startRound();
    void endRound();
    void endGame();
    
    // Player management
    void addPlayer(std::unique_ptr<core::Player> player);
    void removePlayer(const std::string& playerId);
    core::Player* getPlayer(const std::string& playerId);
    const core::Player* getPlayer(const std::string& playerId) const;
    core::Player* getCurrentPlayer();
    const core::Player* getCurrentPlayer() const;
    std::vector<core::Player*> getActivePlayers();
    std::vector<core::Player*> getActivePlayers() const;
    size_t getPlayerCount() const;
    
    // Turn management
    void advanceTurn();
    void skipNextPlayer();
    void reverseDirection();
    int getCurrentPlayerIndex() const;
    PlayDirection getPlayDirection() const;
    
    // Card management
    core::Deck& getDeck();
    const core::Card* getCallCard() const;
    void setCallCard(std::unique_ptr<core::Card> card);
    void addToDiscardPile(std::unique_ptr<core::Card> card);
    
    // Special card state
    void setActivePickCount(int count);  // For chained 2s or 5s
    int getActivePickCount() const;
    void resetActivePickCount();
    
    void setDemandedSuit(std::optional<core::Suit> suit);
    std::optional<core::Suit> getDemandedSuit() const;
    void clearDemandedSuit();
    
    // Game state queries
    GamePhase getPhase() const;
    void setPhase(GamePhase phase);
    const GameConfig& getConfig() const;
    const std::string& getGameId() const;
    const std::string& getGameCode() const;
    void setGameCode(const std::string& code);
    const std::string& getCreatorPlayerId() const;
    void setCreatorPlayerId(const std::string& playerId);
    
    bool needsReshufffle() const;
    void reshuffleDiscardPile();
    
    // Win conditions
    bool checkRoundEnd() const;
    bool checkGameEnd() const;
    std::optional<std::string> getWinnerId() const;
    
    // Serialization
    std::string toJson() const;
    /// Same as toJson() but other players' hands are replaced with {"count": N}.
    std::string toJsonForPlayer(const std::string& viewerPlayerId) const;
    static std::unique_ptr<GameState> fromJson(const std::string& json);
    
private:
    GameConfig config_;
    GamePhase phase_;
    
    std::vector<std::unique_ptr<core::Player>> players_;
    int currentPlayerIndex_;
    PlayDirection direction_;
    
    core::Deck deck_;
    std::unique_ptr<core::Card> callCard_;
    std::vector<std::unique_ptr<core::Card>> discardPile_;
    
    // Special game state
    int activePickCount_;  // For chained pick 2/3
    std::optional<core::Suit> demandedSuit_;  // When Whot card played
    
    std::string gameId_;
    std::string gameCode_;
    std::string creatorPlayerId_;
    std::chrono::system_clock::time_point createdAt_;
    
    int getNextPlayerIndex() const;
    void eliminatePlayers();  // Check for elimination conditions
};

} // namespace whot::game

#endif // WHOT_GAME_GAME_STATE_HPP