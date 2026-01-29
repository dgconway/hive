# HIVE Backend Framework Specification

## 1. Hexagonal Logic (`app/hex_math.py`)

We utilize **Flat-topped** hexagonal logic with **Axial Coordinates**.

- **Neighbors**:
    - Directions: `(1, 0), (1, -1), (0, -1), (-1, 0), (-1, 1), (0, 1)`
- **Distance**: `(abs(q) + abs(q + r) + abs(r)) / 2`
- **Connectivity**: Use BFS/DFS to traverse all occupied hexes.
    - Removal of a piece (temporarily for movement) must not split the graph.

## 2. Game Logic Rules (`app/game_logic.py`)

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
- Look at the 6 neighbors of the Queen Bee.
- If all 6 neighbors are occupied (by any color), the Queen is surrounded.
- If both Queens surrounded -> Draw.
- If one Queen surrounded -> The player with the surrounded Queen loses. 

## 3. API Specifications (`app/main.py`)

| Method | Endpoint | Description |
| :--- | :--- | :--- |
| `POST` | `/games` | Initialize a new game. Returns `game_id`. |
| `GET` | `/games/{game_id}` | Retrieve current board state and turn info. |
| `POST` | `/games/{game_id}/move` | Execute a `MoveRequest`. Returns updated state or 400 Error. |
| `GET` | `/games/{game_id}/valid_moves` | (Optional) Helper to show legal moves for client. |

