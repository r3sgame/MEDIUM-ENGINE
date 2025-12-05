#include "raylib.h"
#include "structures.h" // Includes Box2D header and unit conversion macros
#include <string>

int main() {
    // Initialization
    GameInstance game("title_screen");

    // Main game loop
    while (!WindowShouldClose()) {
        game.Update();
    }

    // De-Initialization
    CloseWindow();
    return 0;
}