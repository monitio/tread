// animator.c - A terminal-based text animation creation and export tool.
//              This program allows users to draw frame-by-frame animations
//              using characters and colors, and then export them to a simple
//              text file format.

#include "../../tread.h"

#include <ctype.h>  // For isprint()

// --- Configuration ---
#define ANIMATOR_WIDTH  80
#define ANIMATOR_HEIGHT 25
#define FPS       10 // Editor FPS (not animation playback FPS)
#define MAX_FRAMES    100 // Maximum number of animation frames
#define ANIMATION_FILE  "animation.txt" // Default file name for saving/loading

// --- Internal Data Structures ---

// Represents a single character cell with foreground and background colors
typedef struct {
  char character;
  Color fg_color;
  Color bg_color;
} AnimatorCell;

// Represents a single frame of animation
typedef struct {
  AnimatorCell* cells; // 2D grid of cells (width * height)
} AnimationFrame;

// Represents the entire animation
typedef struct {
  int width;
  int height;
  int fps; // Playback FPS for the animation
  AnimationFrame* frames;
  int frame_count;
} Animation;

// --- Global Animator State ---
static Animation g_animation;
static int g_current_frame_index = 0;
static int g_cursor_x = 0;
static int g_cursor_y = 0;
static Color g_current_fg_color = WHITE;
static Color g_current_bg_color = BLACK;
static char g_current_char = '#';

// Onion Skinning State
static int g_onion_skin_level = 0; // 0=Off, 1=Light, 2=Medium, 3=Strong

// Character Input Mode State
static bool g_waiting_for_char_input = false; // New state variable

// --- Color Palette for Cycling (matches tread.h basic colors) ---
static const Color ANIMATOR_PALETTE[] = {
  BLACK, RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, WHITE,
  LIGHTGRAY, DARKGRAY, GOLD, ORANGE, PINK, MAROON, LIME, DARKGREEN,
  SKYBLUE, DARKBLUE, PURPLE, VIOLET, DARKPURPLE, BEIGE, BROWN, DARKBROWN,
  RAYWHITE // Add more as needed from tread.h
};
static const int NUM_PALETTE_COLORS = sizeof(ANIMATOR_PALETTE) / sizeof(Color);

// --- Character Palette for Cycling ---
static const char CHAR_PALETTE[] = {
  '#', '@', 'X', 'O', '*', '+', '-', '=', ':', '.', ' ', '/', '\\', '|', 'A', 'B', 'C'
};
static const int NUM_CHAR_PALETTE = sizeof(CHAR_PALETTE) / sizeof(char);

// --- Function Prototypes ---
void InitAnimator();
void CleanupAnimator();
void UpdateAnimator();
void DrawAnimator();

// Frame management
void AddFrame();
void DeleteCurrentFrame();
void ClearCurrentFrame();
void DuplicateCurrentFrame(); // New prototype for duplicating a frame

// Animation file I/O
bool SaveAnimation(const char* filename);
bool LoadAnimation(const char* filename);

// Helper to get color index (for saving)
static inline short __tr_get_color_index(Color color) {
  for (int i = 0; i < NUM_PALETTE_COLORS; ++i) {
    // Use tread.h's __tr_colors_equal
    if (__tr_colors_equal(color, ANIMATOR_PALETTE[i])) {
      return (short)i;
    }
  }
  return 0; // Default to black index if not found
}

// Helper to get color from index (for loading)
static inline Color __tr_get_color_from_index(short index) {
  if (index >= 0 && index < NUM_PALETTE_COLORS) {
    return ANIMATOR_PALETTE[index];
  }
  return BLACK; // Default to black for invalid index
}

