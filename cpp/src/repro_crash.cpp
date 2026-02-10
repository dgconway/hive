
// #include "game_engine.hpp"
#include "minimax.hpp"
#include <iostream>
#include <chrono>

using namespace bugs;

void game1() {
    GameEngine engine;
    Game game = engine.create_game();
    MoveRequest m;

    m = MoveRequest(ActionType::PLACE, {0,0}, PieceType::BEETLE);
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::PLACE, {-1,0}, PieceType::GRASSHOPPER);
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::PLACE, {0,1}, PieceType::QUEEN);
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::PLACE, {-2,1}, PieceType::QUEEN);
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::PLACE, {1,-1}, PieceType::ANT);
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::PLACE, {-1, -1}, PieceType::ANT);
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::PLACE, {1,0}, PieceType::BEETLE);
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::MOVE, {-1,-1}, {2,0});
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::PLACE, {0,2}, PieceType::ANT);
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::MOVE, {-3,2}, {1,2});
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::PLACE, {-2,3}, PieceType::GRASSHOPPER);
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::PLACE, {-1,-1}, PieceType::ANT);
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::MOVE, {-2,3}, {0,1});
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::MOVE, {-1,-1}, {-3,2});
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::PLACE, {-2,3}, PieceType::ANT);
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::MOVE, {1,2}, {-3,3});
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::PLACE, {1,-1}, PieceType::GRASSHOPPER);
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::MOVE, {2,0}, {2,-2});
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::MOVE, {1,0}, {0,1});
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::MOVE, {-1,0}, {-2,0});
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::MOVE, {-2, 3}, {-3, 4});
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::MOVE, {2,-2}, {-3,1});
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::MOVE, {-1,0}, {-4,3});
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::PLACE, {-3,0}, PieceType::GRASSHOPPER);
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::MOVE, {0,2}, {-3,-1});
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::PLACE, {-1,-1}, PieceType::BEETLE);
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::MOVE, {1,-1}, {-2,2});
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::MOVE, {-1,-1}, {-1,0});
    game = engine.process_move(game.game_id, m);

    // TODO check that game ends in draw. 

}

void game2() {

    GameEngine engine;
    Game game = engine.create_game();
    MoveRequest m;

    m = MoveRequest(ActionType::PLACE, {0,0}, PieceType::BEETLE);
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::PLACE, {-1,0}, PieceType::GRASSHOPPER);
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::PLACE, {0,1}, PieceType::QUEEN);
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::PLACE, {-1,-1}, PieceType::QUEEN);
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::PLACE, {1,-1}, PieceType::ANT);
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::PLACE, {-2,1}, PieceType::ANT);
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::MOVE, {1,-1}, {-2,2});
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::PLACE, {-2,0}, PieceType::ANT);
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::PLACE, {1,-1}, PieceType::ANT);
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::MOVE, {-2,0}, {0,2});
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::MOVE, {1,-1}, {-1,-2});
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::PLACE, {0,3}, PieceType::SPIDER);
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::PLACE, {0,-3}, PieceType::SPIDER);
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::MOVE, {0,3}, {1,0});
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::MOVE, {0,-3}, {-2,-1});
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::PLACE, {2,0}, PieceType::BEETLE);
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::PLACE, {-2,-2}, PieceType::GRASSHOPPER);
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::MOVE, {2,0}, {1,1});
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::MOVE, {0,1}, {-1,2});
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::MOVE, {-2,1}, {-3,3});
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::MOVE, {-2,-2}, {0,-2});
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::PLACE, {2,1}, PieceType::GRASSHOPPER);
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::PLACE, {-3,-1}, PieceType::GRASSHOPPER);
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::MOVE, {2,1}, {0,1});
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::MOVE, {-3,-1}, {0,-1});
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::MOVE, {-1,0}, {-1,-3});
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::PLACE, {1,-3}, PieceType::GRASSHOPPER);
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::MOVE, {-1,-1}, {-2,0});
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::MOVE, {1,-3}, {-1,-1});
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::MOVE, {-2,0}, {-3,0});
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::PLACE, {-1,0}, PieceType::BEETLE);
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::MOVE, {1,1}, {0,2});
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::MOVE, {-1,0}, {-1,1});
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::MOVE, {0,2}, {-1,3});
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::MOVE, {-1,-1}, {-3,-1});
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::MOVE, {-3,3}, {-2,3});
    game = engine.process_move(game.game_id, m);


}

