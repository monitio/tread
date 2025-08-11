#include "../../tread.h"

int main() {
  // Initialization
  //--------------------------------------------------------------------------------------
  const int screenWidth = 80;
  const int screenHeight = 25;
  TR_InitWindow(screenWidth, screenHeight, "tread.h - tgame");
  TR_SetTargetFPS(60); // Try to run at 60 FPS

  int playerX = screenWidth / 2;
  int playerY = screenHeight / 2;

  // Main game loop
  while (!TR_WindowShouldClose()) {
    // Update
    //----------------------------------------------------------------------------------
    // WASD:
    if (TR_IsKeyPressed('w')) playerY--;
    if (TR_IsKeyPressed('s')) playerY++;
    if (TR_IsKeyPressed('a')) playerX--;
    if (TR_IsKeyPressed('d')) playerX++;

    // Arrow keys:
    if (TR_IsKeyPressed(TR_KEY_UP)) playerY--;
    if (TR_IsKeyPressed(TR_KEY_DOWN)) playerY++;
    if (TR_IsKeyPressed(TR_KEY_LEFT)) playerX--;
    if (TR_IsKeyPressed(TR_KEY_RIGHT)) playerX++;


    // Keep player within bounds
    if (playerX < 0) playerX = 0;
    if (playerX >= screenWidth - 1) playerX = screenWidth - 1; // -1 for a 1-char player
    if (playerY < 0) playerY = 0;
    if (playerY >= screenHeight - 1) playerY = screenHeight - 1;

    // Draw
    //----------------------------------------------------------------------------------
    TR_BeginDrawing();

    TR_ClearBackground(DARKBLUE); // Clear the screen with a dark blue background

    // TR_DrawRectangle now expects a foreground and background color
    TR_DrawRectangle(10, 5, 20, 10, GREEN, GREEN); // Filled rectangle, fg and bg are GREEN
    // TR_DrawRectangleLines now expects a foreground and background color
    TR_DrawRectangleLines(50, 15, 15, 5, RED, BLACK); // Red border, BLACK background

    // TR_DrawText now expects a foreground and background color
    TR_DrawText("Hello Terminal!", 1, 1, 20, YELLOW, BLACK); // Yellow text, BLACK background
    TR_DrawText("Use WASD/Arrows to move, ESC/Q to quit", 1, 3, 10, LIGHTGRAY, BLACK); // Light gray text, BLACK background

    // TR_DrawText for player character
    TR_DrawText("@", playerX, playerY, 10, WHITE, BLACK); // White player, BLACK background

    TR_EndDrawing();
    //----------------------------------------------------------------------------------
  }

  // De-Initialization
  //--------------------------------------------------------------------------------------
  TR_CloseWindow();

  return 0;
}
