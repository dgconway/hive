#include "httplib.h"
#include "json.hpp"
#include "game_logic.hpp"
#include "minimax.hpp"
#include "benchmark.hpp"
#include "weight_optimizer.hpp"
#include "tunable_evaluator.hpp"
#include <iostream>
#include <memory>

using namespace bugs;
using json = nlohmann::json;

// Global game engine and AI
std::unique_ptr<GameEngine> g_engine;
std::unique_ptr<MinimaxAI> g_minimax_ai;
std::unique_ptr<WeightOptimizer> g_optimizer;
EvalWeights g_current_weights;

void setup_routes(httplib::Server& svr) {
    // POST /games - Create new game
    svr.Post("/games", [](const httplib::Request& req, httplib::Response& res) {
        try {
            Game game = g_engine->create_game();
            json j = game;
            res.set_content(j.dump(), "application/json");
            res.status = 200;
        } catch (const std::exception& e) {
            json error = {{"error", e.what()}};
            res.set_content(error.dump(), "application/json");
            res.status = 500;
        }
    });
    
    // GET /games/:game_id - Get game state
    svr.Get(R"(/games/([^/]+))", [](const httplib::Request& req, httplib::Response& res) {
        try {
            std::string game_id = req.matches[1];
            auto game_opt = g_engine->get_game(game_id);
            
            if (!game_opt.has_value()) {
                json error = {{"error", "Game not found"}};
                res.set_content(error.dump(), "application/json");
                res.status = 404;
                return;
            }
            
            json j = game_opt.value();
            res.set_content(j.dump(), "application/json");
            res.status = 200;
        } catch (const std::exception& e) {
            json error = {{"error", e.what()}};
            res.set_content(error.dump(), "application/json");
            res.status = 500;
        }
    });
    
    // POST /games/:game_id/move - Submit move
    svr.Post(R"(/games/([^/]+)/move)", [](const httplib::Request& req, httplib::Response& res) {
        try {
            std::string game_id = req.matches[1];
            json request_json = json::parse(req.body);
            MoveRequest move = request_json.get<MoveRequest>();
            
            Game updated_game = g_engine->process_move(game_id, move);
            json j = updated_game;
            res.set_content(j.dump(), "application/json");
            res.status = 200;
        } catch (const json::exception& e) {
            json error = {{"error", std::string("JSON parse error: ") + e.what()}};
            res.set_content(error.dump(), "application/json");
            res.status = 400;
        } catch (const std::runtime_error& e) {
            json error = {{"error", e.what()}};
            res.set_content(error.dump(), "application/json");
            res.status = 400;
        } catch (const std::exception& e) {
            json error = {{"error", e.what()}};
            res.set_content(error.dump(), "application/json");
            res.status = 500;
        }
    });
    
    // GET /games/:game_id/valid_moves?q=&r= - Get valid moves for piece
    svr.Get(R"(/games/([^/]+)/valid_moves)", [](const httplib::Request& req, httplib::Response& res) {
        try {
            std::string game_id = req.matches[1];
            
            if (!req.has_param("q") || !req.has_param("r")) {
                json error = {{"error", "Missing q or r parameter"}};
                res.set_content(error.dump(), "application/json");
                res.status = 400;
                return;
            }
            
            int q = std::stoi(req.get_param_value("q"));
            int r = std::stoi(req.get_param_value("r"));
            
            auto moves = g_engine->get_valid_moves(game_id, q, r);
            
            // Convert to JSON array of arrays
            json moves_json = json::array();
            for (const auto& move : moves) {
                moves_json.emplace_back(json::array({move.first, move.second}));
            }
            
            res.set_content(moves_json.dump(), "application/json");
            res.status = 200;
        } catch (const std::invalid_argument& e) {
            json error = {{"error", "Invalid q or r parameter"}};
            res.set_content(error.dump(), "application/json");
            res.status = 400;
        } catch (const std::exception& e) {
            json error = {{"error", e.what()}};
            res.set_content(error.dump(), "application/json");
            res.status = 500;
        }
    });
    
    // POST /games/:game_id/ai_move?ai_type=minimax - AI move
    svr.Post(R"(/games/([^/]+)/ai_move)", [](const httplib::Request& req, httplib::Response& res) {
        try {
            std::string game_id = req.matches[1];
            std::string ai_type = req.has_param("ai_type") ? req.get_param_value("ai_type") : "minimax";
            
            std::cout << "AI move requested for game: " << game_id << std::endl;
            
            auto game_opt = g_engine->get_game(game_id);
            if (!game_opt.has_value()) {
                json error = {{"error", "Game not found"}};
                res.set_content(error.dump(), "application/json");
                res.status = 404;
                return;
            }
            
            Game game = game_opt.value();
            
            std::cout << "Current turn: " << to_string(game.current_turn) 
                      << ", Turn number: " << game.turn_number << std::endl;
            
            if (game.status != GameStatus::IN_PROGRESS) {
                json error = {{"error", "Game is already finished"}};
                res.set_content(error.dump(), "application/json");
                res.status = 400;
                return;
            }
            
            // Get AI move
            std::cout << "Calling AI to get best move..." << std::endl;
            auto move_opt = g_minimax_ai->get_best_move(game);
            
            if (!move_opt.has_value()) {
                std::cout << "ERROR: AI returned no legal moves!" << std::endl;
                json error = {{"error", "AI could not find any legal moves"}};
                res.set_content(error.dump(), "application/json");
                res.status = 500;
                return;
            }
            
            std::cout << "AI found move, executing..." << std::endl;
            
            // Execute move
            Game updated_game = g_engine->process_move(game_id, move_opt.value());
            json j = updated_game;
            res.set_content(j.dump(), "application/json");
            res.status = 200;
            
            std::cout << "AI move completed successfully" << std::endl;
        } catch (const std::exception& e) {
            std::cout << "AI move exception: " << e.what() << std::endl;
            json error = {{"error", std::string("AI Error: ") + e.what()}};
            res.set_content(error.dump(), "application/json");
            res.status = 500;
        }
    });
    
    // Health check endpoint
    svr.Get("/health", [](const httplib::Request&, httplib::Response& res) {
        json health = {{"status", "ok"}};
        res.set_content(health.dump(), "application/json");
        res.status = 200;
    });
    
    // GET /benchmark - Get benchmark report
    svr.Get("/benchmark", [](const httplib::Request&, httplib::Response& res) {
        std::string report = Benchmark::instance().report();
        json response = {{"report", report}};
        res.set_content(response.dump(), "application/json");
        res.status = 200;
    });
    
    // POST /benchmark/reset - Reset benchmark counters
    svr.Post("/benchmark/reset", [](const httplib::Request&, httplib::Response& res) {
        Benchmark::instance().reset();
        json response = {{"status", "reset"}};
        res.set_content(response.dump(), "application/json");
        res.status = 200;
    });
    
    // ========== TRAINING ENDPOINTS ==========
    
    // POST /training/start - Start training
    svr.Post("/training/start", [](const httplib::Request& req, httplib::Response& res) {
        try {
            if (g_optimizer->is_running()) {
                json error = {{"error", "Training already running"}};
                res.set_content(error.dump(), "application/json");
                res.status = 400;
                return;
            }
            
            // Parse optional config from request body
            OptimizerConfig config;
            if (!req.body.empty()) {
                json body = json::parse(req.body);
                if (body.contains("population_size")) config.population_size = body["population_size"];
                if (body.contains("generations")) config.generations = body["generations"];
                if (body.contains("games_per_evaluation")) config.games_per_evaluation = body["games_per_evaluation"];
                if (body.contains("ai_depth")) config.ai_depth = body["ai_depth"];
                if (body.contains("mutation_rate")) config.mutation_rate = body["mutation_rate"];
            }
            
            g_optimizer = std::make_unique<WeightOptimizer>(config);
            g_optimizer->set_initial_weights(g_current_weights);
            g_optimizer->start_training_async();
            
            json response = {{"status", "started"}, {"config", {
                {"population_size", config.population_size},
                {"generations", config.generations},
                {"games_per_evaluation", config.games_per_evaluation},
                {"ai_depth", config.ai_depth}
            }}};
            res.set_content(response.dump(), "application/json");
            res.status = 200;
        } catch (const std::exception& e) {
            json error = {{"error", e.what()}};
            res.set_content(error.dump(), "application/json");
            res.status = 500;
        }
    });
    
    // GET /training/status - Get training status
    svr.Get("/training/status", [](const httplib::Request&, httplib::Response& res) {
        TrainingStats stats = g_optimizer->get_stats();
        json response = {
            {"is_running", stats.is_running},
            {"current_generation", stats.current_generation},
            {"total_generations", stats.total_generations},
            {"games_played", stats.games_played},
            {"best_fitness", stats.best_fitness},
            {"average_fitness", stats.average_fitness},
            {"status_message", stats.status_message}
        };
        res.set_content(response.dump(), "application/json");
        res.status = 200;
    });
    
    // POST /training/stop - Stop training
    svr.Post("/training/stop", [](const httplib::Request&, httplib::Response& res) {
        g_optimizer->stop();
        json response = {{"status", "stopping"}};
        res.set_content(response.dump(), "application/json");
        res.status = 200;
    });
    
    // GET /weights - Get current weights
    svr.Get("/weights", [](const httplib::Request&, httplib::Response& res) {
        json response = g_current_weights.to_json();
        res.set_content(response.dump(), "application/json");
        res.status = 200;
    });
    
    // POST /weights - Set weights
    svr.Post("/weights", [](const httplib::Request& req, httplib::Response& res) {
        try {
            json body = json::parse(req.body);
            g_current_weights = EvalWeights::from_json(body);
            g_current_weights.save_to_file("weights.json");
            json response = {{"status", "updated"}, {"weights", g_current_weights.to_json()}};
            res.set_content(response.dump(), "application/json");
            res.status = 200;
        } catch (const std::exception& e) {
            json error = {{"error", e.what()}};
            res.set_content(error.dump(), "application/json");
            res.status = 400;
        }
    });
    
    // POST /training/quick - Run quick self-play test (1 game)
    svr.Post("/training/quick", [](const httplib::Request&, httplib::Response& res) {
        try {
            SelfPlayConfig config;
            config.ai_depth = 2;
            config.max_moves = 100;
            config.verbose = true;
            
            SelfPlayEngine engine(config);
            GameResult result = engine.run_game(g_current_weights, g_current_weights);
            
            std::string winner_str = result.was_draw ? "draw" : 
                (result.winner.has_value() ? to_string(result.winner.value()) : "unknown");
            
            json response = {
                {"winner", winner_str},
                {"total_moves", result.total_moves},
                {"was_draw", result.was_draw}
            };
            res.set_content(response.dump(), "application/json");
            res.status = 200;
        } catch (const std::exception& e) {
            json error = {{"error", e.what()}};
            res.set_content(error.dump(), "application/json");
            res.status = 500;
        }
    });
}


int main() {
    std::cout << "=== BUGS Game C++ Backend ===" << std::endl;
    std::cout << "Initializing server..." << std::endl;
    
    // Initialize global instances
    g_engine = std::make_unique<GameEngine>();
    g_minimax_ai = std::make_unique<MinimaxAI>(4); // Depth 4
    g_optimizer = std::make_unique<WeightOptimizer>();
    
    // Load weights if exists
    g_current_weights = EvalWeights::load_from_file("weights.json");
    
    httplib::Server svr;
    
    // Enable CORS for frontend
    svr.set_default_headers({
        {"Access-Control-Allow-Origin", "*"},
        {"Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS"},
        {"Access-Control-Allow-Headers", "Content-Type"}
    });
    
    // Handle OPTIONS requests for CORS preflight
    svr.Options(".*", [](const httplib::Request&, httplib::Response& res) {
        res.status = 204;
    });
    
    setup_routes(svr);
    
    std::cout << "Server starting on http://localhost:8080" << std::endl;
    std::cout << "Press Ctrl+C to stop" << std::endl;
    
    if (!svr.listen("0.0.0.0", 8080)) {
        std::cerr << "Failed to start server" << std::endl;
        return 1;
    }
    
    return 0;
}
