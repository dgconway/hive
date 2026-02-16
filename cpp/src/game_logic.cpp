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
Game GameEngine::create_game() {
    Game game;
    game.game_id = generate_uuid();
    game.current_turn = PlayerColor::WHITE;
    game.turn_number = 1;
    game.white_pieces_hand = create_initial_hand();
    game.black_pieces_hand = create_initial_hand();
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
    } else {
        execute_move(game, move);
    }
    
    game.history.push_back(log);
    
    check_win_condition(game);
    
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
    } else {
        execute_move(game, move);
    }
    
    game.history.push_back(log);
    
    check_win_condition(game);
    
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
            valid = validate_beetle_move(game, move.from_hex.value(), move.to_hex, occupied);
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

bool GameEngine::validate_beetle_move(const Game& game, const Hex& start, const Hex& end,
                                      const std::unordered_set<Hex, HexHash>& occupied) {
    if (!are_neighbors(start, end)) return false;
    
    size_t start_z = game.board.count(start) ? game.board.at(start).size() : 0;
    bool is_dest_empty = !occupied.count(end);
    
    // beetle does not need to respect freedom of movement
    //if (start_z == 1 && is_dest_empty) {
    //    if (!can_slide(start, end, occupied)) return false;
    //}
    
    return true;
}

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

// Get valid moves
std::vector<Hex> GameEngine::get_valid_moves(const std::string& game_id, int q, int r) {
    auto game_opt = get_game(game_id);
    if (!game_opt || game_opt->status == GameStatus::FINISHED) {
        return {};
    }
    
    auto occupied = get_occupied_hexes(game_opt->board);
    return get_valid_moves_for_piece(game_opt.value(), {q, r}, occupied);
}

std::vector<Hex> GameEngine::get_valid_moves_for_piece(
    const Game& game, const Hex& from_hex, const std::unordered_set<Hex, HexHash>& occupied) {
    
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
    
    // One Hive check
    size_t stack_height = game.board.at(from_hex).size();
    auto occupied_after_lift = occupied;
    if (stack_height == 1) {
        occupied_after_lift.erase(from_hex);
    }
    
    if (!is_connected(occupied_after_lift)) {
        return {}; // Pinned
    }
    
    // Generate candidates
    std::unordered_set<Hex, HexHash> candidates;
    
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
    }
    
    return std::vector<Hex>(candidates.begin(), candidates.end());
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

