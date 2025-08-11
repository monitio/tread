#include "../../tread.h"

#include <time.h>   // For time (to seed rand)

// --- Game Configuration ---
// MAP_WIDTH and MAP_HEIGHT define the logical size of the game board.
// The game will be drawn centered within the actual terminal size.
#define MAP_WIDTH   31
#define MAP_HEIGHT 21
#define FPS    10

// --- Game Characters ---
#define WALL_CHAR   '#'
#define PELLET_CHAR   '.'
#define PACMAN_CHAR   '@'
#define GHOST_CHAR   'M'
#define EMPTY_CHAR   ' '

// --- Game Colors ---
#define WALL_COLOR   BLUE
#define PELLET_COLOR WHITE
#define PACMAN_COLOR YELLOW
#define GHOST_COLOR   RED
#define TEXT_COLOR   WHITE
#define BG_COLOR   BLACK
#define GAME_OVER_COLOR MAROON
#define WIN_COLOR   LIME

// --- Game State Structs ---
typedef struct {
  int x;
  int y;
  int dx; // direction x
  int dy; // direction y
} Entity;

// --- Game Variables ---
char game_map[MAP_HEIGHT][MAP_WIDTH + 1] = { // +1 for null terminator
  "###############################",
  "#.............................#",
  "#.###.###.###.###.###.###.###.#",
  "#.#...#.#.#.#.#.#.#.#.#.#...#.#",
  "#.###.###.###.###.###.###.###.#",
  "#.............................#",
  "#.###.###.#.###.#.###.#.###.###",
  "#.#...#.#.#.#.#.#.#.#.#.#...#.#",
  "#.###.###.#.###.#.###.#.###.###",
  "#.........#.....#.............#",
  "#.###.###.###.#.#.###.###.###.#",
  "#.#...#.#.#.#.#.#.#.#.#.#...#.#",
  "#.###.###.###.###.###.###.###.#",
  "#.............................#",
  "#.###.###.###.###.###.###.###.#",
  "#.#...#.#.#.#.#.#.#.#.#.#...#.#",
  "#.###.###.###.###.###.###.###.#",
  "#.............................#",
  "#.###.###.###.###.###.###.###.#",
  "#.............................#",
  "###############################"
};

Entity pacman;
Entity ghosts[2]; // Two ghosts for simplicity
int score = 0;
int total_pellets = 0;
bool game_over = false;
bool game_won = false;

// --- Function Prototypes ---
void InitGame();
void UpdateGame();
void DrawGame();
bool IsColliding(Entity e1, Entity e2);
void GetRandomDirection(int* dx, int* dy);

