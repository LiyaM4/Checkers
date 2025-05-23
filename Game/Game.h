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
        // Засекаем время начала партии
        auto start = chrono::steady_clock::now();

        // Если выбран режим повтора, пересоздаём логику и перезагружаем конфиг
        if (is_replay)
        {
            logic = Logic(&board, &config);
            config.reload();
            board.redraw();
        }
        else
        {
            // Иначе обычный старт отрисовки доски
            board.start_draw();
        }
        is_replay = false;

        int turn_num = -1;
        bool is_quit = false;
        // Получаем максимальное количество ходов из настроек
        const int Max_turns = config("Game", "MaxNumTurns");
        // Основной игровой цикл
        while (++turn_num < Max_turns)
        {
            beat_series = 0;
            // Находим все возможные ходы для текущего игрока
            logic.find_turns(turn_num % 2);
            if (logic.turns.empty())
                break;
            // Устанавливаем глубину поиска для бота из настроек
            logic.Max_depth = config("Bot", string((turn_num % 2) ? "Black" : "White") + string("BotLevel"));
            // Если текущий игрок — человек
            if (!config("Bot", string("Is") + string((turn_num % 2) ? "Black" : "White") + string("Bot")))
            {
                auto resp = player_turn(turn_num % 2);
                // Обработка выхода, повтора или отката хода
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
                    // Откат ходов при необходимости
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
                // Если текущий игрок — бот, выполняем ход бота
                bot_turn(turn_num % 2);
        }
        // Засекаем время окончания партии
        auto end = chrono::steady_clock::now();
        // Записываем время игры в лог
        ofstream fout(project_path + "log.txt", ios_base::app);
        fout << "Game time: " << (int)chrono::duration<double, milli>(end - start).count() << " millisec\n";
        fout.close();

        // Если был выбран повтор — запускаем игру заново
        if (is_replay)
            return play();
        // Если был выход — возвращаем 0
        if (is_quit)
            return 0;
        int res = 2;
        // Определяем результат партии
        if (turn_num == Max_turns)
        {
            res = 0; // ничья по лимиту ходов
        }
        else if (turn_num % 2)
        {
            res = 1; // победа чёрных
        }
        board.show_final(res);
        // Ожидаем действия пользователя после окончания партии
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
        // Засекаем время начала хода бота
        auto start = chrono::steady_clock::now();

        // Получаем задержку для бота из настроек
        auto delay_ms = config("Bot", "BotDelayMS");
        // Запускаем отдельный поток для имитации задержки перед ходом
        thread th(SDL_Delay, delay_ms);
        // Получаем лучший ход(ы) для бота с помощью логики игры
        auto turns = logic.find_best_turns(color);
        th.join();
        bool is_first = true;
        // Выполняем все ходы из цепочки (например, серия взятий)
        for (auto turn : turns)
        {
            if (!is_first)
            {
                // Задержка между последовательными ходами в серии
                SDL_Delay(delay_ms);
            }
            is_first = false;
            // Увеличиваем счетчик серии взятий, если был побит противник
            beat_series += (turn.xb != -1);
            // Делаем ход на доске
            board.move_piece(turn, beat_series);
        }

        // Засекаем время окончания хода бота
        auto end = chrono::steady_clock::now();
        // Записываем время хода бота в лог
        ofstream fout(project_path + "log.txt", ios_base::app);
        fout << "Bot turn time: " << (int)chrono::duration<double, milli>(end - start).count() << " millisec\n";
        fout.close();
    }

    Response player_turn(const bool color)
    {
        // Формируем список клеток, с которых возможен ход
        vector<pair<POS_T, POS_T>> cells;
        for (auto turn : logic.turns)
        {
            cells.emplace_back(turn.x, turn.y);
        }
        // Подсвечиваем возможные клетки для выбора
        board.highlight_cells(cells);
        move_pos pos = { -1, -1, -1, -1 };
        POS_T x = -1, y = -1;
        // Попытка сделать первый ход
        while (true)
        {
            // Получаем выбор клетки от пользователя
            auto resp = hand.get_cell();
            if (get<0>(resp) != Response::CELL)
                return get<0>(resp); // Если не клетка — возвращаем соответствующий ответ (выход, откат и т.д.)
            pair<POS_T, POS_T> cell{ get<1>(resp), get<2>(resp) };

            bool is_correct = false;
            // Проверяем, выбрана ли корректная клетка для начала хода
            for (auto turn : logic.turns)
            {
                if (turn.x == cell.first && turn.y == cell.second)
                {
                    is_correct = true;
                    break;
                }
                // Проверяем, выбран ли сразу полный ход (откуда-куда)
                if (turn == move_pos{ x, y, cell.first, cell.second })
                {
                    pos = turn;
                    break;
                }
            }
            if (pos.x != -1)
                break; // Если выбран полный ход — выходим из цикла
            if (!is_correct)
            {
                // Если некорректно — сбрасываем выделение и повторяем ввод
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
            // Если выбрана корректная клетка — подсвечиваем возможные ходы с неё
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
        // Снимаем выделение и выполняем ход
        board.clear_highlight();
        board.clear_active();
        board.move_piece(pos, pos.xb != -1);
        if (pos.xb == -1)
            return Response::OK; // Если не было взятия — ход завершён

        // Если было взятие — продолжаем серию взятий, пока возможно
        beat_series = 1;
        while (true)
        {
            logic.find_turns(pos.x2, pos.y2);
            if (!logic.have_beats)
                break;

            // Подсвечиваем возможные клетки для следующего взятия
            vector<pair<POS_T, POS_T>> cells;
            for (auto turn : logic.turns)
            {
                cells.emplace_back(turn.x2, turn.y2);
            }
            board.highlight_cells(cells);
            board.set_active(pos.x2, pos.y2);
            // Ожидаем выбор следующего взятия
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

        return Response::OK; // Ход игрока завершён
    }

  private:
      Config config;
      Board board;
      Hand hand;
      Logic logic;
      int beat_series;
      bool is_replay = false;
};
