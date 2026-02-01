#pragma once

#include "models.hpp"
#include "game_logic.hpp"
#include "game_interface.hpp"
#include "tunable_evaluator.hpp"
#include <vector>
#include <memory>
#include <random>
#include <functional>
#include <atomic>

namespace bugs {

// Result of a single self-play game
struct GameResult {
    std::optional<PlayerColor> winner;
    int total_moves;
    std::vector<MoveLog> move_history;
    float white_final_eval;
    float black_final_eval;
    bool was_draw;
    
    // Return 1.0 for win, 0.5 for draw, 0.0 for loss
    float get_score(PlayerColor player) const {
        if (was_draw || !winner.has_value()) return 0.5f;
        return (winner.value() == player) ? 1.0f : 0.0f;
    }
};

// Configuration for self-play
struct SelfPlayConfig {
    int max_moves = 200;        // Max moves before declaring draw
    int ai_depth = 3;           // Search depth for AI (lower for faster training)
    bool verbose = false;       // Print game progress
    int num_games = 10;         // Number of games per evaluation
};

// Forward declaration
class TunableMinimaxAI;

// Self-play manager
class SelfPlayEngine {
public:
    SelfPlayEngine();
    explicit SelfPlayEngine(const SelfPlayConfig& config);
    
    // Run a single game between two weight configurations
    GameResult run_game(const EvalWeights& white_weights, const EvalWeights& black_weights);
    
    // Run multiple games and return aggregate results
    std::vector<GameResult> run_tournament(
        const EvalWeights& weights_a, 
        const EvalWeights& weights_b,
        int num_games
    );
    
    // Calculate win rate for weights_a against weights_b
    float evaluate_matchup(
        const EvalWeights& weights_a, 
        const EvalWeights& weights_b,
        int num_games
    );
    
    // Stop ongoing training
    void stop() { should_stop_ = true; }
    bool is_stopped() const { return should_stop_.load(); }
    void reset_stop() { should_stop_ = false; }
    
    // Progress callback
    using ProgressCallback = std::function<void(int current, int total, const std::string& message)>;
    void set_progress_callback(ProgressCallback callback) { progress_callback_ = callback; }
    
private:
    SelfPlayConfig config_;
    GameEngine engine_;
    GameInterface interface_;
    std::atomic<bool> should_stop_{false};
    ProgressCallback progress_callback_;
    
    void report_progress(int current, int total, const std::string& message);
};

// A version of MinimaxAI that uses TunableEvaluator
class TunableMinimaxAI {
public:
    TunableMinimaxAI(int depth, const EvalWeights& weights);
    
    std::optional<MoveRequest> get_best_move(const Game& game);
    
private:
    int depth_;
    TunableEvaluator evaluator_;
    GameInterface interface_;
    GameEngine engine_;
    
    std::pair<float, std::optional<Action>> minimax(
        const GameState& state,
        int depth,
        float alpha,
        float beta,
        bool is_maximizing,
        PlayerColor player
    );
};

} // namespace bugs
