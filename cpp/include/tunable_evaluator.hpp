#pragma once

#include "models.hpp"
#include "hex_math.hpp"
#include "game_logic.hpp"
#include <unordered_map>
#include <string>
#include <fstream>
#include "nlohmann/json.hpp"

namespace bugs {

// Tunable weights for evaluation function
struct EvalWeights {
    // Material values
    float queen_value = 1000.0f;
    float ant_value = 80.0f;
    float beetle_value = 60.0f;
    float grasshopper_value = 40.0f;
    float spider_value = 30.0f;
    
    // Queen surrounding bonuses (indexed by number of neighbors 0-6)
    float surround_opponent_multiplier = 2.0f;
    float surround_self_multiplier = 5.0f;
    
    // Mobility weights
    float mobility_weight = 2.0f;
    
    // Proximity to opponent queen
    float proximity_weight = 10.0f;
    float proximity_max_distance = 3.0f;
    
    // Ant-specific bonuses
    float ant_freedom_bonus = 20.0f;
    float ant_trapped_penalty = 15.0f;
    float trapped_opponent_ant_bonus = 30.0f;
    
    // Hand piece multiplier (pieces in hand worth less than on board)
    float hand_piece_multiplier = 0.5f;
    
    // Get piece value by type
    float get_piece_value(PieceType type) const {
        switch (type) {
            case PieceType::QUEEN: return queen_value;
            case PieceType::ANT: return ant_value;
            case PieceType::BEETLE: return beetle_value;
            case PieceType::GRASSHOPPER: return grasshopper_value;
            case PieceType::SPIDER: return spider_value;
        }
        return 0.0f;
    }
    
    // Serialize to JSON
    nlohmann::json to_json() const {
        return nlohmann::json{
            {"queen_value", queen_value},
            {"ant_value", ant_value},
            {"beetle_value", beetle_value},
            {"grasshopper_value", grasshopper_value},
            {"spider_value", spider_value},
            {"surround_opponent_multiplier", surround_opponent_multiplier},
            {"surround_self_multiplier", surround_self_multiplier},
            {"mobility_weight", mobility_weight},
            {"proximity_weight", proximity_weight},
            {"proximity_max_distance", proximity_max_distance},
            {"ant_freedom_bonus", ant_freedom_bonus},
            {"ant_trapped_penalty", ant_trapped_penalty},
            {"trapped_opponent_ant_bonus", trapped_opponent_ant_bonus},
            {"hand_piece_multiplier", hand_piece_multiplier}
        };
    }
    
    // Deserialize from JSON
    static EvalWeights from_json(const nlohmann::json& j) {
        EvalWeights w;
        if (j.contains("queen_value")) w.queen_value = j["queen_value"];
        if (j.contains("ant_value")) w.ant_value = j["ant_value"];
        if (j.contains("beetle_value")) w.beetle_value = j["beetle_value"];
        if (j.contains("grasshopper_value")) w.grasshopper_value = j["grasshopper_value"];
        if (j.contains("spider_value")) w.spider_value = j["spider_value"];
        if (j.contains("surround_opponent_multiplier")) w.surround_opponent_multiplier = j["surround_opponent_multiplier"];
        if (j.contains("surround_self_multiplier")) w.surround_self_multiplier = j["surround_self_multiplier"];
        if (j.contains("mobility_weight")) w.mobility_weight = j["mobility_weight"];
        if (j.contains("proximity_weight")) w.proximity_weight = j["proximity_weight"];
        if (j.contains("proximity_max_distance")) w.proximity_max_distance = j["proximity_max_distance"];
        if (j.contains("ant_freedom_bonus")) w.ant_freedom_bonus = j["ant_freedom_bonus"];
        if (j.contains("ant_trapped_penalty")) w.ant_trapped_penalty = j["ant_trapped_penalty"];
        if (j.contains("trapped_opponent_ant_bonus")) w.trapped_opponent_ant_bonus = j["trapped_opponent_ant_bonus"];
        if (j.contains("hand_piece_multiplier")) w.hand_piece_multiplier = j["hand_piece_multiplier"];
        return w;
    }
    
    // Save weights to file
    bool save_to_file(const std::string& filename) const {
        std::ofstream file(filename);
        if (!file.is_open()) return false;
        file << to_json().dump(2);
        return true;
    }
    
    // Load weights from file
    static EvalWeights load_from_file(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            return EvalWeights(); // Return defaults
        }
        nlohmann::json j;
        file >> j;
        return from_json(j);
    }
};

// Tunable evaluator class
class TunableEvaluator {
public:
    TunableEvaluator() = default;
    explicit TunableEvaluator(const EvalWeights& weights) : weights_(weights) {}
    
    // Evaluate game state from perspective of player
    float evaluate(const Game& game, PlayerColor player, GameEngine& engine) const;
    
    // Get/set weights
    const EvalWeights& get_weights() const { return weights_; }
    void set_weights(const EvalWeights& weights) { weights_ = weights; }
    
    // Persistence
    bool save_weights(const std::string& filename) const { return weights_.save_to_file(filename); }
    void load_weights(const std::string& filename) { weights_ = EvalWeights::load_from_file(filename); }
    
private:
    EvalWeights weights_;
};

} // namespace bugs
