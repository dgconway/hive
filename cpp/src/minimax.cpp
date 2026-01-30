#include <algorithm>
#include <sstream>
#include <iostream>
#include <chrono>
#include <omp.h>
#include <mutex>

namespace bugs {

MinimaxAI::MinimaxAI(int depth) : depth_(depth) {
    // Initialize killer moves to empty
    for (auto& depth_killers : killer_moves_) {
        for (auto& killer : depth_killers) {
            killer = std::nullopt;
        }
    }
}

std::string MinimaxAI::get_state_key(const GameState& state) {
    BENCHMARK_SCOPE("get_state_key");
    std::stringstream ss;
    
    // Sort board keys for consistent hashing
    std::vector<std::string> sorted_keys;
    sorted_keys.reserve(state.game.board.size());
    for (const auto& [key, stack] : state.game.board) {
        if (!stack.empty()) {
            sorted_keys.push_back(key);
        }
    }
    std::sort(sorted_keys.begin(), sorted_keys.end());
    
    for (const auto& key : sorted_keys) {
        const auto& stack = state.game.board.at(key);
        ss << key << ":";
        for (const auto& p : stack) {
            ss << to_string(p.type)[0] << to_string(p.color)[0];
        }
        ss << "|";
    }
    
    ss << "T:" << to_string(state.game.current_turn)[0];
    
    // Include hands
    ss << "|W:";
    for (const auto& [type, count] : state.game.white_pieces_hand) {
        ss << to_string(type)[0] << count;
    }
    ss << "|B:";
    for (const auto& [type, count] : state.game.black_pieces_hand) {
        ss << to_string(type)[0] << count;
    }
    
    return ss.str();
}

uint64_t MinimaxAI::get_zobrist_hash(const GameState& state) const {
    return compute_zobrist_hash(state.game);
}

uint64_t MinimaxAI::action_hash(const Action& action) const {
    uint64_t hash = static_cast<uint64_t>(action.action_type);
    hash ^= static_cast<uint64_t>(action.to_hex.first + 100) << 8;
    hash ^= static_cast<uint64_t>(action.to_hex.second + 100) << 16;
    if (action.from_hex) {
        hash ^= static_cast<uint64_t>(action.from_hex->first + 100) << 24;
        hash ^= static_cast<uint64_t>(action.from_hex->second + 100) << 32;
    }
    if (action.piece_type) {
        hash ^= static_cast<uint64_t>(*action.piece_type) << 40;
    }
    return hash;
}


bool MinimaxAI::is_killer_move(const Action& action, int ply) const {
    if (ply >= MAX_DEPTH) return false;
    std::lock_guard<std::mutex> lock(minmax_mutex_);
    for (const auto& killer : killer_moves_[ply]) {
        if (killer.has_value()) {
            const auto& k = *killer;
            if (k.action_type == action.action_type &&
                k.to_hex == action.to_hex &&
                k.from_hex == action.from_hex &&
                k.piece_type == action.piece_type) {
                return true;
            }
        }
    }
    return false;
}

void MinimaxAI::update_killer_move(const Action& action, int ply) {
    if (ply >= MAX_DEPTH) return;
    // Don't store captures as killers (they have their own ordering)
    if (action.action_type != ActionType::MOVE) return;
    
    std::lock_guard<std::mutex> lock(minmax_mutex_);
    // Shift killer moves and insert new one
    killer_moves_[ply][1] = killer_moves_[ply][0];
    killer_moves_[ply][0] = action;
}

void MinimaxAI::update_history_score(const Action& action, int depth) {
    uint64_t hash = action_hash(action);
    std::lock_guard<std::mutex> lock(minmax_mutex_);
    history_scores_[hash] += depth * depth;  // Deeper moves get more weight
}

float MinimaxAI::score_action(const Action& action, const GameState& state, PlayerColor player, int ply) {
    float score = 0.0f;
    
    // TT best move gets highest priority (handled separately in move ordering)
    
    // Killer moves get high priority
    if (is_killer_move(action, ply)) {
        score += 5000.0f;
    }
    
    // History heuristic
    {
        std::lock_guard<std::mutex> lock(minmax_mutex_);
        auto hist_it = history_scores_.find(action_hash(action));
        if (hist_it != history_scores_.end()) {
            score += std::min(hist_it->second * 0.1f, 1000.0f);
        }
    }
    
    if (action.action_type == ActionType::PLACE) {
        if (action.piece_type == PieceType::QUEEN) score += 2000.0f;
        
        // Discourage Ants in early game - they get trapped easily
        if (action.piece_type == PieceType::ANT) {
            if (state.game.turn_number >= 6) {
                score += 40.0f;
            } else {
                score += 5.0f;
            }
        }
        
        if (action.piece_type == PieceType::GRASSHOPPER) score += 30.0f;
        if (action.piece_type == PieceType::BEETLE) score += 25.0f;
        if (action.piece_type == PieceType::SPIDER) score += 15.0f;
    } else if (action.action_type == ActionType::MOVE) {
        score += 50.0f;
    }
    
    return score;
}

void MinimaxAI::update_killer_move(const Action& action, int ply) {
    if (ply >= MAX_DEPTH) return;
    // Don't store captures as killers (they have their own ordering)
    if (action.action_type != ActionType::MOVE) return;
    
    // Shift killer moves and insert new one
    killer_moves_[ply][1] = killer_moves_[ply][0];
    killer_moves_[ply][0] = action;
}

void MinimaxAI::update_history_score(const Action& action, int depth) {
    uint64_t hash = action_hash(action);
    history_scores_[hash] += depth * depth;  // Deeper moves get more weight
}

float MinimaxAI::score_action(const Action& action, const GameState& state, PlayerColor player, int ply) {
    float score = 0.0f;
    
    // TT best move gets highest priority (handled separately in move ordering)
    
    // Killer moves get high priority
    if (is_killer_move(action, ply)) {
        score += 5000.0f;
    }
    
    // History heuristic
    auto hist_it = history_scores_.find(action_hash(action));
    if (hist_it != history_scores_.end()) {
        score += std::min(hist_it->second * 0.1f, 1000.0f);
    }
    
    if (action.action_type == ActionType::PLACE) {
        if (action.piece_type == PieceType::QUEEN) score += 2000.0f;
        
        // Discourage Ants in early game - they get trapped easily
        if (action.piece_type == PieceType::ANT) {
            if (state.game.turn_number >= 6) {
                score += 40.0f;
            } else {
                score += 5.0f;
            }
        }
        
        if (action.piece_type == PieceType::GRASSHOPPER) score += 30.0f;
        if (action.piece_type == PieceType::BEETLE) score += 25.0f;
        if (action.piece_type == PieceType::SPIDER) score += 15.0f;
    } else if (action.action_type == ActionType::MOVE) {
        score += 50.0f;
        
        // Bonus for moves that attack opponent queen
        // (would need queen position to implement fully)
    }
    
    return score;
}

std::optional<MoveRequest> MinimaxAI::get_best_move(const Game& game) {
    BENCHMARK_SCOPE("get_best_move");
    
    GameState state(game);
    PlayerColor player = game.current_turn;
    
    // Reset search statistics
    nodes_searched_ = 0;
    tt_hits_ = 0;
    tt_cutoffs_ = 0;
    
    // === OPENING BOOK ===
    int ai_pieces_played = 0;
    for (const auto& [key, stack] : game.board) {
        for (const auto& piece : stack) {
            if (piece.color == player) {
                ai_pieces_played++;
            }
        }
    }
    
    // First move: Play Grasshopper
    if (ai_pieces_played == 0) {
        auto legal_actions = interface_.get_legal_actions(state);
        for (const auto& action : legal_actions) {
            if (action.action_type == ActionType::PLACE && 
                action.piece_type == PieceType::GRASSHOPPER) {
                std::cout << "Opening Book: Playing GRASSHOPPER" << std::endl;
                return action.to_move_request();
            }
        }
    }
    
    // Second move: Play Queen Bee
    if (ai_pieces_played == 1) {
        auto legal_actions = interface_.get_legal_actions(state);
        for (const auto& action : legal_actions) {
            if (action.action_type == ActionType::PLACE && 
                action.piece_type == PieceType::QUEEN) {
                std::cout << "Opening Book: Playing QUEEN" << std::endl;
                return action.to_move_request();
            }
        }
    }
    
    // === ITERATIVE DEEPENING SEARCH ===
    auto start_time = std::chrono::high_resolution_clock::now();
    
    auto [score, best_action] = iterative_deepening(state, depth_, player);
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "Minimax complete. Score: " << score 
              << ", Time: " << duration.count() / 1000.0 << "s"
              << ", Nodes: " << nodes_searched_
              << ", TT hits: " << tt_hits_
              << ", TT cutoffs: " << tt_cutoffs_
              << std::endl;
    
    return best_action.has_value() ? std::make_optional(best_action->to_move_request()) : std::nullopt;
}

std::pair<float, std::optional<Action>> MinimaxAI::iterative_deepening(
    const GameState& state,
    int max_depth,
    PlayerColor player) {
    
    BENCHMARK_SCOPE("iterative_deepening");
    
    std::optional<Action> best_action;
    float best_score = -std::numeric_limits<float>::infinity();
    
    // Initial legal actions
    auto legal_actions = interface_.get_legal_actions(state);
    if (legal_actions.empty()) {
        float score = evaluate_state(state.game, player, engine_);
        return {score, std::nullopt};
    }

    // Search from depth 1 to max_depth
    for (int depth = 1; depth <= max_depth; ++depth) {
        // Move ordering: PV first, then others
        // We use score_action which uses killer/history/heuristics
        std::sort(legal_actions.begin(), legal_actions.end(),
            [this, &state, player, &best_action](const Action& a, const Action& b) {
                // If we have a best action from previous iteration, prioritize it
                bool a_is_best = best_action.has_value() && 
                    a.action_type == best_action->action_type &&
                    a.to_hex == best_action->to_hex &&
                    a.from_hex == best_action->from_hex;
                bool b_is_best = best_action.has_value() && 
                    b.action_type == best_action->action_type &&
                    b.to_hex == best_action->to_hex &&
                    b.from_hex == best_action->from_hex;
                
                if (a_is_best && !b_is_best) return true;
                if (b_is_best && !a_is_best) return false;
                
                return score_action(a, state, player, 0) > score_action(b, state, player, 0);
            });

        float current_iter_alpha = -std::numeric_limits<float>::infinity();
        float current_iter_beta = std::numeric_limits<float>::infinity();
        
        std::optional<Action> iteration_best_action;
        float iteration_best_score = -std::numeric_limits<float>::infinity();
        
        // 1. Search PV move serially
        {
            GameState new_state = interface_.apply_action(state, legal_actions[0]);
            auto [val, _] = minimax(new_state, depth - 1, current_iter_alpha, current_iter_beta, false, player, 1);
            if (val > iteration_best_score) {
                iteration_best_score = val;
                iteration_best_action = legal_actions[0];
                current_iter_alpha = val; // Update alpha for subsequent searches
            }
        }
        
        // 2. Search other moves in parallel with updated alpha
        // We can't update alpha across threads easily in pure OpenMP, so each thread sees the initial alpha from PV
        // But the TT is shared and locked, which might help cutoffs.
        
        std::vector<std::pair<float, Action>> parallel_results;
        parallel_results.resize(legal_actions.size());
        
        // Fix initial results for 0-th element
        parallel_results[0] = {iteration_best_score, legal_actions[0]};

        #pragma omp parallel for schedule(dynamic)
        for (int i = 1; i < (int)legal_actions.size(); ++i) {
            GameState new_state = interface_.apply_action(state, legal_actions[i]);
            // Use current_iter_alpha as lower bound, but valid window is still important
            auto [val, _] = minimax(new_state, depth - 1, current_iter_alpha, current_iter_beta, false, player, 1);
            parallel_results[i] = {val, legal_actions[i]};
        }
        
        // 3. Collect results
        for (int i = 1; i < (int)legal_actions.size(); ++i) {
            if (parallel_results[i].first > iteration_best_score) {
                iteration_best_score = parallel_results[i].first;
                iteration_best_action = parallel_results[i].second;
            }
        }
        
        if (iteration_best_action.has_value()) {
            best_action = iteration_best_action;
            best_score = iteration_best_score;
        }
        
        std::cout << "  Depth " << depth << ": score=" << best_score << std::endl;
        
        if (best_score > 500000.0f) break;
    }
    
    return {best_score, best_action};
}

std::pair<float, std::optional<Action>> MinimaxAI::minimax(
    const GameState& state,
    int depth,
    float alpha,
    float beta,
    bool is_maximizing,
    PlayerColor player,
    int ply) {
    
    BENCHMARK_SCOPE_DEBUG("minimax");
    nodes_searched_++;
    
    // Zobrist hash for transposition table
    // Zobrist hash for transposition table
    uint64_t state_hash = get_zobrist_hash(state);
    
    // Transposition table lookup
    {
        std::lock_guard<std::mutex> lock(minmax_mutex_);
        auto tt_it = transposition_table_.find(state_hash);
        if (tt_it != transposition_table_.end()) {
            const TTEntry& entry = tt_it->second;
            if (entry.depth >= depth) {
                tt_hits_++;
                // Check bound type
                if (entry.bound == TTBound::EXACT) {
                    tt_cutoffs_++;
                    return {entry.score, entry.best_action};
                } else if (entry.bound == TTBound::LOWER && entry.score >= beta) {
                    tt_cutoffs_++;
                    return {entry.score, entry.best_action};
                } else if (entry.bound == TTBound::UPPER && entry.score <= alpha) {
                    tt_cutoffs_++;
                    return {entry.score, entry.best_action};
                }
            }
        }
    }
    
    // Terminal or depth limit
    if (depth == 0 || state.is_terminal()) {
        BENCHMARK_SCOPE("evaluate_state");
        float score = evaluate_state(state.game, player, engine_);
        
        // Store in TT
        std::lock_guard<std::mutex> lock(minmax_mutex_);
        TTEntry entry;
        entry.score = score;
        entry.depth = depth;
        entry.bound = TTBound::EXACT;
        transposition_table_[state_hash] = entry;
        
        return {score, std::nullopt};
    }
    
    // Get legal actions
    std::vector<Action> legal_actions;
    {
        BENCHMARK_SCOPE("get_legal_actions");
        legal_actions = interface_.get_legal_actions(state);
    }
    
    if (legal_actions.empty()) {
        float score = evaluate_state(state.game, player, engine_);
        return {score, std::nullopt};
    }
    
    // Move ordering: TT best move first, then sorted by score
    std::optional<Action> tt_best_action;
    {
        std::lock_guard<std::mutex> lock(minmax_mutex_);
        auto tt_it = transposition_table_.find(state_hash);
        if (tt_it != transposition_table_.end() && tt_it->second.best_action.has_value()) {
            tt_best_action = tt_it->second.best_action;
        }
    }
    
    {
        BENCHMARK_SCOPE("move_ordering");
        std::sort(legal_actions.begin(), legal_actions.end(),
            [this, &state, player, ply, &tt_best_action](const Action& a, const Action& b) {
                // TT best move gets highest priority
                bool a_is_tt = tt_best_action.has_value() && 
                    a.action_type == tt_best_action->action_type &&
                    a.to_hex == tt_best_action->to_hex &&
                    a.from_hex == tt_best_action->from_hex;
                bool b_is_tt = tt_best_action.has_value() && 
                    b.action_type == tt_best_action->action_type &&
                    b.to_hex == tt_best_action->to_hex &&
                    b.from_hex == tt_best_action->from_hex;
                
                if (a_is_tt && !b_is_tt) return true;
                if (b_is_tt && !a_is_tt) return false;
                
                return score_action(a, state, player, ply) > score_action(b, state, player, ply);
            });
    }
    
    std::optional<Action> best_action;
    float original_alpha = alpha;
    
    if (is_maximizing) {
        float curr_max = -std::numeric_limits<float>::infinity();
        for (const auto& action : legal_actions) {
            GameState new_state = interface_.apply_action(state, action);
            auto [eval_val, _] = minimax(new_state, depth - 1, alpha, beta, false, player, ply + 1);
            
            if (eval_val > curr_max) {
                curr_max = eval_val;
                best_action = action;
            }
            alpha = std::max(alpha, eval_val);
            if (beta <= alpha) {
                // Beta cutoff - update killer and history
                update_killer_move(action, ply);
                update_history_score(action, depth);
                break;
            }
        }
        
        // Store in transposition table
        {
            std::lock_guard<std::mutex> lock(minmax_mutex_);
            TTEntry entry;
            entry.score = curr_max;
            entry.depth = depth;
            entry.best_action = best_action;
            if (curr_max <= original_alpha) {
                entry.bound = TTBound::UPPER;
            } else if (curr_max >= beta) {
                entry.bound = TTBound::LOWER;
            } else {
                entry.bound = TTBound::EXACT;
            }
            transposition_table_[state_hash] = entry;
        }
        
        return {curr_max, best_action};
    } else {
        float curr_min = std::numeric_limits<float>::infinity();
        for (const auto& action : legal_actions) {
            GameState new_state = interface_.apply_action(state, action);
            auto [eval_val, _] = minimax(new_state, depth - 1, alpha, beta, true, player, ply + 1);
            
            if (eval_val < curr_min) {
                curr_min = eval_val;
                best_action = action;
            }
            beta = std::min(beta, eval_val);
            if (beta <= alpha) {
                update_killer_move(action, ply);
                update_history_score(action, depth);
                break;
            }
        }
        
        // Store in transposition table
        {
            std::lock_guard<std::mutex> lock(minmax_mutex_);
            TTEntry entry;
            entry.score = curr_min;
            entry.depth = depth;
            entry.best_action = best_action;
            if (curr_min >= beta) {
                entry.bound = TTBound::LOWER;
            } else if (curr_min <= original_alpha) {
                entry.bound = TTBound::UPPER;
            } else {
                entry.bound = TTBound::EXACT;
            }
            transposition_table_[state_hash] = entry;
        }
        
        return {curr_min, best_action};
    }
}

std::string MinimaxAI::get_benchmark_report() const {
    return Benchmark::instance().report();
}

void MinimaxAI::reset_benchmarks() {
    Benchmark::instance().reset();
}

} // namespace bugs
