#include "weight_optimizer.hpp"
#include <iostream>
#include <algorithm>
#include <numeric>
#include <chrono>

namespace bugs {

WeightOptimizer::WeightOptimizer() 
    : config_(), rng_(std::chrono::steady_clock::now().time_since_epoch().count()) {}

WeightOptimizer::WeightOptimizer(const OptimizerConfig& config) 
    : config_(config), rng_(std::chrono::steady_clock::now().time_since_epoch().count()) {}

void WeightOptimizer::set_initial_weights(const EvalWeights& weights) {
    best_weights_ = weights;
}

TrainingStats WeightOptimizer::get_stats() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return stats_;
}

void WeightOptimizer::initialize_population() {
    population_.clear();
    population_.reserve(config_.population_size);
    
    // First individual is the current best (or default)
    population_.push_back(best_weights_);
    
    // Generate mutations of the best weights
    for (int i = 1; i < config_.population_size; i++) {
        population_.push_back(mutate(best_weights_));
    }
    
    fitness_scores_.resize(config_.population_size, 0.0f);
}

float WeightOptimizer::mutate_value(float value, float min_val, float max_val) {
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
    float mutation = dist(rng_) * config_.mutation_strength * std::abs(value);
    float new_value = value + mutation;
    return std::clamp(new_value, min_val, max_val);
}

EvalWeights WeightOptimizer::mutate(const EvalWeights& weights) {
    EvalWeights mutated = weights;
    std::uniform_real_distribution<float> prob_dist(0.0f, 1.0f);
    
    if (prob_dist(rng_) < config_.mutation_rate)
        mutated.queen_value = mutate_value(weights.queen_value, 100.0f, 5000.0f);
    if (prob_dist(rng_) < config_.mutation_rate)
        mutated.ant_value = mutate_value(weights.ant_value, 10.0f, 200.0f);
    if (prob_dist(rng_) < config_.mutation_rate)
        mutated.beetle_value = mutate_value(weights.beetle_value, 10.0f, 200.0f);
    if (prob_dist(rng_) < config_.mutation_rate)
        mutated.grasshopper_value = mutate_value(weights.grasshopper_value, 10.0f, 150.0f);
    if (prob_dist(rng_) < config_.mutation_rate)
        mutated.spider_value = mutate_value(weights.spider_value, 5.0f, 100.0f);
    if (prob_dist(rng_) < config_.mutation_rate)
        mutated.surround_opponent_multiplier = mutate_value(weights.surround_opponent_multiplier, 0.5f, 10.0f);
    if (prob_dist(rng_) < config_.mutation_rate)
        mutated.surround_self_multiplier = mutate_value(weights.surround_self_multiplier, 1.0f, 20.0f);
    if (prob_dist(rng_) < config_.mutation_rate)
        mutated.mobility_weight = mutate_value(weights.mobility_weight, 0.5f, 10.0f);
    if (prob_dist(rng_) < config_.mutation_rate)
        mutated.proximity_weight = mutate_value(weights.proximity_weight, 1.0f, 50.0f);
    if (prob_dist(rng_) < config_.mutation_rate)
        mutated.ant_freedom_bonus = mutate_value(weights.ant_freedom_bonus, 5.0f, 100.0f);
    if (prob_dist(rng_) < config_.mutation_rate)
        mutated.ant_trapped_penalty = mutate_value(weights.ant_trapped_penalty, 5.0f, 50.0f);
    if (prob_dist(rng_) < config_.mutation_rate)
        mutated.trapped_opponent_ant_bonus = mutate_value(weights.trapped_opponent_ant_bonus, 10.0f, 100.0f);
    if (prob_dist(rng_) < config_.mutation_rate)
        mutated.hand_piece_multiplier = mutate_value(weights.hand_piece_multiplier, 0.1f, 1.0f);
    
    return mutated;
}

EvalWeights WeightOptimizer::crossover(const EvalWeights& parent1, const EvalWeights& parent2) {
    EvalWeights child;
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    
    // Blend crossover - average with random interpolation
    auto blend = [&](float v1, float v2) {
        float alpha = dist(rng_);
        return alpha * v1 + (1.0f - alpha) * v2;
    };
    
    child.queen_value = blend(parent1.queen_value, parent2.queen_value);
    child.ant_value = blend(parent1.ant_value, parent2.ant_value);
    child.beetle_value = blend(parent1.beetle_value, parent2.beetle_value);
    child.grasshopper_value = blend(parent1.grasshopper_value, parent2.grasshopper_value);
    child.spider_value = blend(parent1.spider_value, parent2.spider_value);
    child.surround_opponent_multiplier = blend(parent1.surround_opponent_multiplier, parent2.surround_opponent_multiplier);
    child.surround_self_multiplier = blend(parent1.surround_self_multiplier, parent2.surround_self_multiplier);
    child.mobility_weight = blend(parent1.mobility_weight, parent2.mobility_weight);
    child.proximity_weight = blend(parent1.proximity_weight, parent2.proximity_weight);
    child.proximity_max_distance = blend(parent1.proximity_max_distance, parent2.proximity_max_distance);
    child.ant_freedom_bonus = blend(parent1.ant_freedom_bonus, parent2.ant_freedom_bonus);
    child.ant_trapped_penalty = blend(parent1.ant_trapped_penalty, parent2.ant_trapped_penalty);
    child.trapped_opponent_ant_bonus = blend(parent1.trapped_opponent_ant_bonus, parent2.trapped_opponent_ant_bonus);
    child.hand_piece_multiplier = blend(parent1.hand_piece_multiplier, parent2.hand_piece_multiplier);
    
    return child;
}