// Helper to dim a color for onion skinning
static inline Color __tr_dim_color(Color original_color, int level) {
  if (level == 0) return original_color; // No dimming

  // Explicitly map to a darker version from the palette for better terminal compatibility
  if (level == 1) { // Light dim
    if (__tr_colors_equal(original_color, WHITE)) return LIGHTGRAY;
    if (__tr_colors_equal(original_color, LIGHTGRAY)) return GRAY;
    if (__tr_colors_equal(original_color, YELLOW)) return GOLD;
    if (__tr_colors_equal(original_color, GREEN)) return LIME;
    if (__tr_colors_equal(original_color, BLUE)) return SKYBLUE;
    if (__tr_colors_equal(original_color, RED)) return MAROON;
    if (__tr_colors_equal(original_color, MAGENTA)) return PURPLE;
    if (__tr_colors_equal(original_color, CYAN)) return BLUE; // Cyan to a darker blue
    // Fallback for other colors
    return (Color){(unsigned char)(original_color.r * 0.7f), (unsigned char)(original_color.g * 0.7f), (unsigned char)(original_color.b * 0.7f), original_color.a};
  } else if (level == 2) { // Medium dim
    if (__tr_colors_equal(original_color, WHITE)) return GRAY;
    if (__tr_colors_equal(original_color, LIGHTGRAY)) return DARKGRAY;
    if (__tr_colors_equal(original_color, YELLOW)) return ORANGE;
    if (__tr_colors_equal(original_color, GREEN)) return DARKGREEN;
    if (__tr_colors_equal(original_color, BLUE)) return DARKBLUE;
    if (__tr_colors_equal(original_color, RED)) return DARKGRAY; // Red to dark gray
    if (__tr_colors_equal(original_color, MAGENTA)) return DARKPURPLE;
    if (__tr_colors_equal(original_color, CYAN)) return DARKBLUE;
    // Fallback for other colors
    return (Color){(unsigned char)(original_color.r * 0.5f), (unsigned char)(original_color.g * 0.5f), (unsigned char)(original_color.b * 0.5f), original_color.a};
  } else if (level == 3) { // Strong dim / Grayscale
    // Convert to grayscale for strong dimming
    unsigned char gray = (unsigned char)((original_color.r * 0.299 + original_color.g * 0.587 + original_color.b * 0.114));
    return (Color){gray, gray, gray, original_color.a};
  }

  return original_color; // Should not be reached
}


int main() {
  InitAnimator();

  // Main editor loop
  while (!TR_WindowShouldClose()) {
    UpdateAnimator();
    DrawAnimator();
  }

  CleanupAnimator();
  return 0;
}

// --- Animator Functions ---

void InitAnimator() {
  TR_InitWindow(ANIMATOR_WIDTH, ANIMATOR_HEIGHT, "tread.h - Animator");
  TR_SetTargetFPS(FPS);

  g_animation.width = ANIMATOR_WIDTH;
  g_animation.height = ANIMATOR_HEIGHT - 5; // Reserve space for UI
  g_animation.fps = 10; // Default animation playback FPS
  g_animation.frames = (AnimationFrame*)malloc(sizeof(AnimationFrame) * MAX_FRAMES);
  if (g_animation.frames == NULL) {
    fprintf(stderr, "ERROR: Failed to allocate animation frames.\n");
    exit(1);
  }
  g_animation.frame_count = 0;

  // Add initial empty frame
  AddFrame();

  // Initialize Z-buffer for 3D drawing if enabled in tread.h
  #ifdef TR_3D
  // tread.h's __tr_z_buffer is initialized in TR_InitWindow if TR_3D is defined.
  // We only need to ensure its size is appropriate for the drawing area.
  // This check ensures it's allocated if not already (e.g., if TR_3D was defined
  // but TR_InitWindow didn't allocate it due to a bug or different context).
  // However, the current tread.h initializes it based on TR_GetScreenWidth/Height
  // which are the full terminal dimensions. If the drawing area is smaller,
  // the Z-buffer might be larger than strictly needed for the drawing area.
  // For this animator, we'll rely on tread.h's Z-buffer if 3D is enabled.
  // If you plan to draw 3D objects in the animator, ensure the Z-buffer
  // is cleared appropriately per frame.
  #endif
}

void CleanupAnimator() {
  for (int i = 0; i < g_animation.frame_count; ++i) {
    free(g_animation.frames[i].cells);
  }
  free(g_animation.frames); // Free the array of frames itself
  // tread.h handles its own Z-buffer cleanup via TR_CloseWindow
  TR_CloseWindow();
}

