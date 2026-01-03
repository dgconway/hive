"""
State Encoder for Neural Network
Converts game state to tensor representation.
"""
import torch
import numpy as np
from typing import Dict, List, Tuple
import sys
import os

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from app.models import Game, PieceType, PlayerColor


# Grid bounds for state encoding
# We use a fixed grid size centered at origin
GRID_SIZE = 11  # 11x11 grid, centered at (5,5) which maps to hex (0,0)
GRID_CENTER = GRID_SIZE // 2

# Channel indices
CHANNEL_WHITE_QUEEN = 0
CHANNEL_WHITE_ANT = 1
CHANNEL_WHITE_SPIDER = 2
CHANNEL_WHITE_BEETLE = 3
CHANNEL_WHITE_GRASSHOPPER = 4
CHANNEL_BLACK_QUEEN = 5
CHANNEL_BLACK_ANT = 6
CHANNEL_BLACK_SPIDER = 7
CHANNEL_BLACK_BEETLE = 8
CHANNEL_BLACK_GRASSHOPPER = 9
CHANNEL_CURRENT_PLAYER = 10
CHANNEL_TURN_NUMBER = 11

NUM_CHANNELS = 12

PIECE_TO_CHANNEL = {
    (PlayerColor.WHITE, PieceType.QUEEN): CHANNEL_WHITE_QUEEN,
    (PlayerColor.WHITE, PieceType.ANT): CHANNEL_WHITE_ANT,
    (PlayerColor.WHITE, PieceType.SPIDER): CHANNEL_WHITE_SPIDER,
    (PlayerColor.WHITE, PieceType.BEETLE): CHANNEL_WHITE_BEETLE,
    (PlayerColor.WHITE, PieceType.GRASSHOPPER): CHANNEL_WHITE_GRASSHOPPER,
    (PlayerColor.BLACK, PieceType.QUEEN): CHANNEL_BLACK_QUEEN,
    (PlayerColor.BLACK, PieceType.ANT): CHANNEL_BLACK_ANT,
    (PlayerColor.BLACK, PieceType.SPIDER): CHANNEL_BLACK_SPIDER,
    (PlayerColor.BLACK, PieceType.BEETLE): CHANNEL_BLACK_BEETLE,
    (PlayerColor.BLACK, PieceType.GRASSHOPPER): CHANNEL_BLACK_GRASSHOPPER,
}


def hex_to_grid(q: int, r: int) -> Tuple[int, int]:
    """
    Convert hex coordinates to grid indices.
    Returns (row, col) for the grid.
    """
    # Map hex (0,0) to grid center
    col = q + GRID_CENTER
    row = r + GRID_CENTER
    return row, col


def grid_to_hex(row: int, col: int) -> Tuple[int, int]:
    """Convert grid indices back to hex coordinates."""
    q = col - GRID_CENTER
    r = row - GRID_CENTER
    return q, r


def is_valid_grid_pos(row: int, col: int) -> bool:
    """Check if grid position is within bounds."""
    return 0 <= row < GRID_SIZE and 0 <= col < GRID_SIZE


class StateEncoder:
    """Encodes game state to tensor for neural network input."""
    
    def __init__(self):
        self.num_channels = NUM_CHANNELS
        self.grid_size = GRID_SIZE
    
    def encode(self, game: Game) -> torch.Tensor:
        """
        Encode game state to tensor.
        
        Returns:
            Tensor of shape (NUM_CHANNELS, GRID_SIZE, GRID_SIZE)
        """
        state = np.zeros((NUM_CHANNELS, GRID_SIZE, GRID_SIZE), dtype=np.float32)
        
        # Encode pieces
        for key, stack in game.board.items():
            if not stack:
                continue
            
            q, r = map(int, key.split(','))
            row, col = hex_to_grid(q, r)
            
            if not is_valid_grid_pos(row, col):
                continue  # Skip pieces outside grid bounds
            
            # Encode all pieces in stack (for beetle stacking)
            for piece in stack:
                channel = PIECE_TO_CHANNEL.get((piece.color, piece.type))
                if channel is not None:
                    state[channel, row, col] += 1.0
        
        # Encode current player (1 for WHITE's perspective)
        if game.current_turn == PlayerColor.WHITE:
            state[CHANNEL_CURRENT_PLAYER, :, :] = 1.0
        
        # Encode turn number (normalized to 0-1 range, capped at 100 turns)
        normalized_turn = min(game.turn_number / 100.0, 1.0)
        state[CHANNEL_TURN_NUMBER, :, :] = normalized_turn
        
        return torch.from_numpy(state)
    
    def encode_batch(self, games: List[Game]) -> torch.Tensor:
        """Encode multiple game states to batch tensor."""
        batch = torch.stack([self.encode(g) for g in games])
        return batch
    
    def get_canonical_form(self, game: Game, player: PlayerColor) -> torch.Tensor:
        """
        Get state from perspective of given player.
        If player is BLACK, flip the board representation.
        """
        state = self.encode(game)
        
        if player == PlayerColor.BLACK:
            # Swap white and black channels
            state_copy = state.clone()
            # Swap piece channels
            for i in range(5):
                state[i], state[i + 5] = state_copy[i + 5].clone(), state_copy[i].clone()
            # Flip current player channel
            state[CHANNEL_CURRENT_PLAYER] = 1.0 - state[CHANNEL_CURRENT_PLAYER]
        
        return state
