// launcher.c - A simple game launcher for tread.h applications.
//              This launcher displays a menu with ASCII art and allows the user
//              to select and launch other compiled tread.h games.

#include "../../tread.h"

// --- Game Configuration ---
#define LAUNCHER_WIDTH  80
#define LAUNCHER_HEIGHT 25
#define FPS             60

// --- Menu Options ---
typedef enum {
  MENU_SNAKE = 0,
  MENU_PACMAN,
  MENU_SELECTOR,
  MENU_EXIT,
  NUM_MENU_ITEMS
} MenuItem;

const char* menu_items[NUM_MENU_ITEMS] = {
  "Play Snake",
  "Play Pac-Man",
  "View 3D Selector",
  "Exit Launcher"
};

// --- Game State Variables ---
static int selected_item = 0;

// --- Function Prototypes ---
void InitLauncher();
void UpdateLauncher();
void DrawLauncher();
void DrawLogo(int offset_x, int offset_y);
void LaunchGame(MenuItem game);

int main() {
  InitLauncher();

  // Main launcher loop
  while (!TR_WindowShouldClose()) {
    UpdateLauncher();
    DrawLauncher();
  }

  // De-Initialization
  TR_CloseWindow();
  return 0;
}

// --- Launcher Functions ---

void InitLauncher() {
  TR_InitWindow(LAUNCHER_WIDTH, LAUNCHER_HEIGHT, "tread.h - Game Launcher");
  TR_SetTargetFPS(FPS);
}

void UpdateLauncher() {
  int key = TR_GetKeyPressed();

  if (key != 0) {
    if (key == TR_KEY_UP) {
      selected_item--;
      if (selected_item < 0) selected_item = NUM_MENU_ITEMS - 1;
    } else if (key == TR_KEY_DOWN) {
      selected_item++;
      if (selected_item >= NUM_MENU_ITEMS) selected_item = 0;
    } else if (key == 13) { // Enter key
      LaunchGame((MenuItem)selected_item);
    } else if (key == 'q' || key == 27) { // Q or ESC
      // Set a special key to signal window should close
      // This relies on TR_WindowShouldClose checking for 'q' or ESC
      // which the provided tread.h does.
      TR_CloseWindow(); // Explicitly close window to exit launcher
      exit(0); // Force exit the launcher application
    }
  }
}

void DrawLauncher() {
  TR_BeginDrawing();
  TR_ClearBackground(TREADGRAY); // A nice dark background

  int screen_width = TR_GetScreenWidth();
  int screen_height = TR_GetScreenHeight();

  // Calculate center offsets for content
  int center_x = screen_width / 2;
  int center_y = screen_height / 2;

  // Draw Title
  const char* title = "TREAD.H GAME LAUNCHER";
  TR_DrawText(title, center_x - (int)(strlen(title) / 2), 2, 20, RAYWHITE, BLANK);

  // Draw the custom ASCII logo
  DrawLogo(center_x - 10, 5); // Position the logo

  // Draw Menu Items
  int menu_start_y = center_y + 2; // Start drawing menu below logo
  for (int i = 0; i < NUM_MENU_ITEMS; i++) {
    Color text_color = LIGHTGRAY;
    Color bg_color = BLANK;

    if (i == selected_item) {
      text_color = YELLOW; // Highlight selected item
      bg_color = DARKGRAY; // Add a background to selected item
    }
    TR_DrawText(menu_items[i], center_x - (int)(strlen(menu_items[i]) / 2), menu_start_y + i * 2, 10, text_color, bg_color);
  }

  // Draw Instructions
  const char* instructions = "Use ARROWS to navigate, ENTER to select, Q/ESC to quit";
  TR_DrawText(instructions, center_x - (int)(strlen(instructions) / 2), screen_height - 2, 10, GRAY, BLANK);

  TR_EndDrawing();
}

// Draws the custom ASCII art logo
void DrawLogo(int offset_x, int offset_y) {
  // The "#" character with a cut-off top right corner and a cog
  // Using universally supported ASCII characters.

  // Main block (large character, possibly multiple characters to form a block)
  // Using '#' to simulate a block character
  TR_DrawText("####################", offset_x,   offset_y,   10, WHITE, BLANK);
  TR_DrawText("####################", offset_x,   offset_y + 1, 10, WHITE, BLANK);
  TR_DrawText("####################", offset_x,   offset_y + 2, 10, WHITE, BLANK);
  TR_DrawText("####################", offset_x,   offset_y + 3, 10, WHITE, BLANK);
  TR_DrawText("####################", offset_x,   offset_y + 4, 10, WHITE, BLANK);

  // Simulate the cut-off corner (top right)
  TR_DrawText("  ", offset_x + 18, offset_y,   10, BLANK, TREADGRAY); // Overwrite with background
  TR_DrawText(" ",  offset_x + 19, offset_y + 1, 10, BLANK, TREADGRAY); // Overwrite with background

  // The diagonal "break" line (using backslash)
  TR_DrawText("\\", offset_x + 17, offset_y + 1, 10, LIGHTGRAY, BLANK); // Diagonal character
  TR_DrawText(" \\", offset_x + 18, offset_y + 2, 10, LIGHTGRAY, BLANK);
  TR_DrawText("  \\", offset_x + 19, offset_y + 3, 10, LIGHTGRAY, BLANK);

  // ASCII Cog (simple representation using 'O' and other basic characters)
  TR_DrawText("  _ _  ", offset_x + 15, offset_y + 4, 10, DARKGRAY, BLANK);
  TR_DrawText(" /   \\ ", offset_x + 15, offset_y + 5, 10, DARKGRAY, BLANK);
  TR_DrawText("|  O  |", offset_x + 15, offset_y + 6, 10, WHITE, BLANK); // Using 'O' for the cog symbol
  TR_DrawText(" \\_ _/ ", offset_x + 15, offset_y + 7, 10, DARKGRAY, BLANK);
  TR_DrawText("   v   ", offset_x + 15, offset_y + 8, 10, DARKGRAY, BLANK); // Small pointer from cog
}


// Launches the selected game as a separate process
void LaunchGame(MenuItem game) {
  char command[256];
  const char* game_exec_name = NULL;

  switch (game) {
    case MENU_SNAKE:
      game_exec_name = "snake";
      break;
    case MENU_PACMAN:
      game_exec_name = "pacman";
      break;
    case MENU_SELECTOR:
      game_exec_name = "selector";
      break;
    case MENU_EXIT:
      TR_CloseWindow(); // Close launcher window
      exit(0);      // Exit launcher application
    default:
      // This case should not be reached with normal menu navigation,
      // as NUM_MENU_ITEMS is a count, not a selectable item.
      // However, adding a default case handles the compiler warning.
      fprintf(stderr, "Warning: Attempted to launch invalid menu item: %d\n", game);
      break;
  }

  if (game_exec_name != NULL) {
    // Close the launcher's window before launching the game
    TR_CloseWindow();

    // Construct the command based on OS
    #ifdef _WIN32
      sprintf(command, "%s.exe", game_exec_name);
    #else
      sprintf(command, "./%s", game_exec_name);
    #endif

    // Execute the game
    int result = system(command);
    if (result == -1) {
      // Handle error if system call fails
      fprintf(stderr, "Error launching game: %s\n", command);
    }

    // Re-initialize the launcher window after the game exits
    // This clears the screen and restores terminal settings for the launcher.
    InitLauncher();
  }
}
