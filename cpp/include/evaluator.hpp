#pragma once

#include "models.hpp"
#include "hex_math.hpp"
#include <unordered_map>

namespace bugs {

// Piece value constants
const std::unordered_map<PieceType, float> PIECE_VALUES = {
    {PieceType::QUEEN, 1000.0f},
    {PieceType::ANT, 80.0f},
    {PieceType::BEETLE, 60.0f},
    {PieceType::GRASSHOPPER, 40.0f},
    {PieceType::SPIDER, 30.0f},
    {PieceType::LADYBUG, 50.0f},
    {PieceType::MOSQUITO, 70.0f},
    {PieceType::PILLBUG, 45.0f}
};

// Forward declaration
class GameEngine;

// Evaluate game state from perspective of player
float evaluate_state(const Game& game, PlayerColor player, GameEngine& engine);

} // namespace bugs
