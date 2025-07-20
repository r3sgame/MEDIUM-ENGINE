#include "raylib.h"
#include "structures.h" // Includes Box2D header and unit conversion macros
#include <string>

// Global Box2D world ID (Changed from b2World* to b2WorldId)
b2WorldId world;

int main() {
    // Initialization

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "MEDIUM");
    ToggleFullscreen();

    SetTargetFPS(60);

    b2WorldDef worldDef = b2DefaultWorldDef(); // Get default world definition
    worldDef.gravity = {0.0f, 9.81f};    // Set the gravity in the definition (b2Vec2 can be initialized directly)
    world = b2CreateWorld(&worldDef);          // Pass the address of the world definition

    // Player parameters
    float width = 15.0f;
    float height = 30.0f;
    float moveSpeed = 190.0f;
    float jumpForce = 450.0f;
    int maxHealth = 100;
    int maxMedium = 100;
    bool isPlayer = true;
    
    Character player(world, width, height, moveSpeed, jumpForce, maxHealth, maxMedium, isPlayer);

    Level testLevel(world, player);
    testLevel.LoadLevel((Vector2){25, 75});
    
    Camera2D camera = { 0 };

    camera.target = player.position;
    camera.offset = { (float)SCREEN_WIDTH / 2, (float)SCREEN_HEIGHT / 2 };
    camera.rotation = 0.0f;
    camera.zoom = 4.0f;

    // Main game loop
    while (!WindowShouldClose()) {
        // Drawing
        BeginDrawing();
            BeginMode2D(camera);
            ClearBackground(SKYBLUE);
            testLevel.Update();
            camera.target = (Vector2){ camera.target.x + CAM_X_INTERPOLATION * (player.position.x - camera.target.x), camera.target.y + CAM_Y_INTERPOLATION * (player.position.y - camera.target.y) };
            
            // Get the current FPS
            int fps = GetFPS();
            // Convert the integer FPS to a string
            std::string fpsText = "FPS: " + std::to_string(fps);

            // Draw the FPS text on the screen
            // Arguments: text, posX, posY, fontSize, color
            DrawText(fpsText.c_str(), 10, 10, 20, DARKGRAY); // Top-left corner, font size 20, dark gray color

            EndMode2D();
        EndDrawing();
    }

    // De-Initialization
    b2DestroyWorld(world); // Destroy the Box2D world using b2DestroyWorld
    CloseWindow();

    return 0;
}