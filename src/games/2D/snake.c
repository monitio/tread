#include "../../tread.h"

#include <time.h>   // For time (to seed rand)

// --- Game Configuration ---
#define MAP_WIDTH   40
#define MAP_HEIGHT  20
#define FPS         10
#define MAX_SNAKE_LENGTH (MAP_WIDTH * MAP_HEIGHT)

// --- Game Characters ---
#define WALL_CHAR   '#'
#define FOOD_CHAR   '*'
#define SNAKE_HEAD  '@'
#define SNAKE_BODY  'o'
#define EMPTY_CHAR  ' '

// --- Game Colors ---
#define WALL_COLOR   WHITE
#define FOOD_COLOR   RED
#define SNAKE_HEAD_COLOR GREEN
#define SNAKE_BODY_COLOR LIME
#define TEXT_COLOR   WHITE
#define BG_COLOR     BLACK
#define GAME_OVER_COLOR MAROON
#define WIN_COLOR    LIME

// --- Game State Structs ---
typedef struct {
  int x;
  int y;
} Segment; // Represents a part of the snake or food position

// --- Game Variables ---
Segment snake[MAX_SNAKE_LENGTH];
int snake_length;
int current_dx; // current direction x for snake
int current_dy; // current direction y for snake
Segment food;
int score;
bool game_over;

// --- Function Prototypes ---
void InitGame();
void UpdateGame();
void DrawGame();
void PlaceFoodRandomly();
bool IsCollidingWithSelf(int head_x, int head_y);

int main() {
  // Initialize random seed
  srand((unsigned int)time(NULL));

  // Initialize the game window. tread.h will use the actual terminal size.
  TR_InitWindow(MAP_WIDTH, MAP_HEIGHT + 3, "tread.h - TRSnake");

  // Get actual screen dimensions for positioning
  int actual_screen_width = TR_GetScreenWidth();
  int actual_screen_height = TR_GetScreenHeight();

  // Adjust FPS based on your monitor or preference
  TR_SetTargetFPS(FPS);

  InitGame();

  // Main game loop
  while (!TR_WindowShouldClose() && !game_over) {
    UpdateGame();
    DrawGame();
  }

  // Final draw for game over screen
  TR_BeginDrawing();
  TR_ClearBackground(BG_COLOR); // Clear with game background color

  // Center game over text
  int text_center_x = actual_screen_width / 2;
  int text_center_y = actual_screen_height / 2;

  TR_DrawText("GAME OVER!", text_center_x - (int)(strlen("GAME OVER!") / 2), text_center_y - 1, 20, GAME_OVER_COLOR, BLACK);
  TR_DrawText("Final Score:", text_center_x - (int)(strlen("Final Score:") / 2), text_center_y + 1, 10, TEXT_COLOR, BLACK);
  char score_str[20];
  sprintf(score_str, "%d", score);
  TR_DrawText(score_str, text_center_x - (int)(strlen(score_str) / 2), text_center_y + 2, 10, TEXT_COLOR, BLACK);
  TR_EndDrawing();

  // Small delay to show final screen
  #ifdef _WIN32
    Sleep(3000);
  #else
    struct timespec req = { .tv_sec = 3, .tv_nsec = 0 };
    nanosleep(&req, NULL);
  #endif

  // De-Initialization
  TR_CloseWindow();

  return 0;
}

// --- Game Functions ---

void InitGame() {
  // Initialize snake
  snake_length = 1;
  snake[0].x = MAP_WIDTH / 2;
  snake[0].y = MAP_HEIGHT / 2;
  current_dx = 1; // Initial direction: right
  current_dy = 0;

  score = 0;
  game_over = false;

  PlaceFoodRandomly();
}

void UpdateGame() {
  // --- Handle Input ---
  int key = TR_GetKeyPressed();
  if (key != 0) {
    // Prevent immediate reverse
    if ((key == 'w' || key == TR_KEY_UP) && current_dy == 0) { current_dx = 0; current_dy = -1; }
    else if ((key == 's' || key == TR_KEY_DOWN) && current_dy == 0) { current_dx = 0; current_dy = 1; }
    else if ((key == 'a' || key == TR_KEY_LEFT) && current_dx == 0) { current_dx = -1; current_dy = 0; }
    else if ((key == 'd' || key == TR_KEY_RIGHT) && current_dx == 0) { current_dx = 1; current_dy = 0; }
  }

  // --- Move Snake ---
  // Shift body segments
  for (int i = snake_length - 1; i > 0; i--) {
    snake[i] = snake[i-1];
  }

  // Move head
  snake[0].x += current_dx;
  snake[0].y += current_dy;

  // --- Check for Collisions ---
  // Wall collision
  if (snake[0].x < 0 || snake[0].x >= MAP_WIDTH ||
      snake[0].y < 0 || snake[0].y >= MAP_HEIGHT) {
    game_over = true;
    return;
  }

  // Self-collision
  if (IsCollidingWithSelf(snake[0].x, snake[0].y)) {
    game_over = true;
    return;
  }

  // Food collision
  if (snake[0].x == food.x && snake[0].y == food.y) {
    score += 10;
    snake_length++;
    // Ensure snake doesn't exceed max length
    if (snake_length > MAX_SNAKE_LENGTH) {
      snake_length = MAX_SNAKE_LENGTH; // Cap length, though unlikely to be hit
    }
    PlaceFoodRandomly();
  }
}

