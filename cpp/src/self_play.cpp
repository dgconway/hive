#include "self_play.hpp"
#include <iostream>
#include <algorithm>

namespace bugs {

// TunableMinimaxAI implementation
TunableMinimaxAI::TunableMinimaxAI(int depth, const EvalWeights& weights)
    : depth_(depth), evaluator_(weights) {}

std::optional<MoveRequest> TunableMinimaxAI::get_best_move(const Game& game) {
    GameState state(game);
    PlayerColor player = game.current_turn;
    
    auto [score, best_action] = minimax(
        state, depth_,
        -std::numeric_limits<float>::infinity(),
        std::numeric_limits<float>::infinity(),
        true, player
    );
    
    return best_action.has_value() ? std::make_optional(best_action->to_move_request()) : std::nullopt;
}

std::pair<float, std::optional<Action>> TunableMinimaxAI::minimax(
    const GameState& state,
    int depth,
    float alpha,
    float beta,
    bool is_maximizing,
    PlayerColor player) {
    
    // Terminal or depth limit
    if (depth == 0 || state.is_terminal()) {
        float score = evaluator_.evaluate(state.game, player, engine_);
        return {score, std::nullopt};
    }
    
    // Get legal actions
    std::vector<Action> legal_actions = interface_.get_legal_actions(state);
    
    if (legal_actions.empty()) {
        float score = evaluator_.evaluate(state.game, player, engine_);
        return {score, std::nullopt};
    }
    
    std::optional<Action> best_action;
    
    if (is_maximizing) {
        float curr_max = -std::numeric_limits<float>::infinity();
        for (const auto& action : legal_actions) {
            GameState new_state = interface_.apply_action(state, action);
            auto [eval_val, _] = minimax(new_state, depth - 1, alpha, beta, false, player);
            
            if (eval_val > curr_max) {
                curr_max = eval_val;
                best_action = action;
            }
            alpha = std::max(alpha, eval_val);
            if (beta <= alpha) break;
        }
        return {curr_max, best_action};
    } else {
        float curr_min = std::numeric_limits<float>::infinity();
        for (const auto& action : legal_actions) {
            GameState new_state = interface_.apply_action(state, action);
            auto [eval_val, _] = minimax(new_state, depth - 1, alpha, beta, true, player);
            
            if (eval_val < curr_min) {
                curr_min = eval_val;
                best_action = action;
            }
            beta = std::min(beta, eval_val);
            if (beta <= alpha) break;
        }
        return {curr_min, best_action};
    }
}

// SelfPlayEngine implementation
SelfPlayEngine::SelfPlayEngine() : config_() {}

SelfPlayEngine::SelfPlayEngine(const SelfPlayConfig& config) : config_(config) {}

void SelfPlayEngine::report_progress(int current, int total, const std::string& message) {
    if (progress_callback_) {
        progress_callback_(current, total, message);
    }
}

GameResult SelfPlayEngine::run_game(const EvalWeights& white_weights, const EvalWeights& black_weights) {
    GameResult result;
    result.was_draw = false;
    result.total_moves = 0;
    
    // Create game
    Game game = engine_.create_game();
    
    // Create AIs
    TunableMinimaxAI white_ai(config_.ai_depth, white_weights);
    TunableMinimaxAI black_ai(config_.ai_depth, black_weights);
    
    // Play game
    while (game.status == GameStatus::IN_PROGRESS && result.total_moves < config_.max_moves) {
        if (should_stop_) break;
        
        TunableMinimaxAI& current_ai = (game.current_turn == PlayerColor::WHITE) ? white_ai : black_ai;
        
        auto move_opt = current_ai.get_best_move(game);
        
        if (!move_opt.has_value()) {
            // No legal moves - game over or error
            if (config_.verbose) {
                std::cout << "No legal moves for " << to_string(game.current_turn) << std::endl;
            }
            break;
        }
        
        try {
            game = engine_.process_move(game.game_id, move_opt.value());
            result.total_moves++;
            
            if (config_.verbose && result.total_moves % 10 == 0) {
                std::cout << "Move " << result.total_moves << std::endl;
            }
        } catch (const std::exception& e) {
            if (config_.verbose) {
                std::cout << "Error processing move: " << e.what() << std::endl;
            }
            break;
        }
    }
    
    // Record result
    result.winner = game.winner;
    result.was_draw = (game.status == GameStatus::FINISHED && !game.winner.has_value()) ||
                      result.total_moves >= config_.max_moves;
    result.move_history = game.history;
    
    // Final evaluations
    TunableEvaluator white_eval(white_weights);
    TunableEvaluator black_eval(black_weights);
    result.white_final_eval = white_eval.evaluate(game, PlayerColor::WHITE, engine_);
    result.black_final_eval = black_eval.evaluate(game, PlayerColor::BLACK, engine_);
    
    return result;
}

std::vector<GameResult> SelfPlayEngine::run_tournament(
    const EvalWeights& weights_a, 
    const EvalWeights& weights_b,
    int num_games) {
    
    std::vector<GameResult> results;
    results.reserve(num_games);
    
    for (int i = 0; i < num_games && !should_stop_; i++) {
        // Alternate colors
        bool a_is_white = (i % 2 == 0);
        
        const EvalWeights& white_weights = a_is_white ? weights_a : weights_b;
        const EvalWeights& black_weights = a_is_white ? weights_b : weights_a;
        
        report_progress(i + 1, num_games, "Game " + std::to_string(i + 1) + "/" + std::to_string(num_games));
        
        GameResult result = run_game(white_weights, black_weights);
        results.push_back(result);
        
        if (config_.verbose) {
            std::string winner_str = result.was_draw ? "Draw" : 
                (result.winner.has_value() ? to_string(result.winner.value()) : "Unknown");
            std::cout << "Game " << (i + 1) << ": " << winner_str 
                      << " in " << result.total_moves << " moves" << std::endl;
        }
    }
    
    return results;
}

float SelfPlayEngine::evaluate_matchup(
    const EvalWeights& weights_a, 
    const EvalWeights& weights_b,
    int num_games) {
    
    auto results = run_tournament(weights_a, weights_b, num_games);
    
    float total_score = 0.0f;
    for (size_t i = 0; i < results.size(); i++) {
        bool a_was_white = (i % 2 == 0);
        PlayerColor a_color = a_was_white ? PlayerColor::WHITE : PlayerColor::BLACK;
        total_score += results[i].get_score(a_color);
    }
    
    return total_score / static_cast<float>(results.size());
}

} // namespace bugs
