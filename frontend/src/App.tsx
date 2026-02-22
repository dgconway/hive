import { useState, useEffect, useRef } from 'react';
// ... imports fixed below
import { createGame, makeMove, getValidMoves, requestAiMove, PieceType, PlayerColor } from './api';
import type { Game } from './api';
import './index.css';

// ... constants ...
const HEX_SIZE = 35; // Size (radius)

// Axial to Pixel conversion (Flat topped)
const hexToPixel = (q: number, r: number, size: number) => {
    const x = size * (3 / 2) * q;
    const y = size * Math.sqrt(3) * (r + q / 2);
    return { x, y };
};

const pixelToHex = (x: number, y: number, size: number) => {
    const q = (2 / 3 * x) / size;
    const r = (-1 / 3 * x + Math.sqrt(3) / 3 * y) / size;
    return hexRound(q, r);
};

const hexRound = (q: number, r: number) => {
    let rq = Math.round(q);
    let rr = Math.round(r);
    let s = -q - r;
    let rs = Math.round(s);

    const q_diff = Math.abs(rq - q);
    const r_diff = Math.abs(rr - r);
    const s_diff = Math.abs(rs - s);

    if (q_diff > r_diff && q_diff > s_diff) {
        rq = -rr - rs;
    } else if (r_diff > s_diff) {
        rr = -rq - rs;
    }
    return { q: rq, r: rr };
};

// Neighbor offsets for generating clickable zones
const HEX_DIRECTIONS = [
    [1, 0], [1, -1], [0, -1],
    [-1, 0], [-1, 1], [0, 1]
];

// --- Icons ---
const PieceIcon = ({ type, color }: { type: PieceType; color: PlayerColor }) => {
    const emojiMap: Record<PieceType, string> = {
        [PieceType.QUEEN]: "🐝",
        [PieceType.ANT]: "🐜",
        [PieceType.SPIDER]: "🕷️",
        [PieceType.BEETLE]: "🪲",
        [PieceType.GRASSHOPPER]: "🦗",
        [PieceType.LADYBUG]: "🐞",
        [PieceType.MOSQUITO]: "🦟",
        [PieceType.PILLBUG]: "🪳"
    };

    return (
        <span
            style={{
                fontSize: '24px',
                lineHeight: '1',
                filter: color === PlayerColor.WHITE ? 'drop-shadow(0 0 2px rgba(0,0,0,0.5))' : 'drop-shadow(0 0 2px rgba(255,255,255,0.8))',
                cursor: 'pointer'
            }}
            role="img"
            aria-label={type}
        >
            {emojiMap[type]}
        </span>
    );
};