int main() {
  // Initialize random seed
  srand((unsigned int)time(NULL));

  // Initialize the game window. tread.h will use the actual terminal size.
  TR_InitWindow(MAP_WIDTH, MAP_HEIGHT + 3, "tread.h - TRPac-Man");

  // Get actual screen dimensions for positioning
  int actual_screen_width = TR_GetScreenWidth();
  int actual_screen_height = TR_GetScreenHeight();

  // Adjust FPS based on your monitor or preference
  TR_SetTargetFPS(FPS);

  InitGame();

  // Main game loop
  while (!TR_WindowShouldClose() && !game_over && !game_won) {
    UpdateGame();
    DrawGame();
  }

  // Final draw for game over/win screen
  TR_BeginDrawing();
  TR_ClearBackground(BG_COLOR); // Clear with game background color

  // Center game over/win text
  int text_center_x = actual_screen_width / 2;
  int text_center_y = actual_screen_height / 2;

  if (game_won) {
    TR_DrawText("YOU WIN!", text_center_x - (int)(strlen("YOU WIN!") / 2), text_center_y - 1, 20, WIN_COLOR, BLACK);
    TR_DrawText("Score:", text_center_x - (int)(strlen("Score:") / 2), text_center_y + 1, 10, TEXT_COLOR, BLACK);
    char score_str[20];
    sprintf(score_str, "%d", score);
    TR_DrawText(score_str, text_center_x - (int)(strlen(score_str) / 2), text_center_y + 2, 10, TEXT_COLOR, BLACK);
  } else {
    TR_DrawText("GAME OVER!", text_center_x - (int)(strlen("GAME OVER!") / 2), text_center_y - 1, 20, GAME_OVER_COLOR, BLACK);
    TR_DrawText("Final Score:", text_center_x - (int)(strlen("Final Score:") / 2), text_center_y + 1, 10, TEXT_COLOR, BLACK);
    char score_str[20];
    sprintf(score_str, "%d", score);
    TR_DrawText(score_str, text_center_x - (int)(strlen(score_str) / 2), text_center_y + 2, 10, TEXT_COLOR, BLACK);
  }
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
  // Count total pellets and set initial positions
  total_pellets = 0; // Reset for new game
  for (int y = 0; y < MAP_HEIGHT; y++) {
    for (int x = 0; x < MAP_WIDTH; x++) {
      if (game_map[y][x] == PELLET_CHAR) {
        total_pellets++;
      }
    }
  }

  // Set Pac-Man's initial position
  pacman.x = MAP_WIDTH / 2;
  pacman.y = MAP_HEIGHT / 2;
  pacman.dx = 0;
  pacman.dy = 0;

  // Set Ghost initial positions
  ghosts[0].x = 1;
  ghosts[0].y = 1;
  GetRandomDirection(&ghosts[0].dx, &ghosts[0].dy);

  ghosts[1].x = MAP_WIDTH - 2;
  ghosts[1].y = MAP_HEIGHT - 2;
  GetRandomDirection(&ghosts[1].dx, &ghosts[1].dy);
}

void UpdateGame() {
  // --- Handle Input ---
  int key = TR_GetKeyPressed();
  if (key != 0) {
    if (key == 'w' || key == TR_KEY_UP) { pacman.dx = 0; pacman.dy = -1; }
    else if (key == 's' || key == TR_KEY_DOWN) { pacman.dx = 0; pacman.dy = 1; }
    else if (key == 'a' || key == TR_KEY_LEFT) { pacman.dx = -1; pacman.dy = 0; }
    else if (key == 'd' || key == TR_KEY_RIGHT) { pacman.dx = 1; pacman.dy = 0; }
  }

  // --- Move Pac-Man ---
  int nextPacmanX = pacman.x + pacman.dx;
  int nextPacmanY = pacman.y + pacman.dy;

  // Check bounds and wall collision for Pac-Man
  if (nextPacmanX >= 0 && nextPacmanX < MAP_WIDTH &&
    nextPacmanY >= 0 && nextPacmanY < MAP_HEIGHT &&
    game_map[nextPacmanY][nextPacmanX] != WALL_CHAR) {
    pacman.x = nextPacmanX;
    pacman.y = nextPacmanY;

    // Check for pellet
    if (game_map[pacman.y][pacman.x] == PELLET_CHAR) {
      game_map[pacman.y][pacman.x] = EMPTY_CHAR; // Eat pellet
      score += 10;
      total_pellets--;
    }
  }

  // --- Move Ghosts ---
  for (int i = 0; i < sizeof(ghosts) / sizeof(Entity); i++) {
    int nextGhostX = ghosts[i].x + ghosts[i].dx;
    int nextGhostY = ghosts[i].y + ghosts[i].dy;

    // Simple ghost AI: change direction if hitting a wall or randomly
    if (nextGhostX < 0 || nextGhostX >= MAP_WIDTH ||
      nextGhostY < 0 || nextGhostY >= MAP_HEIGHT ||
      game_map[nextGhostY][nextGhostX] == WALL_CHAR ||
      (rand() % 10 == 0)) // 10% chance to change direction randomly
    {
      GetRandomDirection(&ghosts[i].dx, &ghosts[i].dy);
    } else {
      ghosts[i].x = nextGhostX;
      ghosts[i].y = nextGhostY;
    }
  }

  // --- Check Collisions ---
  for (int i = 0; i < sizeof(ghosts) / sizeof(Entity); i++) {
    if (IsColliding(pacman, ghosts[i])) {
      game_over = true;
      break;
    }
  }

  // --- Check Win Condition ---
  if (total_pellets <= 0) {
    game_won = true;
  }
}

