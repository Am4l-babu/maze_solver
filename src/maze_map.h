#pragma once
#include <Arduino.h>
#include "config.h"

// Classic flood-fill maze model. NOT used in PRIM.E single-path mode
// (MODE_SINGLE_PATH=1) — kept for practice mazes and as a fallback if
// the revealed track turns out to have real junctions after all.

enum Heading : uint8_t { NORTH = 0, EAST = 1, SOUTH = 2, WEST = 3 };

struct Cell {
    uint8_t walls   = 0;      // bit per Heading: 1 = wall present
    bool    visited = false;
    uint8_t flood   = 255;
};

class MazeMap {
public:
    void reset();
    void setWall(int x, int y, Heading h);        // mirrors into neighbour
    bool hasWall(int x, int y, Heading h) const;
    bool isVisited(int x, int y) const { return inBounds(x, y) && _grid[y][x].visited; }
    void markVisited(int x, int y) { if (inBounds(x, y)) _grid[y][x].visited = true; }

    // BFS flood fill from (tx, ty). Unknown edges treated as open so
    // exploration is drawn toward unvisited territory.
    void floodFill(int tx, int ty);
    uint8_t floodAt(int x, int y) const { return inBounds(x, y) ? _grid[y][x].flood : 255; }

    // Neighbour with lowest flood value reachable from (x, y).
    Heading bestMove(int x, int y, Heading current) const;

    static bool inBounds(int x, int y) {
        return x >= 0 && x < MAZE_COLS && y >= 0 && y < MAZE_ROWS;
    }
    static void step(int& x, int& y, Heading h) {
        switch (h) {
            case NORTH: y--; break;
            case EAST:  x++; break;
            case SOUTH: y++; break;
            case WEST:  x--; break;
        }
    }
    static Heading opposite(Heading h) { return (Heading)((h + 2) & 3); }

private:
    Cell _grid[MAZE_ROWS][MAZE_COLS];
};
