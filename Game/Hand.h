#pragma once
#include <tuple>

#include "../Models/Move.h"
#include "../Models/Response.h"
#include "Board.h"

// Класс Hand отвечает за обработку пользовательского ввода (мышь, окно)
class Hand
{
public:
    Hand(Board* board) : board(board)
    {
    }

    // Получение выбранной пользователем клетки или команды (откат, выход, повтор)
    tuple<Response, POS_T, POS_T> get_cell() const
    {
        SDL_Event windowEvent;
        Response resp = Response::OK;
        int x = -1, y = -1;
        int xc = -1, yc = -1;
        while (true)
        {
            if (SDL_PollEvent(&windowEvent))
            {
                switch (windowEvent.type)
                {
                case SDL_QUIT:
                    // Пользователь закрыл окно — выход из игры
                    resp = Response::QUIT;
                    break;
                case SDL_MOUSEBUTTONDOWN:
                    // Обработка клика мыши: вычисление координат клетки
                    x = windowEvent.motion.x;
                    y = windowEvent.motion.y;
                    xc = int(y / (board->H / 10) - 1);
                    yc = int(x / (board->W / 10) - 1);
                    // Если клик по области "назад" — откат хода
                    if (xc == -1 && yc == -1 && board->history_mtx.size() > 1)
                    {
                        resp = Response::BACK;
                    }
                    // Если клик по области "повтор" — повторить партию
                    else if (xc == -1 && yc == 8)
                    {
                        resp = Response::REPLAY;
                    }
                    // Если клик по игровой клетке — вернуть координаты клетки
                    else if (xc >= 0 && xc < 8 && yc >= 0 && yc < 8)
                    {
                        resp = Response::CELL;
                    }
                    // Клик вне допустимых областей — игнорируем
                    else
                    {
                        xc = -1;
                        yc = -1;
                    }
                    break;
                case SDL_WINDOWEVENT:
                    // Обработка изменения размера окна
                    if (windowEvent.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
                    {
                        board->reset_window_size();
                        break;
                    }
                }
                if (resp != Response::OK)
                    break;
            }
        }
        // Возвращаем результат: тип действия и координаты клетки (если выбрана клетка)
        return { resp, xc, yc };
    }

    // Ожидание действия пользователя (например, после окончания партии)
    Response wait() const
    {
        SDL_Event windowEvent;
        Response resp = Response::OK;
        while (true)
        {
            if (SDL_PollEvent(&windowEvent))
            {
                switch (windowEvent.type)
                {
                case SDL_QUIT:
                    // Пользователь закрыл окно — выход из игры
                    resp = Response::QUIT;
                    break;
                case SDL_WINDOWEVENT_SIZE_CHANGED:
                    // Изменение размера окна
                    board->reset_window_size();
                    break;
                case SDL_MOUSEBUTTONDOWN: {
                    // Клик мыши — проверяем, был ли клик по области "повтор"
                    int x = windowEvent.motion.x;
                    int y = windowEvent.motion.y;
                    int xc = int(y / (board->H / 10) - 1);
                    int yc = int(x / (board->W / 10) - 1);
                    if (xc == -1 && yc == 8)
                        resp = Response::REPLAY;
                }
                                        break;
                }
                if (resp != Response::OK)
                    break;
            }
        }
        // Возвращаем тип действия пользователя
        return resp;
    }

private:
    Board* board; // Указатель на игровое поле для получения размеров и истории ходов
};
