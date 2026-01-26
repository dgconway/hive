"""
AI Player Wrapper
Loads the trained neural network and uses MCTS to select moves.
"""
import os
import sys
import torch
from typing import Optional, Tuple

# Add parent directory to path for imports
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from app.models import Game, MoveRequest, PieceType, PlayerColor
from neural_guided_mcts.network.model import create_model
from neural_guided_mcts.network.training import load_checkpoint
from neural_guided_mcts.mcts.search import MCTS
from neural_guided_mcts.game_interface import GameInterface, GameState
from neural_guided_mcts.state_encoder import StateEncoder
from neural_guided_mcts.action_space import ActionEncoder
from minimax_ai.search import MinimaxAI

class AIPlayer:
    def __init__(self, checkpoint_path: Optional[str] = None, num_simulations: int = 1000):
        if checkpoint_path is None:
            # Default path relative to this file's parent's parent (the project root)
            root_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
            checkpoint_path = os.path.join(root_dir, "neural_guided_mcts", "checkpoints", "latest.pt")
            
        self.device = 'cuda' if torch.cuda.is_available() else 'cpu'
        self.network = create_model(self.device)
        
        # Load checkpoint if it exists
        if os.path.exists(checkpoint_path):
            load_checkpoint(self.network, path=checkpoint_path)
            print(f"Loaded AI checkpoint from {checkpoint_path}")
        else:
            print(f"Warning: No checkpoint found at {checkpoint_path}. Using initial model.")
            
        self.network.eval()
        
        self.game_interface = GameInterface()
        self.state_encoder = StateEncoder()
        self.action_encoder = ActionEncoder()
        self.num_simulations = num_simulations
        
        self.mcts = MCTS(
            network=self.network,
            game_interface=self.game_interface,
            state_encoder=self.state_encoder,
            action_encoder=self.action_encoder,
            num_simulations=self.num_simulations,
            device=self.device
        )
        
        # Minimax AI
        self.minimax = MinimaxAI(depth=4)

    def get_move(self, game: Game, ai_type: str = "minimax") -> MoveRequest:
        """Calculate the best move using specific AI type."""
        if ai_type == "mcts":
            state = GameState(game)
            action, _ = self.mcts.get_action_with_search(state, temperature=0.0)
            if action:
                return action.to_move_request()
        else:
            # --- OPENING BOOK (MINIMAX ONLY) ---
            # Calculate how many pieces the AI has already played
            ai_pieces_played = sum(1 for stack in game.board.values() for p in stack if p.color == game.current_turn)
            
            # First move: Force Grasshopper
            if ai_pieces_played == 0:
                state = GameState(game)
                legal = self.game_interface.get_legal_actions(state)
                for a in legal:
                    if a.action_type == "PLACE" and a.piece_type == PieceType.GRASSHOPPER:
                        print("AI Opening Book: Playing GRASSHOPPER")
                        return a.to_move_request()
            
            # Second move: Force Queen Bee
            if ai_pieces_played == 1:
                state = GameState(game)
                legal = self.game_interface.get_legal_actions(state)
                for a in legal:
                    if a.action_type == "PLACE" and a.piece_type == PieceType.QUEEN:
                        print("AI Opening Book: Playing QUEEN")
                        return a.to_move_request()
            
            # Default to standard minimax search
            move = self.minimax.get_best_move(game)
            if move:
                return move
                
        raise ValueError(f"AI ({ai_type}) could not find any legal moves")

# Singleton instance for the app
_ai_instance = None

def get_ai_player() -> AIPlayer:
    global _ai_instance
    if _ai_instance is None:
        _ai_instance = AIPlayer()
    return _ai_instance