void game3() {

    GameEngine engine;
    Game game = engine.create_game();
    MoveRequest m;

    
    m = MoveRequest(ActionType::PLACE, {0,0}, PieceType::BEETLE);
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::PLACE, {-1,0}, PieceType::GRASSHOPPER);
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::PLACE, {0,1}, PieceType::QUEEN);
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::PLACE, {-1,-1}, PieceType::QUEEN);
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::PLACE, {1,-1}, PieceType::ANT);
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::PLACE, {-2,1}, PieceType::ANT);
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::MOVE, {1,-1}, {-1,-2});
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::MOVE, {-2,1}, {0,2});
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::PLACE, {0,-3}, PieceType::SPIDER);
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::PLACE, {-2,0}, PieceType::ANT);
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::PLACE, {1,-1}, PieceType::BEETLE);
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::MOVE, {-2,0}, {-1,2});
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::MOVE, {0,-3}, {-2,0});
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::PLACE, {-1,3}, PieceType::SPIDER);
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::PLACE, {-3,-1}, PieceType::ANT);
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::PLACE, {1,2}, PieceType::BEETLE);
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::MOVE, {1,-1}, {0,-1});
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::MOVE, {-1,0}, {1,0});
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::MOVE, {-3,-1}, {-2,4});
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::MOVE, {1,2}, {1,1});
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::PLACE, {-3,-1}, PieceType::ANT);
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::PLACE, {2,-1}, PieceType::ANT);
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::MOVE, {-3,-1}, {3,-2});
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::PLACE, {2,1}, PieceType::GRASSHOPPER);
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::MOVE, {-1,-2}, {3,1});
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::PLACE, {-2,2}, PieceType::BEETLE);
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::MOVE, {-2,-1}, {-3,2});
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::PLACE, {2,0}, PieceType::GRASSHOPPER);
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::MOVE, {-1,4}, {1,-1});
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::MOVE, {0,2}, {-1,1});
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::MOVE, {0,0}, {-1,1});
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::MOVE, {2,0}, {0,0});
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::MOVE, {1,-1}, {-1,-2});
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::MOVE, {-1,3}, {-4,3});
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::MOVE, {3,1}, {-2,0});
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::MOVE, {2,1}, {-2,1});
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::PLACE, {0,-3}, PieceType::GRASSHOPPER);
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::PLACE, {2,0}, PieceType::SPIDER);
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::MOVE, {-2,0}, {3,0});
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::MOVE, {-2,2}, {-2,3});
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::MOVE, {0,-3}, {-2,-1});
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::MOVE, {-2,3}, {-1,3});
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::MOVE, {-1,-2}, {-1,4});
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::MOVE, {-4,3}, {-2,3});
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::MOVE, {-3,2}, {-3,4});
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::MOVE, {-2,1}, {2,1});
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::PLACE, {-3,-1}, PieceType::SPIDER);
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::MOVE, {2,1}, {-2,1});
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::MOVE, {-3,-1}, {0,-2});
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::MOVE, {1,1}, {0,2});
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::MOVE, {-3,4}, {-3,2});
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::MOVE, {-2,3}, {-1,5});
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::PLACE, {1,-3}, PieceType::GRASSHOPPER);
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::MOVE, {0,2}, {1,1});
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::MOVE, {1,-3}, {-2,0});
    game = engine.process_move(game.game_id, m);

    m = MoveRequest(ActionType::MOVE, {-1,5}, {0,2});
    game = engine.process_move(game.game_id, m);


}

int main() {
    std::cout << "Starting game 1..." << std::endl;
    game1();
    std::cout << "Starting game 2..." << std::endl;
    game2();
    std::cout << "Starting game 3..." << std::endl;
    game3();
    std::cout << "Tests passed!" << std::endl;
    return 0;
}

int repro_crash() {
    std::cout << "Starting reproduction test..." << std::endl;
    
    GameEngine engine;
    Game game = engine.create_game();
    
    // Setup a few moves to reach turn 6 or so
    // Turn 1 (White): Place Grasshopper
    MoveRequest m1; m1.action = ActionType::PLACE; m1.piece_type = PieceType::GRASSHOPPER; m1.to_hex = {0,0};
    game = engine.process_move(game.game_id, m1);
    
    // Turn 2 (Black): Place Grasshopper
    MoveRequest m2; 
    m2.action = ActionType::PLACE; 
    m2.piece_type = PieceType::GRASSHOPPER; 
    m2.to_hex = {0,-1};
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
