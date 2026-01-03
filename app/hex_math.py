from typing import Tuple, List, Set

# Type alias for clarity
Hex = Tuple[int, int]

# Directions in Axial Coordinates (q, r)
# (1, 0), (1, -1), (0, -1), (-1, 0), (-1, 1), (0, 1)
HEX_DIRECTIONS = [
    (1, 0), (1, -1), (0, -1),
    (-1, 0), (-1, 1), (0, 1)
]

def add_hex(a: Hex, b: Hex) -> Hex:
    return (a[0] + b[0], a[1] + b[1])

def subtract_hex(a: Hex, b: Hex) -> Hex:
    return (a[0] - b[0], a[1] - b[1])

def get_neighbors(hex: Hex) -> List[Hex]:
    """Returns the 6 constituent neighbors of a hex."""
    return [add_hex(hex, d) for d in HEX_DIRECTIONS]

def hex_distance(a: Hex, b: Hex) -> int:
    """Calculates Manhattan distance on a hex grid."""
    vec = subtract_hex(a, b)
    return (abs(vec[0]) + abs(vec[0] + vec[1]) + abs(vec[1])) // 2

def are_neighbors(a: Hex, b: Hex) -> bool:
    return hex_distance(a, b) == 1

def get_common_neighbors(a: Hex, b: Hex) -> List[Hex]:
    """Returns neighbors shared by two hexes."""
    a_neighbors = set(get_neighbors(a))
    b_neighbors = set(get_neighbors(b))
    return list(a_neighbors.intersection(b_neighbors))

def is_connected(hexes: Set[Hex]) -> bool:
    """Checks if a set of hexes forms a single connected component."""
    if not hexes:
        return True
    
    start = next(iter(hexes))
    visited = {start}
    queue = [start]
    
    while queue:
        current = queue.pop(0)
        for neighbor in get_neighbors(current):
            if neighbor in hexes and neighbor not in visited:
                visited.add(neighbor)
                queue.append(neighbor)
                
    return len(visited) == len(hexes)
