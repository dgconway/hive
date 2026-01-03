"""
Game Interface for MCTS
Wraps the GameEngine to provide a clean interface for MCTS operations.
"""
import copy
from typing import List, Tuple, Optional, Dict, Any
from dataclasses import dataclass
import sys
import os

# Add parent directory to path for imports
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from app.game_logic import GameEngine
from app.models import Game, GameStatus, PieceType, PlayerColor, MoveRequest, Piece
from app.hex_math import get_neighbors


@dataclass
class Action:
    """Represents a game action (placement or move)."""
    action_type: str  # "PLACE" or "MOVE"
    piece_type: Optional[PieceType]
    from_hex: Optional[Tuple[int, int]]
    to_hex: Tuple[int, int]
    
    def to_move_request(self) -> MoveRequest:
        return MoveRequest(
            action=self.action_type,
            piece_type=self.piece_type,
            from_hex=self.from_hex,
            to_hex=self.to_hex
        )


class GameState:
    """
    Immutable game state wrapper for MCTS.
    Contains a deep copy of the game state.
    """
    
    def __init__(self, game: Game):
        # Optimized shallow copy of the board structure
        # Since we only modify the board by pushing/popping pieces, 
        # we can share the Piece objects (which we treat as immutable here)
        self.game = Game(
            game_id=game.game_id,
            board={k: v.copy() for k, v in game.board.items()},
            current_turn=game.current_turn,
            turn_number=game.turn_number,
            white_pieces_hand=game.white_pieces_hand.copy(),
            black_pieces_hand=game.black_pieces_hand.copy(),
            winner=game.winner,
            status=game.status
        )
    
    def copy(self) -> 'GameState':
        return GameState(self.game)
    
    @property
    def current_player(self) -> PlayerColor:
        return self.game.current_turn
    
    @property
    def is_terminal(self) -> bool:
        return self.game.status == GameStatus.FINISHED
    
    @property
    def winner(self) -> Optional[PlayerColor]:
        return self.game.winner
    
    def get_reward(self, player: PlayerColor) -> float:
        """Get reward from perspective of player (-1, 0, or 1)."""
        if not self.is_terminal:
            return 0.0
        if self.winner is None:
            return 0.0  # Draw
        return 1.0 if self.winner == player else -1.0


class GameInterface:
    """
    Interface between MCTS and game engine.
    Provides methods for getting legal actions and applying them.
    """
    
    def __init__(self):
        self.engine = GameEngine()
    
    def get_initial_state(self) -> GameState:
        """Create a new game and return initial state."""
        game = self.engine.create_game()
        return GameState(game)
    
    def get_legal_actions(self, state: GameState) -> List[Action]:
        """Get all legal actions for the current player."""
        game = state.game
        actions = []
        
        if game.status == GameStatus.FINISHED:
            return actions
        
        hand = (game.white_pieces_hand if game.current_turn == PlayerColor.WHITE 
                else game.black_pieces_hand)
        
        # Check if queen must be placed (turn 4 rule)
        is_fourth_turn = ((game.current_turn == PlayerColor.WHITE and game.turn_number == 7) or
                          (game.current_turn == PlayerColor.BLACK and game.turn_number == 8))
        must_place_queen = is_fourth_turn and hand[PieceType.QUEEN] == 1
        
        # Check if queen is placed (required for moves)
        queen_placed = hand[PieceType.QUEEN] == 0
        
        # Get placement actions
        placement_hexes = self._get_valid_placement_hexes(game)
        
        if must_place_queen:
            # Must place queen
            for hex_pos in placement_hexes:
                actions.append(Action("PLACE", PieceType.QUEEN, None, hex_pos))
        else:
            # Can place any available piece
            for piece_type, count in hand.items():
                if count > 0:
                    for hex_pos in placement_hexes:
                        actions.append(Action("PLACE", piece_type, None, hex_pos))
            
            # Get move actions (only if queen is placed)
            if queen_placed:
                for from_hex, valid_destinations in self._get_all_valid_moves(game):
                    for to_hex in valid_destinations:
                        actions.append(Action("MOVE", None, from_hex, to_hex))
        
        return actions
    
    def _get_valid_placement_hexes(self, game: Game) -> List[Tuple[int, int]]:
        """Get all hexes where pieces can be placed."""
        valid_hexes = []
        
        # First move: place at origin
        if not game.board:
            return [(0, 0)]
        
        # Second move: place adjacent to any piece
        if game.turn_number == 2:
            occupied = set()
            for key in game.board:
                q, r = map(int, key.split(','))
                occupied.add((q, r))
            
            candidates = set()
            for pos in occupied:
                for n in get_neighbors(pos):
                    if n not in occupied:
                        candidates.add(n)
            return list(candidates)
        
        # General case: touch own, not opponent
        occupied = {}
        for key, stack in game.board.items():
            if stack:
                q, r = map(int, key.split(','))
                top_piece = stack[-1]
                occupied[(q, r)] = top_piece.color
        
        candidates = set()
        for pos, color in occupied.items():
            if color == game.current_turn:
                for n in get_neighbors(pos):
                    if n not in occupied:
                        candidates.add(n)
        
        # Filter: must not touch opponent
        valid = []
        for pos in candidates:
            touches_opponent = False
            for n in get_neighbors(pos):
                if n in occupied and occupied[n] != game.current_turn:
                    touches_opponent = True
                    break
            if not touches_opponent:
                valid.append(pos)
        
        return valid
    
    def _get_all_valid_moves(self, game: Game) -> List[Tuple[Tuple[int, int], List[Tuple[int, int]]]]:
        """Get all valid moves for all pieces of current player."""
        moves = []
        
        # We need to temporarily register the game with engine
        self.engine.games[game.game_id] = game
        
        for key, stack in game.board.items():
            if not stack:
                continue
            top_piece = stack[-1]
            if top_piece.color != game.current_turn:
                continue
            
            q, r = map(int, key.split(','))
            valid_destinations = self.engine.get_valid_moves(game.game_id, q, r)
            
            if valid_destinations:
                moves.append(((q, r), valid_destinations))
        
        # Clean up
        del self.engine.games[game.game_id]
        
        return moves
    
    def apply_action(self, state: GameState, action: Action) -> GameState:
        """Apply an action and return the new state."""
        new_state = state.copy()
        game = new_state.game
        
        # Register game with engine temporarily
        self.engine.games[game.game_id] = game
        
        try:
            self.engine.process_move(game.game_id, action.to_move_request())
        finally:
            # Update the state from engine and clean up
            new_state.game = self.engine.games[game.game_id]
            del self.engine.games[game.game_id]
        
        return new_state
    
    def get_action_count(self) -> int:
        """Return approximate size of action space for neural network."""
        # 5 piece types * ~100 placement positions + ~30 pieces * ~100 move destinations
        # Simplified to a fixed size for neural network output
        return 2000