void DrawGame() {
  TR_BeginDrawing();

  // Clear the background for the entire game area
  TR_ClearBackground(BG_COLOR);

  // Get actual screen dimensions for positioning
  int actual_screen_width = TR_GetScreenWidth();
  int actual_screen_height = TR_GetScreenHeight();

  // Calculate offset to center the game map
  int offset_x = (actual_screen_width - MAP_WIDTH) / 2;
  int offset_y = (actual_screen_height - (MAP_HEIGHT + 3)) / 2; // +3 for score/messages below map

  // Ensure offsets are not negative (if screen is smaller than map)
  if (offset_x < 0) offset_x = 0;
  if (offset_y < 0) offset_y = 0;

  // Draw the map
  for (int y = 0; y < MAP_HEIGHT; y++) {
    for (int x = 0; x < MAP_WIDTH; x++) {
      int draw_x = x + offset_x;
      int draw_y = y + offset_y;

      // Only draw if within actual screen bounds
      if (draw_x >= 0 && draw_x < actual_screen_width &&
        draw_y >= 0 && draw_y < actual_screen_height) {
        if (game_map[y][x] == WALL_CHAR) {
          TR_DrawText("#", draw_x, draw_y, 10, WALL_COLOR, BG_COLOR); // Walls blend with maze background
        } else if (game_map[y][x] == PELLET_CHAR) {
          TR_DrawText(".", draw_x, draw_y, 10, PELLET_COLOR, BG_COLOR); // Pellets blend with maze background
        }
        // Empty spaces are handled by ClearBackground, no need to draw ' '
      }
    }
  }

  // Draw Pac-Man (with offset)
  int pacman_draw_x = pacman.x + offset_x;
  int pacman_draw_y = pacman.y + offset_y;
  if (pacman_draw_x >= 0 && pacman_draw_x < actual_screen_width &&
    pacman_draw_y >= 0 && pacman_draw_y < actual_screen_height) {
    TR_DrawText(PACMAN_CHAR == '@' ? "@" : "P", pacman_draw_x, pacman_draw_y, 10, PACMAN_COLOR, BG_COLOR); // Pac-Man blends with maze background
  }


  // Draw Ghosts (with offset)
  for (int i = 0; i < sizeof(ghosts) / sizeof(Entity); i++) {
    int ghost_draw_x = ghosts[i].x + offset_x;
    int ghost_draw_y = ghosts[i].y + offset_y;
    if (ghost_draw_x >= 0 && ghost_draw_x < actual_screen_width &&
      ghost_draw_y >= 0 && ghost_draw_y < actual_screen_height) {
      TR_DrawText(GHOST_CHAR == 'M' ? "M" : "G", ghost_draw_x, ghost_draw_y, 10, GHOST_COLOR, BG_COLOR); // Ghosts blend with maze background
    }
  }

  // Draw Score and Messages below the map, relative to actual screen height
  char score_text[50];
  sprintf(score_text, "SCORE: %d", score);
  TR_DrawText(score_text, offset_x + 1, actual_screen_height - 2, 10, TEXT_COLOR, BLACK); // Score text has black background

  TR_DrawText("WASD/Arrows to move, ESC/Q to quit", offset_x + 1, actual_screen_height - 1, 10, LIGHTGRAY, BLACK); // Instructions text has black background

  TR_EndDrawing();
}

// Simple collision check (character overlap)
bool IsColliding(Entity e1, Entity e2) {
  return (e1.x == e2.x && e1.y == e2.y);
}

// Get a random direction (dx, dy)
void GetRandomDirection(int* dx, int* dy) {
  int dir = rand() % 4; // 0: up, 1: down, 2: left, 3: right
  switch (dir) {
    case 0: *dx = 0; *dy = -1; break; // Up
    case 1: *dx = 0; *dy = 1; break;  // Down
    case 2: *dx = -1; *dy = 0; break; // Left
    case 3: *dx = 1; *dy = 0; break;  // Right
  }
}
