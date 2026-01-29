#include "httplib.h"
#include "json.hpp"
#include "game_logic.hpp"
#include "minimax.hpp"
#include "benchmark.hpp"
#include <iostream>
#include <memory>

using namespace bugs;
using json = nlohmann::json;

// Global game engine and AI
std::unique_ptr<GameEngine> g_engine;
std::unique_ptr<MinimaxAI> g_minimax_ai;

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
}


int main() {
    std::cout << "=== BUGS Game C++ Backend ===" << std::endl;
    std::cout << "Initializing server..." << std::endl;
    
    // Initialize global instances
    g_engine = std::make_unique<GameEngine>();
    g_minimax_ai = std::make_unique<MinimaxAI>(4); // Depth 4
    
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
