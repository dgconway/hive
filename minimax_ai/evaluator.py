from typing import Dict, List, Tuple, Set, Optional
import sys
import os

# Add parent directory to path for imports
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from app.models import Game, PlayerColor, PieceType, Piece, GameStatus
from app.hex_math import get_neighbors, hex_distance

# Heuristic Constants
PIECE_VALUES = {
    PieceType.QUEEN: 1000,
    PieceType.ANT: 80,
    PieceType.BEETLE: 60,
    PieceType.GRASSHOPPER: 40,
    PieceType.SPIDER: 30,
}

# Shared engine instance to avoid overhead
from app.game_logic import GameEngine
_EVAL_ENGINE = GameEngine()

def evaluate_state(game: Game, player: PlayerColor) -> float:
    """
    Advanced heuristic evaluation for BUGS game.
    """
    if game.status == GameStatus.FINISHED:
        if game.winner == player:
            return 1000000.0  # Big win bonus
        elif game.winner is None:
            return 0.0        # Draw
        else:
            return -1000000.0 # Loss penalty

    opponent = PlayerColor.BLACK if player == PlayerColor.WHITE else PlayerColor.WHITE
    score = 0.0

    # 1. Queen Surroundings (Critical)
    # This remains the primary goal but we refine the weights
    player_queen_pos = None
    opponent_queen_pos = None
    
    occupied_hexes = set()
    for key, stack in game.board.items():
        if stack:
            q, r = map(int, key.split(','))
            occupied_hexes.add((q, r))
            top_piece = stack[-1]
            if top_piece.type == PieceType.QUEEN:
                if top_piece.color == player:
                    player_queen_pos = (q, r)
                else:
                    opponent_queen_pos = (q, r)

    if opponent_queen_pos:
        opp_queen_neighbors = get_neighbors(opponent_queen_pos)
        occupied_opp_neighbors = 0
        for neighbor in opp_queen_neighbors:
            if neighbor in occupied_hexes:
                occupied_opp_neighbors += 1
        
        # Exponential reward for surrounding the enemy queen
        score += [0, 5, 15, 40, 100, 300, 1000][occupied_opp_neighbors] * 2.0
                
    if player_queen_pos:
        play_queen_neighbors = get_neighbors(player_queen_pos)
        occupied_play_neighbors = 0
        for neighbor in play_queen_neighbors:
            if neighbor in occupied_hexes:
                occupied_play_neighbors += 1
        
        # Exponential penalty for your queen being surrounded
        score -= [0, 5, 15, 40, 100, 300, 1000][occupied_play_neighbors] * 5.0

    # 2. Material Value (Pieces on board)
    for stack in game.board.values():
        if stack:
            top_piece = stack[-1]
            val = PIECE_VALUES.get(top_piece.type, 0)
            if top_piece.color == player:
                score += val
            else:
                score -= val

    # 3. Mobility and Ants
    _EVAL_ENGINE.games[game.game_id] = game
    
    player_mobility = 0
    opponent_mobility = 0
    
    opponent_queen_neighbors_set = set(get_neighbors(opponent_queen_pos)) if opponent_queen_pos else set()

    try:
        for key, stack in game.board.items():
            if not stack: continue
            top_piece = stack[-1]
            q, r = map(int, key.split(','))
            
            try:
                moves = _EVAL_ENGINE.get_valid_moves(game.game_id, q, r)
                num_moves = len(moves)
            except Exception:
                num_moves = 0
                moves = []

            if top_piece.color == player:
                player_mobility += num_moves
                
                # Proximity bonus: pieces closer to opponent queen are better
                if opponent_queen_pos:
                    dist = hex_distance((q, r), opponent_queen_pos)
                    # Reward pieces within distance 2-3 of enemy queen
                    if dist <= 3:
                        score += (5 - dist) * 10 
                
                # Expert Ant Rule
                if top_piece.type == PieceType.ANT:
                    non_surrounding_moves = [m for m in moves if tuple(m) not in opponent_queen_neighbors_set]
                    if len(non_surrounding_moves) >= 3:
                        score += 20.0 # "Free Ant"
            else:
                opponent_mobility += num_moves
                # Trapped opponent ant
                if top_piece.type == PieceType.ANT and num_moves == 0:
                    score += 30.0

        # Hand material weighting
        for ptype, count in game.white_pieces_hand.items():
            val = PIECE_VALUES.get(ptype, 0) * 0.5 * count
            if player == PlayerColor.WHITE: score += val
            else: score -= val
        for ptype, count in game.black_pieces_hand.items():
            val = PIECE_VALUES.get(ptype, 0) * 0.5 * count
            if player == PlayerColor.BLACK: score += val
            else: score -= val

    finally:
        if game.game_id in _EVAL_ENGINE.games:
            del _EVAL_ENGINE.games[game.game_id]

    # Mobility weight
    score += (player_mobility - opponent_mobility) * 2.0

    return score
