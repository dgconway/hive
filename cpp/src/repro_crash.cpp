
// #include "game_engine.hpp"
#include "minimax.hpp"
#include <iostream>
#include <chrono>

using namespace bugs;

int main() {
    std::cout << "Starting reproduction test..." << std::endl;
    
    GameEngine engine;
    Game game = engine.create_game();
    
    // Setup a few moves to reach turn 6 or so
    // Turn 1 (White): Place Grasshopper
    MoveRequest m1; m1.action = ActionType::PLACE; m1.piece_type = PieceType::GRASSHOPPER; m1.to_hex = {0,0};
    game = engine.process_move(game.game_id, m1);
    
    // Turn 2 (Black): Place Grasshopper
    MoveRequest m2; m2.action = ActionType::PLACE; m2.piece_type = PieceType::GRASSHOPPER; m2.to_hex = {0,-1};
    game = engine.process_move(game.game_id, m2);

    // Turn 3 (White): Place Queen
    MoveRequest m3; m3.action = ActionType::PLACE; m3.piece_type = PieceType::QUEEN; m3.to_hex = {0,1};
    game = engine.process_move(game.game_id, m3);

    // Turn 4 (Black): Place Queen
    MoveRequest m4; m4.action = ActionType::PLACE; m4.piece_type = PieceType::QUEEN; m4.to_hex = {0,-2};
    game = engine.process_move(game.game_id, m4);
    
    // Turn 5 (White): Move Queen
    MoveRequest m5; m5.action = ActionType::MOVE; m5.from_hex = {0,1}; m5.to_hex = {1,0};
    game = engine.process_move(game.game_id, m5);
    
    // Turn 6 (Black): Move Queen
    MoveRequest m6; m6.action = ActionType::MOVE; m6.from_hex = {0,-2}; m6.to_hex = {-1,-1};
    game = engine.process_move(game.game_id, m6);

    std::cout << "Game setup complete. Turn: " << game.turn_number << std::endl;
    
    MinimaxAI ai(4); // Depth 4

    for (int i = 0; i < 10; ++i) {
        auto start = std::chrono::high_resolution_clock::now();
        try {
            auto move = ai.get_best_move(game);
            if (move) {
                std::cout << "AI returned move: " << to_string(move->action) << std::endl;
            } else {
                std::cout << "AI returned no move." << std::endl;
            }
        } catch (const std::exception& e) {
            std::cout << "Caught exception: " << e.what() << std::endl;
        } catch (...) {
            std::cout << "Caught unknown exception" << std::endl;
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        std::cout << "Duration: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms" << std::endl;
    
    }
    

    return 0;
}
