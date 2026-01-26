#pragma once

#include "models.hpp"
#include "game_interface.hpp"
#include "evaluator.hpp"
#include <unordered_map>
#include <string>
#include <optional>
#include <memory>

namespace bugs {

class MinimaxAI {
public:
    explicit MinimaxAI(int depth = 4);
    
    std::optional<MoveRequest> get_best_move(const Game& game);
    
private:
    int depth_;
    GameInterface interface_;
    GameEngine engine_;
    std::unordered_map<std::string, float> transposition_table_;
    
    std::string get_state_key(const GameState& state);
    
    std::pair<float, std::optional<Action>> minimax(
        const GameState& state,
        int depth,
        float alpha,
        float beta,
        bool is_maximizing,
        PlayerColor player
    );
    
    float score_action(const Action& action, const GameState& state, PlayerColor player);
};

} // namespace bugs
