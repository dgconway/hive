#pragma once

#include "models.hpp"
#include "hex_math.hpp"
#include "game_logic.hpp"
#include <vector>
#include <memory>

namespace bugs {

// Action representation for AI
struct Action {
    ActionType action_type;
    std::optional<PieceType> piece_type;
    std::optional<Hex> from_hex;
    Hex to_hex;

    Action() = default;
    Action(ActionType type, Hex to) : action_type(type), to_hex(to) {}
    
    MoveRequest to_move_request() const;
};

// Game state wrapper for AI
class GameState {
public:
    Game game;
    
    explicit GameState(const Game& g);
    GameState(const GameState& other);
    GameState& operator=(const GameState& other);
    
    PlayerColor current_player() const { return game.current_turn; }
    bool is_terminal() const { return game.status == GameStatus::FINISHED; }
    std::optional<PlayerColor> winner() const { return game.winner; }
    
    float get_reward(PlayerColor player) const;
};

// Game interface for AI
class GameInterface {
public:
    GameInterface();
    
    GameState get_initial_state();
    std::vector<Action> get_legal_actions(const GameState& state);
    GameState apply_action(const GameState& state, const Action& action);
    
private:
    GameEngine engine_;
    
    std::vector<Hex> get_valid_placement_hexes(const Game& game);
    std::vector<std::pair<Hex, std::vector<Hex>>> get_all_valid_moves(const Game& game);
};

} // namespace bugs
