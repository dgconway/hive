from fastapi import FastAPI, HTTPException
from typing import List
from .models import Game, MoveRequest, GameStatus
from .game_logic import GameEngine
from .ai_player import get_ai_player

app = FastAPI(title="BUGS Game API")
engine = GameEngine()

@app.post("/games", response_model=Game)
def create_game():
    """Create a new game."""
    return engine.create_game()

@app.get("/games/{game_id}", response_model=Game)
def get_game(game_id: str):
    """Get the current state of a game."""
    game = engine.get_game(game_id)
    if not game:
        raise HTTPException(status_code=404, detail="Game not found")
    return game

@app.post("/games/{game_id}/move", response_model=Game)
def make_move(game_id: str, move: MoveRequest):
    """Submit a move (Place or Move)."""
    try:
        updated_game = engine.process_move(game_id, move)
        return updated_game
    except ValueError as e:
        raise HTTPException(status_code=400, detail=str(e))
@app.get("/games/{game_id}/valid_moves", response_model=List[List[int]])
def get_valid_moves(game_id: str, q: int, r: int):
    """Get valid destination hexes for a piece at (q, r)."""
    try:
        moves = engine.get_valid_moves(game_id, q, r)
        return [list(m) for m in moves]
    except ValueError as e:
        raise HTTPException(status_code=400, detail=str(e))


@app.post("/games/{game_id}/ai_move", response_model=Game)
def make_ai_move(game_id: str, ai_type: str = "minimax"):
    """Trigger the AI to make a move for the current player."""
    game = engine.get_game(game_id)
    if not game:
        raise HTTPException(status_code=404, detail="Game not found")
    
    if game.status != GameStatus.IN_PROGRESS:
        raise HTTPException(status_code=400, detail="Game is already finished")
    
    ai_player = get_ai_player()
    try:
        move = ai_player.get_move(game, ai_type=ai_type)
        updated_game = engine.process_move(game_id, move)
        return updated_game
    except ValueError as e:
        raise HTTPException(status_code=400, detail=str(e))
    except Exception as e:
        import traceback
        traceback.print_exc()
        raise HTTPException(status_code=500, detail=f"AI Error: {str(e)}")