void UpdateAnimator() {
  int key = TR_GetKeyPressed();

  // --- Handle Character Input Mode First ---
  if (g_waiting_for_char_input) {
    if (key != 0) { // A key was pressed
      bool is_banned = false;

      // Explicitly banned common non-character keys defined in tread.h
      if (key == TR_KEY_LEFT || key == TR_KEY_RIGHT || key == TR_KEY_UP || key == TR_KEY_DOWN ||
        key == TR_KEY_ENTER || key == TR_KEY_BACKSPACE || key == TR_KEY_DELETE || key == TR_KEY_ESCAPE) {
        is_banned = true;
      }
      // Check for F-keys if TR_KEY_F1 is defined and they are within the F-key range
      #ifdef TR_KEY_F1
      else if (key >= TR_KEY_F1 && key <= TR_KEY_F12) {
        is_banned = true;
      }
      #endif
      // Additional check for control characters (ASCII 0-31), if tread.h passes them raw
      // This covers many keys that might not have a TR_KEY_ mapping but produce non-printable output.
      else if (key >= 0 && key < 32) {
        is_banned = true;
      }

      // Note: Keys like Shift, Ctrl, Alt, Caps Lock, Insert, Home, End, Page Up/Down, etc.,
      // typically do not produce a 'character' returned by TR_GetKeyPressed().
      // If they did, and they were not one of the explicitly banned TR_KEY_ constants,
      // the `isprint()` check below would filter out non-visual characters.

      if (!is_banned) {
        // Check if the character is a printable ASCII character (0x20 to 0x7E)
        // This covers most standard characters, numbers, and symbols.
        if (isprint(key)) {
          g_current_char = (char)key;
          g_waiting_for_char_input = false; // Exit char input mode
        }
        // If it's not banned by explicit TR_KEY_ code or control char range,
        // but also not printable (e.g., some obscure non-ASCII key or a non-printable
        // key that somehow returns a non-zero non-printable value), we just ignore it
        // and stay in the waiting state.
      }
    }
    return; // Skip normal update logic if waiting for char input
  }

  // --- End Character Input Mode Handling ---


  // Cursor movement (only if not waiting for char input)
  if (key == TR_KEY_LEFT) g_cursor_x--;
  else if (key == TR_KEY_RIGHT) g_cursor_x++;
  else if (key == TR_KEY_UP) g_cursor_y--;
  else if (key == TR_KEY_DOWN) g_cursor_y++;

  // Clamp cursor to drawing area
  if (g_cursor_x < 0) g_cursor_x = 0;
  if (g_cursor_x >= g_animation.width) g_cursor_x = g_animation.width - 1;
  if (g_cursor_y < 0) g_cursor_y = 0;
  if (g_cursor_y >= g_animation.height) g_cursor_y = g_animation.height - 1;

  // Drawing
  if (key == 'd' || key == 'D') { // Draw
    g_animation.frames[g_current_frame_index].cells[g_cursor_y * g_animation.width + g_cursor_x] = (AnimatorCell){g_current_char, g_current_fg_color, g_current_bg_color};
  } else if (key == 'e' || key == 'E') { // Erase
    g_animation.frames[g_current_frame_index].cells[g_cursor_y * g_animation.width + g_cursor_x] = (AnimatorCell){' ', BLACK, BLACK}; // Erase with black background
  }

  // Character/Color cycling
  if (key == 'c' || key == 'C') { // Enter character selection mode
    g_waiting_for_char_input = true;
    // No need to cycle through CHAR_PALETTE anymore for this mode
  } else if (key == 'f' || key == 'F') { // Cycle foreground color
    static int fg_idx = 0;
    fg_idx = (fg_idx + 1) % NUM_PALETTE_COLORS;
    g_current_fg_color = ANIMATOR_PALETTE[fg_idx];
  } else if (key == 'b' || key == 'B') { // Cycle background color
    static int bg_idx = 0;
    bg_idx = (bg_idx + 1) % NUM_PALETTE_COLORS;
    g_current_bg_color = ANIMATOR_PALETTE[bg_idx];
  }

  // Frame management
  if (key == 'n' || key == 'N') { // Next frame
    if (g_current_frame_index < g_animation.frame_count - 1) {
      g_current_frame_index++;
    }
  } else if (key == 'p' || key == 'P') { // Previous frame
    if (g_current_frame_index > 0) {
      g_current_frame_index--;
    }
  } else if (key == 'a' || key == 'A') { // Add frame
    AddFrame();
    g_current_frame_index = g_animation.frame_count - 1; // Go to new frame
  } else if (key == 'x' || key == 'X') { // Delete frame
    DeleteCurrentFrame();
    if (g_animation.frame_count == 0) { // If all frames deleted, add a new one
      AddFrame();
    }
    if (g_current_frame_index >= g_animation.frame_count) {
      g_current_frame_index = g_animation.frame_count - 1;
    }
  } else if (key == 'k' || key == 'K') { // Clear current frame
    ClearCurrentFrame();
  } else if (key == 'u' || key == 'U') { // Duplicate current frame
    DuplicateCurrentFrame();
  }

  // Onion Skinning Toggle
  if (key == 'o' || key == 'O') {
    g_onion_skin_level = (g_onion_skin_level + 1) % 4; // Cycle 0, 1, 2, 3
  }

  // Animation playback
  if (key == 'v' || key == 'V') { // Preview animation
    int original_fps = FPS; // Save editor FPS
    TR_SetTargetFPS(g_animation.fps); // Set animation playback FPS

    for (int i = 0; i < g_animation.frame_count; ++i) {
      if (TR_WindowShouldClose()) break; // Allow early exit during playback
      TR_BeginDrawing();
      TR_ClearBackground(BLACK); // Clear for animation
      for (int y = 0; y < g_animation.height; ++y) {
        for (int x = 0; x < g_animation.width; ++x) {
          AnimatorCell cell = g_animation.frames[i].cells[y * g_animation.width + x];
          TR_DrawText(&cell.character, x, y, 10, cell.fg_color, cell.bg_color);
        }
      }
      TR_EndDrawing();
      // Check for any key press to stop playback
      if (TR_GetKeyPressed() != 0) break;
    }
    TR_SetTargetFPS(original_fps); // Restore editor FPS
  }

  // Save/Load
  if (key == 's' || key == 'S') {
    if (SaveAnimation(ANIMATION_FILE)) {
      // Display a message or log success
    } else {
      // Display error
    }
  } else if (key == 'l' || key == 'L') {
    if (LoadAnimation(ANIMATION_FILE)) {
      g_current_frame_index = 0; // Reset to first frame after loading
    } else {
      // Display error

    }
  }

  // Quit
  if (key == 'q' || key == 27) { // Q or ESC
    TR_CloseWindow(); // Signal to main loop to exit
  }
}

