#pragma once
#include <chrono>
#include <thread>
#include "../Models/Project_path.h"
#include "Board.h"
#include "Config.h"
#include "Hand.h"
#include "Logic.h"

class Game
{
public:
    Game() : board(config("WindowSize", "Width"), config("WindowSize", "Hight")), hand(&board), logic(&board, &config)
    {
        ofstream fout(project_path + "log.txt", ios_base::trunc);
        fout.close();
    }

    // to start checkers
    int play()
    {
        // Record the start time of the game
        auto start = chrono::steady_clock::now();

        // If replay mode is selected, recreate logic and reload config
        if (is_replay)
        {
            logic = Logic(&board, &config);
            config.reload();
            board.redraw();
        }
        else
        {
            // Otherwise, normal board drawing start
            board.start_draw();
        }
        is_replay = false;

        int turn_num = -1;
        bool is_quit = false;
        // Get the maximum number of turns from settings
        const int Max_turns = config("Game", "MaxNumTurns");
        // Main game loop
        while (++turn_num < Max_turns)
        {
            beat_series = 0;
            // Find all possible moves for the current player
            logic.find_turns(turn_num % 2);
            if (logic.turns.empty())
                break;
            // Set bot search depth from settings
            logic.Max_depth = config("Bot", string((turn_num % 2) ? "Black" : "White") + string("BotLevel"));
            // If the current player is a human
            if (!config("Bot", string("Is") + string((turn_num % 2) ? "Black" : "White") + string("Bot")))
            {
                auto resp = player_turn(turn_num % 2);
                // Handle exit, replay, or undo
                if (resp == Response::QUIT)
                {
                    is_quit = true;
                    break;
                }
                else if (resp == Response::REPLAY)
                {
                    is_replay = true;
                    break;
                }
                else if (resp == Response::BACK)
                {
                    // Undo moves if necessary
                    if (config("Bot", string("Is") + string((1 - turn_num % 2) ? "Black" : "White") + string("Bot")) &&
                        !beat_series && board.history_mtx.size() > 2)
                    {
                        board.rollback();
                        --turn_num;
                    }
                    if (!beat_series)
                        --turn_num;

                    board.rollback();
                    --turn_num;
                    beat_series = 0;
                }
            }
            else
                // If the current player is a bot, make the bot's move
                bot_turn(turn_num % 2);
        }
        // Record the end time of the game
        auto end = chrono::steady_clock::now();
        // Write game time to log
        ofstream fout(project_path + "log.txt", ios_base::app);
        fout << "Game time: " << (int)chrono::duration<double, milli>(end - start).count() << " millisec\n";
        fout.close();

        // If replay was selected — restart the game
        if (is_replay)
            return play();
        // If exit — return 0
        if (is_quit)
            return 0;
        int res = 2;
        // Determine the result of the game
        if (turn_num == Max_turns)
        {
            res = 0; // draw by move limit
        }
        else if (turn_num % 2)
        {
            res = 1; // black wins
        }
        board.show_final(res);
        // Wait for user action after the game ends
        auto resp = hand.wait();
        if (resp == Response::REPLAY)
        {
            is_replay = true;
            return play();
        }
        return res;
    }

private:
    void bot_turn(const bool color)
    {
        // Record the start time of the bot's move
        auto start = chrono::steady_clock::now();

        // Get bot delay from settings
        auto delay_ms = config("Bot", "BotDelayMS");
        // Start a separate thread to simulate delay before move
        thread th(SDL_Delay, delay_ms);
        // Get the best move(s) for the bot using game logic
        auto turns = logic.find_best_turns(color);
        th.join();
        bool is_first = true;
        // Execute all moves in the chain (e.g., capture series)
        for (auto turn : turns)
        {
            if (!is_first)
            {
                // Delay between consecutive moves in a series
                SDL_Delay(delay_ms);
            }
            is_first = false;
            // Increase capture series counter if opponent was captured
            beat_series += (turn.xb != -1);
            // Make the move on the board
            board.move_piece(turn, beat_series);
        }

        // Record the end time of the bot's move
        auto end = chrono::steady_clock::now();
        // Write bot move time to log
        ofstream fout(project_path + "log.txt", ios_base::app);
        fout << "Bot turn time: " << (int)chrono::duration<double, milli>(end - start).count() << " millisec\n";
        fout.close();
    }

