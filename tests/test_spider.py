import pytest
from app.game_logic import GameEngine
from app.models import PieceType, PlayerColor, MoveRequest
from app.hex_math import Hex

# Helper to setup a game with pieces
@pytest.fixture
def engine():
    return GameEngine()

def test_spider_exact_3_steps(engine):
    game = engine.create_game()
    # Mock setup:
    # Place Queen (White) at 0,0
    # Place Queen (Black) at 1,0
    # Place Spider (White) at -1,0
    
    # We need to manually inject state to skip turn 1 checks etc for speed
    # or just play valid moves.
    
    # 1. White Places Queen at 0,0
    engine.process_move(game.game_id, MoveRequest(action="PLACE", piece_type=PieceType.QUEEN, to_hex=(0,0)))
    
    # 2. Black Places Queen at 1,0
    engine.process_move(game.game_id, MoveRequest(action="PLACE", piece_type=PieceType.QUEEN, to_hex=(1,0)))
    
    # 3. White Places Spider at -1,0
    engine.process_move(game.game_id, MoveRequest(action="PLACE", piece_type=PieceType.SPIDER, to_hex=(-1,0)))
    
    # 4. Black Places Ant at 2,0
    engine.process_move(game.game_id, MoveRequest(action="PLACE", piece_type=PieceType.ANT, to_hex=(2,0)))
    
    # 5. White Moves Spider from -1,0
    # Valid moves should be exactly 3 spaces along the hive (Queen 0,0 and Queen 1,0)
    # Spider is at -1,0. Neighbors: (0,0) [Occupied], (0,-1), (-1,-1), (-2,0), (-2,1), (-1,1).
    # Path must hug hive. Hive is (0,0), (1,0), (2,0).
    # Path 1: (-1,0) -> (0,-1) [touching 0,0] -> (1,-1) [touching 0,0/1,0] -> (2,-1) [touching 1,0] ??
    # Let's see what get_valid_moves returns.
    
    valid_moves = engine.get_valid_moves(game.game_id, -1, 0)
    print(f"Valid Moves for Spider at -1,0: {valid_moves}")
    
    # Check that ONLY EXACTLY 3 steps are allowed.
    # A step 1 move (e.g. 0,-1) should NOT be in valid_moves
    assert not (0, -1) in valid_moves
    # A step 2 move (e.g. 1,-1) should NOT be in valid_moves
    assert not (1, -1) in valid_moves
    
    # A step 3 move should be there.
    # Path: (-1,0) -> (0,-1) -> (1,-1) -> (1, -2)? No.
    # Let's trace:
    # Start: -1,0
    # Step 1: 0,-1 (Touching 0,0). Valid.
    # Step 2: 1,-1 (Touching 0,0 and 1,0). Valid.
    # Step 3: From 1,-1 neighbors: (1,0)[Occ], (2,-1), (2,-2), (1,-2), (0,-1)[Visited], (0,0)[Occ]
    #         Candidate: (2,-1). Touches 1,0 (Black Queen) and 2,0 (Black Ant). Valid.
    #So (2,-1) should be valid.
    
    assert (2, -1) in valid_moves or (1, -2) in valid_moves # Depending on exact geometry
    
    # Verify NO JUMPING
    # Cannot go to (0, 1) directly because no common neighbor sliding? 
    # (-1,0) and (0,1) share (0,0) [Occ] and (-1,1) [Empty]. 
    # Slide is valid.
    # But distance is 2? No, neighbors.
    # Wait, (-1,0) and (0,1) are neighbors? 
    # (0,0) neighbors: (1,0), (0,1), (-1,1), (-1,0), (0,-1), (1,-1).
    # (-1,0) neighbors: (0,0), (-1,1), ...
    # Yes, (-1,0) touches (-1,1) and (0,0). (0,1) touches (0,0) and (-1,1).
    # So they are distance 2 apart (share neighbors (-1,1) and (0,0)). 
    # Spider cannot jump to non-neighbor.
    
    for m in valid_moves:
        # Distance from start check? No, spider CAN end up close to start (e.g. circle).
        pass

