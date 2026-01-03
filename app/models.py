from enum import Enum
from typing import List, Dict, Tuple, Optional, Literal
from pydantic import BaseModel

class PieceType(str, Enum):
    QUEEN = "QUEEN"
    ANT = "ANT"
    SPIDER = "SPIDER"
    BEETLE = "BEETLE"
    GRASSHOPPER = "GRASSHOPPER"

class PlayerColor(str, Enum):
    WHITE = "WHITE"
    BLACK = "BLACK"

class Piece(BaseModel):
    type: PieceType
    color: PlayerColor
    id: str  # Unique ID to distinguish individual pieces

class MoveRequest(BaseModel):
    action: Literal["PLACE", "MOVE"]
    piece_type: Optional[PieceType] = None # Required for PLACE
    from_hex: Optional[Tuple[int, int]] = None # Required for MOVE
    to_hex: Tuple[int, int]

class GameStatus(str, Enum):
    IN_PROGRESS = "IN_PROGRESS"
    FINISHED = "FINISHED"

class Game(BaseModel):
    game_id: str
    board: Dict[str, List[Piece]] # Key is "q,r", Value is stack of pieces (bottom to top)
    current_turn: PlayerColor
    turn_number: int
    white_pieces_hand: Dict[PieceType, int]
    black_pieces_hand: Dict[PieceType, int]
    winner: Optional[PlayerColor] = None
    status: GameStatus
