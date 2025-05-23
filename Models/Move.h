#pragma once
#include <stdlib.h>

typedef int8_t POS_T;

// —труктура move_pos описывает один ход на доске
struct move_pos
{
    POS_T x, y;             //  оординаты начальной клетки (откуда)
    POS_T x2, y2;           //  оординаты конечной клетки (куда)
    POS_T xb = -1, yb = -1; //  оординаты побитой фигуры (если есть), иначе -1

    //  онструктор дл€ обычного хода (без вз€ти€)
    move_pos(const POS_T x, const POS_T y, const POS_T x2, const POS_T y2) : x(x), y(y), x2(x2), y2(y2)
    {
    }
    //  онструктор дл€ хода с вз€тием (указаны координаты побитой фигуры)
    move_pos(const POS_T x, const POS_T y, const POS_T x2, const POS_T y2, const POS_T xb, const POS_T yb)
        : x(x), y(y), x2(x2), y2(y2), xb(xb), yb(yb)
    {
    }

    // ќператор сравнени€: равны, если совпадают начальные и конечные координаты
    bool operator==(const move_pos& other) const
    {
        return (x == other.x && y == other.y && x2 == other.x2 && y2 == other.y2);
    }
    // ќператор неравенства
    bool operator!=(const move_pos& other) const
    {
        return !(*this == other);
    }
};
