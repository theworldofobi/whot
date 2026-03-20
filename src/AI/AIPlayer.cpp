#include "../../include/AI/AIPlayer.hpp"
#include "../../include/Game/ActionTypes.hpp"
#include "../../include/Utils/Random.hpp"
#include <thread>
#include <chrono>

namespace whot::ai {

AIPlayer::AIPlayer(const std::string& id, const std::string& name, DifficultyLevel difficulty)
    : id_(id)
    , name_(name)
    , difficulty_(difficulty)
    , strategy_(DifficultyConfig::createStrategy(difficulty))
    , thinkingDelay_(DifficultyConfig::getThinkingDelay(difficulty))
{}

game::GameAction AIPlayer::decideAction(const game::GameState& state) {
    if (thinkingDelay_ > 0)
        std::this_thread::sleep_for(std::chrono::milliseconds(thinkingDelay_));

    game::GameAction action;
    action.playerId = id_;
    action.type = game::ActionType::DRAW_CARD;

    std::vector<size_t> playable = getPlayableCards(state);
    if (!playable.empty()) {
        action.type = game::ActionType::PLAY_CARD;
        action.cardIndex = selectBestCard(state, playable);
        if (DifficultyConfig::shouldMakeMistakes(difficulty_) &&
            whot::utils::Random::getInstance().nextBool(DifficultyConfig::getMistakeProbability(difficulty_))) {
            size_t i = whot::utils::Random::getInstance().nextInt(0, static_cast<int>(playable.size()) - 1);
            action.cardIndex = playable[i];
        }
        const core::Player* cur = state.getCurrentPlayer();
        if (cur && action.cardIndex.has_value() && action.cardIndex.value() < cur->getHand().size()) {
            const core::Card& c = cur->getHand().getCard(action.cardIndex.value());
            if (c.isWhotCard())
                action.chosenSuit = chooseSuitForWhotCard(state);
        }
        return action;
    }

    if (state.getActivePickCount() > 0) {
        const core::Card* call = state.getCallCard();
        if (call && shouldDefendAgainstPick(state, *call)) {
            std::vector<size_t> defense;
            const core::Player* cur = state.getCurrentPlayer();
            if (cur)
                for (size_t i = 0; i < cur->getHand().size(); ++i)
                    if (cur->getHand().getCard(i).getValue() == call->getValue())
                        defense.push_back(i);
            if (!defense.empty() && strategy_->shouldDefend(state, *call, defense)) {
                action.type = game::ActionType::PLAY_CARD;
                action.cardIndex = defense[whot::utils::Random::getInstance().nextInt(0, static_cast<int>(defense.size()) - 1)];
                return action;
            }
        }
    }

    return action;
}

std::optional<size_t> AIPlayer::chooseCardToPlay(const game::GameState& state) {
    std::vector<size_t> playable = getPlayableCards(state);
    if (playable.empty()) return std::nullopt;
    return selectBestCard(state, playable);
}

core::Suit AIPlayer::chooseSuitForWhotCard(const game::GameState& state) {
    return strategy_->selectSuit(state);
}

bool AIPlayer::shouldDefendAgainstPick(const game::GameState& state, const core::Card& attackCard) {
    const core::Player* cur = state.getCurrentPlayer();
    if (!cur) return false;
    std::vector<size_t> defense;
    for (size_t i = 0; i < cur->getHand().size(); ++i)
        if (cur->getHand().getCard(i).getValue() == attackCard.getValue())
            defense.push_back(i);
    return strategy_->shouldDefend(state, attackCard, defense);
}

void AIPlayer::setDifficulty(DifficultyLevel level) {
    difficulty_ = level;
    strategy_ = DifficultyConfig::createStrategy(level);
    thinkingDelay_ = DifficultyConfig::getThinkingDelay(level);
}

DifficultyLevel AIPlayer::getDifficulty() const { return difficulty_; }

void AIPlayer::setStrategy(std::unique_ptr<Strategy> strategy) {
    strategy_ = std::move(strategy);
}

void AIPlayer::setThinkingDelay(int milliseconds) {
    thinkingDelay_ = milliseconds;
}

std::vector<size_t> AIPlayer::getPlayableCards(const game::GameState& state) const {
    const core::Card* call = state.getCallCard();
    const core::Player* cur = state.getCurrentPlayer();
    if (!call || !cur || cur->getId() != id_) return {};
    return cur->getHand().getPlayableCardIndices(*call);
}

size_t AIPlayer::selectBestCard(const game::GameState& state,
                                const std::vector<size_t>& playableIndices) {
    if (playableIndices.empty()) return 0;
    if (!strategy_) return playableIndices[0];
    return strategy_->selectCard(state, playableIndices);
}

void AIPlayer::applyRandomness(std::vector<size_t>& choices) {
    if (choices.size() <= 1) return;
    double factor = DifficultyConfig::getRandomnessFactor(difficulty_);
    if (whot::utils::Random::getInstance().nextBool(1.0 - factor))
        return;
    size_t i = whot::utils::Random::getInstance().nextInt(0, static_cast<int>(choices.size()) - 1);
    std::swap(choices[0], choices[i]);
}

} // namespace whot::ai
