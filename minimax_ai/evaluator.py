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
    Advanced heuristic evaluation for BUGS game (Optimized version).
    """
    if game.status == GameStatus.FINISHED:
        if game.winner == player:
            return 1000000.0
        elif game.winner is None:
            return 0.0
        else:
            return -1000000.0

    opponent = PlayerColor.BLACK if player == PlayerColor.WHITE else PlayerColor.WHITE
    score = 0.0

    # 1. Pre-calculate occupied and queen positions
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

    # 2. Queen Surroundings
    if opponent_queen_pos:
        opp_queen_neighbors = get_neighbors(opponent_queen_pos)
        occupied_opp_neighbors = sum(1 for n in opp_queen_neighbors if n in occupied_hexes)
        score += [0, 5, 15, 40, 100, 300, 1000][occupied_opp_neighbors] * 2.0
                
    if player_queen_pos:
        play_queen_neighbors = get_neighbors(player_queen_pos)
        occupied_play_neighbors = sum(1 for n in play_queen_neighbors if n in occupied_hexes)
        score -= [0, 5, 15, 40, 100, 300, 1000][occupied_play_neighbors] * 5.0

    # 3. Material and Mobility
    player_mobility = 0
    opponent_mobility = 0
    opponent_queen_neighbors_set = set(get_neighbors(opponent_queen_pos)) if opponent_queen_pos else set()

    # Temporarily set current turn to calculate mobility properly
    original_turn = game.current_turn

    try:
        for key, stack in game.board.items():
            if not stack: continue
            top_piece = stack[-1]
            q, r = map(int, key.split(','))
            
            # Material value
            val = PIECE_VALUES.get(top_piece.type, 0)
            if top_piece.color == player:
                score += val
            else:
                score -= val

            # Mobility
            game.current_turn = top_piece.color
            try:
                # Use optimized engine call
                moves = _EVAL_ENGINE.get_valid_moves_for_piece(game, (q, r), occupied_hexes)
                num_moves = len(moves)
            except Exception:
                num_moves = 0
                moves = []

            if top_piece.color == player:
                player_mobility += num_moves
                if opponent_queen_pos:
                    dist = hex_distance((q, r), opponent_queen_pos)
                    if dist <= 3:
                        score += (5 - dist) * 10 
                if top_piece.type == PieceType.ANT:
                    non_surrounding_moves_count = sum(1 for m in moves if tuple(m) not in opponent_queen_neighbors_set)
                    if non_surrounding_moves_count >= 3:
                        score += 20.0
            else:
                opponent_mobility += num_moves
                if top_piece.type == PieceType.ANT and num_moves == 0:
                    score += 30.0

    finally:
        game.current_turn = original_turn

    # 4. Hand material weighting
    wp_hand = game.white_pieces_hand
    bp_hand = game.black_pieces_hand
    for ptype, count in wp_hand.items():
        val = PIECE_VALUES.get(ptype, 0) * 0.5 * count
        if player == PlayerColor.WHITE: score += val
        else: score -= val
    for ptype, count in bp_hand.items():
        val = PIECE_VALUES.get(ptype, 0) * 0.5 * count
        if player == PlayerColor.BLACK: score += val
        else: score -= val

    score += (player_mobility - opponent_mobility) * 2.0
    return score
