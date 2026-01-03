import uuid
from typing import Dict, List, Optional, Tuple, Set
from .models import Game, GameStatus, Piece, PieceType, PlayerColor, MoveRequest
from .hex_math import Hex, get_neighbors, are_neighbors, is_connected, get_common_neighbors, hex_distance

INITIAL_HAND = {
    PieceType.QUEEN: 1,
    PieceType.ANT: 3,
    PieceType.GRASSHOPPER: 3,
    PieceType.SPIDER: 2,
    PieceType.BEETLE: 2
}

class GameEngine:
    def __init__(self):
        self.games: Dict[str, Game] = {}

    def create_game(self) -> Game:
        game_id = str(uuid.uuid4())
        new_game = Game(
            game_id=game_id,
            board={},
            current_turn=PlayerColor.WHITE,
            turn_number=1,
            white_pieces_hand=INITIAL_HAND.copy(),
            black_pieces_hand=INITIAL_HAND.copy(),
            status=GameStatus.IN_PROGRESS
        )
        self.games[game_id] = new_game
        return new_game

    def get_game(self, game_id: str) -> Optional[Game]:
        return self.games.get(game_id)

    def process_move(self, game_id: str, move: MoveRequest) -> Game:
        game = self.get_game(game_id)
        if not game:
            raise ValueError("Game not found")
        
        if game.status == GameStatus.FINISHED:
            raise ValueError("Game is finished")

        self.validate_turn(game, move)

        if move.action == "PLACE":
            self.execute_place(game, move)
        elif move.action == "MOVE":
            self.execute_move(game, move)
        
        self.check_win_condition(game)
        
        # Switch turn
        game.current_turn = PlayerColor.BLACK if game.current_turn == PlayerColor.WHITE else PlayerColor.WHITE
        game.turn_number += 1
        
        return game

    def validate_turn(self, game: Game, move: MoveRequest):
        # 1. Turn 4 Rule
        hand = game.white_pieces_hand if game.current_turn == PlayerColor.WHITE else game.black_pieces_hand
        if game.turn_number >= 7 and hand[PieceType.QUEEN] == 1:
            # Turn 1,2,3... actually turn counts both players. 
            # White turns: 1, 3, 5, 7. Black turns: 2, 4, 6, 8.
            # "By turn 4" usually means by the player's 4th turn (round 4).
            # So if it's the 4th action for this player (Turn 7 or 8) and Queen is still in hand
            pass # Implementation below in logic check
            
        if move.action == "MOVE":
            if hand[PieceType.QUEEN] == 1:
                 raise ValueError("Must place Queen Bee before moving pieces")
        elif move.action == "PLACE":
             # If it is the 4th turn for the player (White: 7, Black: 8) and Queen not played, MUST play Queen
            is_fourth_turn = (game.current_turn == PlayerColor.WHITE and game.turn_number == 7) or \
                             (game.current_turn == PlayerColor.BLACK and game.turn_number == 8)
            
            if is_fourth_turn and hand[PieceType.QUEEN] == 1 and move.piece_type != PieceType.QUEEN:
                raise ValueError("Rules require placing Queen Bee by the 4th turn")

    def _get_occupied_hexes(self, board: Dict[str, List[Piece]]) -> Set[Hex]:
        hexes = set()
        for key in board:
            if board[key]: # Only if stack is not empty (though keys should verify empty stacks)
                q, r = map(int, key.split(","))
                hexes.add((q, r))
        return hexes

    def _coord_to_key(self, h: Hex) -> str:
        return f"{h[0]},{h[1]}"

    def execute_place(self, game: Game, move: MoveRequest):
        if not move.piece_type:
            raise ValueError("Piece type required for placement")
            
        hand = game.white_pieces_hand if game.current_turn == PlayerColor.WHITE else game.black_pieces_hand
        if hand.get(move.piece_type, 0) <= 0:
            raise ValueError(f"No {move.piece_type} remaining in hand")

        coord_key = self._coord_to_key(move.to_hex)
        
        if coord_key in game.board and  game.board[coord_key]:
             raise ValueError("Cannot place on occupied tile")

        # Placement Rules
        if not game.board:
            # First piece ever
            pass
        elif game.turn_number == 2:
            # Second piece ever (Black's first). Must touch White's piece.
            neighbors = get_neighbors(move.to_hex)
            has_neighbor = False
            for n in neighbors:
                if self._coord_to_key(n) in game.board:
                    has_neighbor = True
                    break
            if not has_neighbor:
                raise ValueError("Must place next to existing hive")
        else:
            # General rule: Must touch OWN color, must NOT touch OPPONENT color
            neighbors = get_neighbors(move.to_hex)
            touching_own = False
            touching_opponent = False
            
            for n in neighbors:
                n_key = self._coord_to_key(n)
                if n_key in game.board and game.board[n_key]:
                    top_piece = game.board[n_key][-1]
                    if top_piece.color == game.current_turn:
                        touching_own = True
                    else:
                        touching_opponent = True
            
            if not touching_own:
                raise ValueError("New placements must touch your own color")
            if touching_opponent:
                raise ValueError("New placements cannot touch opponent pieces")

        # Create piece
        new_piece = Piece(
            type=move.piece_type,
            color=game.current_turn,
            id=str(uuid.uuid4())
        )
        
        game.board[coord_key] = [new_piece]
        hand[move.piece_type] -= 1

    def execute_move(self, game: Game, move: MoveRequest):
        if not move.from_hex:
            raise ValueError("Origin required for move")
            
        from_key = self._coord_to_key(move.from_hex)
        to_key = self._coord_to_key(move.to_hex) # Although we check models, ensure to_hex is tuple
        
        if from_key not in game.board or not game.board[from_key]:
            raise ValueError("No piece at origin")
            
        piece_to_move = game.board[from_key][-1]
        
        if piece_to_move.color != game.current_turn:
            raise ValueError("Cannot move opponent's piece")

        # --- ONE HIVE RULE ---
        # 1. Check if removal breaks hive (unless it's a stack > 1)
        stack_height = len(game.board[from_key])
        occupied = self._get_occupied_hexes(game.board)
        
        # Determine the set of pieces AFTER the move
        future_occupied = occupied.copy()
        if stack_height == 1:
            future_occupied.remove(move.from_hex)
        
        # Check if intermediate state (piece picked up) is connected
        if not is_connected(future_occupied):
             raise ValueError("Move violates One Hive Rule (disconnects hive)")
             
        future_occupied.add(move.to_hex)
        if not is_connected(future_occupied):
             raise ValueError("Move violates One Hive Rule (destination disconnected)")
        
        if move.from_hex == move.to_hex:
             raise ValueError("Cannot move to same position")
        
        # --- PIECE SPECIFIC LOGIC ---
        valid = False
        if piece_to_move.type == PieceType.QUEEN:
            valid = self._validate_queen_move(game, move.from_hex, move.to_hex, occupied)
        elif piece_to_move.type == PieceType.BEETLE:
            valid = self._validate_beetle_move(game, move.from_hex, move.to_hex, occupied)
        elif piece_to_move.type == PieceType.GRASSHOPPER:
            valid = self._validate_grasshopper_move(game, move.from_hex, move.to_hex, occupied)
        elif piece_to_move.type == PieceType.SPIDER:
            valid = self._validate_spider_move(game, move.from_hex, move.to_hex, occupied)
        elif piece_to_move.type == PieceType.ANT:
            valid = self._validate_ant_move(game, move.from_hex, move.to_hex, occupied)
            
        if not valid:
            raise ValueError(f"Invalid move for {piece_to_move.type}")

        # Execute Move
        game.board[from_key].pop()
        if not game.board[from_key]:
            del game.board[from_key]
            
        if to_key not in game.board:
            game.board[to_key] = []
        game.board[to_key].append(piece_to_move)



    def _can_slide(self, start: Hex, end: Hex, occupied: Set[Hex]) -> bool:
        """
        Freedom to Move Rule + Contact Rule:
        1. Freedom to Move: Gate must not be blocked (2 occupied neighbors).
        2. Contact: Must slide ALONG a piece (>= 1 occupied neighbor).
           (Unless Beetle moving up/down, which is handled separately)
        """
        common = get_common_neighbors(start, end)
        occupied_common = [n for n in common if n in occupied]
        
        # If 2 common neighbors are occupied, the 'gate' is closed.
        if len(occupied_common) >= 2:
            return False
            
        # Must allow sliding ALONG the hive.
        # If 0 common neighbors are occupied, we are 'flying' across a gap
        # or moving into open space disconnected from local neighbors.
        # Strict rule: Must share at least one occupied neighbor to slide.
        if len(occupied_common) == 0:
             return False
             
        return True

    def _validate_queen_move(self, game: Game, start: Hex, end: Hex, occupied: Set[Hex]) -> bool:
        if not are_neighbors(start, end):
            return False
        if end in occupied:
            return False # Queen cannot stack
        if not self._can_slide(start, end, occupied):
            return False
        # Destination must have at least one neighbor (to stay connected)
        # But One Hive rule handled globally check if *removing* broke it.
        # Now we just need to ensure end position touches something (besides start) 
        # Actually, if we just slide around, connection is usually preserved, 
        # but we do need to check if we are just moving into empty space detached from everyone else?
        # The 'One Hive' check verifies the graph *after* removal.
        # But if I move to a spot that is only connected to 'start', and 'start' is now empty, 
        # connectivity is maintained by the move itself being continuous? 
        # Yes, but sliding implies contact. 
        return True

    def _validate_beetle_move(self, game: Game, start: Hex, end: Hex, occupied: Set[Hex]) -> bool:
        if not are_neighbors(start, end):
            return False
        # Beetle CAN move onto occupied (stack)
        # Beetle implements a different 'slide':
        # If moving ONTO a hive (z > 0), no sliding restriction from neighbors?
        # Or if moving FROM on top to down?
        # Simplification: If moving from z=0 to z=0, apply slide.
        start_z = len(game.board.get(self._coord_to_key(start), []))
        end_z = len(game.board.get(self._coord_to_key(end), []))
        
        if start_z == 1 and end_z == 0:
            if not self._can_slide(start, end, occupied):
                return False
        
        return True

    def _validate_grasshopper_move(self, game: Game, start: Hex, end: Hex, occupied: Set[Hex]) -> bool:
        # Must jump over pieces in a straight line
        # 1. Determine direction
        dq = end[0] - start[0]
        dr = end[1] - start[1]
        
        # Normalize direction
        if dq == 0 and dr == 0: return False
        
        # Check if it is a straight line
        # Valid directions: (1,0), (-1,0), (0,1), (0,-1), (1,-1), (-1,1)
        # We can check if dq, dr are integer multiples of one of the 6 unit vectors
        direction = None
        
        # This normalization logic is a bit tricky for non-unit distances, 
        # simplest: iterate units and see if we hit end.
        
        if dq == 0: step = (0, 1 if dr > 0 else -1)
        elif dr == 0: step = (1 if dq > 0 else -1, 0)
        elif dq == -dr: step = (1 if dq > 0 else -1, 1 if dr > 0 else -1)
        else: return False # Not a straight line
        
        current = (start[0] + step[0], start[1] + step[1])
        if current not in occupied:
            return False # Must jump over at least one piece immediately
            
        while current in occupied:
            current = (current[0] + step[0], current[1] + step[1])
            
        if current == end:
            return True
        return False

    def _validate_spider_move(self, game: Game, start: Hex, end: Hex, occupied: Set[Hex]) -> bool:
        # Move exactly 3 spaces. No backtracking.
        # "No backtracking" in a single turn means it cannot visit the same hex twice?
        # Or just cannot reverse direction immediately? 
        # Standard rules: generic path of length 3, visited nodes distinct.
        
        def find_spider_paths(curr: Hex, steps_left: int, visited: Set[Hex]) -> bool:
            if steps_left == 0:
                return curr == end
            
            neighbors = get_neighbors(curr)
            for n in neighbors:
                if n in occupied_for_path: continue # Cannot walk through pieces
                if n in visited: continue
                
                # Must slide and stay connected
                # Check Sliding - IMPORTANT: Use occupied_for_path (spider removed)
                if not self._can_slide(curr, n, occupied_for_path):
                    continue
                
                # Check Contact: The hex 'n' must share a neighbor with the hive (excluding curr)
                # i.e., it must trigger "slide" logic (sliding around the hive)
                # Actually, can_slide handles the restriction of "squeezing", 
                # but we also need to ensure we are "hugging" the hive.
                # If n has NO occupied neighbors (except possibly the one we just left if it wasn't empty, but here space is empty),
                # wait, occupied set includes ALL pieces.
                
                n_neighbors = get_neighbors(n)
                has_contact = any(nb in occupied_for_path for nb in n_neighbors)
                if not has_contact: continue 
                
                new_visited = visited.copy()
                new_visited.add(n)
                if find_spider_paths(n, steps_left - 1, new_visited):
                    return True
            return False

        # Note: 'occupied' includes 'start' in the passed set, but Spider leaves start.
        # So effective occupied for pathing should exclude start.
        occupied_for_path = occupied.copy()
        if start in occupied_for_path: occupied_for_path.remove(start)
        
        return find_spider_paths(start, 3, {start})

    def _validate_ant_move(self, game: Game, start: Hex, end: Hex, occupied: Set[Hex]) -> bool:
        if start == end: return False
        if end in occupied: return False
        
        # Traverse perimeter using BFS
        queue = [start]
        visited = {start}
        
        occupied_for_path = occupied.copy()
        if start in occupied_for_path: occupied_for_path.remove(start)
        
        while queue:
            curr = queue.pop(0)
            if curr == end: return True
            
            neighbors = get_neighbors(curr)
            for n in neighbors:
                if n in occupied_for_path: continue
                if n in visited: continue
                
                # Restrictions:
                # 1. Slide
                if not self._can_slide(curr, n, occupied_for_path): # Pass occupied_for_path to simulate empty start
                    continue
                
                # 2. Hug Hive (must touch at least one occupied hex)
                n_neighbors = get_neighbors(n)
                has_contact = any(nb in occupied_for_path for nb in n_neighbors)
                if not has_contact: continue
                
                visited.add(n)
                queue.append(n)
                
        return False

    def get_valid_moves(self, game_id: str, q: int, r: int) -> List[Tuple[int, int]]:
        game = self.get_game(game_id)
        if not game or game.status == GameStatus.FINISHED:
            return []

        from_hex = (q, r)
        key = self._coord_to_key(from_hex)
        
        # 1. Basic Checks
        if key not in game.board or not game.board[key]:
            return []
        
        piece = game.board[key][-1]
        if piece.color != game.current_turn:
            return []
            
        # Queen Bee Played Check
        hand = game.white_pieces_hand if game.current_turn == PlayerColor.WHITE else game.black_pieces_hand
        if hand[PieceType.QUEEN] == 1 and piece.type != PieceType.QUEEN:
             return []
             
        # 2. One Hive Check (Removal)
        occupied = self._get_occupied_hexes(game.board)
        stack_height = len(game.board[key])
        
        occupied_after_lift = occupied.copy()
        if stack_height == 1:
            occupied_after_lift.remove(from_hex)
            
        if not is_connected(occupied_after_lift):
             return [] # Pinned

        # 3. Generate Candidates based on Piece Type
        candidates = set()
        
        if piece.type == PieceType.QUEEN:
            candidates = self._gen_queen_moves(from_hex, occupied_after_lift)
        elif piece.type == PieceType.BEETLE:
            candidates = self._gen_beetle_moves(game, from_hex, occupied_after_lift)
        elif piece.type == PieceType.GRASSHOPPER:
            candidates = self._gen_grasshopper_moves(from_hex, occupied)
        elif piece.type == PieceType.SPIDER:
            candidates = self._gen_spider_moves(from_hex, occupied_after_lift)
        elif piece.type == PieceType.ANT:
            candidates = self._gen_ant_moves(from_hex, occupied_after_lift)

        return list(candidates)

    def _gen_queen_moves(self, start: Hex, occupied: Set[Hex]) -> Set[Hex]:
        moves = set()
        for n in get_neighbors(start):
            if n in occupied: continue
            if not self._can_slide(start, n, occupied): continue
            
            neighbors_of_n = get_neighbors(n)
            if any(nb in occupied for nb in neighbors_of_n):
                moves.add(n)
        return moves

    def _gen_beetle_moves(self, game: Game, start: Hex, occupied: Set[Hex]) -> Set[Hex]:
        moves = set()
        for n in get_neighbors(start):
            start_key = self._coord_to_key(start)
            end_key = self._coord_to_key(n)
            start_z = len(game.board.get(start_key, []))
            
            is_dest_empty = (n not in occupied)
            
            if start_z == 1 and is_dest_empty:
                if not self._can_slide(start, n, occupied): continue
                
            if is_dest_empty:
                neighbors_of_n = get_neighbors(n)
                if not any(nb in occupied for nb in neighbors_of_n):
                    continue
            
            moves.add(n)
        return moves

    def _gen_grasshopper_moves(self, start: Hex, occupied: Set[Hex]) -> Set[Hex]:
        moves = set()
        from .hex_math import HEX_DIRECTIONS
        for d in HEX_DIRECTIONS:
            curr = (start[0] + d[0], start[1] + d[1])
            if curr not in occupied: continue
            while curr in occupied:
                 curr = (curr[0] + d[0], curr[1] + d[1])
            moves.add(curr)
        return moves

    def _gen_spider_moves(self, start: Hex, occupied: Set[Hex]) -> Set[Hex]:
        valid_ends = set()
        def search(curr, steps, visited):
            if steps == 0:
                valid_ends.add(curr)
                return
            for n in get_neighbors(curr):
                if n in occupied: continue
                if n in visited: continue
                if not self._can_slide(curr, n, occupied): continue
                if not any(nb in occupied for nb in get_neighbors(n)): continue
                new_visited = visited.copy()
                new_visited.add(n)
                search(n, steps-1, new_visited)
        search(start, 3, {start})
        return valid_ends

    def _gen_ant_moves(self, start: Hex, occupied: Set[Hex]) -> Set[Hex]:
        moves = set()
        queue = [start]
        visited = {start}
        while queue:
            curr = queue.pop(0)
            for n in get_neighbors(curr):
                if n in occupied: continue
                if n in visited: continue
                if not self._can_slide(curr, n, occupied): continue
                if not any(nb in occupied for nb in get_neighbors(n)): continue
                visited.add(n)
                queue.append(n)
                moves.add(n)
        return moves

    def check_win_condition(self, game: Game):
        # Check if any Queen is surrounded
        queens = []
        for key, stack in game.board.items():
            for p in stack:
                if p.type == PieceType.QUEEN:
                    # Parse key
                    q, r = map(int, key.split(","))
                    queens.append((p, (q, r)))
                    
        white_surrounded = False
        black_surrounded = False
        
        for piece, loc in queens:
            neighbors = get_neighbors(loc)
            occupied_count = 0
            for n in neighbors:
                if self._coord_to_key(n) in game.board:
                     occupied_count += 1
            
            if occupied_count == 6:
                if piece.color == PlayerColor.WHITE:
                    white_surrounded = True
                else:
                    black_surrounded = True
                    
        if white_surrounded and black_surrounded:
             game.status = GameStatus.FINISHED
             game.winner = None # Draw
        elif white_surrounded:
             game.status = GameStatus.FINISHED
             game.winner = PlayerColor.BLACK
        elif black_surrounded:
             game.status = GameStatus.FINISHED
             game.winner = PlayerColor.WHITE
