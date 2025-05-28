#pragma once

enum class Response
{
    OK,      // Move successfully made, continue the game
    BACK,    // Undo the previous move
    REPLAY,  // Replay the game from the beginning
    QUIT,    // Exit the game
    CELL     // A cell on the board was selected (used for input handling)
};
