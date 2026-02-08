#include "evaluator.hpp"
#include "game_logic.hpp"
#include <cmath>
#include <algorithm>

namespace bugs {

float evaluate_state(const Game& game, PlayerColor player, GameEngine& engine) {
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
        score += surround_bonus[occupied_opp_neighbors] * 2.0f;
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
        score -= surround_penalty[occupied_play_neighbors] * 5.0f;
    }
    
    // Material and mobility
    size_t player_mobility = 0;
    size_t opponent_mobility = 0;
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
        float val = 0.0f;
        auto it = PIECE_VALUES.find(top_piece.type);
        if (it != PIECE_VALUES.end()) {
            val = it->second;
        }
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
        size_t num_moves = moves.size();
        
        if (top_piece.color == player) {
            player_mobility += num_moves;
            if (opponent_queen_pos.has_value()) {
                int dist = hex_distance(pos, opponent_queen_pos.value());
                if (dist <= 3) {
                    score += (5 - dist) * 10.0f;
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
                    score += 20.0f;
                }
                
                // Penalty for trapped ants not surrounding opponent queen
                if (moves.empty() && !opponent_queen_neighbors_set.count(pos)) {
                    score -= 15.0f;
                }
            }
        } else {
            opponent_mobility += num_moves;
            if (top_piece.type == PieceType::ANT && num_moves == 0) {
                score += 30.0f;
            }
        }
    }
    
    // Restore original turn
    mutable_game.current_turn = original_turn;
    
    // Hand material weighting
    for (const auto& [ptype, count] : game.white_pieces_hand) {
        float val = 0.0f;
        auto it = PIECE_VALUES.find(ptype);
        if (it != PIECE_VALUES.end()) {
            val = it->second * 0.5f * count;
        }
        if (player == PlayerColor::WHITE) {
            score += val;
        } else {
            score -= val;
        }
    }
    
    for (const auto& [ptype, count] : game.black_pieces_hand) {
        float val = 0.0f;
        auto it = PIECE_VALUES.find(ptype);
        if (it != PIECE_VALUES.end()) {
            val = it->second * 0.5f * count;
        }
        if (player == PlayerColor::BLACK) {
            score += val;
        } else {
            score -= val;
        }
    }
    
    score += (static_cast<float>(player_mobility) - static_cast<float>(opponent_mobility)) * 2.0f;
    return score;
}

} // namespace bugs