int WeightOptimizer::tournament_select() {
    std::uniform_int_distribution<int> dist(0, static_cast<int>(population_.size()) - 1);
    
    int best_idx = dist(rng_);
    float best_fit = fitness_scores_[best_idx];
    
    for (int i = 1; i < config_.tournament_size; i++) {
        int idx = dist(rng_);
        if (fitness_scores_[idx] > best_fit) {
            best_idx = idx;
            best_fit = fitness_scores_[idx];
        }
    }
    
    return best_idx;
}

float WeightOptimizer::evaluate_individual(const EvalWeights& weights) {
    SelfPlayConfig sp_config;
    sp_config.ai_depth = config_.ai_depth;
    sp_config.max_moves = 150;
    sp_config.verbose = false;
    
    SelfPlayEngine engine(sp_config);
    
    // Play against the current best
    float score = engine.evaluate_matchup(weights, best_weights_, config_.games_per_evaluation);
    
    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.games_played += config_.games_per_evaluation;
    }
    
    return score;
}

void WeightOptimizer::evaluate_population() {
    for (int i = 0; i < static_cast<int>(population_.size()) && !should_stop_; i++) {
        fitness_scores_[i] = evaluate_individual(population_[i]);
        
        if (fitness_scores_[i] > best_fitness_) {
            best_fitness_ = fitness_scores_[i];
            best_weights_ = population_[i];
            
            // Save improved weights
            best_weights_.save_to_file(config_.weights_file);
        }
    }
}

void WeightOptimizer::evolve_generation() {
    // Sort population by fitness (descending)
    std::vector<size_t> indices(population_.size());
    std::iota(indices.begin(), indices.end(), 0);
    std::sort(indices.begin(), indices.end(), [this](size_t a, size_t b) {
        return fitness_scores_[a] > fitness_scores_[b];
    });
    
    std::vector<EvalWeights> new_population;
    new_population.reserve(config_.population_size);
    
    // Keep elites
    for (int i = 0; i < config_.elite_count && i < static_cast<int>(indices.size()); i++) {
        new_population.push_back(population_[indices[i]]);
    }
    
    // Generate rest through crossover and mutation
    while (static_cast<int>(new_population.size()) < config_.population_size) {
        int parent1_idx = tournament_select();
        int parent2_idx = tournament_select();
        
        EvalWeights child = crossover(population_[parent1_idx], population_[parent2_idx]);
        child = mutate(child);
        new_population.push_back(child);
    }
    
    population_ = std::move(new_population);
}

void WeightOptimizer::update_stats(int generation) {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    
    stats_.current_generation = generation;
    stats_.total_generations = config_.generations;
    stats_.best_fitness = best_fitness_;
    stats_.best_weights = best_weights_;
    
    if (!fitness_scores_.empty()) {
        float sum = std::accumulate(fitness_scores_.begin(), fitness_scores_.end(), 0.0f);
        stats_.average_fitness = sum / static_cast<float>(fitness_scores_.size());
    }
    
    stats_.status_message = "Generation " + std::to_string(generation) + "/" + 
                            std::to_string(config_.generations) + 
                            " - Best: " + std::to_string(best_fitness_);
}

void WeightOptimizer::train() {
    is_running_ = true;
    should_stop_ = false;
    
    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.is_running = true;
        stats_.games_played = 0;
        stats_.current_generation = 0;
        stats_.status_message = "Initializing population...";
    }
    
    std::cout << "=== Starting Evolutionary Training ===" << std::endl;
    std::cout << "Population: " << config_.population_size << std::endl;
    std::cout << "Generations: " << config_.generations << std::endl;
    std::cout << "Games per eval: " << config_.games_per_evaluation << std::endl;
    
    initialize_population();
    
    for (int gen = 1; gen <= config_.generations && !should_stop_; gen++) {
        std::cout << "\n--- Generation " << gen << " ---" << std::endl;
        
        evaluate_population();
        
        if (should_stop_) break;
        
        update_stats(gen);
        
        if (progress_callback_) {
            progress_callback_(get_stats());
        }
        
        std::cout << "Best fitness: " << best_fitness_ << std::endl;
        
        if (gen < config_.generations) {
            evolve_generation();
        }
    }
    
    // Save final weights
    best_weights_.save_to_file(config_.weights_file);
    
    std::cout << "\n=== Training Complete ===" << std::endl;
    std::cout << "Final best fitness: " << best_fitness_ << std::endl;
    std::cout << "Weights saved to: " << config_.weights_file << std::endl;
    
    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.is_running = false;
        stats_.status_message = should_stop_ ? "Stopped by user" : "Training complete";
    }
    
    is_running_ = false;
}

void WeightOptimizer::start_training_async() {
    if (is_running_) {
        return;
    }
    
    training_thread_ = std::make_unique<std::thread>([this]() {
        train();
    });
    training_thread_->detach();
}

void WeightOptimizer::stop() {
    should_stop_ = true;
}

} // namespace bugs