    Response player_turn(const bool color)
    {
        // Form a list of cells from which a move is possible
        vector<pair<POS_T, POS_T>> cells;
        for (auto turn : logic.turns)
        {
            cells.emplace_back(turn.x, turn.y);
        }
        // Highlight possible cells for selection
        board.highlight_cells(cells);
        move_pos pos = { -1, -1, -1, -1 };
        POS_T x = -1, y = -1;
        // Try to make the first move
        while (true)
        {
            // Get cell selection from user
            auto resp = hand.get_cell();
            if (get<0>(resp) != Response::CELL)
                return get<0>(resp); // If not a cell — return the corresponding response (exit, undo, etc.)
            pair<POS_T, POS_T> cell{ get<1>(resp), get<2>(resp) };

            bool is_correct = false;
            // Check if the selected cell is correct for starting a move
            for (auto turn : logic.turns)
            {
                if (turn.x == cell.first && turn.y == cell.second)
                {
                    is_correct = true;
                    break;
                }
                // Check if a full move (from-to) is selected immediately
                if (turn == move_pos{ x, y, cell.first, cell.second })
                {
                    pos = turn;
                    break;
                }
            }
            if (pos.x != -1)
                break; // If a full move is selected — exit the loop
            if (!is_correct)
            {
                // If incorrect — reset selection and repeat input
                if (x != -1)
                {
                    board.clear_active();
                    board.clear_highlight();
                    board.highlight_cells(cells);
                }
                x = -1;
                y = -1;
                continue;
            }
            // If a correct cell is selected — highlight possible moves from it
            x = cell.first;
            y = cell.second;
            board.clear_highlight();
            board.set_active(x, y);
            vector<pair<POS_T, POS_T>> cells2;
            for (auto turn : logic.turns)
            {
                if (turn.x == x && turn.y == y)
                {
                    cells2.emplace_back(turn.x2, turn.y2);
                }
            }
            board.highlight_cells(cells2);
        }
        // Remove selection and make the move
        board.clear_highlight();
        board.clear_active();
        board.move_piece(pos, pos.xb != -1);
        if (pos.xb == -1)
            return Response::OK; // If there was no capture — move is finished

        // If there was a capture — continue the capture series while possible
        beat_series = 1;
        while (true)
        {
            logic.find_turns(pos.x2, pos.y2);
            if (!logic.have_beats)
                break;

            // Highlight possible cells for the next capture
            vector<pair<POS_T, POS_T>> cells;
            for (auto turn : logic.turns)
            {
                cells.emplace_back(turn.x2, turn.y2);
            }
            board.highlight_cells(cells);
            board.set_active(pos.x2, pos.y2);
            // Wait for the next capture selection
            while (true)
            {
                auto resp = hand.get_cell();
                if (get<0>(resp) != Response::CELL)
                    return get<0>(resp);
                pair<POS_T, POS_T> cell{ get<1>(resp), get<2>(resp) };

                bool is_correct = false;
                for (auto turn : logic.turns)
                {
                    if (turn.x2 == cell.first && turn.y2 == cell.second)
                    {
                        is_correct = true;
                        pos = turn;
                        break;
                    }
                }
                if (!is_correct)
                    continue;

                board.clear_highlight();
                board.clear_active();
                beat_series += 1;
                board.move_piece(pos, beat_series);
                break;
            }
        }

        return Response::OK; // Player's move is finished
    }

private:
    Config config;
    Board board;
    Hand hand;
    Logic logic;
    int beat_series;
    bool is_replay = false;
};
