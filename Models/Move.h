#pragma once
#include <stdlib.h>

typedef int8_t POS_T;

// The move_pos struct describes a single move on the board
struct move_pos
{
    POS_T x, y;             // Coordinates of the starting cell (from)
    POS_T x2, y2;           // Coordinates of the ending cell (to)
    POS_T xb = -1, yb = -1; // Coordinates of the captured piece (if any), otherwise -1

    // Constructor for a normal move (without capture)
    move_pos(const POS_T x, const POS_T y, const POS_T x2, const POS_T y2) : x(x), y(y), x2(x2), y2(y2)
    {
    }
    // Constructor for a move with capture (captured piece coordinates specified)
    move_pos(const POS_T x, const POS_T y, const POS_T x2, const POS_T y2, const POS_T xb, const POS_T yb)
        : x(x), y(y), x2(x2), y2(y2), xb(xb), yb(yb)
    {
    }

    // Comparison operator: equal if start and end coordinates match
    bool operator==(const move_pos& other) const
    {
        return (x == other.x && y == other.y && x2 == other.x2 && y2 == other.y2);
    }
    // Inequality operator
    bool operator!=(const move_pos& other) const
    {
        return !(*this == other);
    }
};
