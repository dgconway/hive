"""
Action Space for Neural Network
Encodes/decodes actions to/from neural network output indices.
"""
from typing import Dict, List, Tuple, Optional
from dataclasses import dataclass
import sys
import os

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from app.models import PieceType
from .state_encoder import GRID_SIZE, GRID_CENTER, hex_to_grid, grid_to_hex, is_valid_grid_pos


# Action space layout:
# PLACEMENT actions: piece_type (5) * grid_positions (GRID_SIZE^2)
# MOVE actions: from_pos (GRID_SIZE^2) * to_pos (GRID_SIZE^2)
#
# To keep action space manageable, we use a flat encoding:
# [0, NUM_PLACEMENT_ACTIONS): Placement actions
# [NUM_PLACEMENT_ACTIONS, TOTAL_ACTIONS): Move actions

NUM_PIECE_TYPES = 5
NUM_GRID_POSITIONS = GRID_SIZE * GRID_SIZE
NUM_PLACEMENT_ACTIONS = NUM_PIECE_TYPES * NUM_GRID_POSITIONS
NUM_MOVE_ACTIONS = NUM_GRID_POSITIONS * NUM_GRID_POSITIONS
TOTAL_ACTIONS = NUM_PLACEMENT_ACTIONS + NUM_MOVE_ACTIONS

PIECE_TYPE_TO_INDEX = {
    PieceType.QUEEN: 0,
    PieceType.ANT: 1,
    PieceType.SPIDER: 2,
    PieceType.BEETLE: 3,
    PieceType.GRASSHOPPER: 4,
}

INDEX_TO_PIECE_TYPE = {v: k for k, v in PIECE_TYPE_TO_INDEX.items()}


def pos_to_index(row: int, col: int) -> int:
    """Convert grid position to flat index."""
    return row * GRID_SIZE + col


def index_to_pos(idx: int) -> Tuple[int, int]:
    """Convert flat index to grid position."""
    row = idx // GRID_SIZE
    col = idx % GRID_SIZE
    return row, col


class ActionEncoder:
    """Encodes/decodes actions for neural network."""
    
    def __init__(self):
        self.total_actions = TOTAL_ACTIONS
        self.num_placement_actions = NUM_PLACEMENT_ACTIONS
        self.num_move_actions = NUM_MOVE_ACTIONS
    
    def encode_placement(self, piece_type: PieceType, to_hex: Tuple[int, int]) -> int:
        """Encode a placement action to index."""
        piece_idx = PIECE_TYPE_TO_INDEX[piece_type]
        row, col = hex_to_grid(to_hex[0], to_hex[1])
        
        if not is_valid_grid_pos(row, col):
            raise ValueError(f"Hex {to_hex} outside grid bounds")
        
        pos_idx = pos_to_index(row, col)
        return piece_idx * NUM_GRID_POSITIONS + pos_idx
    
    def encode_move(self, from_hex: Tuple[int, int], to_hex: Tuple[int, int]) -> int:
        """Encode a move action to index."""
        from_row, from_col = hex_to_grid(from_hex[0], from_hex[1])
        to_row, to_col = hex_to_grid(to_hex[0], to_hex[1])
        
        if not is_valid_grid_pos(from_row, from_col) or not is_valid_grid_pos(to_row, to_col):
            raise ValueError(f"Hex positions outside grid bounds")
        
        from_idx = pos_to_index(from_row, from_col)
        to_idx = pos_to_index(to_row, to_col)
        
        return NUM_PLACEMENT_ACTIONS + from_idx * NUM_GRID_POSITIONS + to_idx
    
    def encode_action(self, action) -> int:
        """Encode an Action object to index."""
        if action.action_type == "PLACE":
            return self.encode_placement(action.piece_type, action.to_hex)
        else:
            return self.encode_move(action.from_hex, action.to_hex)
    
    def decode(self, action_idx: int) -> dict:
        """
        Decode action index to action components.
        
        Returns:
            dict with keys: action_type, piece_type (for PLACE), from_hex (for MOVE), to_hex
        """
        if action_idx < NUM_PLACEMENT_ACTIONS:
            # Placement action
            piece_idx = action_idx // NUM_GRID_POSITIONS
            pos_idx = action_idx % NUM_GRID_POSITIONS
            
            piece_type = INDEX_TO_PIECE_TYPE[piece_idx]
            row, col = index_to_pos(pos_idx)
            to_hex = grid_to_hex(row, col)
            
            return {
                "action_type": "PLACE",
                "piece_type": piece_type,
                "from_hex": None,
                "to_hex": to_hex
            }
        else:
            # Move action
            move_idx = action_idx - NUM_PLACEMENT_ACTIONS
            from_idx = move_idx // NUM_GRID_POSITIONS
            to_idx = move_idx % NUM_GRID_POSITIONS
            
            from_row, from_col = index_to_pos(from_idx)
            to_row, to_col = index_to_pos(to_idx)
            
            from_hex = grid_to_hex(from_row, from_col)
            to_hex = grid_to_hex(to_row, to_col)
            
            return {
                "action_type": "MOVE",
                "piece_type": None,
                "from_hex": from_hex,
                "to_hex": to_hex
            }
    
    def get_legal_action_mask(self, legal_actions: list) -> List[int]:
        """
        Get indices of legal actions.
        
        Args:
            legal_actions: List of Action objects
            
        Returns:
            List of valid action indices
        """
        indices = []
        for action in legal_actions:
            try:
                idx = self.encode_action(action)
                indices.append(idx)
            except ValueError:
                # Action outside grid bounds, skip
                continue
        return indices
