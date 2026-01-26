#include "hex_math.hpp"
#include <algorithm>
#include <queue>
#include <cmath>
#include <sstream>

namespace bugs {

Hex add_hex(const Hex& a, const Hex& b) {
    return {a.first + b.first, a.second + b.second};
}

Hex subtract_hex(const Hex& a, const Hex& b) {
    return {a.first - b.first, a.second - b.second};
}

std::vector<Hex> get_neighbors(const Hex& hex) {
    std::vector<Hex> neighbors;
    neighbors.reserve(6);
    for (const auto& dir : HEX_DIRECTIONS) {
        neighbors.push_back(add_hex(hex, dir));
    }
    return neighbors;
}

int hex_distance(const Hex& a, const Hex& b) {
    auto vec = subtract_hex(a, b);
    return (std::abs(vec.first) + std::abs(vec.first + vec.second) + std::abs(vec.second)) / 2;
}

bool are_neighbors(const Hex& a, const Hex& b) {
    return hex_distance(a, b) == 1;
}

std::vector<Hex> get_common_neighbors(const Hex& a, const Hex& b) {
    auto a_neighbors = get_neighbors(a);
    auto b_neighbors = get_neighbors(b);
    
    std::unordered_set<Hex, HexHash> a_set(a_neighbors.begin(), a_neighbors.end());
    std::vector<Hex> common;
    
    for (const auto& n : b_neighbors) {
        if (a_set.count(n)) {
            common.push_back(n);
        }
    }
    
    return common;
}

bool is_connected(const std::unordered_set<Hex, HexHash>& hexes) {
    if (hexes.empty()) {
        return true;
    }
    
    // BFS from arbitrary starting hex
    Hex start = *hexes.begin();
    std::unordered_set<Hex, HexHash> visited;
    std::queue<Hex> queue;
    
    visited.insert(start);
    queue.push(start);
    
    while (!queue.empty()) {
        Hex current = queue.front();
        queue.pop();
        
        for (const Hex& neighbor : get_neighbors(current)) {
            if (hexes.count(neighbor) && !visited.count(neighbor)) {
                visited.insert(neighbor);
                queue.push(neighbor);
            }
        }
    }
    
    return visited.size() == hexes.size();
}

std::string coord_to_key(const Hex& h) {
    return std::to_string(h.first) + "," + std::to_string(h.second);
}

Hex key_to_coord(const std::string& key) {
    std::istringstream iss(key);
    int q, r;
    char comma;
    iss >> q >> comma >> r;
    return {q, r};
}

} // namespace bugs
