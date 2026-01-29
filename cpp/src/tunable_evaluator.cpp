#include "tunable_evaluator.hpp"
#include <cmath>
#include <algorithm>

namespace bugs {

float TunableEvaluator::evaluate(const Game& game, PlayerColor player, GameEngine& engine) const {
    if (game.status == GameStatus::FINISHED) {
        if (game.winner == player) {
            return 1000000.0f;
        } else if (!game.winner.has_value()) {
            return 0.0f;
        } else {
            return -1000000.0f;
        }
    }
    
    float score = 0.0f;
    
    // Pre-calculate occupied hexes and queen positions
    std::optional<Hex> player_queen_pos;
    std::optional<Hex> opponent_queen_pos;
    std::unordered_set<Hex, HexHash> occupied_hexes;
    
    for (const auto& [key, stack] : game.board) {
        if (!stack.empty()) {
            Hex pos = key_to_coord(key);
            occupied_hexes.insert(pos);
            const Piece& top_piece = stack.back();
            if (top_piece.type == PieceType::QUEEN) {
                if (top_piece.color == player) {
                    player_queen_pos = pos;
                } else {
                    opponent_queen_pos = pos;
                }
            }
        }
    }
    
    // Queen surroundings
    if (opponent_queen_pos.has_value()) {
        auto opp_queen_neighbors = get_neighbors(opponent_queen_pos.value());
        int occupied_opp_neighbors = 0;
        for (const auto& n : opp_queen_neighbors) {
            if (occupied_hexes.count(n)) {
                occupied_opp_neighbors++;
            }
        }
        const float surround_bonus[] = {0, 5, 15, 40, 100, 300, 1000};
        score += surround_bonus[occupied_opp_neighbors] * weights_.surround_opponent_multiplier;
    }
    
    if (player_queen_pos.has_value()) {
        auto play_queen_neighbors = get_neighbors(player_queen_pos.value());
        int occupied_play_neighbors = 0;
        for (const auto& n : play_queen_neighbors) {
            if (occupied_hexes.count(n)) {
                occupied_play_neighbors++;
            }
        }
        const float surround_penalty[] = {0, 5, 15, 40, 100, 300, 1000};
        score -= surround_penalty[occupied_play_neighbors] * weights_.surround_self_multiplier;
    }
    
    // Material and mobility
    int player_mobility = 0;
    int opponent_mobility = 0;
    std::unordered_set<Hex, HexHash> opponent_queen_neighbors_set;
    if (opponent_queen_pos.has_value()) {
        auto neighbors = get_neighbors(opponent_queen_pos.value());
        opponent_queen_neighbors_set.insert(neighbors.begin(), neighbors.end());
    }
    
    // Save original turn
    PlayerColor original_turn = game.current_turn;
    Game mutable_game = game;
    
    for (const auto& [key, stack] : game.board) {
        if (stack.empty()) continue;
        const Piece& top_piece = stack.back();
        Hex pos = key_to_coord(key);
        
        // Material value
        float val = weights_.get_piece_value(top_piece.type);
        if (top_piece.color == player) {
            score += val;
        } else {
            score -= val;
        }
        
        // Mobility
        mutable_game.current_turn = top_piece.color;
        std::vector<Hex> moves;
        try {
            moves = engine.get_valid_moves_for_piece(mutable_game, pos, occupied_hexes);
        } catch (...) {
            moves.clear();
        }
        int num_moves = static_cast<int>(moves.size());
        
        if (top_piece.color == player) {
            player_mobility += num_moves;
            if (opponent_queen_pos.has_value()) {
                int dist = hex_distance(pos, opponent_queen_pos.value());
                if (dist <= static_cast<int>(weights_.proximity_max_distance)) {
                    score += (weights_.proximity_max_distance + 2 - dist) * weights_.proximity_weight;
                }
            }
            if (top_piece.type == PieceType::ANT) {
                int non_surrounding_moves = 0;
                for (const auto& m : moves) {
                    if (!opponent_queen_neighbors_set.count(m)) {
                        non_surrounding_moves++;
                    }
                }
                if (non_surrounding_moves >= 3) {
                    score += weights_.ant_freedom_bonus;
                }
                
                if (moves.empty() && !opponent_queen_neighbors_set.count(pos)) {
                    score -= weights_.ant_trapped_penalty;
                }
            }
        } else {
            opponent_mobility += num_moves;
            if (top_piece.type == PieceType::ANT && num_moves == 0) {
                score += weights_.trapped_opponent_ant_bonus;
            }
        }
    }
    
    // Restore original turn
    mutable_game.current_turn = original_turn;
    
    // Hand material weighting
    for (const auto& [ptype, count] : game.white_pieces_hand) {
        float val = weights_.get_piece_value(ptype) * weights_.hand_piece_multiplier * count;
        if (player == PlayerColor::WHITE) {
            score += val;
        } else {
            score -= val;
        }
    }
    
    for (const auto& [ptype, count] : game.black_pieces_hand) {
        float val = weights_.get_piece_value(ptype) * weights_.hand_piece_multiplier * count;
        if (player == PlayerColor::BLACK) {
            score += val;
        } else {
            score -= val;
        }
    }
    
    score += (player_mobility - opponent_mobility) * weights_.mobility_weight;
    return score;
}

} // namespace bugs
