#include "raylib.h"
#include "structures.h" // Includes Box2D header and unit conversion macros
#include <string>

int main() {
    // Initialization
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    SetConfigFlags(FLAG_VSYNC_HINT);

    SetTraceLogLevel(LOG_WARNING);

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "MEDIUM");
    ToggleFullscreen();
    
    std::vector<std::string> characterPointers;
    characterPointers.push_back("water");
    characterPointers.push_back("water");

    Level* testLevel = LoadLevel("village", characterPointers);

    // Main game loop
    while (!WindowShouldClose()) {
        testLevel->Update();
    }

    // De-Initialization
    CloseWindow();
    return 0;
}