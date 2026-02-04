#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include <memory>
#include "json.hpp"
#include "hex_math.hpp"

namespace bugs {

// Enumerations
enum class PieceType {
    QUEEN,
    ANT,
    SPIDER,
    BEETLE,
    GRASSHOPPER
};

enum class PlayerColor {
    WHITE,
    BLACK
};

enum class GameStatus {
    IN_PROGRESS,
    FINISHED
};

enum class ActionType {
    PLACE,
    MOVE
};

// String conversion functions
std::string to_string(PieceType type);
std::string to_string(PlayerColor color);
std::string to_string(GameStatus status);
std::string to_string(ActionType action);

PieceType piece_type_from_string(const std::string& str);
PlayerColor player_color_from_string(const std::string& str);
GameStatus game_status_from_string(const std::string& str);
ActionType action_type_from_string(const std::string& str);

// Structures
struct Piece {
    PieceType type;
    PlayerColor color;
    std::string id;

    Piece() = default;
    Piece(PieceType t, PlayerColor c, std::string i)
        : type(t), color(c), id(std::move(i)) {}
};

struct MoveRequest {
    ActionType action;
    std::optional<PieceType> piece_type;
    std::optional<std::pair<int, int>> from_hex;
    std::pair<int, int> to_hex;

    MoveRequest() = default;
    MoveRequest(ActionType a, std::pair<int, int> to)
        : action(a), to_hex(to) {}
};

struct MoveLog {
    MoveRequest move;
    PlayerColor player;
    int turn_number;
    std::string notation; // For algebraic notation (e.g., "wQ", "wA1 /q,r")
};

struct Game {
    std::string game_id;
    // Key is Hex coordinate, Value is stack of pieces (bottom to top)
    std::unordered_map<Hex, std::vector<Piece>, HexHash> board;
    PlayerColor current_turn;
    int turn_number;
    std::unordered_map<PieceType, int> white_pieces_hand;
    std::unordered_map<PieceType, int> black_pieces_hand;
    std::optional<PlayerColor> winner;
    GameStatus status;
    std::vector<MoveLog> history;

    Game() : current_turn(PlayerColor::WHITE), turn_number(1), 
             status(GameStatus::IN_PROGRESS) {}
};

// JSON serialization functions
void to_json(nlohmann::json& j, const PieceType& type);
void from_json(const nlohmann::json& j, PieceType& type);

void to_json(nlohmann::json& j, const PlayerColor& color);
void from_json(const nlohmann::json& j, PlayerColor& color);

void to_json(nlohmann::json& j, const GameStatus& status);
void from_json(const nlohmann::json& j, GameStatus& status);

void to_json(nlohmann::json& j, const ActionType& action);
void from_json(const nlohmann::json& j, ActionType& action);

void to_json(nlohmann::json& j, const Piece& piece);
void from_json(const nlohmann::json& j, Piece& piece);

void to_json(nlohmann::json& j, const MoveRequest& move);
void from_json(const nlohmann::json& j, MoveRequest& move);

void to_json(nlohmann::json& j, const MoveLog& log);
void from_json(const nlohmann::json& j, MoveLog& log);

void to_json(nlohmann::json& j, const Game& game);
void from_json(const nlohmann::json& j, Game& game);

// Helper to create initial hand
std::unordered_map<PieceType, int> create_initial_hand();

} // namespace bugs