void DrawAnimator() {
  TR_BeginDrawing();
  TR_ClearBackground(TREADGRAY); // Overall background for the animator

  int screen_width = TR_GetScreenWidth();
  int screen_height = TR_GetScreenHeight();

  // Draw onion skin (previous frame) if enabled and available
  if (g_onion_skin_level > 0 && g_current_frame_index > 0) {
    AnimationFrame* prev_frame = &g_animation.frames[g_current_frame_index - 1];
    for (int y = 0; y < g_animation.height; ++y) {
      for (int x = 0; x < g_animation.width; ++x) {
        AnimatorCell cell = prev_frame->cells[y * g_animation.width + x];
        if (cell.character != ' ') { // Only draw if there's a character
          TR_DrawText(&cell.character, x, y, 10,
                __tr_dim_color(cell.fg_color, g_onion_skin_level),
                __tr_dim_color(cell.bg_color, g_onion_skin_level));
        }
      }
    }
  }

  // Draw the current animation frame
  if (g_animation.frame_count > 0) {
    AnimationFrame* current_frame = &g_animation.frames[g_current_frame_index];
    for (int y = 0; y < g_animation.height; ++y) {
      for (int x = 0; x < g_animation.width; ++x) {
        AnimatorCell cell = current_frame->cells[y * g_animation.width + x];
        TR_DrawText(&cell.character, x, y, 10, cell.fg_color, cell.bg_color);
      }
    }
  }

  // Draw cursor
  TR_DrawRectangleLines(g_cursor_x, g_cursor_y, 1, 1, YELLOW, BLANK);

  // Draw UI/Status Bar at the bottom
  int ui_start_y = g_animation.height + 1; // Below the drawing area

  TR_DrawRectangle(0, ui_start_y -1, screen_width, 1, DARKGRAY, DARKGRAY); // Separator line

  char status_text[256];
  sprintf(status_text, "Frame: %d/%d | Cursor: (%d,%d) | Char: '%c' | ",
      g_current_frame_index + 1, g_animation.frame_count,
      g_cursor_x, g_cursor_y,
      g_current_char);

  // Add color previews
  char color_preview_text[50];
  // Draw FG color preview as a solid block
  TR_DrawText(" ", strlen(status_text) + 1, ui_start_y, 10, g_current_fg_color, g_current_fg_color);
  sprintf(color_preview_text, "FG: (%d,%d,%d) ", g_current_fg_color.r, g_current_fg_color.g, g_current_fg_color.b);
  strcat(status_text, color_preview_text);

  // Draw BG color preview as a solid block
  TR_DrawText(" ", strlen(status_text) + 1, ui_start_y, 10, g_current_bg_color, g_current_bg_color);
  sprintf(color_preview_text, "BG: (%d,%d,%d)", g_current_bg_color.r, g_current_bg_color.g, g_current_bg_color.b);
  strcat(status_text, color_preview_text);

  TR_DrawText(status_text, 1, ui_start_y, 10, RAYWHITE, BLANK);

  char onion_skin_status[20];
  if (g_onion_skin_level == 0) strcpy(onion_skin_status, "Off");
  else if (g_onion_skin_level == 1) strcpy(onion_skin_status, "Light");
  else if (g_onion_skin_level == 2) strcpy(onion_skin_status, "Medium");
  else strcpy(onion_skin_status, "Strong");

  char controls_line1[256];
  sprintf(controls_line1, "Controls: ARROWS=Move, D=Draw, E=Erase, C=Char, F=FG, B=BG, O=Onion Skin (%s)", onion_skin_status);
  TR_DrawText(controls_line1, 1, ui_start_y + 1, 10, LIGHTGRAY, BLANK);
  TR_DrawText("N=Next Frame, P=Prev Frame, A=Add Frame, X=Del Frame, K=Clear Frame, U=Duplicate Frame", 1, ui_start_y + 2, 10, LIGHTGRAY, BLANK);
  TR_DrawText("V=Preview, S=Save, L=Load, Q/ESC=Quit", 1, ui_start_y + 3, 10, LIGHTGRAY, BLANK);

  // Display message when waiting for character input
  if (g_waiting_for_char_input) {
    TR_DrawText("Press any NON-BANNED key for new character...", screen_width / 2 - 20, screen_height / 2, 10, YELLOW, DARKGRAY);
  }

  TR_EndDrawing();
}

