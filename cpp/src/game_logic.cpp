#include "game_logic.hpp"
#include <algorithm>
#include <queue>
#include <random>
#include <sstream>
#include <stdexcept>

namespace bugs {

namespace {
    // UUID generation helper
    std::string generate_uuid() {
        thread_local static std::random_device rd;
        thread_local static std::mt19937 gen(rd());
        thread_local static std::uniform_int_distribution<> dis(0, 15);
        thread_local static std::uniform_int_distribution<> dis2(8, 11);
        
        std::stringstream ss;
        int i;
        ss << std::hex;
        for (i = 0; i < 8; i++) ss << dis(gen);
        ss << "-";
        for (i = 0; i < 4; i++) ss << dis(gen);
        ss << "-4";
        for (i = 0; i < 3; i++) ss << dis(gen);
        ss << "-";
        ss << dis2(gen);
        for (i = 0; i < 3; i++) ss << dis(gen);
        ss << "-";
        for (i = 0; i < 12; i++) ss << dis(gen);
        return ss.str();
    }
}

// Game management
Game GameEngine::create_game(bool advanced_mode) {
    Game game;
    game.game_id = generate_uuid();
    game.current_turn = PlayerColor::WHITE;
    game.turn_number = 1;
    game.advanced_mode = advanced_mode;
    if (advanced_mode) {
        game.white_pieces_hand = create_advanced_hand();
        game.black_pieces_hand = create_advanced_hand();
    } else {
        game.white_pieces_hand = create_initial_hand();
        game.black_pieces_hand = create_initial_hand();
    }
    game.status = GameStatus::IN_PROGRESS;
    
    games_[game.game_id] = game;
    return game;
}

std::optional<Game> GameEngine::get_game(const std::string& game_id) {
    auto it = games_.find(game_id);
    if (it != games_.end()) {
        return it->second;
    }
    return std::nullopt;
}

Game GameEngine::process_move(const std::string& game_id, const MoveRequest& move) {
    auto it = games_.find(game_id);
    if (it == games_.end()) {
        throw std::runtime_error("Game not found");
    }
    
    Game& game = it->second;
    
    if (game.status == GameStatus::FINISHED) {
        throw std::runtime_error("Game is finished");
    }
    
    validate_turn(game, move);
    
    // Create log entry
    MoveLog log;
    log.move = move;
    log.player = game.current_turn;
    log.turn_number = game.turn_number;
    
    // Fill in piece type for moves if missing (for the log)
    if (move.action == ActionType::MOVE && !log.move.piece_type.has_value()) {
        if (move.from_hex.has_value()) {
            Hex from = move.from_hex.value();
            if (game.board.count(from) && !game.board.at(from).empty()) {
                log.move.piece_type = game.board.at(from).back().type;
            }
        }
    }
    
    if (move.action == ActionType::PLACE) {
        execute_place(game, move);
    } else if (move.action == ActionType::SPECIAL) {
        execute_special(game, move);
    } else {
        execute_move(game, move);
    }
    
    game.history.push_back(log);
    
    check_win_condition(game);
    
    // Set tracking for next turn BEFORE switching
    // last_moved_to: where the piece ended up (for pillbug can't-throw-last-moved rule)
    if (move.action == ActionType::MOVE) {
        game.last_moved_to = move.to_hex;
    } else {
        game.last_moved_to = std::nullopt;
    }
    
    // pillbug_frozen_hex: set by execute_special, persists for opponent's turn
    // (already set in execute_special if applicable)
    
    // Switch turn
    game.current_turn = (game.current_turn == PlayerColor::WHITE) 
        ? PlayerColor::BLACK : PlayerColor::WHITE;
    game.turn_number++;
    
    return game;
}

void GameEngine::process_move_inplace(Game& game, const MoveRequest& move) {
    if (game.status == GameStatus::FINISHED) {
        throw std::runtime_error("Game is finished");
    }
    
    validate_turn(game, move);
    
    // Create log entry
    MoveLog log;
    log.move = move;
    log.player = game.current_turn;
    log.turn_number = game.turn_number;
    
    // Fill in piece type for moves if missing (for the log)
    if (move.action == ActionType::MOVE && !log.move.piece_type.has_value()) {
        if (move.from_hex.has_value()) {
            Hex from = move.from_hex.value();
            if (game.board.count(from) && !game.board.at(from).empty()) {
                log.move.piece_type = game.board.at(from).back().type;
            }
        }
    }
    
    if (move.action == ActionType::PLACE) {
        execute_place(game, move);
    } else if (move.action == ActionType::SPECIAL) {
        execute_special(game, move);
    } else {
        execute_move(game, move);
    }
    
    game.history.push_back(log);
    
    check_win_condition(game);
    
    // Set tracking for next turn
    if (move.action == ActionType::MOVE) {
        game.last_moved_to = move.to_hex;
    } else {
        game.last_moved_to = std::nullopt;
    }
    
    // Switch turn
    game.current_turn = (game.current_turn == PlayerColor::WHITE) 
        ? PlayerColor::BLACK : PlayerColor::WHITE;
    game.turn_number++;
}

// Validation
void GameEngine::validate_turn(const Game& game, const MoveRequest& move) {
    auto& hand = (game.current_turn == PlayerColor::WHITE) 
        ? game.white_pieces_hand : game.black_pieces_hand;
    
    if (move.action == ActionType::MOVE) {
        if (hand.at(PieceType::QUEEN) == 1) {
            throw std::runtime_error("Must place Queen Bee before moving pieces");
        }
    } else if (move.action == ActionType::PLACE) {
        bool is_fourth_turn = (game.current_turn == PlayerColor::WHITE && game.turn_number == 7) ||
                             (game.current_turn == PlayerColor::BLACK && game.turn_number == 8);
        
        if (is_fourth_turn && hand.at(PieceType::QUEEN) == 1 && 
            move.piece_type.value_or(PieceType::QUEEN) != PieceType::QUEEN) {
            throw std::runtime_error("Rules require placing Queen Bee by the 4th turn");
        }
    }
}

// Execution - Place
void GameEngine::execute_place(Game& game, const MoveRequest& move) {
    if (!move.piece_type.has_value()) {
        throw std::runtime_error("Piece type required for placement");
    }
    
    auto& hand = (game.current_turn == PlayerColor::WHITE) 
        ? game.white_pieces_hand : game.black_pieces_hand;
    
    if (hand[move.piece_type.value()] <= 0) {
        throw std::runtime_error("No " + to_string(move.piece_type.value()) + " remaining in hand");
    }
    
    if (game.board.count(move.to_hex) && !game.board.at(move.to_hex).empty()) {
        throw std::runtime_error("Cannot place on occupied tile");
    }
    
    // Placement rules
    if (game.board.empty()) {
        // First piece ever
    } else if (game.turn_number == 2) {
        // Second piece ever - must touch existing piece
        auto neighbors = get_neighbors(move.to_hex);
        bool has_neighbor = false;
        for (const auto& n : neighbors) {
            if (game.board.count(n)) {
                has_neighbor = true;
                break;
            }
        }
        if (!has_neighbor) {
            throw std::runtime_error("Must place next to existing hive");
        }
    } else {
        // General rule: must touch own color, must NOT touch opponent
        auto neighbors = get_neighbors(move.to_hex);
        bool touching_own = false;
        bool touching_opponent = false;
        
        for (const auto& n : neighbors) {
            if (game.board.count(n) && !game.board.at(n).empty()) {
                const Piece& top_piece = game.board.at(n).back();
                if (top_piece.color == game.current_turn) {
                    touching_own = true;
                } else {
                    touching_opponent = true;
                }
            }
        }
        
        if (!touching_own) {
            throw std::runtime_error("New placements must touch your own color");
        }
        if (touching_opponent) {
            throw std::runtime_error("New placements cannot touch opponent pieces");
        }
    }
    
    // Create piece
    Piece new_piece;
    new_piece.type = move.piece_type.value();
    new_piece.color = game.current_turn;
    new_piece.id = generate_uuid();
    
    game.board[move.to_hex] = {new_piece};
    hand[move.piece_type.value()]--;
}

// Execution - Move
void GameEngine::execute_move(Game& game, const MoveRequest& move) {
    if (!move.from_hex.has_value()) {
        throw std::runtime_error("Origin required for move");
    }
    
    if (!game.board.count(move.from_hex.value()) || game.board.at(move.from_hex.value()).empty()) {
        throw std::runtime_error("No piece at origin");
    }
    
    Piece piece_to_move = game.board.at(move.from_hex.value()).back();
    
    if (piece_to_move.color != game.current_turn) {
        throw std::runtime_error("Cannot move opponent's piece");
    }
    
    // One Hive Rule
    size_t stack_height = game.board.at(move.from_hex.value()).size();
    auto occupied = get_occupied_hexes(game.board);
    
    auto future_occupied = occupied;
    if (stack_height == 1) {
        future_occupied.erase(move.from_hex.value());
    }
    
    if (!is_connected(future_occupied)) {
        throw std::runtime_error("Move violates One Hive Rule (disconnects hive)");
    }
    
    future_occupied.insert(move.to_hex);
    if (!is_connected(future_occupied)) {
        throw std::runtime_error("Move violates One Hive Rule (destination disconnected)");
    }
    
    if (move.from_hex.value() == move.to_hex) {
        throw std::runtime_error("Cannot move to same position");
    }
    
    // Piece-specific validation
    bool valid = false;
    switch (piece_to_move.type) {
        case PieceType::QUEEN:
            valid = validate_queen_move(move.from_hex.value(), move.to_hex, occupied);
            break;
        case PieceType::BEETLE:
            valid = validate_beetle_move(move.from_hex.value(), move.to_hex);
            break;
        case PieceType::GRASSHOPPER:
            valid = validate_grasshopper_move(move.from_hex.value(), move.to_hex, occupied);
            break;
        case PieceType::SPIDER:
            valid = validate_spider_move(move.from_hex.value(), move.to_hex, occupied);
            break;
        case PieceType::ANT:
            valid = validate_ant_move(move.from_hex.value(), move.to_hex, occupied);
            break;
        case PieceType::LADYBUG:
            valid = validate_ladybug_move(game, move.from_hex.value(), move.to_hex, occupied);
            break;
        case PieceType::MOSQUITO:
            valid = validate_mosquito_move(game, move.from_hex.value(), move.to_hex, occupied);
            break;
        case PieceType::PILLBUG:
            valid = validate_pillbug_move(move.from_hex.value(), move.to_hex, occupied);
            break;
    }
    
    if (!valid) {
        throw std::runtime_error("Invalid move for " + to_string(piece_to_move.type));
    }
    
    // Execute move
    game.board[move.from_hex.value()].pop_back();
    if (game.board[move.from_hex.value()].empty()) {
        game.board.erase(move.from_hex.value());
    }
    
    if (!game.board.count(move.to_hex)) {
        game.board[move.to_hex] = {};
    }
    game.board[move.to_hex].push_back(piece_to_move);
}

// Execution - Special (Pillbug throw)
void GameEngine::execute_special(Game& game, const MoveRequest& move) {
    if (!move.from_hex.has_value()) {
        throw std::runtime_error("Origin required for special move");
    }
    
    Hex from = move.from_hex.value();
    Hex to = move.to_hex;
    
    if (!game.board.count(from) || game.board.at(from).empty()) {
        throw std::runtime_error("No piece at origin for special move");
    }
    
    // The thrown piece must be unstacked
    if (game.board.at(from).size() > 1) {
        throw std::runtime_error("Cannot throw a stacked piece");
    }
    
    // Cannot throw the piece that was just moved by opponent
    if (game.last_moved_to.has_value() && game.last_moved_to.value() == from) {
        throw std::runtime_error("Cannot throw the piece that was just moved");
    }
    
    // Cannot throw a frozen piece
    if (game.pillbug_frozen_hex.has_value() && game.pillbug_frozen_hex.value() == from) {
        throw std::runtime_error("That piece is frozen by pillbug");
    }
    
    // Destination must be empty
    if (game.board.count(to) && !game.board.at(to).empty()) {
        throw std::runtime_error("Destination must be empty for special move");
    }
    
    // Find an adjacent Pillbug (or Mosquito touching Pillbug) owned by current player
    // that is adjacent to BOTH from and to, AND has clear gates.
    bool found_pillbug = false;
    Hex pillbug_hex;
    
    // One Hive Rule: checking this once is enough as it only depends on removing 'from'
    auto occupied = get_occupied_hexes(game.board);
    auto future_occupied = occupied;
    future_occupied.erase(from);
    if (!is_connected(future_occupied)) {
        throw std::runtime_error("Special move violates One Hive Rule");
    }
    
    auto from_neighbors = get_neighbors(from);
    auto to_neighbors = get_neighbors(to);
    
    for (const auto& n : from_neighbors) {
        if (!game.board.count(n) || game.board.at(n).empty()) continue;
        const Piece& top = game.board.at(n).back();
        if (top.color != game.current_turn) continue;
        
        // Check if this piece is adjacent to destination too
        bool adj_to_dest = false;
        for (const auto& tn : to_neighbors) {
            if (tn == n) { adj_to_dest = true; break; }
        }
        if (!adj_to_dest) continue;
        
        // Must be a Pillbug (not stacked under something)
        bool is_pillbug = (top.type == PieceType::PILLBUG && game.board.at(n).size() == 1);
        
        // Or a Mosquito touching a Pillbug (at ground level)
        bool is_mosquito_as_pillbug = false;
        if (top.type == PieceType::MOSQUITO && game.board.at(n).size() == 1) {
            for (const auto& mn : get_neighbors(n)) {
                if (game.board.count(mn) && !game.board.at(mn).empty()) {
                    if (game.board.at(mn).back().type == PieceType::PILLBUG) {
                        is_mosquito_as_pillbug = true;
                        break;
                    }
                }
            }
        }
        
        if (is_pillbug || is_mosquito_as_pillbug) {
            // Pillbug under another piece can't use ability
            // (check effectively done by size == 1)
            
            // Check if pillbug itself is not frozen
            if (game.pillbug_frozen_hex.has_value() && game.pillbug_frozen_hex.value() == n) {
                continue; // This pillbug is frozen
            }
            
            // Freedom of movement: check climb/slide from piece -> pillbug
            if (!can_climb(from, n, occupied)) {
                continue; // Gate blocked
            }
            
            // Freedom of movement: check climb/slide from pillbug -> destination
            auto occupied_after_lift = occupied;
            occupied_after_lift.erase(from);
            if (!can_climb(n, to, occupied_after_lift)) {
                continue; // Gate blocked
            }
            
            pillbug_hex = n;
            found_pillbug = true;
            break; // Found a valid pillbug path!
        }
    }
    
    if (!found_pillbug) {
        throw std::runtime_error("No valid pillbug path (gate blocked or no pillbug)");
    }
    
    // Execute the throw
    Piece thrown_piece = game.board[from].back();
    game.board[from].pop_back();
    if (game.board[from].empty()) {
        game.board.erase(from);
    }
    
    game.board[to] = {thrown_piece};
    
    // Mark the thrown piece as frozen for opponent's next turn
    game.pillbug_frozen_hex = to;
}

// Helper: can_slide
bool GameEngine::can_slide(const Hex& start, const Hex& end, 
                          const std::unordered_set<Hex, HexHash>& occupied) {
    auto common = get_common_neighbors(start, end);
    int occupied_common = 0;
    for (const auto& n : common) {
        if (occupied.count(n)) {
            occupied_common++;
        }
    }
    
    // Gate is blocked if both common neighbors are occupied
    if (occupied_common >= 2) {
        return false;
    }
    
    // Must slide along hive (at least one common neighbor occupied)
    if (occupied_common == 0) {
        return false;
    }
    
    return true;
}

// Helper: get occupied hexes
std::unordered_set<Hex, HexHash> GameEngine::get_occupied_hexes(
    const std::unordered_map<Hex, std::vector<Piece>, HexHash>& board) {
    std::unordered_set<Hex, HexHash> hexes;
    for (const auto& [hex, stack] : board) {
        if (!stack.empty()) {
            hexes.insert(hex);
        }
    }
    return hexes;
}

// Movement validation functions
bool GameEngine::validate_queen_move(const Hex& start, const Hex& end,
                                    const std::unordered_set<Hex, HexHash>& occupied) {
    if (!are_neighbors(start, end)) return false;
    if (occupied.count(end)) return false;
    if (!can_slide(start, end, occupied)) return false;
    return true;
}

bool GameEngine::validate_beetle_move(const Hex& start, const Hex& end) {
    return are_neighbors(start, end);
}

// bool GameEngine::validate_beetle_move(const Game& game, const Hex& start, const Hex& end,
//                                       const std::unordered_set<Hex, HexHash>& occupied) {
//     if (!are_neighbors(start, end)) return false;
//     size_t start_z = game.board.count(start) ? game.board.at(start).size() : 0;
//     bool is_dest_empty = !occupied.count(end);
//     beetle does not need to respect freedom of movement
//     if (start_z == 1 && is_dest_empty) {
//        if (!can_slide(start, end, occupied)) return false;
//     }
//     return true;
// }

bool GameEngine::validate_grasshopper_move(const Hex& start, const Hex& end,
                                          const std::unordered_set<Hex, HexHash>& occupied) {
    int dq = end.first - start.first;
    int dr = end.second - start.second;
    
    if (dq == 0 && dr == 0) return false;
    
    // Determine step direction
    Hex step;
    if (dq == 0) step = {0, dr > 0 ? 1 : -1};
    else if (dr == 0) step = {dq > 0 ? 1 : -1, 0};
    else if (dq == -dr) step = {dq > 0 ? 1 : -1, dr > 0 ? 1 : -1};
    else return false; // Not a straight line
    
    Hex current = {start.first + step.first, start.second + step.second};
    if (!occupied.count(current)) return false; // Must jump over at least one piece
    
    while (occupied.count(current)) {
        current = {current.first + step.first, current.second + step.second};
    }
    
    return current == end;
}

bool GameEngine::validate_spider_move(const Hex& start, const Hex& end,
                                      const std::unordered_set<Hex, HexHash>& occupied) {
    auto occupied_for_path = occupied;
    if (occupied_for_path.count(start)) {
        occupied_for_path.erase(start);
    }
    
    std::function<bool(Hex, int, std::unordered_set<Hex, HexHash>&)> find_spider_paths;
    find_spider_paths = [&](Hex curr, int steps_left, std::unordered_set<Hex, HexHash>& visited) -> bool {
        if (steps_left == 0) {
            return curr == end;
        }
        
        auto neighbors = get_neighbors(curr);
        for (const auto& n : neighbors) {
            if (occupied_for_path.count(n)) continue;
            if (visited.count(n)) continue;
            if (!can_slide(curr, n, occupied_for_path)) continue;
            
            // Must have contact with hive
            auto n_neighbors = get_neighbors(n);
            bool has_contact = false;
            for (const auto& nb : n_neighbors) {
                if (occupied_for_path.count(nb)) {
                    has_contact = true;
                    break;
                }
            }
            if (!has_contact) continue;
            
            auto new_visited = visited;
            new_visited.insert(n);
            if (find_spider_paths(n, steps_left - 1, new_visited)) {
                return true;
            }
        }
        return false;
    };
    
    std::unordered_set<Hex, HexHash> visited = {start};
    return find_spider_paths(start, 3, visited);
}

bool GameEngine::validate_ant_move(const Hex& start, const Hex& end,
                                   const std::unordered_set<Hex, HexHash>& occupied) {
    if (start == end) return false;
    if (occupied.count(end)) return false;
    
    auto occupied_for_path = occupied;
    if (occupied_for_path.count(start)) {
        occupied_for_path.erase(start);
    }
    
    std::queue<Hex> queue;
    std::unordered_set<Hex, HexHash> visited;
    
    queue.push(start);
    visited.insert(start);
    
    while (!queue.empty()) {
        Hex curr = queue.front();
        queue.pop();
        
        if (curr == end) return true;
        
        auto neighbors = get_neighbors(curr);
        for (const auto& n : neighbors) {
            if (occupied_for_path.count(n)) continue;
            if (visited.count(n)) continue;
            if (!can_slide(curr, n, occupied_for_path)) continue;
            
            // Must hug hive
            auto n_neighbors = get_neighbors(n);
            bool has_contact = false;
            for (const auto& nb : n_neighbors) {
                if (occupied_for_path.count(nb)) {
                    has_contact = true;
                    break;
                }
            }
            if (!has_contact) continue;
            
            visited.insert(n);
            queue.push(n);
        }
    }
    
    return false;
}

// Move generation functions
std::unordered_set<Hex, HexHash> GameEngine::gen_queen_moves(
    const Hex& start, const std::unordered_set<Hex, HexHash>& occupied) {
    std::unordered_set<Hex, HexHash> moves;
    for (const auto& n : get_neighbors(start)) {
        if (occupied.count(n)) continue;
        if (!can_slide(start, n, occupied)) continue;
        
        auto neighbors_of_n = get_neighbors(n);
        for (const auto& nb : neighbors_of_n) {
            if (occupied.count(nb)) {
                moves.insert(n);
                break;
            }
        }
    }
    return moves;
}

std::unordered_set<Hex, HexHash> GameEngine::gen_beetle_moves(
    const Game& game, const Hex& start, const std::unordered_set<Hex, HexHash>& occupied) {
    std::unordered_set<Hex, HexHash> moves;
    size_t start_z = game.board.count(start) ? game.board.at(start).size() : 0;
    
    for (const auto& n : get_neighbors(start)) {
        bool is_dest_empty = !occupied.count(n);
        
        if (start_z == 1 && is_dest_empty) {
            if (!can_slide(start, n, occupied)) continue;
        }
        
        if (is_dest_empty) {
            auto neighbors_of_n = get_neighbors(n);
            bool has_contact = false;
            for (const auto& nb : neighbors_of_n) {
                if (occupied.count(nb)) {
                    has_contact = true;
                    break;
                }
            }
            if (!has_contact) continue;
        }
        
        moves.insert(n);
    }
    return moves;
}

std::unordered_set<Hex, HexHash> GameEngine::gen_grasshopper_moves(
    const Hex& start, const std::unordered_set<Hex, HexHash>& occupied) {
    std::unordered_set<Hex, HexHash> moves;
    for (const auto& d : HEX_DIRECTIONS) {
        Hex curr = {start.first + d.first, start.second + d.second};
        if (!occupied.count(curr)) continue;
        
        while (occupied.count(curr)) {
            curr = {curr.first + d.first, curr.second + d.second};
        }
        moves.insert(curr);
    }
    return moves;
}

std::unordered_set<Hex, HexHash> GameEngine::gen_spider_moves(
    const Hex& start, const std::unordered_set<Hex, HexHash>& occupied) {
    std::unordered_set<Hex, HexHash> valid_ends;
    
    std::function<void(Hex, int, std::unordered_set<Hex, HexHash>&)> search;
    search = [&](Hex curr, int steps, std::unordered_set<Hex, HexHash>& visited) {
        if (steps == 0) {
            valid_ends.insert(curr);
            return;
        }
        
        for (const auto& n : get_neighbors(curr)) {
            if (occupied.count(n)) continue;
            if (visited.count(n)) continue;
            if (!can_slide(curr, n, occupied)) continue;
            
            bool has_contact = false;
            for (const auto& nb : get_neighbors(n)) {
                if (occupied.count(nb)) {
                    has_contact = true;
                    break;
                }
            }
            if (!has_contact) continue;
            
            auto new_visited = visited;
            new_visited.insert(n);
            search(n, steps - 1, new_visited);
        }
    };
    
    std::unordered_set<Hex, HexHash> visited = {start};
    search(start, 3, visited);
    return valid_ends;
}

std::unordered_set<Hex, HexHash> GameEngine::gen_ant_moves(
    const Hex& start, const std::unordered_set<Hex, HexHash>& occupied) {
    std::unordered_set<Hex, HexHash> moves;
    std::queue<Hex> queue;
    std::unordered_set<Hex, HexHash> visited;
    
    queue.push(start);
    visited.insert(start);
    
    while (!queue.empty()) {
        Hex curr = queue.front();
        queue.pop();
        
        for (const auto& n : get_neighbors(curr)) {
            if (occupied.count(n)) continue;
            if (visited.count(n)) continue;
            if (!can_slide(curr, n, occupied)) continue;
            
            bool has_contact = false;
            for (const auto& nb : get_neighbors(n)) {
                if (occupied.count(nb)) {
                    has_contact = true;
                    break;
                }
            }
            if (!has_contact) continue;
            
            visited.insert(n);
            queue.push(n);
            moves.insert(n);
        }
    }
    
    return moves;
}

// Ladybug validation
bool GameEngine::validate_ladybug_move(const Game& game, const Hex& start, const Hex& end,
                                       const std::unordered_set<Hex, HexHash>& occupied) {
    auto candidates = gen_ladybug_moves(start, occupied);
    return candidates.count(end) > 0;
}

// Mosquito validation
bool GameEngine::validate_mosquito_move(const Game& game, const Hex& start, const Hex& end,
                                        const std::unordered_set<Hex, HexHash>& occupied) {
    auto candidates = gen_mosquito_moves(game, start, occupied);
    return candidates.count(end) > 0;
}

// Pillbug validation (queen-like, 1 space)
bool GameEngine::validate_pillbug_move(const Hex& start, const Hex& end,
                                       const std::unordered_set<Hex, HexHash>& occupied) {
    return validate_queen_move(start, end, occupied);
}

// Ladybug move generation
// Moves exactly 3 steps: first 2 on top of hive, last 1 down to empty ground
std::unordered_set<Hex, HexHash> GameEngine::gen_ladybug_moves(
    const Hex& start, const std::unordered_set<Hex, HexHash>& occupied) {
    std::unordered_set<Hex, HexHash> valid_ends;
    
    // Step 1: move on top of an adjacent occupied hex
    for (const auto& step1 : get_neighbors(start)) {
        if (!occupied.count(step1)) continue; // must step onto occupied hex
        
        // Step 2: move on top of another adjacent occupied hex (different from start)
        for (const auto& step2 : get_neighbors(step1)) {
            if (step2 == start) continue; // can't go back to start
            if (!occupied.count(step2)) continue; // must be on top of hive
            
            // Step 3: move down to an empty adjacent hex
            for (const auto& step3 : get_neighbors(step2)) {
                if (step3 == start) continue; // can't return to start
                if (occupied.count(step3)) continue; // must land on empty hex
                
                // Must be connected to hive after landing
                bool has_contact = false;
                for (const auto& nb : get_neighbors(step3)) {
                    if (occupied.count(nb)) {
                        has_contact = true;
                        break;
                    }
                }
                if (has_contact) {
                    valid_ends.insert(step3);
                }
            }
        }
    }
    
    return valid_ends;
}

// Mosquito move generation
// At ground level: copies movement of any touching piece type
// On top of stack (beetle-like): moves as beetle
// If touching only other mosquito(es): cannot move
std::unordered_set<Hex, HexHash> GameEngine::gen_mosquito_moves(
    const Game& game, const Hex& start, const std::unordered_set<Hex, HexHash>& occupied) {
    std::unordered_set<Hex, HexHash> all_moves;
    
    // Check if mosquito is on top of a stack (beetle-like position)
    size_t stack_height = game.board.count(start) ? game.board.at(start).size() : 0;
    if (stack_height > 1) {
        // On top of stack: move as beetle
        return gen_beetle_moves(game, start, occupied);
    }
    
    // At ground level: gather all neighboring piece types
    std::unordered_set<PieceType> neighbor_types;
    for (const auto& n : get_neighbors(start)) {
        if (game.board.count(n) && !game.board.at(n).empty()) {
            const Piece& top = game.board.at(n).back();
            neighbor_types.insert(top.type);
        }
    }
    
    // If touching only mosquito(es) and no other piece types, cannot move
    if (neighbor_types.size() == 1 && neighbor_types.count(PieceType::MOSQUITO)) {
        return all_moves; // empty
    }
    
    // If a stacked beetle is adjacent, mosquito can also move as beetle
    for (const auto& n : get_neighbors(start)) {
        if (game.board.count(n) && game.board.at(n).size() > 1) {
            // There's a stack — the mosquito can copy beetle movement
            neighbor_types.insert(PieceType::BEETLE);
            break;
        }
    }
    
    // Generate moves for each neighboring piece type (excluding mosquito)
    for (PieceType pt : neighbor_types) {
        if (pt == PieceType::MOSQUITO) continue;
        
        std::unordered_set<Hex, HexHash> moves;
        switch (pt) {
            case PieceType::QUEEN:
                moves = gen_queen_moves(start, occupied);
                break;
            case PieceType::BEETLE:
                moves = gen_beetle_moves(game, start, occupied);
                break;
            case PieceType::GRASSHOPPER:
                moves = gen_grasshopper_moves(start, occupied);
                break;
            case PieceType::SPIDER:
                moves = gen_spider_moves(start, occupied);
                break;
            case PieceType::ANT:
                moves = gen_ant_moves(start, occupied);
                break;
            case PieceType::LADYBUG:
                moves = gen_ladybug_moves(start, occupied);
                break;
            case PieceType::PILLBUG:
                moves = gen_pillbug_moves(start, occupied);
                break;
            default:
                break;
        }
        all_moves.insert(moves.begin(), moves.end());
    }
    
    return all_moves;
}

// Pillbug move generation (same as queen: 1 space, ground level)
std::unordered_set<Hex, HexHash> GameEngine::gen_pillbug_moves(
    const Hex& start, const std::unordered_set<Hex, HexHash>& occupied) {
    return gen_queen_moves(start, occupied);
}

// Pillbug special move generation
// Returns pairs of (from_hex, to_hex) for each valid throw
std::vector<std::pair<Hex, Hex>> GameEngine::gen_pillbug_special_moves(
    const Game& game, const Hex& pillbug_hex, const std::unordered_set<Hex, HexHash>& occupied) {
    std::vector<std::pair<Hex, Hex>> special_moves;
    
    // Pillbug under another piece can't use ability
    if (game.board.count(pillbug_hex) && game.board.at(pillbug_hex).size() > 1) {
        return special_moves;
    }
    
    // If the pillbug itself is frozen, it can't use ability
    if (game.pillbug_frozen_hex.has_value() && game.pillbug_frozen_hex.value() == pillbug_hex) {
        return special_moves;
    }
    
    // Get empty neighbors of pillbug (potential destinations)
    std::vector<Hex> empty_neighbors;
    for (const auto& n : get_neighbors(pillbug_hex)) {
        if (!occupied.count(n)) {
            empty_neighbors.push_back(n);
        }
    }
    if (empty_neighbors.empty()) return special_moves;
    
    // For each adjacent piece to the pillbug:
    for (const auto& adj : get_neighbors(pillbug_hex)) {
        if (!game.board.count(adj) || game.board.at(adj).empty()) continue;
        
        // Must be unstacked
        if (game.board.at(adj).size() > 1) continue;
        
        // Cannot throw the piece opponent just moved
        if (game.last_moved_to.has_value() && game.last_moved_to.value() == adj) {
            continue;
        }
        
        // Cannot throw a frozen piece
        if (game.pillbug_frozen_hex.has_value() && game.pillbug_frozen_hex.value() == adj) {
            continue;
        }
        
        // One Hive: removing this piece must not disconnect
        auto future_occupied = occupied;
        future_occupied.erase(adj);
        if (!is_connected(future_occupied)) continue;
        
        // Freedom of movement: piece -> pillbug (slide check)
        if (!can_climb(adj, pillbug_hex, occupied)) continue;
        
        // For each empty destination adjacent to pillbug:
        for (const auto& dest : empty_neighbors) {
            if (dest == adj) continue; // Can't land back where it was
            
            // Freedom of movement: pillbug -> destination (after piece is lifted)
            auto occupied_after_lift = occupied;
            occupied_after_lift.erase(adj);
            if (!can_climb(pillbug_hex, dest, occupied_after_lift)) continue;
            
            special_moves.emplace_back(adj, dest);
        }
    }
    
    return special_moves;
}

// Get valid moves
std::vector<Hex> GameEngine::get_valid_moves(const std::string& game_id, int q, int r) {
    auto game_opt = get_game(game_id);
    if (!game_opt || game_opt->status == GameStatus::FINISHED) {
        return {};
    }
    
    auto occupied = get_occupied_hexes(game_opt->board);
    return get_valid_moves_for_piece(game_opt.value(), {q, r}, occupied, true);
}

// Special move generation integration
// We need to return:
// 1. If 'piece' is Pillbug: valid moves + adjacent pieces it can throw (as "moves" to occupied hexes)
// 2. If 'piece' is adjacent to Pillbug: valid moves + destinations it can be thrown to
std::vector<Hex> GameEngine::get_valid_moves_for_piece(
    const Game& game, const Hex& from_hex, 
    const std::unordered_set<Hex, HexHash>& occupied,
    bool include_interaction_targets) {
    
    if (!game.board.count(from_hex) || game.board.at(from_hex).empty()) {
        return {};
    }
    
    const Piece& piece = game.board.at(from_hex).back();
    if (piece.color != game.current_turn) {
        return {};
    }
    
    // Queen must be played before moving
    auto& hand = (game.current_turn == PlayerColor::WHITE) 
        ? game.white_pieces_hand : game.black_pieces_hand;
    if (hand.at(PieceType::QUEEN) == 1 && piece.type != PieceType::QUEEN) {
        return {};
    }
    
    // One Hive check for normal movement (moving FROM from_hex)
    size_t stack_height = game.board.at(from_hex).size();
    auto occupied_after_lift = occupied;
    if (stack_height == 1) {
        occupied_after_lift.erase(from_hex);
    }
    
    bool pinned = !is_connected(occupied_after_lift);
    
    // Pillbug frozen check: if this piece was thrown by opponent's pillbug, it can't move
    bool frozen = (game.pillbug_frozen_hex.has_value() && game.pillbug_frozen_hex.value() == from_hex);
    
    std::unordered_set<Hex, HexHash> candidates;
    
    // 1. Normal Moves (if not pinned and not frozen)
    if (!pinned && !frozen) {
        switch (piece.type) {
            case PieceType::QUEEN:
                candidates = gen_queen_moves(from_hex, occupied_after_lift);
                break;
            case PieceType::BEETLE:
                candidates = gen_beetle_moves(game, from_hex, occupied_after_lift);
                break;
            case PieceType::GRASSHOPPER:
                candidates = gen_grasshopper_moves(from_hex, occupied);
                break;
            case PieceType::SPIDER:
                candidates = gen_spider_moves(from_hex, occupied_after_lift);
                break;
            case PieceType::ANT:
                candidates = gen_ant_moves(from_hex, occupied_after_lift);
                break;
            case PieceType::LADYBUG:
                candidates = gen_ladybug_moves(from_hex, occupied_after_lift);
                break;
            case PieceType::MOSQUITO:
                candidates = gen_mosquito_moves(game, from_hex, occupied_after_lift);
                break;
            case PieceType::PILLBUG:
                candidates = gen_pillbug_moves(from_hex, occupied_after_lift);
                break;
        }
    }
    
    // 2. Special Moves (Pillbug Logic)
    
    // 2. Special Moves (Pillbug Logic) - UI ONLY
    if (include_interaction_targets) {
        // Case A: Selected piece IS a Pillbug (or Mosquito acting as one)
        // Goal: Highlight adjacent pieces that CAN be thrown.
        // We represent these as "valid moves" to occupied squares.
        bool can_act_as_pillbug = false;
    if (piece.type == PieceType::PILLBUG && stack_height == 1) {
        can_act_as_pillbug = true;
    } else if (piece.type == PieceType::MOSQUITO && stack_height == 1) {
         for (const auto& n : get_neighbors(from_hex)) {
            if (game.board.count(n) && !game.board.at(n).empty()) {
                if (game.board.at(n).back().type == PieceType::PILLBUG) {
                    can_act_as_pillbug = true;
                    break;
                }
            }
        }
    }
    
    if (can_act_as_pillbug && !frozen) {
         auto special_moves = gen_pillbug_special_moves(game, from_hex, occupied);
         for (const auto& move : special_moves) {
             // move.first is the piece being thrown (adjacent to pillbug)
             // move.second is the destination
             // We return the SOURCE (move.first) so the frontend knows it can be grabbed.
             candidates.insert(move.first); 
         }
    }
    
    // Case B: Selected piece is adjacent to a friendly Pillbug (it might be thrown)
    // Goal: Highlight destinations.
    // We need to check if ANY adjacent friendly pillbug can throw THIS piece.
    // This piece is 'from_hex'.
    if (stack_height == 1 && !frozen) {
        for (const auto& n : get_neighbors(from_hex)) {
            if (!game.board.count(n) || game.board.at(n).empty()) continue;
            const Piece& neighbor = game.board.at(n).back();
            
            if (neighbor.color == game.current_turn) {
                bool neighbor_is_pillbug = (neighbor.type == PieceType::PILLBUG);
                bool neighbor_is_mosquito_pillbug = false;
                
                if (neighbor.type == PieceType::MOSQUITO) {
                     // Check if mosquito touches ANOTHER pillbug (neighbors of n)
                     for (const auto& mn : get_neighbors(n)) {
                        if (game.board.count(mn) && !game.board.at(mn).empty()) {
                            if (game.board.at(mn).back().type == PieceType::PILLBUG) {
                                neighbor_is_mosquito_pillbug = true;
                                break;
                            }
                        }
                    }
                }
                
                if ((neighbor_is_pillbug || neighbor_is_mosquito_pillbug) && game.board.at(n).size() == 1) {
                     // Check if 'neighbor' is frozen
                     if (game.pillbug_frozen_hex.has_value() && game.pillbug_frozen_hex.value() == n) continue;
                     
                     // Check if 'neighbor' can throw 'from_hex'
                     // We can use gen_pillbug_special_moves for the neighbor
                     auto specials = gen_pillbug_special_moves(game, n, occupied);
                     for (const auto& move : specials) {
                         if (move.first == from_hex) {
                             candidates.insert(move.second); // Add destination
                         }
                     }
                }
            }
        }
    }
    }

    return std::vector<Hex>(candidates.begin(), candidates.end());
}

// Helper: check if climb is possible (gate not blocked)
bool GameEngine::can_climb(const Hex& start, const Hex& end, 
                          const std::unordered_set<Hex, HexHash>& occupied) {
    auto common = get_common_neighbors(start, end);
    int occupied_common = 0;
    for (const auto& n : common) {
        if (occupied.count(n)) {
            occupied_common++;
        }
    }
    // Gate is blocked if both common neighbors are occupied
    return occupied_common < 2;
}

// Win condition
void GameEngine::check_win_condition(Game& game) {
    std::vector<std::pair<Piece, Hex>> queens;
    
    for (const auto& [hex, stack] : game.board) {
        for (const auto& p : stack) {
            if (p.type == PieceType::QUEEN) {
                queens.emplace_back(p, hex);
            }
        }
    }
    
    bool white_surrounded = false;
    bool black_surrounded = false;
    
    for (const auto& [piece, loc] : queens) {
        auto neighbors = get_neighbors(loc);
        int occupied_count = 0;
        for (const auto& n : neighbors) {
            if (game.board.count(n)) {
                occupied_count++;
            }
        }
        
        if (occupied_count == 6) {
            if (piece.color == PlayerColor::WHITE) {
                white_surrounded = true;
            } else {
                black_surrounded = true;
            }
        }
    }
    
    if (white_surrounded && black_surrounded) {
        game.status = GameStatus::FINISHED;
        game.winner = std::nullopt; // Draw
    } else if (white_surrounded) {
        game.status = GameStatus::FINISHED;
        game.winner = PlayerColor::BLACK;
    } else if (black_surrounded) {
        game.status = GameStatus::FINISHED;
        game.winner = PlayerColor::WHITE;
    }
}

} // namespace bugs

