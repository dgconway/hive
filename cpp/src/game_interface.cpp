#include "game_interface.hpp"
#include "benchmark.hpp"
#include <algorithm>

namespace bugs {

// Action implementation
MoveRequest Action::to_move_request() const {
    MoveRequest move;
    move.action = action_type;
    move.piece_type = piece_type;
    move.from_hex = from_hex;
    move.to_hex = to_hex;
    move.to_hex = to_hex;
    
    // Safety check: if action is PLACE, piece_type must be set
    if (move.action == ActionType::PLACE && !move.piece_type.has_value()) {
        // This shouldn't happen with correct logic, but let's be safe
        move.piece_type = PieceType::ANT; // Fallback to avoid crash, but indicates logic error
    }
    return move;
}

// GameState implementation
GameState::GameState(const Game& g) : game(g) {
    // Deep copy board
    // for (auto& [key, stack] : game.board) {
    //     // Vectors are already copied by default copy constructor
    // }
}

GameState::GameState(const GameState& other) : game(other.game) {}

GameState& GameState::operator=(const GameState& other) {
    if (this != &other) {
        game = other.game;
    }
    return *this;
}

float GameState::get_reward(PlayerColor player) const {
    if (!is_terminal()) {
        return 0.0f;
    }
    if (!winner().has_value()) {
        return 0.0f; // Draw
    }
    return winner().value() == player ? 1.0f : -1.0f;
}

// GameInterface implementation
GameInterface::GameInterface() {}

GameState GameInterface::get_initial_state() {
    return GameState(engine_.create_game());
}

std::vector<Action> GameInterface::get_legal_actions(const GameState& state) {
    std::vector<Action> actions;
    actions.reserve(30);
    const Game& game = state.game;
    
    if (game.status == GameStatus::FINISHED) {
        return actions;
    }
    
    auto& hand = (game.current_turn == PlayerColor::WHITE) 
        ? game.white_pieces_hand : game.black_pieces_hand;
    
    // Check if queen must be placed
    bool is_fourth_turn = (game.current_turn == PlayerColor::WHITE && game.turn_number == 7) ||
                         (game.current_turn == PlayerColor::BLACK && game.turn_number == 8);
    bool must_place_queen = is_fourth_turn && hand.at(PieceType::QUEEN) == 1;
    
    // Check if queen is placed
    bool queen_placed = hand.at(PieceType::QUEEN) == 0;
    
    // Get placement actions
    auto placement_hexes = get_valid_placement_hexes(game);
    
    if (must_place_queen) {
        for (const auto& hex_pos : placement_hexes) {
            Action action(ActionType::PLACE, hex_pos);
            action.piece_type = PieceType::QUEEN;
            actions.emplace_back(action);
        }
    } else {
        // Can place any available piece
        for (const auto& [piece_type, count] : hand) {
            if (count > 0) {
                for (const auto& hex_pos : placement_hexes) {
                    Action action(ActionType::PLACE, hex_pos);
                    action.piece_type = piece_type;
                    actions.emplace_back(action);
                }
            }
        }
        
        // Get move actions (only if queen is placed)
        if (queen_placed) {
            auto all_moves = get_all_valid_moves(game);
            for (const auto& [from_hex, destinations] : all_moves) {
                for (const auto& to_hex : destinations) {
                    Action action(ActionType::MOVE, to_hex);
                    action.from_hex = from_hex;
                    
                    // Populate piece type for move
                    if (game.board.count(from_hex) && !game.board.at(from_hex).empty()) {
                        action.piece_type = game.board.at(from_hex).back().type;
                    }
                    
                    actions.emplace_back(action);
                }
            }
        }
    }
    
    return actions;
}

std::vector<Hex> GameInterface::get_valid_placement_hexes(const Game& game) {
    std::vector<Hex> valid_hexes;
    valid_hexes.reserve(30);
    
    // First move: place at origin
    if (game.board.empty()) {
        return {{0, 0}};
    }
    
    // Second move: place adjacent to any piece
    if (game.turn_number == 2) {
        std::unordered_set<Hex, HexHash> occupied;
        for (const auto& [pos, stack] : game.board) {
            occupied.insert(pos);
        }
        
        std::unordered_set<Hex, HexHash> candidates;
        for (const auto& pos : occupied) {
            for (const auto& n : get_neighbors(pos)) {
                if (!occupied.count(n)) {
                    candidates.insert(n);
                }
            }
        }
        return std::vector<Hex>(candidates.begin(), candidates.end());
    }
    
    // General case: touch own, not opponent
    std::unordered_map<Hex, PlayerColor, HexHash> occupied;
    for (const auto& [pos, stack] : game.board) {
        if (!stack.empty()) {
            occupied[pos] = stack.back().color;
        }
    }
    
    std::unordered_set<Hex, HexHash> candidates;
    for (const auto& [pos, color] : occupied) {
        if (color == game.current_turn) {
            for (const auto& n : get_neighbors(pos)) {
                if (!occupied.count(n)) {
                    candidates.insert(n);
                }
            }
        }
    }
    
    // Filter: must not touch opponent
    std::vector<Hex> valid;
    for (const auto& pos : candidates) {
        bool touches_opponent = false;
        for (const auto& n : get_neighbors(pos)) {
            if (occupied.count(n) && occupied[n] != game.current_turn) {
                touches_opponent = true;
                break;
            }
        }
        if (!touches_opponent) {
            valid.push_back(pos);
        }
    }
    
    return valid;
}

std::vector<std::pair<Hex, std::vector<Hex>>> GameInterface::get_all_valid_moves(const Game& game) {
    std::vector<std::pair<Hex, std::vector<Hex>>> moves;
    
    // Pre-calculate occupied hexes once
    auto occupied = engine_.get_occupied_hexes(game.board);
    
    for (const auto& [pos, stack] : game.board) {
        if (stack.empty()) continue;
        
        const Piece& top_piece = stack.back();
        if (top_piece.color != game.current_turn) continue;
        
        Hex pos = key_to_coord(key);
        auto valid_destinations = engine_.get_valid_moves_for_piece(game, pos, occupied);
        
        if (!valid_destinations.empty()) {
            moves.emplace_back(pos, valid_destinations);
        }
    }
    
    return moves;
}

GameState GameInterface::apply_action(const GameState& state, const Action& action) {
    GameState new_state(state);
    
    // Process move directly on the game copy, no map lookup needed
    engine_.process_move_inplace(new_state.game, action.to_move_request());
    
    return new_state;
}

} // namespace bugs
