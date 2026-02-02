#pragma once

#include "models.hpp"
#include "hex_math.hpp"
#include <cstdint>
#include <random>
#include <array>
#include <unordered_map>

namespace bugs {

// Zobrist hashing for fast, incremental game state hashing
class ZobristHash {
public:
    static ZobristHash& instance() {
        static ZobristHash inst;
        return inst;
    }

    // Get hash for a piece at a position
    uint64_t get_piece_hash(PieceType type, PlayerColor color, const Hex& pos) const {
        int type_idx = static_cast<int>(type);
        int color_idx = static_cast<int>(color);
        auto key = make_key(pos.first, pos.second, type_idx, color_idx);
        auto it = piece_hashes_.find(key);
        if (it != piece_hashes_.end()) {
            return it->second;
        }
        // Generate on demand for positions not pre-cached
        return generate_hash(pos.first, pos.second, type_idx, color_idx);
    }

    // Get hash for current turn
    uint64_t get_turn_hash(PlayerColor turn) const {
        return turn == PlayerColor::WHITE ? turn_hash_white_ : turn_hash_black_;
    }

    // Get hash for a piece in hand
    uint64_t get_hand_hash(PieceType type, PlayerColor color, int count) const {
        int type_idx = static_cast<int>(type);
        int color_idx = static_cast<int>(color);
        // Use a simple combination
        return hand_base_hash_ ^ (type_idx * 7919) ^ (color_idx * 6997) ^ (count * 5501);
    }

private:
    ZobristHash() {
        std::mt19937_64 rng(0xDEADBEEF);  // Fixed seed for reproducibility
        
        // Generate turn hashes
        turn_hash_white_ = rng();
        turn_hash_black_ = rng();
        hand_base_hash_ = rng();
        
        // Pre-generate hashes for common board positions (-20 to 20 for q and r)
        for (int q = -20; q <= 20; ++q) {
            for (int r = -20; r <= 20; ++r) {
                for (int type = 0; type < 5; ++type) {
                    for (int color = 0; color < 2; ++color) {
                        auto key = make_key(q, r, type, color);
                        piece_hashes_[key] = rng();
                    }
                }
            }
        }
    }

    static uint64_t make_key(int q, int r, int type, int color) {
        // Pack into a single key: offset q,r by 100 to handle negatives
        return ((uint64_t)(q + 100) << 32) | ((uint64_t)(r + 100) << 16) | (type << 2) | color;
    }

    uint64_t generate_hash(int q, int r, int type, int color) const {
        // For positions outside the pre-cached range
        std::mt19937_64 rng(0xDEADBEEF ^ make_key(q, r, type, color));
        return rng();
    }

    std::unordered_map<uint64_t, uint64_t> piece_hashes_;
    uint64_t turn_hash_white_;
    uint64_t turn_hash_black_;
    uint64_t hand_base_hash_;
};

// Compute full Zobrist hash for a game state
inline uint64_t compute_zobrist_hash(const Game& game) {
    uint64_t hash = 0;
    const auto& zobrist = ZobristHash::instance();

    // Hash board pieces
    for (const auto& [pos, stack] : game.board) {
        if (stack.empty()) continue;
        for (size_t i = 0; i < stack.size(); ++i) {
            const auto& piece = stack[i];
            hash ^= zobrist.get_piece_hash(piece.type, piece.color, pos);
            // Add stack level info
            hash ^= (i + 1) * 0x9E3779B97F4A7C15ULL;
        }
    }

    // Hash turn
    hash ^= zobrist.get_turn_hash(game.current_turn);

    // Hash hands
    for (const auto& [type, count] : game.white_pieces_hand) {
        hash ^= zobrist.get_hand_hash(type, PlayerColor::WHITE, count);
    }
    for (const auto& [type, count] : game.black_pieces_hand) {
        hash ^= zobrist.get_hand_hash(type, PlayerColor::BLACK, count);
    }

    return hash;
}

} // namespace bugs