// --- Frame Management Implementations ---

void AddFrame() {
  if (g_animation.frame_count >= MAX_FRAMES) {
    fprintf(stderr, "WARNING: Maximum frames reached (%d).\n", MAX_FRAMES);
    return;
  }

  AnimationFrame new_frame;
  new_frame.cells = (AnimatorCell*)malloc(sizeof(AnimatorCell) * g_animation.width * g_animation.height);
  if (new_frame.cells == NULL) {
    fprintf(stderr, "ERROR: Failed to allocate cells for new frame.\n");
    return;
  }

  // Initialize new frame with empty (black) cells
  for (int i = 0; i < g_animation.width * g_animation.height; ++i) {
    new_frame.cells[i] = (AnimatorCell){' ', BLACK, BLACK};
  }

  g_animation.frames[g_animation.frame_count] = new_frame;
  g_animation.frame_count++;
}

void DeleteCurrentFrame() {
  if (g_animation.frame_count <= 0) return;

  free(g_animation.frames[g_current_frame_index].cells);

  // Shift remaining frames
  for (int i = g_current_frame_index; i < g_animation.frame_count - 1; ++i) {
    g_animation.frames[i] = g_animation.frames[i + 1];
  }
  g_animation.frame_count--;
}

void ClearCurrentFrame() {
  if (g_animation.frame_count <= 0) return;
  AnimationFrame* current_frame = &g_animation.frames[g_current_frame_index];
  for (int i = 0; i < g_animation.width * g_animation.height; ++i) {
    current_frame->cells[i] = (AnimatorCell){' ', BLACK, BLACK};
  }
}

// Duplicates the current frame and inserts it after the current frame
void DuplicateCurrentFrame() {
  if (g_animation.frame_count >= MAX_FRAMES) {
    fprintf(stderr, "WARNING: Maximum frames reached (%d). Cannot duplicate frame.\n", MAX_FRAMES);
    return;
  }
  if (g_animation.frame_count == 0) { // Should not happen if AddFrame is called on init
    AddFrame(); // Add a blank frame if somehow no frames exist
    return;
  }

  // Create a new frame
  AnimationFrame new_frame;
  new_frame.cells = (AnimatorCell*)malloc(sizeof(AnimatorCell) * g_animation.width * g_animation.height);
  if (new_frame.cells == NULL) {
    fprintf(stderr, "ERROR: Failed to allocate cells for duplicated frame.\n");
    return;
  }

  // Copy content from the current frame to the new frame
  memcpy(new_frame.cells, g_animation.frames[g_current_frame_index].cells,
       sizeof(AnimatorCell) * g_animation.width * g_animation.height);

  // Shift frames to make space for the new frame
  for (int i = g_animation.frame_count; i > g_current_frame_index + 1; --i) {
    g_animation.frames[i] = g_animation.frames[i - 1];
  }

  // Insert the new frame
  g_animation.frames[g_current_frame_index + 1] = new_frame;
  g_animation.frame_count++;
  g_current_frame_index++; // Move to the new duplicated frame
}


