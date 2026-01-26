import sys
import os
import random
import time
import concurrent.futures
import json
from typing import Tuple, List, Optional, Dict

# Add parent directory to path for imports
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from app.models import Game, PlayerColor, MoveRequest, PieceType
from neural_guided_mcts.game_interface import GameInterface, GameState, Action
from .evaluator import evaluate_state

def score_action(action: Action, state: GameState, player: PlayerColor) -> float:
    """Heuristic move ordering shared between process and worker."""
    score = 0.0
    if action.action_type == "PLACE":
        if action.piece_type == PieceType.QUEEN: score += 1000.0
        if action.piece_type == PieceType.ANT: score += 40.0
    elif action.action_type == "MOVE":
        score += 50.0
    return score

# Helper for parallel worker
def worker_minimax(game_json: dict, action_idx: int, depth: int, player: PlayerColor) -> Tuple[float, int]:
    try:
        from neural_guided_mcts.game_interface import GameInterface, GameState
        from app.models import Game
        
        interface = GameInterface()
        game = Game(**game_json)
        state = GameState(game)
        
        # Must match the ordering in the main process!
        legal_actions = interface.get_legal_actions(state)
        legal_actions.sort(key=lambda a: score_action(a, state, player), reverse=True)
        
        if action_idx >= len(legal_actions):
            return -float('inf'), action_idx
            
        action = legal_actions[action_idx]
        new_state = interface.apply_action(state, action)
        
        ai = MinimaxAI(depth=depth)
        score, _ = ai._minimax(new_state, depth, -float('inf'), float('inf'), False, player)
        return score, action_idx
    except Exception as e:
        print(f"Worker Exception: {e}")
        return -float('inf'), action_idx

class MinimaxAI:
    def __init__(self, depth: int = 4):
        self.depth = depth
        self.interface = GameInterface()
        self.transposition_table: Dict[str, float] = {}

    def _get_state_key(self, state: GameState) -> str:
        """
        Truly hashable state key. 
        Using a string representation of the board and hand.
        """
        # board keys are 'q,r', values are stacks of pieces.
        # We only care about the type and color for hashing.
        board_part = []
        for k in sorted(state.game.board.keys()):
            stack = state.game.board[k]
            if stack:
                stack_str = "".join(f"{p.type[0]}{p.color[0]}" for p in stack)
                board_part.append(f"{k}:{stack_str}")
        
        key = "|".join(board_part) + f"|T:{state.game.current_turn[0]}"
        # Include hands to distinguish states with same board but different options
        w_hand = "".join(f"{k[0]}{v}" for k, v in sorted(state.game.white_pieces_hand.items()))
        b_hand = "".join(f"{k[0]}{v}" for k, v in sorted(state.game.black_pieces_hand.items()))
        return f"{key}|W:{w_hand}|B:{b_hand}"

    def get_best_move(self, game: Game) -> Optional[MoveRequest]:
        state = GameState(game)
        player = game.current_turn
        legal_actions = self.interface.get_legal_actions(state)
        
        if not legal_actions:
            return None

        # Sort actions identically to the worker
        legal_actions.sort(key=lambda a: score_action(a, state, player), reverse=True)

        start_time = time.time()
        game_dict = game.model_dump()
        best_action = None
        max_eval = -float('inf')

        # Limit workers for stability on Windows
        num_actions = len(legal_actions)
        max_workers = min(os.cpu_count() or 4, num_actions)
        
        # If very few actions or depth is shallow, don't bother with overhead
        if num_actions <= 1 or self.depth <= 1:
             score, action = self._minimax(state, self.depth, -float('inf'), float('inf'), True, player)
             return action.to_move_request() if action else None

        with concurrent.futures.ProcessPoolExecutor(max_workers=max_workers) as executor:
            futures = [
                executor.submit(worker_minimax, game_dict, i, self.depth - 1, player)
                for i in range(num_actions)
            ]
            
            # Map of results to preserve order if needed, but we just need max
            results = []
            for future in concurrent.futures.as_completed(futures):
                eval_val, idx = future.result()
                results.append((eval_val, idx))
            
            # Find the best result among completed workers
            if results:
                best_eval, best_idx = max(results, key=lambda x: x[0])
                if best_eval > -float('inf'):
                    best_action = legal_actions[best_idx]
                    max_eval = best_eval

        # Fallback if parallel search failed completely
        if best_action is None:
            print("Parallel search failed or returned no valid moves. Falling back to serial.")
            _, best_action = self._minimax(state, self.depth, -float('inf'), float('inf'), True, player)

        duration = time.time() - start_time
        print(f"Minimax complete. Score: {max_eval:.2f}, Time: {duration:.2f}s")
        
        return best_action.to_move_request() if best_action else None

    def _minimax(
        self, 
        state: GameState, 
        depth: int, 
        alpha: float, 
        beta: float, 
        is_maximizing: bool, 
        player: PlayerColor
    ) -> Tuple[float, Optional[Action]]:
        state_key = self._get_state_key(state)
        if state_key in self.transposition_table:
            return self.transposition_table[state_key], None

        if depth == 0 or state.is_terminal:
            score = evaluate_state(state.game, player)
            self.transposition_table[state_key] = score
            return score, None

        legal_actions = self.interface.get_legal_actions(state)
        if not legal_actions:
            score = evaluate_state(state.game, player)
            return score, None

        legal_actions.sort(key=lambda a: score_action(a, state, player), reverse=True)

        best_action = None
        if is_maximizing:
            curr_max = -float('inf')
            for action in legal_actions:
                new_state = self.interface.apply_action(state, action)
                eval_val, _ = self._minimax(new_state, depth - 1, alpha, beta, False, player)
                if eval_val > curr_max:
                    curr_max = eval_val
                    best_action = action
                alpha = max(alpha, eval_val)
                if beta <= alpha: break
            self.transposition_table[state_key] = curr_max
            return curr_max, best_action
        else:
            curr_min = float('inf')
            for action in legal_actions:
                new_state = self.interface.apply_action(state, action)
                eval_val, _ = self._minimax(new_state, depth - 1, alpha, beta, True, player)
                if eval_val < curr_min:
                    curr_min = eval_val
                    best_action = action
                beta = min(beta, eval_val)
                if beta <= alpha: break
            self.transposition_table[state_key] = curr_min
            return curr_min, best_action
