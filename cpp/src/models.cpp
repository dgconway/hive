#include "models.hpp"
#include <stdexcept>
#include <sstream>

namespace bugs {

// String conversion implementations
std::string to_string(PieceType type) {
    switch (type) {
        case PieceType::QUEEN: return "QUEEN";
        case PieceType::ANT: return "ANT";
        case PieceType::SPIDER: return "SPIDER";
        case PieceType::BEETLE: return "BEETLE";
        case PieceType::GRASSHOPPER: return "GRASSHOPPER";
    }
    throw std::invalid_argument("Invalid PieceType");
}

std::string to_string(PlayerColor color) {
    return color == PlayerColor::WHITE ? "WHITE" : "BLACK";
}

std::string to_string(GameStatus status) {
    return status == GameStatus::IN_PROGRESS ? "IN_PROGRESS" : "FINISHED";
}

std::string to_string(ActionType action) {
    return action == ActionType::PLACE ? "PLACE" : "MOVE";
}

PieceType piece_type_from_string(const std::string& str) {
    if (str == "QUEEN") return PieceType::QUEEN;
    if (str == "ANT") return PieceType::ANT;
    if (str == "SPIDER") return PieceType::SPIDER;
    if (str == "BEETLE") return PieceType::BEETLE;
    if (str == "GRASSHOPPER") return PieceType::GRASSHOPPER;
    throw std::invalid_argument("Invalid PieceType string: " + str);
}

PlayerColor player_color_from_string(const std::string& str) {
    if (str == "WHITE") return PlayerColor::WHITE;
    if (str == "BLACK") return PlayerColor::BLACK;
    throw std::invalid_argument("Invalid PlayerColor string: " + str);
}

GameStatus game_status_from_string(const std::string& str) {
    if (str == "IN_PROGRESS") return GameStatus::IN_PROGRESS;
    if (str == "FINISHED") return GameStatus::FINISHED;
    throw std::invalid_argument("Invalid GameStatus string: " + str);
}

ActionType action_type_from_string(const std::string& str) {
    if (str == "PLACE") return ActionType::PLACE;
    if (str == "MOVE") return ActionType::MOVE;
    throw std::invalid_argument("Invalid ActionType string: " + str);
}

// JSON serialization implementations
void to_json(nlohmann::json& j, const PieceType& type) {
    j = to_string(type);
}

void from_json(const nlohmann::json& j, PieceType& type) {
    type = piece_type_from_string(j.get<std::string>());
}

void to_json(nlohmann::json& j, const PlayerColor& color) {
    j = to_string(color);
}

void from_json(const nlohmann::json& j, PlayerColor& color) {
    color = player_color_from_string(j.get<std::string>());
}

void to_json(nlohmann::json& j, const GameStatus& status) {
    j = to_string(status);
}

void from_json(const nlohmann::json& j, GameStatus& status) {
    status = game_status_from_string(j.get<std::string>());
}

void to_json(nlohmann::json& j, const ActionType& action) {
    j = to_string(action);
}

void from_json(const nlohmann::json& j, ActionType& action) {
    action = action_type_from_string(j.get<std::string>());
}

void to_json(nlohmann::json& j, const Piece& piece) {
    j = nlohmann::json{
        {"type", piece.type},
        {"color", piece.color},
        {"id", piece.id}
    };
}

void from_json(const nlohmann::json& j, Piece& piece) {
    j.at("type").get_to(piece.type);
    j.at("color").get_to(piece.color);
    j.at("id").get_to(piece.id);
}

void to_json(nlohmann::json& j, const MoveRequest& move) {
    j = nlohmann::json{
        {"action", move.action},
        {"to_hex", nlohmann::json::array({move.to_hex.first, move.to_hex.second})}
    };
    
    if (move.piece_type.has_value()) {
        j["piece_type"] = move.piece_type.value();
    }
    
    if (move.from_hex.has_value()) {
        j["from_hex"] = nlohmann::json::array({
            move.from_hex.value().first,
            move.from_hex.value().second
        });
    }
}

void from_json(const nlohmann::json& j, MoveRequest& move) {
    j.at("action").get_to(move.action);
    
    auto to_hex_arr = j.at("to_hex");
    move.to_hex = {to_hex_arr[0].get<int>(), to_hex_arr[1].get<int>()};
    
    if (j.contains("piece_type") && !j["piece_type"].is_null()) {
        move.piece_type = j["piece_type"].get<PieceType>();
    }
    
    if (j.contains("from_hex") && !j["from_hex"].is_null()) {
        auto from_hex_arr = j["from_hex"];
        move.from_hex = std::make_pair(
            from_hex_arr[0].get<int>(),
            from_hex_arr[1].get<int>()
        );
    }
}

void to_json(nlohmann::json& j, const MoveLog& log) {
    j = nlohmann::json{
        {"move", log.move},
        {"player", log.player},
        {"turn_number", log.turn_number},
        {"notation", log.notation}
    };
}

void from_json(const nlohmann::json& j, MoveLog& log) {
    j.at("move").get_to(log.move);
    j.at("player").get_to(log.player);
    j.at("turn_number").get_to(log.turn_number);
    if (j.contains("notation")) {
        j.at("notation").get_to(log.notation);
    }
}

void to_json(nlohmann::json& j, const Game& game) {
    // Convert board map to JSON
    nlohmann::json board_json = nlohmann::json::object();
    for (const auto& [key, stack] : game.board) {
        board_json[key] = stack;
    }
    
    // Convert hand maps to JSON
    nlohmann::json white_hand = nlohmann::json::object();
    for (const auto& [piece_type, count] : game.white_pieces_hand) {
        white_hand[to_string(piece_type)] = count;
    }
    
    nlohmann::json black_hand = nlohmann::json::object();
    for (const auto& [piece_type, count] : game.black_pieces_hand) {
        black_hand[to_string(piece_type)] = count;
    }
    
    j = nlohmann::json{
        {"game_id", game.game_id},
        {"board", board_json},
        {"current_turn", game.current_turn},
        {"turn_number", game.turn_number},
        {"white_pieces_hand", white_hand},
        {"black_pieces_hand", black_hand},
        {"status", game.status},
        {"history", game.history}
    };
    
    if (game.winner.has_value()) {
        j["winner"] = game.winner.value();
    } else {
        j["winner"] = nullptr;
    }
}

void from_json(const nlohmann::json& j, Game& game) {
    j.at("game_id").get_to(game.game_id);
    j.at("current_turn").get_to(game.current_turn);
    j.at("turn_number").get_to(game.turn_number);
    j.at("status").get_to(game.status);
    
    // Parse board
    game.board.clear();
    for (auto& [key, stack_json] : j.at("board").items()) {
        std::vector<Piece> stack;
        for (auto& piece_json : stack_json) {
            stack.emplace_back(piece_json.get<Piece>());
        }
        game.board[key] = stack;
    }
    
    // Parse hands
    game.white_pieces_hand.clear();
    for (auto& [type_str, count] : j.at("white_pieces_hand").items()) {
        game.white_pieces_hand[piece_type_from_string(type_str)] = count.get<int>();
    }
    
    game.black_pieces_hand.clear();
    for (auto& [type_str, count] : j.at("black_pieces_hand").items()) {
        game.black_pieces_hand[piece_type_from_string(type_str)] = count.get<int>();
    }

    // Parse history
    if (j.contains("history")) {
        game.history = j.at("history").get<std::vector<MoveLog>>();
    } else {
        game.history.clear();
    }
    
    // Parse winner
    if (j.contains("winner") && !j["winner"].is_null()) {
        game.winner = j["winner"].get<PlayerColor>();
    } else {
        game.winner = std::nullopt;
    }
}

std::unordered_map<PieceType, int> create_initial_hand() {
    return {
        {PieceType::QUEEN, 1},
        {PieceType::ANT, 3},
        {PieceType::GRASSHOPPER, 3},
        {PieceType::SPIDER, 2},
        {PieceType::BEETLE, 2}
    };
}

} // namespace bugs
