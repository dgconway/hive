#pragma once

#include "models.hpp"
#include "game_interface.hpp"
#include "evaluator.hpp"
#include "zobrist.hpp"
#include <unordered_map>
#include <string>
#include <optional>
#include <memory>
#include <array>
#include <cstdint>
#include <mutex>

namespace bugs {

// Transposition table entry bounds
enum class TTBound {
    EXACT,      // Exact score
    LOWER,      // Beta cutoff (score >= stored)
    UPPER       // Alpha cutoff (score <= stored)
};

// Transposition table entry
struct TTEntry {
    float score = 0.0f;
    int depth = 0;
    TTBound bound = TTBound::EXACT;
    std::optional<Action> best_action;
};

// Killer move slots per depth
static constexpr int KILLER_SLOTS = 2;
static constexpr int MAX_DEPTH = 16;

class MinimaxAI {
public:
    explicit MinimaxAI(int depth = 4);
    
    std::optional<MoveRequest> get_best_move(const Game& game);
    
    // Get benchmark statistics
    std::string get_benchmark_report() const;
    void reset_benchmarks();
    
private:
    int depth_;
    GameInterface interface_;
    GameEngine engine_;
    mutable std::mutex minmax_mutex_;
    
    // Improved transposition table using Zobrist hash
    std::unordered_map<uint64_t, TTEntry> transposition_table_;
    
    // Killer moves for move ordering (indexed by depth)
    std::array<std::array<std::optional<Action>, KILLER_SLOTS>, MAX_DEPTH> killer_moves_;
    
    // History heuristic scores
    std::unordered_map<uint64_t, int> history_scores_;
    
    // Search statistics
    std::atomic<int64_t> nodes_searched_ = 0;
    std::atomic<int64_t> tt_hits_ = 0;
    std::atomic<int64_t> tt_cutoffs_ = 0;
    
    // Legacy string-based key (kept for compatibility)
    std::string get_state_key(const GameState& state);
    
    // Get Zobrist hash for a state
    uint64_t get_zobrist_hash(const GameState& state) const;
    
    // Main search with iterative deepening
    std::pair<float, std::optional<Action>> iterative_deepening(
        const GameState& state,
        int max_depth,
        PlayerColor player
    );
    
    std::pair<float, std::optional<Action>> minimax(
        const GameState& state,
        int depth,
        float alpha,
        float beta,
        bool is_maximizing,
        PlayerColor player,
        int ply  // Current ply from root
    );
    
    // Move ordering
    float score_action(const Action& action, const GameState& state, PlayerColor player, int ply);
    void update_killer_move(const Action& action, int ply);
    void update_history_score(const Action& action, int depth);
    uint64_t action_hash(const Action& action) const;
    
    // Check if action matches a killer move
    bool is_killer_move(const Action& action, int ply) const;
};

} // namespace bugs
