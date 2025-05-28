#pragma once
#include <tuple>

#include "../Models/Move.h"
#include "../Models/Response.h"
#include "Board.h"

// The Hand class is responsible for handling user input (mouse, window)
class Hand
{
public:
    Hand(Board* board) : board(board)
    {
    }

    // Get the cell selected by the user or a command (undo, exit, replay)
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
                    // User closed the window — exit the game
                    resp = Response::QUIT;
                    break;
                case SDL_MOUSEBUTTONDOWN:
                    // Handle mouse click: calculate cell coordinates
                    x = windowEvent.motion.x;
                    y = windowEvent.motion.y;
                    xc = int(y / (board->H / 10) - 1);
                    yc = int(x / (board->W / 10) - 1);
                    // If click on "back" area — undo move
                    if (xc == -1 && yc == -1 && board->history_mtx.size() > 1)
                    {
                        resp = Response::BACK;
                    }
                    // If click on "replay" area — replay the game
                    else if (xc == -1 && yc == 8)
                    {
                        resp = Response::REPLAY;
                    }
                    // If click on a game cell — return cell coordinates
                    else if (xc >= 0 && xc < 8 && yc >= 0 && yc < 8)
                    {
                        resp = Response::CELL;
                    }
                    // Click outside valid areas — ignore
                    else
                    {
                        xc = -1;
                        yc = -1;
                    }
                    break;
                case SDL_WINDOWEVENT:
                    // Handle window resize event
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
        // Return result: action type and cell coordinates (if a cell was selected)
        return { resp, xc, yc };
    }

    // Wait for user action (e.g., after the game ends)
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
                    // User closed the window — exit the game
                    resp = Response::QUIT;
                    break;
                case SDL_WINDOWEVENT_SIZE_CHANGED:
                    // Window resize event
                    board->reset_window_size();
                    break;
                case SDL_MOUSEBUTTONDOWN: {
                    // Mouse click — check if click was on "replay" area
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
        // Return the type of user action
        return resp;
    }

private:
    Board* board; // Pointer to the game board for getting sizes and move history
};
