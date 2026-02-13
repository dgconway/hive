# BUGS Game C++ Backend

C++ implementation of the BUGS game backend with REST API.

## Requirements

- CMake 3.20+
- C++20 compatible compiler (MSVC 2019+, GCC 10+, Clang 10+)
- OpenMP support

## Dependencies (Header-only, included)

- **cpp-httplib**: HTTP server library
- **nlohmann/json**: JSON parsing and serialization

## Building

```bash
cd cpp
cmake --build build --config Release
```

## Running

```bash
# From cpp/build directory
./bugs_server  # Linux/Mac
bugs_server.exe  # Windows
```

The server will start on `http://localhost:8080`

## API Endpoints

- `POST /games` - Create new game
- `GET /games/{game_id}` - Get game state
- `POST /games/{game_id}/move` - Submit move
- `GET /games/{game_id}/valid_moves?q={q}&r={r}` - Get valid moves
- `POST /games/{game_id}/ai_move?ai_type=minimax` - AI move

