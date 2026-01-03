# BUGS Backend Framework Specification

This document outlines the technical architecture, data models, and logic algorithms for the BUGS game backend.

## 1. Technology Stack

- **Language**: Python 3.10+
- **Web Framework**: FastAPI
- **Server**: Uvicorn
- **Validation**: Pydantic
- **Testing**: Pytest

## 2. Project Structure

```text
bugs_game/
├── app/
│   ├── __init__.py
│   ├── main.py           # API Entry point
│   ├── models.py         # Pydantic models & Enums
│   ├── game_logic.py     # Core game rules & state machine
│   ├── hex_math.py       # Hexagonal grid math utilities
│   └── routers/
│       └── games.py      # Game management endpoints
├── tests/
│   ├── test_mechanics.py
│   └── test_api.py
├── requirements.txt
└── README.md
```

## 3. Data Models (`app/models.py`)

### Enums
- `PieceType`: `QUEEN`, `ANT`, `SPIDER`, `BEETLE`, `GRASSHOPPER`
- `PlayerColor`: `WHITE`, `BLACK`

### Core Structures
- **Coordinates**: Represented as a tuple `(q, r)` (Axial coordinates).
- `Piece`:
    ```python
    class Piece(BaseModel):
        type: PieceType
        color: PlayerColor
        id: str  # Unique ID to track specific pieces (vital for Beetles in stacks)
    ```
- `BoardState`:
    - A dictionary mapping coordinates to a **stack** of pieces.
    - Structure: `Dict[str, List[Piece]]` (Key is usually `"{q},{r}"`)
    - *Note*: A list is required because Beetles can stack on top of other pieces. The last item in the list is the "top" piece.

### Game State
- `Game`:
    ```python
    class Game(BaseModel):
        game_id: str
        board: Dict[str, List[Piece]]
        current_turn: PlayerColor
        turn_number: int
        white_pieces_hand: Dict[PieceType, int] # Counts of available pieces
        black_pieces_hand: Dict[PieceType, int]
        winner: Optional[PlayerColor]
        status: str # "IN_PROGRESS", "FINISHED"
    ```

### API Requests
- `MoveRequest`:
    ```python
    class MoveRequest(BaseModel):
        action: Literal["PLACE", "MOVE"]
        piece_type: Optional[PieceType] # Required for PLACE
        from_hex: Optional[Tuple[int, int]] # Required for MOVE
        to_hex: Tuple[int, int]
    ```

## 4. Hexagonal Logic (`app/hex_math.py`)

We utilize **Flat-topped** hexagonal logic with **Axial Coordinates**.

- **Neighbors**:
    - Directions: `(1, 0), (1, -1), (0, -1), (-1, 0), (-1, 1), (0, 1)`
- **Distance**: `(abs(q) + abs(q + r) + abs(r)) / 2`
- **Connectivity**: Use BFS/DFS to traverse all occupied hexes.
    - *One Hive Rule*: Removal of a piece (temporarily for movement) must not split the graph.

## 5. Game Logic Rules (`app/game_logic.py`)

### Movement Validation
1.  **Placement**:
    - Turn 1: Can place anywhere (White) / Next to white (Black).
    - Turn > 1: Must touch **only** own color pieces.
    - **Queen Bee Rule**: If not placed by Turn 4, next move *must* be placing the Queen.
2.  **Movement**:
    - **General**: Must obey "One Hive" rule (checked *after* lifting piece but *before* placing it, effectively).
    - **Freedom to Move**: "Sliding" check. A piece traverses edges; it cannot squeeze through a gap smaller than itself (i.e., between two pieces).
3.  **Piece Specifics**:
    - **Queen**: Moves 1 space.
    - **Beetle**: Moves 1 space. Can move onto occupied tiles (z-index change).
    - **Grasshopper**: Jumps over at least one piece to the next empty space in a straight line.
    - **Spider**: Moves exactly 3 spaces. Must not backtrack in same turn.
    - **Ant**: Can move to any accessible space on the perimeter (sliding rules apply).

### Win Condition
- **Check**: Look at the 6 neighbors of the Queen Bee.
- If all 6 neighbors are occupied (by any color), the Queen is surrounded.
- If both Queens surrounded -> Draw.

## 6. API Specifications (`app/main.py`)

| Method | Endpoint | Description |
| :--- | :--- | :--- |
| `POST` | `/games` | Initialize a new game. Returns `game_id`. |
| `GET` | `/games/{game_id}` | Retrieve current board state and turn info. |
| `POST` | `/games/{game_id}/move` | Execute a `MoveRequest`. Returns updated state or 400 Error. |
| `GET` | `/games/{game_id}/valid_moves` | (Optional) Helper to show legal moves for client. |

## 7. Implementation Roadmap

1.  **Setup**: Initialize Fast API app structure.
2.  **Phase 1 - Core**: Implement `hex_math` and basic `Board` model.
3.  **Phase 2 - Rules**: Implement `validate_move` with connectivity checks.
4.  **Phase 3 - Pieces**: Add specific movement logic (Ant, Spider, etc.).
5.  **Phase 4 - API**: Wire up endpoints to game logic.
