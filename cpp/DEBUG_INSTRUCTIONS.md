# Debug Instructions

## Issue
After the first two moves, the AI returns "AI could not find any legal moves" error.

## Debug Build Ready
I've added debug logging to help identify the issue. The new build will show:
- How many legal actions the AI finds
- Current turn and turn number when AI move is requested
- Step-by-step progress through the AI move process

## To Test

1. **Stop the current server** (press Ctrl+C in the terminal running bugs_server.exe)

2. **The build is ready** - just run it again:
   ```bash
   cd c:\Users\16089\Downloads\bugs_game\cpp\build\Release
   .\bugs_server.exe
   ```

3. **Play the game** and watch the server console output

4. **Look for these debug messages**:
   - "AI move requested for game: ..."
   - "Current turn: ..., Turn number: ..."
   - "Legal actions found: X"
   - If X = 0, that's the problem!

## Likely Causes
1. **Queen placement rule**: AI must place queen by turn 4
2. **No valid moves**: All pieces might be pinned (One Hive Rule)
3. **State synchronization**: Game state might not be updating correctly

Once we see the debug output, we'll know exactly what's wrong!