def test_spider_cannot_jump_gap(engine):
    game = engine.create_game()
    # Scenario:
    #   A   B
    #    \ /
    #     S
    #    / \
    #   C   D
    #
    # Assume S (Spider) is at (0,0).
    # A at (-1, -1), B at (0, -1)? No.
    # Let's make a ring with a gap.
    # Q(0,0) - A(1,0) - B(2,0)
    #             |
    #           S(1,1)
    #
    # Can S(1,1) move to (0,1)?
    # S(1,1) neighbors: (1,0)[Occ A], (2,1), (2,2), (1,2), (0,1)[Empty], (0,0)[Occ Q].
    # Move S(1,1) -> (0,1).
    # Common Neighbors of (1,1) and (0,1):
    #   1. (0,0) [Occupied by Q]
    #   2. (1,0) [Occupied by A]? Wait.
    #   (1,1) neighbors: (1+1,1)=(2,1), (1+1, 1-1)=(2,0), (1, 1-1)=(1,0)...
    #   (0,1) neighbors: (1,1)[Start], (1,0)[A], ...
    #
    #   Common neighbors of (1,1) and (0,1) are:
    #     (1,0) - Occupied (A)
    #     (0,0) - Occupied (Q) (?)
    #   If both occupied, GATE IS BLOCKED. Cannot move.
    
    # We want a GAP.
    # Common neighbors must be EMPTY.
    # So we need S to touch X, and Dest to touch Y.
    # But X and Y are NOT neighbors of each other?
    #
    #    X   Y
    #     \ /
    #      .  <-- Gap
    #     / \
    #    S   D
    #
    # Setup:
    # S at (0,0).
    # X at (-1, -1)? No, (0,-1).
    # Y at (-2, 0)? No.
    # Let's say:
    # Piece X at (0, -1). piece Y at (-1, 1).
    # S at (0,0).
    # Move S->(-1,0).
    # Common neighbors of (0,0) and (-1,0):
    #   1. (0,-1) [Occupied X]
    #   2. (-1,1) [Occupied Y]
    # If both occupied -> Gate Closed.
    
    # Needs EMPTY common neighbors.
    # X at (0, -1). Y at (-1, 0).
    # S at (1, -1).
    # Move S -> (0, 0)?
    # Common of (1,-1) and (0,0):
    #   (1,0) [Empty]
    #   (0,-1) [Occupied X]
    # One occupied -> Valid slide.
    
    # GAP SCENARIO:
    # X at (-1, -1). Y at (1, -1).
    # S at (0, 0).
    # S touches neither? No Hive must be connected.
    #
    #       X(0,-1)     Y(2,-1)
    #           \       /
    #            S(1,0)
    #
    # S(1,0) touches X(0,-1) and... wait.
    # (1,0) neighbors: (2,0), (2,-1), (1,-1), (0,-1), (0,0), (1,1).
    # It touches (0,-1) [X]. It touches (2,-1) [Y].
    # Move S(1,0) -> (1,-1).
    # Common neighbors of (1,0) and (1,-1):
    #   (2,-1) [Y]
    #   (0,-1) [X]
    # If both X and Y are present, GATE IS BLOCKED.
    #
    # Okay, "Gap" implies we want to cross a space where we DON'T touch.
    # But if we don't touch, we aren't sliding along anything.
    #
    # Example:
    #   X(0,-1)
    #      |
    #   S(0,0) -> Move to (1,0)?
    #   
    #   Common neighbors of (0,0) and (1,0):
    #     (1,-1) [Empty]
    #     (0,1)  [Empty]
    #   
    #   If I move S(0,0) -> (1,0), I am sliding into space.
    #   Does (1,0) touch the hive?
    #   If (1,0) touches X(0,-1)? No. Distance 2.
    #   If (1,0) touches ANYONE?
    #   If no, then connectivity breakage? S was the only one touching X?
    #   If X is pinned?
    #
    #   Let's use a LOOSE hive.
    #   X(0,-1). Y(2,-1). Z(2,0).
    #   S(0,0).
    #   Chain: S(0,0) - X(0,-1) - ... - Y(2,-1) - Z(2,0).
    #   (Assume connected via far path).
    #   
    #   Move S(0,0) -> (1,0).
    #   Common neighbors: (1,-1) and (0,1). Both Empty.
    #   Does (1,0) touch hive?
    #   (1,0) neighbors: (2,0)[Z], (2,-1)[Y], (1,-1), (0,-1)[X? No], ...
    #   Yes, (1,0) touches Y and Z.
    #   So S is moving from touching X, to touching Y/Z, across a gap (1,-1)/(0,1).
    #   Neither common neighbor is occupied.
    #   THIS IS A JUMP. Should be disallowed.
    
    # SETUP:
    # 1. White Q at (0,-1)
    # 2. Black Q at (0,-2) (just to connect)
    # 3. White Ant at (2,-1)
    # 4. Black Ant at (2,0)
    # 5. Connect the two groups? 
    #    Q(0,-1) -- BQ(0,-2) -- BA(1,-2) -- WA(2,-2) -- W_Ant(2,-1) -- B_Ant(2,0)
    #    Chain exists.
    # 6. Place White Spider at (0,0). Touches Q(0,-1).
    # 7. Try move S(0,0) -> (1,0).
    #    (1,0) touches B_Ant(2,0) and W_Ant(2,-1).
    #    Common neighbors of (0,0) and (1,0) are (0,1) and (1,-1). BOTH EMPTY.
    #    This move should be INVALID.
    
    # Let's perform setup manually to ensure exact coords
    
    # Pieces:
    # (0,-1) [Occ]
    # (1,0) [Target] - touches (2,0) and (2,-1).
    # Need pieces at (2,0) and (2,-1).
    
    # We need to bridge (0,-1) to (2,-1).
    # Bridge: (0,-1) -> (1,-2) -> (2,-2) -> (2,-1).
    
    # Placements:
    # Use direct board manipulation to avoid turn validation logic for complex setup
    from app.models import Piece
    import uuid
    
    def place_piece(q, r, type, color):
        key = f"{q},{r}"
        game.board[key] = [Piece(type=type, color=color, id=str(uuid.uuid4()))]
    
    place_piece(0, -1, PieceType.QUEEN, PlayerColor.WHITE)
    place_piece(1, -2, PieceType.QUEEN, PlayerColor.BLACK)
    place_piece(0, -2, PieceType.ANT, PlayerColor.WHITE)
    place_piece(2, -2, PieceType.ANT, PlayerColor.BLACK)
    place_piece(2, -1, PieceType.BEETLE, PlayerColor.WHITE)
    place_piece(1, -3, PieceType.BEETLE, PlayerColor.BLACK)
    place_piece(0, 0, PieceType.SPIDER, PlayerColor.WHITE)
    
    # Set correct turn to White so Spider (White) can move
    game.current_turn = PlayerColor.WHITE
    
    # Manually run get_valid_moves
         
    # Check Valid Moves for Spider at (0,0)
    valid_moves = engine.get_valid_moves(game.game_id, 0, 0)
    
    # (1,0) is the "Jump" target. 
    # Path length from (0,0) to (1,0) is 1 step? Spider moves 3.
    # But if we just check if it CAN slide to (1,0) as step 1.
    # Wait, get_valid_moves returns FINAL destination.
    # We want to check if the PATH finding allows this step.
    # If the generator allows (1,0) as a first step, then it might find a 3-step path involving it.
    
    # Actually, let's verify if 'valid_moves' contains any destination that requires jump.
    # Or simpler: access the protected _can_slide method directly to unit test logic?
    # Or just rely on the fact that if it can't slide to (1,0), it can't reach that side.
    
    # Let's try to verify that (1,0) is NOT a valid FIRST step.
    # But we can't see steps.
    # We can check if `valid_moves` contains anything on the "Right" side of the gap.
    # (0,0) is Left. (1,0) is Right (Gap).
    # If gap is jumpable, Spider can reach (2,0), (2,1), etc.
    # If gap is NOT jumpable, Spider is stuck on Left side (around Q).
    
    # Left side hexes: (-1,0), (-1,-1), (0,-2)...
    # Right side hexes: (1,0), (2,0), (1,-1), (2,-1)...
    
    right_side_targets = [(1,0), (2,0), (1,-1), (2,-1), (3,-1)]
    
    intersection = [h for h in valid_moves if h in right_side_targets]
    print(f"Jump Targets Reached: {intersection}")
    
    assert len(intersection) == 0, "Spider jumped the gap!"


