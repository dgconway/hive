#pragma once

#include "tunable_evaluator.hpp"
#include "self_play.hpp"
#include <vector>
#include <random>
#include <atomic>
#include <functional>
#include <thread>
#include <mutex>

namespace bugs {

// Configuration for evolutionary optimization
struct OptimizerConfig {
    int population_size = 10;           // Number of weight configurations to maintain
    int games_per_evaluation = 4;       // Games to play for fitness evaluation
    int generations = 50;               // Number of evolution iterations
    float mutation_rate = 0.3f;         // Probability of mutating each weight
    float mutation_strength = 0.2f;     // Magnitude of mutations (as fraction of value)
    int elite_count = 2;                // Top performers to keep unchanged
    int ai_depth = 2;                   // Search depth for training games (lower = faster)
    int tournament_size = 3;            // Size of tournament selection
    std::string weights_file = "weights.json";  // File to save best weights
};

// Training statistics
struct TrainingStats {
    int current_generation = 0;
    int total_generations = 0;
    int games_played = 0;
    float best_fitness = 0.0f;
    float average_fitness = 0.0f;
    EvalWeights best_weights;
    bool is_running = false;
    std::string status_message;
};

// Evolutionary weight optimizer
class WeightOptimizer {
public:
    WeightOptimizer();
    explicit WeightOptimizer(const OptimizerConfig& config);
    
    // Start training (blocking)
    void train();
    
    // Start training in background thread
    void start_training_async();
    
    // Stop training
    void stop();
    
    // Check if training is running
    bool is_running() const { return is_running_.load(); }
    
    // Get current training statistics
    TrainingStats get_stats() const;
    
    // Get/set the best weights found
    EvalWeights get_best_weights() const { return best_weights_; }
    void set_initial_weights(const EvalWeights& weights);
    
    // Progress callback
    using ProgressCallback = std::function<void(const TrainingStats&)>;
    void set_progress_callback(ProgressCallback callback) { progress_callback_ = callback; }
    
private:
    OptimizerConfig config_;
    std::vector<EvalWeights> population_;
    std::vector<float> fitness_scores_;
    EvalWeights best_weights_;
    float best_fitness_ = 0.0f;
    
    std::atomic<bool> should_stop_{false};
    std::atomic<bool> is_running_{false};
    TrainingStats stats_;
    mutable std::mutex stats_mutex_;
    std::unique_ptr<std::thread> training_thread_;
    
    ProgressCallback progress_callback_;
    std::mt19937 rng_;
    
    // Evolution operations
    void initialize_population();
    void evaluate_population();
    float evaluate_individual(const EvalWeights& weights);
    EvalWeights mutate(const EvalWeights& weights);
    EvalWeights crossover(const EvalWeights& parent1, const EvalWeights& parent2);
    int tournament_select();
    void evolve_generation();
    void update_stats(int generation);
    
    // Mutate a single weight value
    float mutate_value(float value, float min_val, float max_val);
};

} // namespace bugs
