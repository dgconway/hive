#pragma once

#include <utility>
#include <vector>
#include <unordered_set>
#include <array>
#include <string>

namespace bugs {

// Type alias for hex coordinates
using Hex = std::pair<int, int>;

// Hex directions in axial coordinates (q, r)
constexpr std::array<Hex, 6> HEX_DIRECTIONS = {{
    {1, 0}, {1, -1}, {0, -1},
    {-1, 0}, {-1, 1}, {0, 1}
}};

// Hash function for Hex (for unordered_set/map)
struct HexHash {
    std::size_t operator()(const Hex& h) const noexcept {
        // Combine hash of both coordinates
        return std::hash<int>{}(h.first) ^ (std::hash<int>{}(h.second) << 1);
    }
};

// Hex arithmetic
Hex add_hex(const Hex& a, const Hex& b);
Hex subtract_hex(const Hex& a, const Hex& b);

// Hex operations
std::array<Hex, 6> get_neighbors(const Hex& hex);
int hex_distance(const Hex& a, const Hex& b);
bool are_neighbors(const Hex& a, const Hex& b);
std::vector<Hex> get_common_neighbors(const Hex& a, const Hex& b);

// Connectivity check
bool is_connected(const std::unordered_set<Hex, HexHash>& hexes);

// Helper to create coordinate key string
std::string coord_to_key(const Hex& h);
Hex key_to_coord(const std::string& key);

} // namespace bugs