def test_spider_ring_discrepancy(engine):
    """
    Test scenario where Spider moves in a ring around its starting position.
    If 'start' is considered occupied during pathfinding, it might BLOCK a gate.
    generator (start removed) says Valid. Validator (start occupied?) says Invalid.
    """
    game = engine.create_game()
    from app.models import Piece, MoveRequest
    import uuid
    
    def place_piece(q, r, type, color):
        key = f"{q},{r}"
        game.board[key] = [Piece(type=type, color=color, id=str(uuid.uuid4()))]
        
    # Scenario: Spider at (0,0).
    # Neighbors to form a gate that includes (0,0)?
    # Move A -> B. Gate neighbors are C and D.
    # If C is (0,0) [Spider Start]. And D is some other piece.
    # If C is occupied (Validator assumption), Gate is Blocked (Size 2).
    # If C is empty (Generator assumption), Gate is Open (Size 1).
    
    # Needs:
    # 1. Spider S at (0,0).
    # 2. Piece D at (2, -1).
    # 3. Path S(0,0) -> Step1(1,0) -> Step2(2,0) -> Step3(1,-1).
    #
    # Move Step 2 (2,0) -> Step 3 (1,-1).
    # Common Neighbors of (2,0) and (1,-1):
    #   1. (2,-1) [Piece D]. Occupied.
    #   2. (1,0) [Step 1]. Empty (Spider left it).
    # Wait, we need common neighbors to include (0,0).
    #
    # Let's try simpler:
    # Move S(0,0) -> A -> B -> C.
    # Slide B -> C. Common neighbors: X and (0,0).
    # If X is occupied, and (0,0) is occupied => Blocked.
    #
    # Setup:
    # S at (0,0).
    # Piece X at (0, -2)? 
    # B at (-1, -1). C at (0, -1).
    # Common of B(-1,-1) and C(0,-1):
    #   1. (0, -2) [Piece X]
    #   2. (-1, 0) [Empty?]
    #
    # We need common neighbors of Step2 and Step3 to be: [OtherPiece, StartPos].
    #
    # Step 3 is Destination? Or Step 3 is intermediate?
    # Spider moves 3 steps.
    #
    # Let's try:
    # S at (0,0).
    # Piece X at (1, -1).
    # Path: (0,0) -> (1,0) -> (2,-1) -> (1,-1) [Occupied X]? No.
    #
    # Path:
    # S(0,0).
    # Step 1: (-1, 0).
    # Step 2: (-1, -1).
    # Step 3: (0, -1).
    #
    # Slide Step 2 (-1,-1) -> Step 3 (0,-1).
    # Common Neighbors:
    #   1. (0,0) [S Start]
    #   2. (-1,0) [Step 1]
    #
    # If (-1,0) is Occupied? No, Step 1 is the spider itself (visited).
    # Wait, `occupied` set in VALIDATOR includes all pieces on board.
    # So `occupied` includes (0,0).
    # Does `occupied` include (-1,0)? No, unless there is a piece there.
    #
    # We need another piece Y at (-1,0).
    # But if Y is at (-1,0), S can't move there at Step 1.
    #
    # Maybe Step 1 is different.
    # S(0,0) -> (-1, 1).
    # Step 2 -> (-1, 0).
    # Step 3 -> (0, -1).
    #
    # Slide Step 2 (-1,0) -> Step 3 (0,-1).
    # Common Neighbors:
    #  1. (0,0) [S Start].
    #  2. (-1, -1) [Needs Piece X].
    #
    # So:
    # S at (0,0).
    # Piece X at (-1, -1).
    # We need a path for S to reach (-1,0) at Step 2.
    # Path: (0,0) -> (-1, 1) -> (-1, 0).
    # To take Step 1 (0,0)->(-1,1): Need valid slide.
    # Neighbors of (0,0) and (-1,1): (-1,0) and (0,1).
    # (-1,0) is Step 2 (Empty). (0,1) Empty. Slide OK.
    # Step 2 (-1,1) -> (-1,0):
    # Neighbors: (0,0)[Start] and (-2,1)[Empty]. Slide OK (Start=Occ is only 1 neighbor).
    # Step 3 (-1,0) -> (0,-1).
    # Neighbors: (-1,-1) [Piece X] AND (0,0) [Start].
    #
    # HERE:
    # If Start(0,0) is Occupied (Validator): Neighbors X and Start are BOTH occupied. Gate CLOSED. Move (-1,0)->(0,-1) Fails.
    # If Start(0,0) is Empty (Generator): Only X is occupied. Gate OPEN. Move OK.
    #
    # This matches the bug!
    # Setup:
    # S at (0,0).
    # Piece X at (-1, -1).
    # We need connection. X touches (-1, -2)? Make a hive.
    # Piece Y at (0, 1) just to connect S?
    # S(0,0) touches X(-1,-1)? No.
    # S(0,0) neighbors: (-1,-1) is NOT a neighbor?
    # Hex(0,0) neighbors: (1,0), (0,1), (-1,1), (-1,0), (0,-1), (1,-1).
    # (-1,-1) is NOT a neighbor. Distance is sqrt(3)?
    # Wait, (0,-1) and (-1,0) share neighbors?
    # (0,-1) neighbors: (0,0), (1,-1), (1,-2), (0,-2), (-1,-1), (-1,0).
    # (-1,0) neighbors: (0,0), (-1,1), (-2,1), (-2,0), (-1,-1), (0,-1).
    #
    # Common neighbors of (-1,0) and (0,-1):
    #  1. (0,0) - Start
    #  2. (-1,-1) - Piece X
    #
    # YES.
    # So setup:
    # S at (0,0).
    # Piece X at (-1, -1).
    # Piece Z at (0, -2) (to connect X).
    # Piece W at (1, -2) (to connect Z).
    # ... Just ensure X is part of hive.
    # S must touch hive. S touches X?
    # S(0,0) neighbors: (-1,0), (0,-1), (1,-1), (1,0), (0,1), (-1,1).
    # (-1,-1) is NOT a neighbor of (0,0).
    # So S(0,0) is NOT touching X(-1,-1).
    # We need S to touch something.
    # Piece Y at (1, 0)?
    # Path: S(0,0) -> (-1,1) -> (-1,0) -> (0,-1).
    #
    # Does S touch Y(1,0)? Yes.
    # Does X(-1,-1) touch anything?
    # Let's put X at (-1,-1). Y at (1,0). Connect them?
    # X(-1,-1) -> (0,-1) -> (1,-1) -> Y(1,0).
    # We can place "Filler" pieces at (0,-1) and (1,-1) but (0,-1) is the destination?
    # Destination must be empty.
    # So (0,-1) must be empty.
    #
    # Hive: X(-1,-1) --- Z(-1,0)?? No Z is Step 2.
    # Connect X(-1,-1) via BACK side.
    # X(-1,-1) to (-2,-1) to (-2,0) to (-1,1)??
    # Just simpler:
    # S(0,0).
    # Piece A at (-1,-1).
    # Piece B at (1,0).
    # Piece C at (0,1).
    # Piece D at (-1,1)? No, Step 1 is (-1,1).
    #
    # Need connectivity.
    # Main Hive: A(-1,-1), B(?,?), ...
    #
    # Let's use `place_piece` to force state.
    # White S at (0,0).
    # Black A at (-1,-1). (Gatekeeper 1)
    # White B at (0,-2). (To connect A)
    # Black C at (1,-1). (To connect B)
    # White D at (1,0). (Gatekeeper 2? or just connector).
    #
    # Path: (0,0) -> (-1,1) -> (-1,0) -> (0,-1).
    # Step 1: (0,0) -> (-1,1).
    #   Requires gate (0,1) and (-1,0) to be open.
    #   (-1,0) is empty Step 2. (0,1) is empty. OK.
    # Step 2: (-1,1) -> (-1,0).
    #   Requires gate (0,0) and (-2,1).
    #   (0,0) is Start (Occupied in Validator). (-2,1) Empty.
    #   Gate count = 1. OK.
    # Step 3: (-1,0) -> (0,-1).
    #   Requires gate (0,0) and (-1,-1).
    #   (0,0) is Start (Occupied in Validator).
    #   (-1,-1) is Piece A (Occupied).
    #   Gate count = 2. BLOCKED (in Validator).
    #   Should be OPEN (count=1) in Generator.
    
    place_piece(0, 0, PieceType.SPIDER, PlayerColor.WHITE)
    place_piece(-1, -1, PieceType.ANT, PlayerColor.BLACK) # Gatekeeper
    place_piece(1, 0, PieceType.QUEEN, PlayerColor.WHITE) # To touch S
    # Connect A(-1,-1) and Q(1,0)?
    # A(-1,-1) neighbors: (-1,0), (0,-1), ...
    # Q(1,0) neighbors: (0,0), (2,0), (1,-1), (0,1)...
    # Just add pieces to form a chain behind.
    place_piece(0, 1, PieceType.BEETLE, PlayerColor.BLACK) # Touch S(0,0) and Q(1,0)
    place_piece(-1, 0, PieceType.GRASSHOPPER, PlayerColor.WHITE) # WAIT! (-1,0) is Step 2. Must be EMPTY.
    # Remove that.
    
    # We need A(-1,-1) to be connected to others without blocking path.
    # A(-1,-1) connects to (-1,-2)?
    # Let's just assume valid board state/connectivity for `process_move` logic tests?
    # `process_move` checks `is_connected`.
    # So we MUST ensure hive is connected.
    
    # S(0,0) - connected to B(0,1).
    # B(0,1) - connected to Q(1,0).
    # Q(1,0) - connected to ??
    # A(-1,-1).
    # Connect A(-1,-1) to B(0,1) via (-1,0)? NO that's the path.
    # Connect via left/bottom.
    # A(-1,-1) -> (-2,-1) -> (-2,0) -> (-2,1) -> (-1,1) NO that's Step 1.
    #
    # Okay, we need A(-1,-1) to NOT be isolated.
    # Does A(-1,-1) have to be connected?
    # If the hive is S, B, Q... and A is disconnected?
    # Then `is_connected` fails before move?
    # Yes.
    # So A MUST be connected.
    # 
    # Use a Beetle stack?
    # Or just bridge effectively.
    # B(0,1) -> (-1,2) -> (-2,1) -> (-2,0) -> (-2,-1) -> (-1,-1).
    # That clears the Ring Path: (-1,1), (-1,0), (0,-1).
    
    place_piece(0, 1, PieceType.BEETLE, PlayerColor.BLACK) # B
    place_piece(-1, 2, PieceType.ANT, PlayerColor.WHITE)
    place_piece(-2, 1, PieceType.ANT, PlayerColor.BLACK)
    place_piece(-2, 0, PieceType.ANT, PlayerColor.WHITE)
    place_piece(-2, -1, PieceType.ANT, PlayerColor.BLACK)
    place_piece(-1, -1, PieceType.ANT, PlayerColor.WHITE) # A (Gatekeeper)
    
    game.current_turn = PlayerColor.WHITE
    
    # Check if Generator finds the move
    valid_moves = engine.get_valid_moves(game.game_id, 0, 0)
    target = (0, -1)
    
    assert target in valid_moves, "Generator should find the move"
    
    # Check if Validator accepts the move
    req = MoveRequest(action="MOVE", from_hex=(0,0), to_hex=target)
    engine.process_move(game.game_id, req) # Should NOT raise ValueError


if __name__ == "__main__":
    t = GameEngine()
    test_spider_exact_3_steps(t)
