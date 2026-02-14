#include "../../include/Core/Player.hpp"
#include <chrono>
#include <nlohmann/json.hpp>

namespace whot::core {

Player::Player(const std::string& id, const std::string& name, PlayerType type)
    : id_(id)
    , name_(name)
    , type_(type)
    , status_(PlayerStatus::ACTIVE)
    , currentScore_(0)
    , cumulativeScore_(0)
    , saidLastCard_(false)
    , saidCheckUp_(false)
    , lastActionTime_(std::chrono::steady_clock::now())
    , gamesPlayed_(0)
    , gamesWon_(0)
{
}

std::string Player::getId() const { return id_; }
std::string Player::getName() const { return name_; }
PlayerType Player::getType() const { return type_; }
void Player::setName(const std::string& name) { name_ = name; }

Hand& Player::getHand() { return hand_; }
const Hand& Player::getHand() const { return hand_; }

PlayerStatus Player::getStatus() const { return status_; }
void Player::setStatus(PlayerStatus status) { status_ = status; }

int Player::getCurrentScore() const { return currentScore_; }
int Player::getCumulativeScore() const { return cumulativeScore_; }
void Player::addToScore(int points)
{
    currentScore_ += points;
    cumulativeScore_ += points;
}
void Player::resetCurrentScore() { currentScore_ = 0; }
void Player::resetAllScores()
{
    currentScore_ = 0;
    cumulativeScore_ = 0;
}

bool Player::hasSaidLastCard() const { return saidLastCard_; }
void Player::setSaidLastCard(bool said) { saidLastCard_ = said; }
bool Player::hasSaidCheckUp() const { return saidCheckUp_; }
void Player::setSaidCheckUp(bool said) { saidCheckUp_ = said; }
void Player::resetTurnFlags()
{
    saidLastCard_ = false;
    saidCheckUp_ = false;
}

std::chrono::steady_clock::time_point Player::getLastActionTime() const { return lastActionTime_; }
void Player::updateLastActionTime() { lastActionTime_ = std::chrono::steady_clock::now(); }
bool Player::hasExceededTurnTime(int maxSeconds) const
{
    if (maxSeconds <= 0) return false;
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - lastActionTime_).count();
    return elapsed >= maxSeconds;
}

int Player::getGamesWon() const { return gamesWon_; }
int Player::getGamesPlayed() const { return gamesPlayed_; }
void Player::incrementGamesPlayed() { ++gamesPlayed_; }
void Player::incrementGamesWon() { ++gamesWon_; }

std::string Player::toJson() const
{
    using json = nlohmann::json;
    json j;
    j["id"] = id_;
    j["name"] = name_;
    j["type"] = static_cast<int>(type_);
    j["status"] = static_cast<int>(status_);
    j["hand"] = json::parse(hand_.toJson());
    j["currentScore"] = currentScore_;
    j["cumulativeScore"] = cumulativeScore_;
    j["saidLastCard"] = saidLastCard_;
    j["saidCheckUp"] = saidCheckUp_;
    j["gamesPlayed"] = gamesPlayed_;
    j["gamesWon"] = gamesWon_;
    return j.dump();
}

std::unique_ptr<Player> Player::fromJson(const std::string& jsonStr)
{
    using json = nlohmann::json;
    json j = json::parse(jsonStr);
    std::string id = j.value("id", "");
    std::string name = j.value("name", "");
    auto type = static_cast<PlayerType>(j.value("type", 0));
    auto p = std::make_unique<Player>(id, name, type);
    p->status_ = static_cast<PlayerStatus>(j.value("status", 0));
    if (j.contains("hand") && j["hand"].is_array())
        p->hand_ = Hand::fromJson(j["hand"].dump());
    p->currentScore_ = j.value("currentScore", 0);
    p->cumulativeScore_ = j.value("cumulativeScore", 0);
    p->saidLastCard_ = j.value("saidLastCard", false);
    p->saidCheckUp_ = j.value("saidCheckUp", false);
    p->gamesPlayed_ = j.value("gamesPlayed", 0);
    p->gamesWon_ = j.value("gamesWon", 0);
    return p;
}

} // namespace whot::core