// --- Main Component ---
function App() {
    const [game, setGame] = useState<Game | null>(null);
    // Removed unused gameId state if truly unused
    // const [gameId, setGameId] = useState<string | null>(null);

    const [error, setError] = useState<string | null>(null);

    // Interaction State
    const [selectedHex, setSelectedHex] = useState<{ q: number, r: number } | null>(null);
    const [selectedPieceType, setSelectedPieceType] = useState<PieceType | null>(null);
    const [hoverHex, setHoverHex] = useState<{ q: number, r: number } | null>(null);
    const [validMoves, setValidMoves] = useState<Array<[number, number]>>([]);
    const [validPlacements, setValidPlacements] = useState<Array<[number, number]>>([]);

    // AI State
    const [aiEnabled, setAiEnabled] = useState(true);
    const [aiColor, setAiColor] = useState<PlayerColor>(PlayerColor.BLACK);
    const [aiType, setAiType] = useState<string>("minimax");
    const [isAiThinking, setIsAiThinking] = useState(false);

    // Advanced Mode
    const [advancedMode, setAdvancedMode] = useState(false);

    // Viewport Pan/Zoom
    const [offset, setOffset] = useState({ x: window.innerWidth / 2, y: window.innerHeight / 2 });
    const [isDragging, setIsDragging] = useState(false);
    const lastMousePos = useRef({ x: 0, y: 0 });

    useEffect(() => {
        startNewGame();
    }, []);

    // Trigger AI move
    useEffect(() => {
        if (game && aiEnabled && game.current_turn === aiColor && game.status === "IN_PROGRESS" && !isAiThinking) {
            handleAiMove();
        }
    }, [game, aiEnabled, aiColor, isAiThinking]);

    const handleAiMove = async () => {
        if (!game) return;
        setIsAiThinking(true);
        setError(null);
        try {
            const updated = await requestAiMove(game.game_id, aiType);
            setGame(updated);
        } catch (err: any) {
            console.error(err);
            setError(err.response?.data?.detail || "AI Move Error");
            // If AI fails, maybe disable it so user can't get stuck
            setAiEnabled(false);
        } finally {
            setIsAiThinking(false);
        }
    };

    const startNewGame = async (modeOverride?: boolean) => {
        const useAdvanced = modeOverride !== undefined ? modeOverride : advancedMode;
        try {
            const g = await createGame(useAdvanced);
            setGame(g);
            setValidMoves([]);
            setValidPlacements([]);
            setError(null);
        } catch (e) {
            console.error(e);
            setError("Failed to create game");
        }
    };

    // Calculate valid placement hexes for the current player
    const getValidPlacements = (g: Game): Array<[number, number]> => {
        const occupied: Set<string> = new Set(Object.keys(g.board).filter(k => g.board[k].length > 0));
        const placements: Array<[number, number]> = [];

        // First move: place at origin
        if (occupied.size === 0) {
            return [[0, 0]];
        }

        // Second move (turn 2): place adjacent to any piece
        if (g.turn_number === 2) {
            const candidates = new Set<string>();
            occupied.forEach(key => {
                const [q, r] = key.split(',').map(Number);
                HEX_DIRECTIONS.forEach(([dq, dr]) => {
                    const nKey = `${q + dq},${r + dr}`;
                    if (!occupied.has(nKey)) {
                        candidates.add(nKey);
                    }
                });
            });
            candidates.forEach(key => {
                const [q, r] = key.split(',').map(Number);
                placements.push([q, r]);
            });
            return placements;
        }

        // General case: touch own, not opponent
        const pieceColors: Map<string, PlayerColor> = new Map();
        Object.entries(g.board).forEach(([key, stack]) => {
            if (stack.length > 0) {
                pieceColors.set(key, stack[stack.length - 1].color);
            }
        });

        // Find candidates: empty hexes adjacent to own color
        const candidates = new Set<string>();
        pieceColors.forEach((color, key) => {
            if (color === g.current_turn) {
                const [q, r] = key.split(',').map(Number);
                HEX_DIRECTIONS.forEach(([dq, dr]) => {
                    const nKey = `${q + dq},${r + dr}`;
                    if (!occupied.has(nKey)) {
                        candidates.add(nKey);
                    }
                });
            }
        });

        // Filter: must not touch opponent
        candidates.forEach(key => {
            const [q, r] = key.split(',').map(Number);
            let touchesOpponent = false;
            HEX_DIRECTIONS.forEach(([dq, dr]) => {
                const nKey = `${q + dq},${r + dr}`;
                const neighborColor = pieceColors.get(nKey);
                if (neighborColor && neighborColor !== g.current_turn) {
                    touchesOpponent = true;
                }
            });
            if (!touchesOpponent) {
                placements.push([q, r]);
            }
        });

        return placements;
    };

    const handlePieceSelection = (pieceType: PieceType) => {
        if (!game) return;
        setSelectedPieceType(pieceType);
        setSelectedHex(null);
        setValidMoves([]);

        // Calculate and show valid placements
        const placements = getValidPlacements(game);
        setValidPlacements(placements);
    };

    const handleHexClick = async (q: number, r: number) => {
        if (!game) return;

        // ACTION: PLACE
        if (selectedPieceType) {
            const key = `${q},${r}`;
            // If clicking on an occupied hex with current player's piece, switch to move mode
            if (game.board[key] && game.board[key].length > 0) {
                const topPiece = game.board[key][game.board[key].length - 1];
                if (topPiece.color === game.current_turn) {
                    // Switch to selecting this piece for movement
                    setSelectedPieceType(null);
                    setSelectedHex({ q, r });
                    // Fetch valid moves for this piece
                    try {
                        const moves = await getValidMoves(game.game_id, q, r);
                        setValidMoves(moves);
                        if (moves.length === 0) {
                            setError("No valid moves for this piece!");
                        } else {
                            setError(null);
                        }
                    } catch (e) {
                        console.error(e);
                        setValidMoves([]);
                    }
                    setValidPlacements([]);
                    return;
                }
            }

            // Place the piece
            try {
                const updated = await makeMove(game.game_id, "PLACE", [q, r], selectedPieceType);
                setGame(updated);
                setSelectedPieceType(null); // Reset selection
                setValidMoves([]);
                setValidPlacements([]);
                setError(null);
            } catch (err: any) {
                setError(err.response?.data?.detail || "Invalid Move");
            }
            return;
        }

        // Check if we are in "Move Target" mode (Piece selected)
        if (selectedHex) {
            // If clicking the SAME hex, deselect
            if (selectedHex.q === q && selectedHex.r === r) {
                setSelectedHex(null);
                setValidMoves([]);
                return;
            }

            // Check if this (q,r) is a VALID MOVE
            const isMoveTarget = validMoves.some(([vq, vr]) => vq === q && vr === r);

            if (isMoveTarget) {
                // Check if target is occupied (Pillbug throw source selection)
                const key = `${q},${r}`;
                if (game.board[key] && game.board[key].length > 0) {
                    // Select this piece (throwable target)
                    setSelectedHex({ q, r });

                    // Fetch valid moves for this piece
                    try {
                        const moves = await getValidMoves(game.game_id, q, r);
                        setValidMoves(moves);
                        if (moves.length === 0) {
                            setError("No valid moves for this piece!");
                        } else {
                            setError(null);
                        }
                    } catch (e) {
                        console.error(e);
                        setValidMoves([]);
                    }
                    return;
                }

                // EXECUTE MOVE
                try {
                    const updated = await makeMove(game.game_id, "MOVE", [q, r], undefined, [selectedHex.q, selectedHex.r]);
                    setGame(updated);
                    setSelectedHex(null);
                    setValidMoves([]);
                    setError(null);
                } catch (err: any) {
                    setError(err.response?.data?.detail || "Invalid Move");
                }
                return;
            }

            // If NOT a valid move, but IS my piece, then change selection (fallthrough to Select logic below)
            // Check if standard helper covers this? 
            // We need to verify if the clicked hex HAS a piece of mine.
        }

        // ACTION: MOVE (Select Source)
        const key = `${q},${r}`;
        if (game.board[key] && game.board[key].length > 0) {
            // Check if correct player
            const stack = game.board[key];
            const top = stack[stack.length - 1];
            if (top.color === game.current_turn) {

                // If re-clicking same hex, deselect
                if (selectedHex && selectedHex.q === q && selectedHex.r === r) {
                    setSelectedHex(null);
                    setValidMoves([]);
                    return;
                }

                setSelectedHex({ q, r });

                // Fetch Valid Moves
                try {
                    const moves = await getValidMoves(game.game_id, q, r);
                    setValidMoves(moves);
                    if (moves.length === 0) {
                        setError("No valid moves for this piece!");
                    } else {
                        setError(null);
                    }
                } catch (e) {
                    console.error(e);
                    setValidMoves([]);
                }
                return;
            }
        }


    };

    const renderHexes = () => {
        if (!game) return null;
        const hexes: JSX.Element[] = [];

        // occupied list
        const occupiedKeys = new Set(Object.keys(game.board));

        // Add all occupied hexes
        occupiedKeys.forEach(key => {
            const [q, r] = key.split(',').map(Number);
            const stack = game.board[key];
            if (stack.length === 0) return;

            const topPiece = stack[stack.length - 1];
            const { x, y } = hexToPixel(q, r, HEX_SIZE);

            const isSelected = selectedHex?.q === q && selectedHex?.r === r;
            // Check if this occupied hex is a valid move target (e.g. beetle stack?)
            // validMoves is array of [q,r].
            const isValidMove = validMoves.some(([vq, vr]) => vq === q && vr === r);

            hexes.push(
                <g key={key} transform={`translate(${x},${y})`} onClick={(e) => { e.stopPropagation(); handleHexClick(q, r); }}>
                    <polygon
                        points={getHexPoints(HEX_SIZE)}
                        className={`hex ${topPiece.color === PlayerColor.WHITE ? 'hex-white' : 'hex-black'} ${isSelected ? 'hex-selected' : ''}`}
                        style={isValidMove ? { stroke: (stack.length > 0 ? '#0000ff' : '#00ff00'), strokeWidth: 3 } : {}}
                    />
                    <foreignObject x={-12} y={-12} width={24} height={24}>
                        <div style={{ display: 'flex', justifyContent: 'center' }}>
                            <PieceIcon type={topPiece.type} color={topPiece.color} />
                        </div>
                    </foreignObject>
                    {stack.length > 1 && (
                        <circle r={5} cx={15} cy={-15} fill="red" /> // Stack indicator
                    )}
                    <text x={0} y={20} fontSize="8" textAnchor="middle" fill="#999">{q},{r}</text>
                </g>
            );
        });

        // Add empty neighbors for placement hint (if something selected)
        // To make UI easy, we render a grid of clickable areas around the hive + 1 layer?
        // Or just render specific interaction targets? 
        // Let's render 'Ghost' hexes around the hive to allow clicks.

        const potentialSpots = new Set<string>();

        // Always include valid moves in potential spots if they are empty
        validMoves.forEach(([vq, vr]) => {
            const key = `${vq},${vr}`;
            if (!occupiedKeys.has(key)) {
                potentialSpots.add(key);
            }
        });

        if (occupiedKeys.size === 0) {
            potentialSpots.add("0,0"); // First move always 0,0 center
        } else {
            occupiedKeys.forEach(key => {
                const [q, r] = key.split(',').map(Number);
                HEX_DIRECTIONS.forEach(([dq, dr]) => {
                    const nKey = `${q + dq},${r + dr}`;
                    if (!occupiedKeys.has(nKey)) {
                        potentialSpots.add(nKey);
                    }
                });
            });
        }

        potentialSpots.forEach(key => {
            const [q, r] = key.split(',').map(Number);
            const { x, y } = hexToPixel(q, r, HEX_SIZE);
            const isHover = hoverHex?.q === q && hoverHex?.r === r;

            const isValidMove = validMoves.some(([vq, vr]) => vq === q && vr === r);
            const isValidPlacement = validPlacements.some(([vq, vr]) => vq === q && vr === r);
            const isActive = selectedPieceType || selectedHex;
            const isHighlighted = isValidMove || isValidPlacement;

            if (isActive || isHighlighted) {
                hexes.push(
                    <g key={`empty-${key}`} transform={`translate(${x},${y})`}
                        onClick={(e) => { e.stopPropagation(); handleHexClick(q, r); }}
                        style={{ opacity: (isHover || isHighlighted) ? 1 : 0.3 }}
                    >
                        <polygon
                            points={getHexPoints(HEX_SIZE)}
                            className="hex hex-preview"
                            fill="transparent"
                            style={isHighlighted ? { stroke: '#00ff00', strokeWidth: 3, fill: 'rgba(0,255,0,0.1)' } : {}}
                        />
                    </g>
                );
            }
        });

        return hexes;
    };

    const getHexPoints = (size: number) => {
        // Flat Topped vertices
        return [0, 1, 2, 3, 4, 5].map(i => {
            const angle_deg = 60 * i;
            const angle_rad = Math.PI / 180 * angle_deg;
            return `${size * Math.cos(angle_rad)},${size * Math.sin(angle_rad)}`;
        }).join(' ');
    };

    // Mouse Handlers for Pan
    const handleMouseDown = (e: React.MouseEvent) => {
        setIsDragging(true);
        lastMousePos.current = { x: e.clientX, y: e.clientY };
    };
    const handleMouseMove = (e: React.MouseEvent) => {
        if (isDragging) {
            const dx = e.clientX - lastMousePos.current.x;
            const dy = e.clientY - lastMousePos.current.y;
            setOffset(prev => ({ x: prev.x + dx, y: prev.y + dy }));
            lastMousePos.current = { x: e.clientX, y: e.clientY };
        }

        // Calculate Hover Hex
        const rect = e.currentTarget.getBoundingClientRect();
        // Mouse Relative to center of SVG group
        const mx = e.clientX - rect.left - offset.x;
        const my = e.clientY - rect.top - offset.y;
        const h = pixelToHex(mx, my, HEX_SIZE);
        setHoverHex(h);
    };
    const handleMouseUp = () => setIsDragging(false);

    return (
        <div className="game-container">
            {/* Header */}
            <div className="header">
                <h2>BUGS</h2>
                {game && (
                    <div className="status-bar">
                        <span className={`turn-indicator ${game.current_turn === PlayerColor.WHITE ? 'turn-white' : 'turn-black'}`}>
                            Turn {game.turn_number}: {game.current_turn} {isAiThinking ? "(AI Thinking...)" : ""}
                        </span>
                        <div className="ai-controls">
                            <label>
                                <input type="checkbox" checked={aiEnabled} onChange={e => setAiEnabled(e.target.checked)} />
                                Play vs AI
                            </label>
                            {aiEnabled && (
                                <>
                                    <select value={aiColor} onChange={e => setAiColor(e.target.value as PlayerColor)}>
                                        <option value={PlayerColor.BLACK}>AI as Black</option>
                                        <option value={PlayerColor.WHITE}>AI as White</option>
                                    </select>
                                    <select value={aiType} onChange={e => setAiType(e.target.value)}>
                                        <option value="minimax">Strategy: Minimax</option>
                                        <option value="mcts">Strategy: Neural MCTS</option>
                                    </select>
                                </>
                            )}
                        </div>
                        <button onClick={() => startNewGame()}>Reset</button>
                        <label style={{ marginLeft: '12px' }}>
                            <input type="checkbox" checked={advancedMode} onChange={e => {
                                const newVal = e.target.checked;
                                setAdvancedMode(newVal);
                                startNewGame(newVal);
                            }} />
                            Advanced Mode
                        </label>
                    </div>
                )}
                {error && <div className="error-toast">{error}</div>}
            </div>

            {/* Canvas */}
            <div className="canvas-container"
                onMouseDown={handleMouseDown}
                onMouseMove={handleMouseMove}
                onMouseUp={handleMouseUp}
                onMouseLeave={handleMouseUp}
            >
                <svg width="100%" height="100%">
                    <g transform={`translate(${offset.x},${offset.y})`}>
                        {renderHexes()}
                    </g>
                </svg>
            </div>

            {/* Controls / Hand */}
            {game && !game.winner && (
                <div className="hand-panel">
                    <div className="player-hand">
                        <h4>Available Pieces</h4>
                        <div className="piece-buttons">
                            {/* Only show hand for current player for simplicity */}
                            {Object.entries(game.current_turn === PlayerColor.WHITE ? game.white_pieces_hand : game.black_pieces_hand).map(([type, count]) => (
                                <button
                                    key={type}
                                    className={`piece-btn ${selectedPieceType === type ? 'selected' : ''}`}
                                    onClick={() => handlePieceSelection(type as PieceType)}
                                    disabled={count === 0}
                                >
                                    <PieceIcon type={type as PieceType} color={game.current_turn} />
                                    <span>x{count}</span>
                                </button>
                            ))}
                        </div>
                    </div>
                    {selectedHex && (
                        <div style={{ alignSelf: 'center', color: '#888' }}>
                            Selected: {selectedHex.q},{selectedHex.r} <br />
                            Click empty hex to Move here.
                        </div>
                    )}
                </div>
            )}
            {game?.winner && (
                <div className="header" style={{ top: '50%', fontSize: '2rem' }}>
                    Game Over! Winner: {game.winner}
                </div>
            )}
        </div>
    );
}

export default App;
