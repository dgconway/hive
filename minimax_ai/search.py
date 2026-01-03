import sys
import os
import random
import time
from typing import Tuple, List, Optional, Dict

# Add parent directory to path for imports
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from app.models import Game, PlayerColor, MoveRequest, PieceType
from neural_guided_mcts.game_interface import GameInterface, GameState, Action
from .evaluator import evaluate_state

class MinimaxAI:
    def __init__(self, depth: int = 4):
        self.depth = depth
        self.interface = GameInterface()
        self.transposition_table: Dict[str, Tuple[float, int]] = {} # Hash -> (score, depth)

    def get_best_move(self, game: Game) -> Optional[MoveRequest]:
        """Entry point for the AI to find the best move."""
        state = GameState(game)
        player = game.current_turn
        
        # Clear cache for a new search to avoid stale values
        # (Though in a real engine we'd keep it and manage memory/staleness)
        self.transposition_table = {}
        
        score, best_action = self._minimax(state, self.depth, -float('inf'), float('inf'), True, player)
        print(f"Minimax search complete. Score: {score:.2f}, Cache size: {len(self.transposition_table)}")
        
        if best_action:
            return best_action.to_move_request()
        return None

    def _hash_state(self, state: GameState) -> str:
        """Create a simple hash of the game state."""
        # A real engine would use Zobrist hashing, but stringifying the board is okay for BUGS
        return f"{state.game.board}:{state.game.current_turn}:{state.game.white_pieces_hand}:{state.game.black_pieces_hand}"

    def _score_action(self, action: Action, state: GameState, player: PlayerColor) -> float:
        """
        Heuristic move ordering: score actions to search best ones first.
        """
        score = 0.0
        
        # Priority 1: Queen placement
        if action.action_type == "PLACE" and action.piece_type == PieceType.QUEEN:
            score += 1000.0
            
        # Priority 2: Moving pieces already on board (usually more dynamic)
        if action.action_type == "MOVE":
            score += 50.0
            
        # Priority 3: Placing Ants (very powerful)
        if action.action_type == "PLACE" and action.piece_type == PieceType.ANT:
            score += 40.0
            
        return score

    def _minimax(
        self, 
        state: GameState, 
        depth: int, 
        alpha: float, 
        beta: float, 
        is_maximizing: bool, 
        player: PlayerColor
    ) -> Tuple[float, Optional[Action]]:
        """
        Minimax algorithm with Alpha-Beta pruning, Move Ordering, and Transposition Table.
        """
        state_hash = self._hash_state(state)
        if state_hash in self.transposition_table:
            cached_score, cached_depth = self.transposition_table[state_hash]
            if cached_depth >= depth:
                return cached_score, None

        if depth == 0 or state.is_terminal:
            score = evaluate_state(state.game, player)
            # Store at depth 0
            self.transposition_table[state_hash] = (score, depth)
            return score, None

        legal_actions = self.interface.get_legal_actions(state)
        
        if not legal_actions:
            score = evaluate_state(state.game, player)
            return score, None

        # Move Ordering: Sort actions by heuristic score descending
        legal_actions.sort(key=lambda a: self._score_action(a, state, player), reverse=True)

        best_action = None
        
        if is_maximizing:
            max_eval = -float('inf')
            for action in legal_actions:
                new_state = self.interface.apply_action(state, action)
                eval_val, _ = self._minimax(new_state, depth - 1, alpha, beta, False, player)
                
                if eval_val > max_eval:
                    max_eval = eval_val
                    best_action = action
                
                alpha = max(alpha, eval_val)
                if beta <= alpha:
                    break
            
            self.transposition_table[state_hash] = (max_eval, depth)
            return max_eval, best_action
        else:
            min_eval = float('inf')
            for action in legal_actions:
                new_state = self.interface.apply_action(state, action)
                eval_val, _ = self._minimax(new_state, depth - 1, alpha, beta, True, player)
                
                if eval_val < min_eval:
                    min_eval = eval_val
                    best_action = action
                    
                beta = min(beta, eval_val)
                if beta <= alpha:
                    break
            
            self.transposition_table[state_hash] = (min_eval, depth)
            return min_eval, best_action