void DrawGame() {
  TR_BeginDrawing();

  // TR_ClearBackground(BG_COLOR); // Removed: Redundant as TR_BeginDrawing already clears the buffer

  // Get actual screen dimensions for positioning
  int actual_screen_width = TR_GetScreenWidth();
  int actual_screen_height = TR_GetScreenHeight();

  // Calculate offset to center the game map
  int offset_x = (actual_screen_width - MAP_WIDTH) / 2;
  int offset_y = (actual_screen_height - (MAP_HEIGHT + 3)) / 2; // +3 for score/messages below map

  // Ensure offsets are not negative (if screen is smaller than map)
  if (offset_x < 0) offset_x = 0;
  if (offset_y < 0) offset_y = 0;

  // Draw Walls (simple border for now)
  for (int x = 0; x < MAP_WIDTH; x++) {
    TR_DrawText("#", x + offset_x, offset_y, 10, WALL_COLOR, BG_COLOR); // Top wall
    TR_DrawText("#", x + offset_x, MAP_HEIGHT - 1 + offset_y, 10, WALL_COLOR, BG_COLOR); // Bottom wall
  }
  for (int y = 0; y < MAP_HEIGHT; y++) {
    TR_DrawText("#", offset_x, y + offset_y, 10, WALL_COLOR, BG_COLOR); // Left wall
    TR_DrawText("#", MAP_WIDTH - 1 + offset_x, y + offset_y, 10, WALL_COLOR, BG_COLOR); // Right wall
  }


  // Draw Food
  char food_char_str[2] = {FOOD_CHAR, '\0'}; // Create a null-terminated string
  TR_DrawText(food_char_str, food.x + offset_x, food.y + offset_y, 10, FOOD_COLOR, BG_COLOR);

  // Draw Snake
  for (int i = 0; i < snake_length; i++) {
    Color segment_color = (i == 0) ? SNAKE_HEAD_COLOR : SNAKE_BODY_COLOR;
    char segment_char_val = (i == 0) ? SNAKE_HEAD : SNAKE_BODY;
    char segment_char_str[2] = {segment_char_val, '\0'}; // Create a null-terminated string
    TR_DrawText(segment_char_str, snake[i].x + offset_x, snake[i].y + offset_y, 10, segment_color, BG_COLOR);
  }

  // Draw Score and Messages below the map, relative to actual screen height
  char score_text[50];
  sprintf(score_text, "SCORE: %d", score);
  TR_DrawText(score_text, offset_x + 1, actual_screen_height - 2, 10, TEXT_COLOR, BLACK); // Score text has black background

  TR_DrawText("WASD/Arrows to move, ESC/Q to quit", offset_x + 1, actual_screen_height - 1, 10, LIGHTGRAY, BLACK); // Instructions text has black background

  TR_EndDrawing();
}

// Places food randomly on the map, ensuring it doesn't overlap with the snake
void PlaceFoodRandomly() {
  bool placed = false;
  while (!placed) {
    int new_x = rand() % (MAP_WIDTH - 2) + 1; // Avoid walls
    int new_y = rand() % (MAP_HEIGHT - 2) + 1; // Avoid walls

    bool overlap_with_snake = false;
    for (int i = 0; i < snake_length; i++) {
      if (snake[i].x == new_x && snake[i].y == new_y) {
        overlap_with_snake = true;
        break;
      }
      // If the snake is very long, this loop can be slow.
      // For a simple game, it's usually fine.
    }

    if (!overlap_with_snake) {
      food.x = new_x;
      food.y = new_y;
      placed = true;
    }
  }
}

// Checks if the snake's head is colliding with its own body
bool IsCollidingWithSelf(int head_x, int head_y) {
  for (int i = 1; i < snake_length; i++) { // Start from 1 to ignore head itself
    if (head_x == snake[i].x && head_y == snake[i].y) {
      return true;
    }
  }
  return false;
}
