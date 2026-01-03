import pytest
from app.game_logic import GameEngine
from app.models import PieceType, PlayerColor, MoveRequest

@pytest.fixture
def engine():
    return GameEngine()

def test_initial_placement(engine):
    game = engine.create_game()
    assert game.turn_number == 1
    assert game.current_turn == PlayerColor.WHITE
    
    # White places random
    engine.process_move(game.game_id, MoveRequest(
        action="PLACE",
        piece_type=PieceType.ANT,
        to_hex=(0, 0)
    ))
    
    assert game.turn_number == 2
    assert game.current_turn == PlayerColor.BLACK
    assert "0,0" in game.board

def test_black_placement_constraint(engine):
    game = engine.create_game()
    # White 0,0
    engine.process_move(game.game_id, MoveRequest(action="PLACE", piece_type=PieceType.ANT, to_hex=(0,0)))
    
    # Black tries to place disconnected
    with pytest.raises(ValueError, match="next to existing"):
        engine.process_move(game.game_id, MoveRequest(action="PLACE", piece_type=PieceType.ANT, to_hex=(2,2)))
        
    # Black places connected
    engine.process_move(game.game_id, MoveRequest(action="PLACE", piece_type=PieceType.ANT, to_hex=(1,0)))
    assert "1,0" in game.board

def test_unique_color_placement(engine):
    game = engine.create_game()
    # 1. White (0,0)
    engine.process_move(game.game_id, MoveRequest(action="PLACE", piece_type=PieceType.ANT, to_hex=(0,0)))
    # 2. Black (1,0)
    engine.process_move(game.game_id, MoveRequest(action="PLACE", piece_type=PieceType.ANT, to_hex=(1,0)))
    
    # 3. White Place. Must touch White (0,0) and NOT touch Black (1,0).
    # (0,1) touches (0,0) [White] and (1,0) [Black]. Should fail.
    # Neighbors of (1,0) include (0,1). 
    # Let's check coords: 
    # (0,0) neighbors: (1,0), (1,-1), (0,-1), (-1,0), (-1,1), (0,1)
    # (1,0) is Black.
    # (0,1) is neighbor of (0,0) AND (1,0).
    
    with pytest.raises(ValueError, match="cannot touch opponent"):
        engine.process_move(game.game_id, MoveRequest(action="PLACE", piece_type=PieceType.ANT, to_hex=(0,1)))

    # (-1, 0) touches only (0,0). Should succeed.
    engine.process_move(game.game_id, MoveRequest(action="PLACE", piece_type=PieceType.ANT, to_hex=(-1,0)))

def test_queen_bee_movement(engine):
    game = engine.create_game()
    # Setup a scenario where Queen can move
    # 1. W Queen (0,0)
    engine.process_move(game.game_id, MoveRequest(action="PLACE", piece_type=PieceType.QUEEN, to_hex=(0,0)))
    # 2. B Queen (1,0)
    engine.process_move(game.game_id, MoveRequest(action="PLACE", piece_type=PieceType.QUEEN, to_hex=(1,0)))
    
    # 3. W Moves Queen to (-1,0)? 
    # But wait, One Hive Rule. 
    # If W moves (0,0) -> (-1,0). Start (0,0) removed. remaining: {(1,0)}. Connected? Yes.
    # Move (-1,0) touches (0,0) [Empty]?? No 
    # Queen needs connected neighbors at DESTINATION excluding itself.
    # If (0,0) moves to (-1,0), the only other piece is (1,0).
    # (-1,0) and (1,0) are distance 2. Disconnected! 
    # So Queen verify_queen_move -> _can_slide logic doesn't catch connectivity?
    # Actually, execute_move One Hive Check:
    # remove (0,0). occupied={(1,0)}. Is connected? Yes (size 1).
    # move (0,0)->(-1,0).
    # Queen validation: see logic.
    # _validate_queen_move: ... end in occupied... can_slide... 
    # It does NOT explicitly check if the new position is connected to the hive (it assumes sliding along hive implies connection).
    # But if sliding from (0,0) to (-1,0)... 
    # Neighbors of (0,0): (1,0) [Occupied].
    # Common neighbors of (0,0) and (-1,0): (0,-1) and (-1,1). Both empty.
    # Slide check: valid.
    # But result configuration: (-1,0) and (1,0). Gap. Disconnected hive.
    # The Generic One Hive rule only checks if *removal* breaks the hive.
    # It does NOT check if *placement* (destination) maintains a single component (it should!).
    # Correct Hive Logic: The resulting set of pieces MUST be connected.
    # My implementation removed start, checked connection. Then added start back.
    # It missed the check: "Does the new position connect to the Main Hive?"
    
    # Let's see if this fails or passes (logic might be missing that check).
    # Actually, if I slide, I am effectively "rolling" along the edge.
    # If I slide, I must share a neighbor with the CURRENT spot?
    # Yes, start and end are neighbors.
    # If I slide from A to B. And A touched C. 
    # Does B necessarily touch C? No.
    # B could touch D.
    # But if A was the only bridge?
    # Removal check covers A being a bridge (articulation point).
    # If removing A leaves 2 components, then A cannot move.
    # If removing A leaves 1 component (C).
    # Then A moves to B.
    # Does B touch C? Not necessarily.
    # If B does not touch C, and B is not C, then we have {B} and {C} disconnected.
    # So yes, we MUST check connectivity of the FINAL configuration (excluding A, including B).
    
    # Try to move Queen to (-1,0).
    # Removal of (0,0) (White Queen) leaves {(1,0)} (Black Queen). Connected.
    # Placement at (-1,0). Neighbors of (-1,0) are (0,-1) and (-1,1) [Empty].
    # (-1,0) is NOT connected to (1,0) (Distance 2).
    # So valid_move should fail with "destination disconnected" or "Must slide" logic?
    # Actually, is_connected check on future_occupied should fail.
    
    with pytest.raises(ValueError, match="One Hive Rule"):
         engine.process_move(game.game_id, MoveRequest(action="MOVE", from_hex=(0,0), to_hex=(-1,0)))