// --- Animation File I/O Implementations ---

bool SaveAnimation(const char* filename) {
  FILE* file = fopen(filename, "w"); // Open in text write mode
  if (file == NULL) {
    fprintf(stderr, "ERROR: Could not open file for saving: %s\n", filename);
    return false;
  }

  fprintf(file, "ANIMATION_START\n");
  fprintf(file, "WIDTH %d\n", g_animation.width);
  fprintf(file, "HEIGHT %d\n", g_animation.height);
  fprintf(file, "FPS %d\n", g_animation.fps);
  fprintf(file, "FRAME_COUNT %d\n", g_animation.frame_count);

  for (int i = 0; i < g_animation.frame_count; ++i) {
    fprintf(file, "FRAME_START\n");
    AnimationFrame* frame = &g_animation.frames[i];

    // Write characters
    for (int y = 0; y < g_animation.height; ++y) {
      for (int x = 0; x < g_animation.width; ++x) {
        fprintf(file, "%c", frame->cells[y * g_animation.width + x].character);
      }
      fprintf(file, "\n");
    }

    // Write foreground color indices
    fprintf(file, "FG_COLORS\n");
    for (int y = 0; y < g_animation.height; ++y) {
      for (int x = 0; x < g_animation.width; ++x) {
        fprintf(file, "%d ", __tr_get_color_index(frame->cells[y * g_animation.width + x].fg_color));
      }
      fprintf(file, "\n");
    }

    // Write background color indices
    fprintf(file, "BG_COLORS\n");
    for (int y = 0; y < g_animation.height; ++y) {
      for (int x = 0; x < g_animation.width; ++x) {
        fprintf(file, "%d ", __tr_get_color_index(frame->cells[y * g_animation.width + x].bg_color));
      }
      fprintf(file, "\n");
    }
    fprintf(file, "FRAME_END\n");
  }

  fprintf(file, "ANIMATION_END\n");
  fclose(file);
  return true;
}

