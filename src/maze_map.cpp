#include "maze_map.h"

void MazeMap::reset() {
    for (int y = 0; y < MAZE_ROWS; y++)
        for (int x = 0; x < MAZE_COLS; x++)
            _grid[y][x] = Cell{};
    // Outer boundary walls
    for (int x = 0; x < MAZE_COLS; x++) {
        _grid[0][x].walls             |= (1 << NORTH);
        _grid[MAZE_ROWS - 1][x].walls |= (1 << SOUTH);
    }
    for (int y = 0; y < MAZE_ROWS; y++) {
        _grid[y][0].walls             |= (1 << WEST);
        _grid[y][MAZE_COLS - 1].walls |= (1 << EAST);
    }
}

void MazeMap::setWall(int x, int y, Heading h) {
    if (!inBounds(x, y)) return;
    _grid[y][x].walls |= (1 << h);
    int nx = x, ny = y;
    step(nx, ny, h);
    if (inBounds(nx, ny))
        _grid[ny][nx].walls |= (1 << opposite(h));
}

bool MazeMap::hasWall(int x, int y, Heading h) const {
    if (!inBounds(x, y)) return true;
    return _grid[y][x].walls & (1 << h);
}

void MazeMap::floodFill(int tx, int ty) {
    for (int y = 0; y < MAZE_ROWS; y++)
        for (int x = 0; x < MAZE_COLS; x++)
            _grid[y][x].flood = 255;

    // Fixed ring buffer BFS — no heap allocation in the control path.
    static uint16_t queue[MAZE_ROWS * MAZE_COLS];
    int head = 0, tail = 0;

    _grid[ty][tx].flood = 0;
    queue[tail++] = (uint16_t)(ty * MAZE_COLS + tx);

    while (head != tail) {
        uint16_t idx = queue[head++];
        int x = idx % MAZE_COLS, y = idx / MAZE_COLS;
        uint8_t d = _grid[y][x].flood;

        for (uint8_t h = 0; h < 4; h++) {
            if (hasWall(x, y, (Heading)h)) continue;
            int nx = x, ny = y;
            step(nx, ny, (Heading)h);
            if (!inBounds(nx, ny)) continue;
            if (_grid[ny][nx].flood <= d + 1) continue;
            _grid[ny][nx].flood = d + 1;
            queue[tail++] = (uint16_t)(ny * MAZE_COLS + nx);
        }
    }
}

Heading MazeMap::bestMove(int x, int y, Heading current) const {
    uint8_t best = 255;
    Heading bestH = current;
    // Check current heading first so ties prefer going straight.
    Heading order[4] = { current, (Heading)((current + 1) & 3),
                         (Heading)((current + 3) & 3), opposite(current) };
    for (Heading h : order) {
        if (hasWall(x, y, h)) continue;
        int nx = x, ny = y;
        step(nx, ny, h);
        if (!inBounds(nx, ny)) continue;
        uint8_t f = _grid[ny][nx].flood;
        if (f < best) { best = f; bestH = h; }
    }
    return bestH;
}
