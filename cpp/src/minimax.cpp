#include "minimax.hpp"
#include <algorithm>
#include <sstream>
#include <iostream>
#include <chrono>
#include <omp.h>

namespace bugs {

MinimaxAI::MinimaxAI(int depth) : depth_(depth) {}

std::string MinimaxAI::get_state_key(const GameState& state) {
    std::stringstream ss;
    
    // Sort board keys for consistent hashing
    std::vector<std::string> sorted_keys;
    for (const auto& [key, stack] : state.game.board) {
        if (!stack.empty()) {
            sorted_keys.push_back(key);
        }
    }
    std::sort(sorted_keys.begin(), sorted_keys.end());
    
    for (const auto& key : sorted_keys) {
        const auto& stack = state.game.board.at(key);
        ss << key << ":";
        for (const auto& p : stack) {
            ss << to_string(p.type)[0] << to_string(p.color)[0];
        }
        ss << "|";
    }
    
    ss << "T:" << to_string(state.game.current_turn)[0];
    
    // Include hands
    ss << "|W:";
    for (const auto& [type, count] : state.game.white_pieces_hand) {
        ss << to_string(type)[0] << count;
    }
    ss << "|B:";
    for (const auto& [type, count] : state.game.black_pieces_hand) {
        ss << to_string(type)[0] << count;
    }
    
    return ss.str();
}

float MinimaxAI::score_action(const Action& action, const GameState& state, PlayerColor player) {
    float score = 0.0f;
    if (action.action_type == ActionType::PLACE) {
        if (action.piece_type == PieceType::QUEEN) score += 1000.0f;
        
        // Discourage Ants in early game - they get trapped easily
        if (action.piece_type == PieceType::ANT) {
            // Only prioritize Ants after turn 6 (when more pieces are on board)
            if (state.game.turn_number >= 6) {
                score += 40.0f;
            } else {
                score += 5.0f; // Low priority early game
            }
        }
        
        // Prefer Grasshoppers and Beetles early
        if (action.piece_type == PieceType::GRASSHOPPER) score += 30.0f;
        if (action.piece_type == PieceType::BEETLE) score += 25.0f;
        if (action.piece_type == PieceType::SPIDER) score += 15.0f;
    } else if (action.action_type == ActionType::MOVE) {
        score += 50.0f;
    }
    return score;
}

std::optional<MoveRequest> MinimaxAI::get_best_move(const Game& game) {
    GameState state(game);
    PlayerColor player = game.current_turn;
    
    // === OPENING BOOK ===
    // Count how many pieces the AI has played
    int ai_pieces_played = 0;
    for (const auto& [key, stack] : game.board) {
        for (const auto& piece : stack) {
            if (piece.color == player) {
                ai_pieces_played++;
            }
        }
    }
    
    // First move: Play Grasshopper (good mobility, hard to trap)
    if (ai_pieces_played == 0) {
        auto legal_actions = interface_.get_legal_actions(state);
        for (const auto& action : legal_actions) {
            if (action.action_type == ActionType::PLACE && 
                action.piece_type == PieceType::GRASSHOPPER) {
                std::cout << "Opening Book: Playing GRASSHOPPER" << std::endl;
                return action.to_move_request();
            }
        }
    }
    
    // Second move: Play Queen Bee (required by turn 4, better to play early)
    if (ai_pieces_played == 1) {
        auto legal_actions = interface_.get_legal_actions(state);
        for (const auto& action : legal_actions) {
            if (action.action_type == ActionType::PLACE && 
                action.piece_type == PieceType::QUEEN) {
                std::cout << "Opening Book: Playing QUEEN" << std::endl;
                return action.to_move_request();
            }
        }
    }
    
    // === STANDARD MINIMAX SEARCH ===
    auto legal_actions = interface_.get_legal_actions(state);
    
    std::cout << "Legal actions found: " << legal_actions.size() << std::endl;
    
    if (legal_actions.empty()) {
        std::cout << "No legal actions available!" << std::endl;
        return std::nullopt;
    }
    
    // Sort actions for better pruning
    std::sort(legal_actions.begin(), legal_actions.end(),
        [this, &state, player](const Action& a, const Action& b) {
            return score_action(a, state, player) > score_action(b, state, player);
        });
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    std::optional<Action> best_action;
    float max_eval = -std::numeric_limits<float>::infinity();
    
    // Use serial search to avoid thread safety issues
    // OpenMP parallelization was causing crashes due to shared state
    auto [score, action] = minimax(state, depth_, 
        -std::numeric_limits<float>::infinity(),
        std::numeric_limits<float>::infinity(),
        true, player);
    best_action = action;
    max_eval = score;
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "Minimax complete. Score: " << max_eval 
              << ", Time: " << duration.count() / 1000.0 << "s" << std::endl;
    
    return best_action.has_value() ? std::make_optional(best_action->to_move_request()) : std::nullopt;
}

std::pair<float, std::optional<Action>> MinimaxAI::minimax(
    const GameState& state,
    int depth,
    float alpha,
    float beta,
    bool is_maximizing,
    PlayerColor player) {
    
    // Don't use transposition table at root level (we need the action!)
    // Only use it for deeper levels where we just need the score
    std::string state_key = get_state_key(state);
    if (depth < depth_ && transposition_table_.count(state_key)) {
        return {transposition_table_[state_key], std::nullopt};
    }
    
    if (depth == 0 || state.is_terminal()) {
        float score = evaluate_state(state.game, player, engine_);
        transposition_table_[state_key] = score;
        return {score, std::nullopt};
    }
    
    auto legal_actions = interface_.get_legal_actions(state);
    if (legal_actions.empty()) {
        float score = evaluate_state(state.game, player, engine_);
        return {score, std::nullopt};
    }
    
    // Sort actions for better pruning
    std::sort(legal_actions.begin(), legal_actions.end(),
        [this, &state, player](const Action& a, const Action& b) {
            return score_action(a, state, player) > score_action(b, state, player);
        });
    
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
            if (beta <= alpha) break; // Beta cutoff
        }
        transposition_table_[state_key] = curr_max;
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
            if (beta <= alpha) break; // Alpha cutoff
        }
        transposition_table_[state_key] = curr_min;
        return {curr_min, best_action};
    }
}

} // namespace bugs
