#ifndef WHOT_CORE_PLAYER_HPP
#define WHOT_CORE_PLAYER_HPP

#include "Hand.hpp"
#include <string>
#include <cstdint>
#include <chrono>

namespace whot::core {

enum class PlayerType : uint8_t {
    HUMAN,
    AI_EASY,      // coming soon
    AI_MEDIUM,    // coming soon
    AI_HARD       // coming soon
};

enum class PlayerStatus : uint8_t {
    ACTIVE,
    WAITING,
    ELIMINATED,
    DISCONNECTED,
    WINNER
};

class Player {
public:
    Player(const std::string& id, const std::string& name, PlayerType type);
    
    std::string getId() const;
    std::string getName() const;
    PlayerType getType() const;
    void setName(const std::string& name);
    
    Hand& getHand();
    const Hand& getHand() const;
    PlayerStatus getStatus() const;
    void setStatus(PlayerStatus status);
    
    int getCurrentScore() const;
    int getCumulativeScore() const;
    void addToScore(int points);
    void resetCurrentScore();
    void resetAllScores();
    
    bool hasSaidLastCard() const;
    void setSaidLastCard(bool said);
    bool hasSaidCheckUp() const;
    void setSaidCheckUp(bool said);
    void resetTurnFlags();
    
    std::chrono::steady_clock::time_point getLastActionTime() const;
    void updateLastActionTime();
    bool hasExceededTurnTime(int maxSeconds) const;
    
    int getGamesWon() const;
    int getGamesPlayed() const;
    void incrementGamesPlayed();
    void incrementGamesWon();
    
    std::string toJson() const;
    static std::unique_ptr<Player> fromJson(const std::string& json);
    
private:
    std::string id_;
    std::string name_;
    PlayerType type_;
    PlayerStatus status_;
    
    Hand hand_;
    
    int currentScore_;
    int cumulativeScore_;
    
    bool saidLastCard_;
    bool saidCheckUp_;
    
    std::chrono::steady_clock::time_point lastActionTime_;
    
    int gamesPlayed_;
    int gamesWon_;
};

} // namespace whot::core

#endif // WHOT_CORE_PLAYER_HPP
