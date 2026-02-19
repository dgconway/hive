#pragma once

#include "models.hpp"
#include "hex_math.hpp"
#include <unordered_map>
#include <vector>
#include <string>
#include <optional>
#include <memory>

namespace bugs {

// Forward declaration
class GameInterface;

class GameEngine {
public:
    GameEngine() = default;
    
    // Allow GameInterface to access private members
    friend class GameInterface;

    // Game management
    Game create_game(bool advanced_mode = false);
    std::optional<Game> get_game(const std::string& game_id);
    Game process_move(const std::string& game_id, const MoveRequest& move);
    void process_move_inplace(Game& game, const MoveRequest& move);
    
    // Move generation
    std::vector<Hex> get_valid_moves(const std::string& game_id, int q, int r);
    std::vector<Hex> get_valid_moves_for_piece(
        const Game& game, 
        const Hex& from_hex, 
        const std::unordered_set<Hex, HexHash>& occupied
    );

private:
    std::unordered_map<std::string, Game> games_;

    // Validation
    void validate_turn(const Game& game, const MoveRequest& move);
    
    // Execution
    void execute_place(Game& game, const MoveRequest& move);
    void execute_move(Game& game, const MoveRequest& move);
    void execute_special(Game& game, const MoveRequest& move);
    
    // Movement validation
    bool validate_queen_move(const Hex& start, const Hex& end, 
                            const std::unordered_set<Hex, HexHash>& occupied);
    bool validate_beetle_move(const Hex& start, const Hex& end);
    bool validate_grasshopper_move(const Hex& start, const Hex& end, 
                                  const std::unordered_set<Hex, HexHash>& occupied);
    bool validate_spider_move(const Hex& start, const Hex& end, 
                             const std::unordered_set<Hex, HexHash>& occupied);
    bool validate_ant_move(const Hex& start, const Hex& end, 
                          const std::unordered_set<Hex, HexHash>& occupied);
    bool validate_ladybug_move(const Game& game, const Hex& start, const Hex& end,
                              const std::unordered_set<Hex, HexHash>& occupied);
    bool validate_mosquito_move(const Game& game, const Hex& start, const Hex& end,
                               const std::unordered_set<Hex, HexHash>& occupied);
    bool validate_pillbug_move(const Hex& start, const Hex& end,
                              const std::unordered_set<Hex, HexHash>& occupied);
    
    // Move generation
    std::unordered_set<Hex, HexHash> gen_queen_moves(
        const Hex& start, const std::unordered_set<Hex, HexHash>& occupied);
    std::unordered_set<Hex, HexHash> gen_beetle_moves(
        const Game& game, const Hex& start, const std::unordered_set<Hex, HexHash>& occupied);
    std::unordered_set<Hex, HexHash> gen_grasshopper_moves(
        const Hex& start, const std::unordered_set<Hex, HexHash>& occupied);
    std::unordered_set<Hex, HexHash> gen_spider_moves(
        const Hex& start, const std::unordered_set<Hex, HexHash>& occupied);
    std::unordered_set<Hex, HexHash> gen_ant_moves(
        const Hex& start, const std::unordered_set<Hex, HexHash>& occupied);
    std::unordered_set<Hex, HexHash> gen_ladybug_moves(
        const Game& game, const Hex& start, const std::unordered_set<Hex, HexHash>& occupied);
    std::unordered_set<Hex, HexHash> gen_mosquito_moves(
        const Game& game, const Hex& start, const std::unordered_set<Hex, HexHash>& occupied);
    std::unordered_set<Hex, HexHash> gen_pillbug_moves(
        const Hex& start, const std::unordered_set<Hex, HexHash>& occupied);
    
    // Pillbug special ability: returns pairs of (from_hex, to_hex) for throwable pieces
    std::vector<std::pair<Hex, Hex>> gen_pillbug_special_moves(
        const Game& game, const Hex& pillbug_hex, const std::unordered_set<Hex, HexHash>& occupied);
    
    // Helpers
    bool can_slide(const Hex& start, const Hex& end, 
                  const std::unordered_set<Hex, HexHash>& occupied);
    std::unordered_set<Hex, HexHash> get_occupied_hexes(
        const std::unordered_map<Hex, std::vector<Piece>, HexHash>& board);
    void check_win_condition(Game& game);
};

} // namespace bugs
