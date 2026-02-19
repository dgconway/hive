import axios from 'axios';

const api = axios.create({
    baseURL: '/api', // Proxied to http://127.0.0.1:8000
});

export enum PieceType {
    QUEEN = "QUEEN",
    ANT = "ANT",
    SPIDER = "SPIDER",
    BEETLE = "BEETLE",
    GRASSHOPPER = "GRASSHOPPER",
    LADYBUG = "LADYBUG",
    MOSQUITO = "MOSQUITO",
    PILLBUG = "PILLBUG"
}

export enum PlayerColor {
    WHITE = "WHITE",
    BLACK = "BLACK"
}

export interface Piece {
    type: PieceType;
    color: PlayerColor;
    id: string;
}

export interface Game {
    game_id: string;
    board: Record<string, Piece[]>; // key "q,r", value: stack of pieces (bottom to top)
    current_turn: PlayerColor;
    turn_number: number;
    white_pieces_hand: Record<PieceType, number>;
    black_pieces_hand: Record<PieceType, number>;
    winner: PlayerColor | null;
    status: string;
    advanced_mode: boolean;
}

export const createGame = async (advancedMode: boolean = false): Promise<Game> => {
    const response = await api.post<Game>('/games', { advanced_mode: advancedMode });
    return response.data;
};

export const getGame = async (gameId: string): Promise<Game> => {
    const response = await api.get<Game>(`/games/${gameId}`);
    return response.data;
};

export const makeMove = async (
    gameId: string,
    action: "PLACE" | "MOVE",
    toHex: [number, number],
    pieceType?: PieceType,
    fromHex?: [number, number]
): Promise<Game> => {
    const payload: any = {
        action,
        to_hex: toHex
    };
    if (action === "PLACE") payload.piece_type = pieceType;
    if (action === "MOVE") payload.from_hex = fromHex;

    const response = await api.post<Game>(`/games/${gameId}/move`, payload);
    return response.data;
};

export const getValidMoves = async (
    gameId: string,
    q: number,
    r: number
): Promise<Array<[number, number]>> => {
    const response = await api.get<Array<[number, number]>>(`/games/${gameId}/valid_moves`, {
        params: { q, r }
    });
    return response.data;
};

export const requestAiMove = async (gameId: string, aiType: string = "minimax"): Promise<Game> => {
    const response = await api.post<Game>(`/games/${gameId}/ai_move`, null, {
        params: { ai_type: aiType }
    });
    return response.data;
};