bool LoadAnimation(const char* filename) {
  FILE* file = fopen(filename, "r"); // Open in text read mode
  if (file == NULL) {
    fprintf(stderr, "ERROR: Could not open file for loading: %s\n", filename);
    return false;
  }

  // Clean up existing animation data
  TR_CloseWindow(); // Close window to ensure tread.h internal state is reset

  char line_buffer[512]; // Max line length for reading
  int width = 0, height = 0, fps = 0, frame_count = 0;

  // Read header
  if (fgets(line_buffer, sizeof(line_buffer), file) == NULL || strcmp(line_buffer, "ANIMATION_START\n") != 0) { fprintf(stderr, "Load Error: Missing ANIMATION_START\n"); fclose(file); return false; }
  if (fscanf(file, "WIDTH %d\n", &width) != 1) { fprintf(stderr, "Load Error: Missing WIDTH\n"); fclose(file); return false; }
  if (fscanf(file, "HEIGHT %d\n", &height) != 1) { fprintf(stderr, "Load Error: Missing HEIGHT\n"); fclose(file); return false; }
  if (fscanf(file, "FPS %d\n", &fps) != 1) { fprintf(stderr, "Load Error: Missing FPS\n"); fclose(file); return false; }
  if (fscanf(file, "FRAME_COUNT %d\n", &frame_count) != 1) { fprintf(stderr, "Load Error: Missing FRAME_COUNT\n"); fclose(file); return false; }

  // Consume the newline after FRAME_COUNT
  int c;
  while ((c = fgetc(file)) != '\n' && c != EOF);

  // Re-initialize animator with loaded dimensions
  TR_InitWindow(width, height + 5, "tread.h - Animator (Loaded)");
  g_animation.width = width;
  g_animation.height = height;
  g_animation.fps = fps;

  // Free existing frames before re-allocating for loaded data
  for (int i = 0; i < g_animation.frame_count; ++i) {
    free(g_animation.frames[i].cells);
  }
  g_animation.frame_count = 0; // Reset frame count before adding new ones

  // Load frames
  for (int i = 0; i < frame_count; ++i) {
    if (fgets(line_buffer, sizeof(line_buffer), file) == NULL || strcmp(line_buffer, "FRAME_START\n") != 0) { fprintf(stderr, "Load Error: Missing FRAME_START for frame %d\n", i); fclose(file); return false; }

    // Allocate new frame
    AnimationFrame new_frame;
    new_frame.cells = (AnimatorCell*)malloc(sizeof(AnimatorCell) * width * height);
    if (new_frame.cells == NULL) {
      fprintf(stderr, "ERROR: Failed to allocate cells for loaded frame %d.\n", i);
      fclose(file);
      return false;
    }

    // Read characters
    for (int y = 0; y < height; ++y) {
      if (fgets(line_buffer, sizeof(line_buffer), file) == NULL) { fprintf(stderr, "Load Error: Missing char line %d for frame %d\n", y, i); fclose(file); free(new_frame.cells); return false; }
      // Ensure the line is null-terminated at the expected width to prevent reading beyond bounds
      // Subtract 1 for the null terminator, ensure it doesn't go negative
      size_t len = strlen(line_buffer);
      if (len > width) { // If line is longer than expected width, truncate
        line_buffer[width] = '\0';
      } else if (len > 0 && line_buffer[len - 1] == '\n') { // Remove newline if present
        line_buffer[len - 1] = '\0';
      }

      for (int x = 0; x < width; ++x) {
        char ch = (x < strlen(line_buffer)) ? line_buffer[x] : ' '; // Default to space if line is too short
        // Sanitize character: convert non-printable/non-standard chars to a regular space
        if (!isprint((unsigned char)ch) || (unsigned char)ch == 0xA0) { // Check for non-printable or non-breaking space
          ch = ' '; // Convert to standard space
        }
        new_frame.cells[y * width + x].character = ch;
      }
    }

    // Read foreground color indices
    if (fgets(line_buffer, sizeof(line_buffer), file) == NULL || strcmp(line_buffer, "FG_COLORS\n") != 0) { fprintf(stderr, "Load Error: Missing FG_COLORS tag for frame %d\n", i); fclose(file); free(new_frame.cells); return false; }
    for (int y = 0; y < height; ++y) {
      for (int x = 0; x < width; ++x) {
        short color_idx;
        if (fscanf(file, "%hd ", &color_idx) != 1) { fprintf(stderr, "Load Error: Missing FG color %d,%d for frame %d\n", y, x, i); fclose(file); free(new_frame.cells); return false; }
        new_frame.cells[y * width + x].fg_color = __tr_get_color_from_index(color_idx);
      }
      // Consume newline after each row of numbers
      while ((c = fgetc(file)) != '\n' && c != EOF);
    }

    // Read background color indices
    if (fgets(line_buffer, sizeof(line_buffer), file) == NULL || strcmp(line_buffer, "BG_COLORS\n") != 0) { fprintf(stderr, "Load Error: Missing BG_COLORS tag for frame %d\n", i); fclose(file); free(new_frame.cells); return false; }
    for (int y = 0; y < height; ++y) {
      for (int x = 0; x < width; ++x) {
        short color_idx;
        if (fscanf(file, "%hd ", &color_idx) != 1) { fprintf(stderr, "Load Error: Missing BG color %d,%d for frame %d\n", y, x, i); fclose(file); free(new_frame.cells); return false; }
        new_frame.cells[y * width + x].bg_color = __tr_get_color_from_index(color_idx);
      }
      // Consume newline after each row of numbers
      while ((c = fgetc(file)) != '\n' && c != EOF);
    }

    if (fgets(line_buffer, sizeof(line_buffer), file) == NULL || strcmp(line_buffer, "FRAME_END\n") != 0) { fprintf(stderr, "Load Error: Missing FRAME_END for frame %d\n", i); fclose(file); return false; }

    // Add the loaded frame to the animation
    g_animation.frames[g_animation.frame_count] = new_frame;
    g_animation.frame_count++;
  }

  if (fgets(line_buffer, sizeof(line_buffer), file) == NULL || strcmp(line_buffer, "ANIMATION_END\n") != 0) { fprintf(stderr, "Load Error: Missing ANIMATION_END\n"); fclose(file); return false; }
  fclose(file);
  return true;
}
