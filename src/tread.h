// tread.h - A header-only library providing a simple API for C
//           that works directly with Windows and POSIX terminals,
//           without relying on NCurses.
//
// This library aims to abstract low-level terminal complexities, offering a simpler
// interface for basic terminal-based "graphics" and input, similar to Raylib.
//
// Limitations and Design Choices:
// - All drawing is character-based. Shapes are approximations.
// - "Font size" in DrawText is largely ignored or used for character repetition.
// - Colors are mapped to basic 8/16 terminal colors (or closest approximation).
// - No true alpha blending; alpha component in Color struct is ignored.
// - Performance is tied to terminal refresh rates and direct character output.
// - Input handling is basic (single key press).
// - Terminal resizing is not automatically handled for drawing bounds.
// - Uses platform-specific APIs: Windows.h for Windows, termios/unistd for POSIX.
// - All public functions and internal global variables (except Color and color macros)
//   are prefixed with 'TR_' to avoid name clashes.
// - Implements double buffering to reduce screen flickering.
// - Adds terminal resize detection: if the terminal size changes, the program exits.
// - Drawing functions now accept an optional background color (use BLANK for transparency).
// - IMPORTANT: Ctrl+C (SIGINT) is now disabled by default when TR_InitWindow is called.
//   Applications must provide an alternative way to exit (e.g., 'q' or ESC key).

#ifndef TREAD_H
#define TREAD_H

#include <stdio.h>   // For printf, fflush, fprintf
#include <stdlib.h>  // For malloc, free, exit
#include <string.h>  // For strlen, memset, memcpy
#include <stdbool.h> // For bool type
#include <errno.h>   // For errno in nanosleep
#include <math.h>  // For sin, cos, tan (for 3D math)

// Define M_PI if not already defined (common in math.h but not guaranteed)
#ifndef M_PI
  #define M_PI 3.14159265358979323846
#endif

// --- Platform-Specific Includes and Definitions ---

#ifdef _WIN32
  #include <windows.h> // Windows API for console manipulation
  #include <conio.h>   // For _kbhit, _getch (non-blocking input)
#else
  #include <termios.h> // For tcgetattr, tcsetattr (terminal modes)
  #include <unistd.h>  // For read, write, STDIN_FILENO
  #include <sys/time.h> // For select, timeval
  #include <fcntl.h>   // For fcntl (non-blocking file descriptor)
  #include <time.h>  // For nanosleep, clock_gettime
  #include <sys/ioctl.h> // For ioctl and TIOCGWINSZ
  #include <signal.h> // For sigaction (POSIX signal handling)
#endif

// --- Type Definitions ---

// Represents a color with RGBA components. Alpha is ignored.
typedef struct {
  unsigned char r;
  unsigned char g;
  unsigned char b;
  unsigned char a;
} Color;

// Represents a single character cell in the terminal buffer
typedef struct {
  char character;
  Color fg_color;
  Color bg_color;
} __TR_Cell;

// --- Function Prototypes (to resolve C99 implicit declaration errors) ---
static inline int TR_GetScreenWidth();
static inline int TR_GetScreenHeight();

// --- Global State and Configuration ---

static bool __tr_window_open = false;
static long long __tr_frame_time_us = 0; // Target frame time in microseconds
static int __tr_key_buffer = 0;     // Stores the last key pressed

// Double buffering related globals
static __TR_Cell* __tr_screen_buffer = NULL;    // Current frame buffer
static __TR_Cell* __tr_prev_screen_buffer = NULL; // Previous frame buffer
static int __tr_buffer_width = 0;          // Actual width of the buffer
static int __tr_buffer_height = 0;         // Actual height of the buffer
static Color __tr_current_bg_color = {0,0,0,255};  // Stores the last background color set by ClearBackground

// Initial screen dimensions for resize detection
static int __tr_initial_width = 0;
static int __tr_initial_height = 0;

// Global 3D State (Declared here, initialized/freed conditionally in Init/CloseWindow)
static float* __tr_z_buffer = NULL; // Z-buffer for depth testing
static int __tr_z_buffer_size = 0;

// Platform-specific terminal state for restoring original mode
#ifdef _WIN32
  static HANDLE __tr_h_stdout;
  static HANDLE __tr_h_stdin;
  static DWORD __tr_original_out_mode;
  static DWORD __tr_original_in_mode;
  // Removed __tr_console_buffer_size as we are no longer setting it in InitWindow
  static long long __tr_last_frame_start_ns = 0; // Nanoseconds for last frame start (Windows)
#else
  static struct termios __tr_original_termios;
  static struct timespec __tr_last_frame_start_ts; // Timespec for last frame start (POSIX)
  static struct sigaction __tr_original_sigint_action; // For restoring SIGINT handler
#endif

// --- Predefined Colors (Raylib-like) ---
// These are standard Raylib colors, mapped to basic terminal colors.
// The actual display depends on the terminal's color support.
#define BLANK      (Color){ 1, 0, 0, 0 }
#define RAYWHITE   (Color){ 245, 245, 245, 255 }
#define TREADGRAY  (Color){ 30, 30, 30, 255 }

#define LIGHTGRAY  (Color){ 200, 200, 200, 255 }
#define GRAY       (Color){ 130, 130, 130, 255 }
#define DARKGRAY   (Color){ 80, 80, 80, 255 }
#define YELLOW     (Color){ 253, 249, 0, 255 }
#define GOLD       (Color){ 255, 203, 0, 255 }
#define ORANGE     (Color){ 255, 161, 0, 255 }
#define PINK       (Color){ 255, 109, 194, 255 }
#define RED        (Color){ 230, 41, 55, 255 }
#define MAROON     (Color){ 190, 33, 55, 255 }
#define GREEN      (Color){ 0, 200, 0, 255 }
#define LIME       (Color){ 0, 255, 0, 255 }
#define DARKGREEN  (Color){ 0, 82, 17, 255 }
#define SKYBLUE    (Color){ 102, 191, 255, 255 }
#define BLUE       (Color){ 0, 121, 241, 255 }
#define DARKBLUE   (Color){ 0, 82, 172, 255 }
#define PURPLE     (Color){ 200, 122, 255, 255 }
#define VIOLET     (Color){ 135, 60, 190, 255 }
#define DARKPURPLE (Color){ 112, 31, 126, 255 }
#define BEIGE      (Color){ 211, 176, 131, 255 }
#define BROWN      (Color){ 127, 106, 79, 255 }
#define DARKBROWN  (Color){ 76, 63, 47, 255 }
#define WHITE      (Color){ 255, 255, 255, 255 }
#define BLACK      (Color){ 0, 0, 0, 255 }
#define MAGENTA    (Color){ 255, 0, 255, 255 }
#define CYAN       (Color){ 0, 255, 255, 255 }

// --- Custom Key Codes for Special Keys (to avoid multi-character literals) ---
// These are arbitrary integer values chosen to not conflict with ASCII characters.
#define TR_KEY_UP     256
#define TR_KEY_DOWN   257
#define TR_KEY_LEFT   258
#define TR_KEY_RIGHT  259

#define TR_KEY_ENTER  13
#define TR_KEY_BACKSPACE 8
#define TR_KEY_DELETE 127
#define TR_KEY_ESCAPE 27

#define TR_KEY_F1  260
#define TR_KEY_F2  261
#define TR_KEY_F3  262
#define TR_KEY_F4  263
#define TR_KEY_F5  264
#define TR_KEY_F6  265
#define TR_KEY_F7  266
#define TR_KEY_F8  267
#define TR_KEY_F9  268
#define TR_KEY_F10 269
#define TR_KEY_F11 270
#define TR_KEY_F12 271

// --- Internal Helper Functions (static inline to ensure header-only) ---

// Compares two Color structs (ignoring alpha)
static inline bool __tr_colors_equal(Color c1, Color c2) {
  return c1.r == c2.r && c1.g == c2.g && c1.b == c2.b;
}

// Maps an RGB Color to a basic terminal color code (0-7 for foreground/background)
// This is a very rough approximation.
static inline short __tr_map_color_to_terminal(Color color, bool is_background) {
  // Simple Euclidean distance approximation to basic 8 colors
  // Black, Red, Green, Yellow, Blue, Magenta, Cyan, White
  const Color terminal_colors[] = {
    {0, 0, 0, 255},     // Black
    {255, 0, 0, 255},   // Red
    {0, 255, 0, 255},   // Green (bright green)
    {255, 255, 0, 255},   // Yellow
    {0, 0, 255, 255},   // Blue
    {255, 0, 255, 255},   // Magenta
    {0, 255, 255, 255},   // Cyan
    {255, 255, 255, 255}  // White
  };

  double min_dist = -1.0;
  short best_match = 0; // Default to black

  for (int i = 0; i < 8; ++i) {
    double dist = (double)(color.r - terminal_colors[i].r) * (color.r - terminal_colors[i].r) +
            (double)(color.g - terminal_colors[i].g) * (color.g - terminal_colors[i].g) +
            (double)(color.b - terminal_colors[i].b) * (color.b - terminal_colors[i].b);
    if (min_dist == -1.0 || dist < min_dist) {
      min_dist = dist;
      best_match = i;
    }
  }

  // Windows console colors are 0-7 for foreground, 0-7 for background (shifted by 4 bits)
  // ANSI escape codes use 30-37 for foreground, 40-47 for background.
  return best_match;
}

// --- Platform-Specific Terminal Control Functions ---

#ifdef _WIN32

// Custom Ctrl+C handler for Windows
static BOOL WINAPI __tr_ctrl_c_handler(DWORD dwCtrlType) {
    if (dwCtrlType == CTRL_C_EVENT) {
        // Return TRUE to indicate that the event has been handled
        // and should not be passed to the next handler (default termination).
        return TRUE;
    }
    return FALSE; // Pass unhandled events to the next handler
}

// Sets cursor position on Windows console
static inline void __tr_set_cursor_position(int x, int y) {
  COORD coord = { (SHORT)x, (SHORT)y };
  SetConsoleCursorPosition(__tr_h_stdout, coord);
}

// Sets foreground and background colors on Windows console
static inline void __tr_set_terminal_color(Color fg_color, Color bg_color) {
  WORD attributes = 0;
  short fg_code = __tr_map_color_to_terminal(fg_color, false);
  short bg_code = __tr_map_color_to_terminal(bg_color, true);

  if (fg_code & 0x1) attributes |= FOREGROUND_RED;
  if (fg_code & 0x2) attributes |= FOREGROUND_GREEN;
  if (fg_code & 0x4) attributes |= FOREGROUND_BLUE;
  // Add intensity if the mapped color is a bright one (e.g., Yellow, White, Cyan, Magenta, Red, Green, Blue)
  // This is a heuristic, can be improved.
  if (fg_color.r > 128 || fg_color.g > 128 || fg_color.b > 128) {
    attributes |= FOREGROUND_INTENSITY;
  }

  if (bg_code & 0x1) attributes |= BACKGROUND_RED;
  if (bg_code & 0x2) attributes |= BACKGROUND_GREEN;
  if (bg_code & 0x4) attributes |= BACKGROUND_BLUE;
  if (bg_color.r > 128 || bg_color.g > 128 || bg_color.b > 128) {
    attributes |= BACKGROUND_INTENSITY;
  }

  SetConsoleTextAttribute(__tr_h_stdout, attributes);
}

// Clears the Windows console screen (used internally for initial setup or full clear)
static inline void __tr_clear_screen_direct(Color bg_color) {
  COORD coord = { 0, 0 };
  DWORD count;
  CONSOLE_SCREEN_BUFFER_INFO csbi;
  GetConsoleScreenBufferInfo(__tr_h_stdout, &csbi);
  DWORD cell_count = csbi.dwSize.X * csbi.dwSize.Y;

  // Fill with spaces
  FillConsoleOutputCharacter(__tr_h_stdout, ' ', cell_count, coord, &count);

  // Fill with background color
  WORD attributes = 0;
  short bg_code = __tr_map_color_to_terminal(bg_color, true);
  if (bg_code & 0x1) attributes |= BACKGROUND_RED;
  if (bg_code & 0x2) attributes |= BACKGROUND_GREEN;
  if (bg_code & 0x4) attributes |= BACKGROUND_BLUE;
  if (bg_color.r > 128 || bg_color.g > 128 || bg_color.b > 128) {
    attributes |= BACKGROUND_INTENSITY;
  }
  FillConsoleOutputAttribute(__tr_h_stdout, attributes, cell_count, coord, &count);

  __tr_set_cursor_position(0, 0); // Move cursor to home
}

// Hides/shows cursor on Windows
static inline void __tr_set_cursor_visibility(bool visible) {
  CONSOLE_CURSOR_INFO cursor_info;
  GetConsoleCursorInfo(__tr_h_stdout, &cursor_info);
  cursor_info.bVisible = visible ? TRUE : FALSE;
  SetConsoleCursorInfo(__tr_h_stdout, &cursor_info);
}

// Sets Windows console title
static inline void __tr_set_console_title(const char* title) {
  SetConsoleTitleA(title);
}

// Reads a key press non-blockingly on Windows
static inline int __tr_get_key_nonblocking() {
  if (_kbhit()) {
    int key = _getch();
    if (key == 0 || key == 0xE0) { // Extended key
      key = _getch(); // Get the actual scan code
      switch (key) {
        // Arrow keys
        case 72: return TR_KEY_UP;
        case 80: return TR_KEY_DOWN;
        case 75: return TR_KEY_LEFT;
        case 77: return TR_KEY_RIGHT;
        // F-row keys
        case 59: return TR_KEY_F1;
        case 60: return TR_KEY_F2;
        case 61: return TR_KEY_F3;
        case 62: return TR_KEY_F4;
        case 63: return TR_KEY_F5;
        case 64: return TR_KEY_F6;
        case 65: return TR_KEY_F7;
        case 66: return TR_KEY_F8;
        case 67: return TR_KEY_F9;
        case 68: return TR_KEY_F10;
        case 85: return TR_KEY_F11;
        case 86: return TR_KEY_F12;
        default: return 0; // Unhandled extended key
      }
    }
    return key; // Regular ASCII key
  }
  return 0; // No key pressed
}

// High-resolution timer for Windows
static inline long long __tr_get_time_ns() {
  LARGE_INTEGER freq, count;
  QueryPerformanceFrequency(&freq);
  QueryPerformanceCounter(&count);
  return (count.QuadPart * 1000000000LL) / freq.QuadPart;
}

// Gets current screen width on Windows
static inline int __tr_get_screen_width_platform() {
  CONSOLE_SCREEN_BUFFER_INFO csbi;
  if (GetConsoleScreenBufferInfo(__tr_h_stdout, &csbi)) {
    return csbi.srWindow.Right - csbi.srWindow.Left + 1;
  }
  return 0; // Error or not initialized
}

// Gets current screen height on Windows
static inline int __tr_get_screen_height_platform() {
  CONSOLE_SCREEN_BUFFER_INFO csbi;
  if (GetConsoleScreenBufferInfo(__tr_h_stdout, &csbi)) {
    return csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
  }
  return 0; // Error or not initialized
}


#else // POSIX (Linux, macOS)

// Custom SIGINT handler for POSIX
static void __tr_sigint_handler(int signum) {
    // Do nothing, effectively ignoring Ctrl+C
    (void)signum; // Suppress unused parameter warning
}

// Sets cursor position using ANSI escape codes
static inline void __tr_set_cursor_position(int x, int y) {
  // ANSI: \x1b[<ROW>;<COL>H or \x1b[<ROW>;<COL>f
  // Terminals are 1-indexed for rows/cols
  printf("\x1b[%d;%dH", y + 1, x + 1);
}

// Sets foreground and background colors using ANSI escape codes
static inline void __tr_set_terminal_color(Color fg_color, Color bg_color) {
  short fg_code = __tr_map_color_to_terminal(fg_color, false);
  short bg_code = __tr_map_color_to_terminal(bg_color, true);

  // ANSI 8-color codes: 30-37 for foreground, 40-47 for background
  // Add 60 for bright colors (e.g., 90-97 for bright foreground)
  int ansi_fg = 30 + fg_code;
  int ansi_bg = 40 + bg_code;

  // A simple heuristic for bright colors (can be improved)
  if (fg_color.r > 128 || fg_color.g > 128 || fg_color.b > 128) ansi_fg += 60;
  if (bg_color.r > 128 || bg_color.g > 128 || bg_color.b > 128) ansi_bg += 60;

  printf("\x1b[%d;%dm", ansi_fg, ansi_bg);
}

// Clears the POSIX terminal screen (used internally for initial setup or full clear)
static inline void __tr_clear_screen_direct(Color bg_color) {
  // Set background color for the clear operation
  __tr_set_terminal_color(BLACK, bg_color); // Use BLACK foreground, as it's just clearing
  printf("\x1b[2J"); // Clear entire screen
  printf("\x1b[H");  // Move cursor to home (top-left)
}

// Hides/shows cursor on POSIX
static inline void __tr_set_cursor_visibility(bool visible) {
  if (visible) {
    printf("\x1b[?25h"); // Show cursor
  } else {
    printf("\x1b[?25l"); // Hide cursor
  }
}

// Sets POSIX terminal title (not universally supported, but common)
static inline void __tr_set_console_title(const char* title) {
  printf("\x1b]0;%s\x07", title); // OSC 0; title ST (String Terminator)
}

// Sets terminal to raw mode for non-canonical input
static inline void __tr_set_raw_mode() {
  tcgetattr(STDIN_FILENO, &__tr_original_termios); // Save original settings
  struct termios raw = __tr_original_termios;
  raw.c_lflag &= ~(ICANON | ECHO); // Disable canonical mode (line buffering) and echoing
  raw.c_cc[VMIN] = 0;  // Minimum number of characters for non-blocking read
  raw.c_cc[VTIME] = 0; // Timeout in 0.1s for non-blocking read
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw); // Apply new settings
}

// Restores original terminal mode
static inline void __tr_restore_terminal_mode() {
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &__tr_original_termios);
}

// Reads a key press non-blockingly on POSIX
static inline int __tr_get_key_nonblocking() {
  char buf[8]; // A larger buffer for longer escape sequences
  struct timeval tv = { 0L, 0L };
  fd_set fds;
  FD_ZERO(&fds);
  FD_SET(STDIN_FILENO, &fds);

  if (select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv) > 0) {
    ssize_t bytes_read = read(STDIN_FILENO, buf, sizeof(buf));
    if (bytes_read > 0) {
      if (buf[0] == '\x1b') { // Escape sequence
        if (bytes_read == 3 && buf[1] == '[') {
          switch (buf[2]) {
            case 'A': return TR_KEY_UP;
            case 'B': return TR_KEY_DOWN;
            case 'C': return TR_KEY_RIGHT;
            case 'D': return TR_KEY_LEFT;
          }
        } else if (bytes_read == 3 && buf[1] == 'O') {
          switch (buf[2]) {
            case 'P': return TR_KEY_F1;
            case 'Q': return TR_KEY_F2;
            case 'R': return TR_KEY_F3;
            case 'S': return TR_KEY_F4;
          }
        } else if (bytes_read == 4 && buf[1] == '[') {
          // Modern terminals often use these for F5-F12
          // The codes can be inconsistent, but these are common
          if (strcmp(&buf[2], "15~") == 0) return TR_KEY_F5;
          if (strcmp(&buf[2], "17~") == 0) return TR_KEY_F6;
          if (strcmp(&buf[2], "18~") == 0) return TR_KEY_F7;
          if (strcmp(&buf[2], "19~") == 0) return TR_KEY_F8;
          if (strcmp(&buf[2], "20~") == 0) return TR_KEY_F9;
          if (strcmp(&buf[2], "21~") == 0) return TR_KEY_F10;
          if (strcmp(&buf[2], "23~") == 0) return TR_KEY_F11;
          if (strcmp(&buf[2], "24~") == 0) return TR_KEY_F12;
        }
        return 0; // Unrecognized or incomplete escape sequence
      }
      return buf[0]; // Regular ASCII character
    }
  }
  return 0; // No key pressed
}

// High-resolution timer for POSIX
static inline long long __tr_get_time_ns() {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (long long)ts.tv_sec * 1000000000LL + ts.tv_nsec;
}

// Gets current screen width on POSIX
static inline int __tr_get_screen_width_platform() {
  struct winsize ws;
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0) {
    return ws.ws_col;
  }
  return 0; // Error or not initialized
}

// Gets current screen height on POSIX
static inline int __tr_get_screen_height_platform() {
  struct winsize ws;
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0) {
    return ws.ws_row;
  }
  return 0; // Error or not initialized
}

#endif // _WIN32 / POSIX

// --- Raylib-like API Functions ---

// Initializes the terminal window for drawing.
// `width` and `height` are logical dimensions; actual terminal size may vary.
// `title` sets the terminal window title.
static inline void TR_InitWindow(int width, int height, const char* title) {
  (void)width; // Suppress unused parameter warning
  (void)height; // Suppress unused parameter warning

  if (__tr_window_open) return;

#ifdef _WIN32
  __tr_h_stdout = GetStdHandle(STD_OUTPUT_HANDLE);
  __tr_h_stdin = GetStdHandle(STD_INPUT_HANDLE);

  // Save original console modes
  GetConsoleMode(__tr_h_stdout, &__tr_original_out_mode);
  DWORD current_in_mode; // Declare current_in_mode here
  GetConsoleMode(__tr_h_stdin, &current_in_mode);
  SetConsoleMode(__tr_h_stdin, current_in_mode & ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT));

  // Register Ctrl+C handler to disable default termination
  if (!SetConsoleCtrlHandler(__tr_ctrl_c_handler, TRUE)) {
      fprintf(stderr, "TREAD WARNING: Could not set console control handler. Ctrl+C might still terminate the app.\n");
  }

  // Initialize stdout handle if it's not already set (for TR_GetScreenWidth/Height)
  if (__tr_h_stdout == NULL) {
    __tr_h_stdout = GetStdHandle(STD_OUTPUT_HANDLE);
  }
#else // POSIX
  __tr_set_raw_mode();
  // Set up SIGINT handler to ignore Ctrl+C
  struct sigaction sa;
  sa.sa_handler = __tr_sigint_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  if (sigaction(SIGINT, &sa, &__tr_original_sigint_action) == -1) {
      perror("TREAD WARNING: Could not set SIGINT handler. Ctrl+C might still terminate the app.");
  }
#endif

  // Get actual terminal dimensions for buffer allocation
  __tr_buffer_width = TR_GetScreenWidth();
  __tr_buffer_height = TR_GetScreenHeight();

  // Store initial dimensions for resize detection
  __tr_initial_width = __tr_buffer_width;
  __tr_initial_height = __tr_buffer_height;

  if (__tr_buffer_width == 0 || __tr_buffer_height == 0) {
    fprintf(stderr, "TREAD ERROR: Could not get valid terminal dimensions. Exiting.\n");
    exit(1);
  }

  // Allocate screen buffers
  __tr_screen_buffer = (__TR_Cell*)malloc(sizeof(__TR_Cell) * __tr_buffer_width * __tr_buffer_height);
  __tr_prev_screen_buffer = (__TR_Cell*)malloc(sizeof(__TR_Cell) * __tr_buffer_width * __tr_buffer_height);

  if (__tr_screen_buffer == NULL || __tr_prev_screen_buffer == NULL) {
    fprintf(stderr, "TREAD ERROR: Failed to allocate screen buffers. Exiting.\n");
    exit(1);
  }

#ifdef TR_3D
  __tr_z_buffer_size = __tr_buffer_width * __tr_buffer_height;
  __tr_z_buffer = (float*)malloc(sizeof(float) * __tr_z_buffer_size);
  if (__tr_z_buffer == NULL) {
    fprintf(stderr, "TREAD ERROR: Failed to allocate Z-buffer. Exiting.\n");
    exit(1);
  }
#endif

  // Initialize buffers to empty spaces with black background
  for (int i = 0; i < __tr_buffer_width * __tr_buffer_height; ++i) {
    __tr_screen_buffer[i] = (__TR_Cell){' ', BLACK, BLACK};
    __tr_prev_screen_buffer[i] = (__TR_Cell){' ', BLACK, BLACK};
  }
  __tr_current_bg_color = BLACK; // Default background color

#ifdef _WIN32
  __tr_set_console_title(title);
  __tr_set_cursor_visibility(false); // Hide cursor
  __tr_clear_screen_direct(BLACK); // Initial full clear for a clean slate
#else // POSIX
  __tr_set_console_title(title);
  __tr_set_cursor_visibility(false); // Hide cursor
  __tr_clear_screen_direct(BLACK); // Initial full clear for a clean slate
  fflush(stdout); // Ensure changes are applied
#endif

  __tr_window_open = true;
  __tr_frame_time_us = 0; // Reset frame time
  __tr_key_buffer = 0;  // Clear key buffer
}

// Closes the terminal window and restores original terminal settings.
static inline void TR_CloseWindow() {
  if (!__tr_window_open) return;

#ifdef _WIN32
  // Restore original console modes
  SetConsoleMode(__tr_h_stdout, __tr_original_out_mode);
  SetConsoleMode(__tr_h_stdin, __tr_original_in_mode);

  // Restore default Ctrl+C handler
  SetConsoleCtrlHandler(__tr_ctrl_c_handler, FALSE);

  // Explicitly reset colors to default before final clear
  __tr_set_terminal_color(WHITE, BLACK);
  __tr_set_cursor_visibility(true); // Show cursor
  __tr_set_cursor_position(0, 0);   // Move cursor to home
  __tr_clear_screen_direct(BLACK);  // Clear screen one last time
#else // POSIX
  __tr_restore_terminal_mode();
  // Restore original SIGINT handler
  if (sigaction(SIGINT, &__tr_original_sigint_action, NULL) == -1) {
      perror("TREAD WARNING: Could not restore original SIGINT handler.");
  }
  printf("\x1b[0m"); // Reset all ANSI attributes (colors, bold, etc.)
  __tr_set_cursor_visibility(true); // Show cursor
  __tr_set_cursor_position(0, 0);   // Move cursor to home
  __tr_clear_screen_direct(BLACK);  // Clear screen one last time
  fflush(stdout); // Ensure changes are applied
#endif

  // Free allocated buffers
  if (__tr_screen_buffer != NULL) {
    free(__tr_screen_buffer);
    __tr_screen_buffer = NULL;
  }
  if (__tr_prev_screen_buffer != NULL) {
    free(__tr_prev_screen_buffer);
    __tr_prev_screen_buffer = NULL;
  }
#ifdef TR_3D
  if (__tr_z_buffer != NULL) {
    free(__tr_z_buffer);
    __tr_z_buffer = NULL;
  }
#endif

  __tr_window_open = false;
}

// Checks if the window should close (e.g., if ESC is pressed).
static inline bool TR_WindowShouldClose() {
  // For this simple implementation, we'll check for ESC key (27)
  // or 'q' for quit.
  return __tr_key_buffer == 27 || __tr_key_buffer == 'q';
}

// Sets the target frames per second (FPS).
static inline void TR_SetTargetFPS(int fps) {
  if (fps > 0) {
    __tr_frame_time_us = 1000000LL / fps; // Convert to microseconds
  } else {
    __tr_frame_time_us = 0; // No FPS limit
  }
}

// Begins the drawing phase. Reads input and prepares for drawing to buffer.
static inline void TR_BeginDrawing() {
  if (!__tr_window_open) return;

  // --- Check for terminal resize ---
  int current_width = TR_GetScreenWidth();
  int current_height = TR_GetScreenHeight();

  if (current_width != __tr_initial_width || current_height != __tr_initial_height) {
    // Clear screen and reset terminal attributes before printing error and exiting
#ifdef _WIN32
    __tr_set_terminal_color(WHITE, BLACK); // Reset colors to default
    __tr_set_cursor_visibility(true);    // Show cursor
    __tr_clear_screen_direct(BLACK);     // Clear the screen
#else // POSIX
    printf("\x1b[0m");           // Reset all ANSI attributes
    __tr_set_cursor_visibility(true);    // Show cursor
    __tr_clear_screen_direct(BLACK);     // Clear the screen
    fflush(stdout);            // Ensure clear is applied
#endif
    fprintf(stderr, "TREAD ERROR: Terminal screen size changed from %dx%d to %dx%d. Exiting.\n",
        __tr_initial_width, __tr_initial_height, current_width, current_height);
    exit(1); // Crash on purpose
  }
  // --- End resize check ---

  // Record start time for frame timing
#ifdef _WIN32
  __tr_last_frame_start_ns = __tr_get_time_ns();
#else
  clock_gettime(CLOCK_MONOTONIC, &__tr_last_frame_start_ts);
#endif

  // Read input at beginning of frame
  __tr_key_buffer = __tr_get_key_nonblocking();

  // The buffer is cleared by TR_ClearBackground, which should be called by the user.
  // If not called, the previous frame's content will persist unless overwritten.
  // For consistency, we'll reset the current buffer with the last known background color.
  for (int i = 0; i < __tr_buffer_width * __tr_buffer_height; ++i) {
    __tr_screen_buffer[i] = (__TR_Cell){' ', __tr_current_bg_color, __tr_current_bg_color};
  }
}

// Ends the drawing phase. Flushes output and handles frame timing.
static inline void TR_EndDrawing() {
  if (!__tr_window_open) return;

  // Compare buffers and draw only changed cells
  for (int y = 0; y < __tr_buffer_height; ++y) {
    for (int x = 0; x < __tr_buffer_width; ++x) {
      int index = y * __tr_buffer_width + x;
      __TR_Cell current_cell = __tr_screen_buffer[index];
      __TR_Cell prev_cell = __tr_prev_screen_buffer[index];

      // Only update if character or colors have changed
      if (current_cell.character != prev_cell.character ||
        !__tr_colors_equal(current_cell.fg_color, prev_cell.fg_color) ||
        !__tr_colors_equal(current_cell.bg_color, prev_cell.bg_color))
      {
        __tr_set_cursor_position(x, y);
        __tr_set_terminal_color(current_cell.fg_color, current_cell.bg_color);
        printf("%c", current_cell.character);
      }
    }
  }

  fflush(stdout); // Ensure all printed characters are displayed

  // Copy current buffer to previous buffer for next frame's comparison
  memcpy(__tr_prev_screen_buffer, __tr_screen_buffer, sizeof(__TR_Cell) * __tr_buffer_width * __tr_buffer_height);

  if (__tr_frame_time_us > 0) {
    long long current_time_ns;
    long long elapsed_ns;

#ifdef _WIN32
    current_time_ns = __tr_get_time_ns();
    elapsed_ns = current_time_ns - __tr_last_frame_start_ns;
#else
    struct timespec current_ts;
    clock_gettime(CLOCK_MONOTONIC, &current_ts);
    current_time_ns = (long long)current_ts.tv_sec * 1000000000LL + current_ts.tv_nsec;
    elapsed_ns = current_time_ns - ((long long)__tr_last_frame_start_ts.tv_sec * 1000000000LL + __tr_last_frame_start_ts.tv_nsec);
#endif

    long long target_ns = __tr_frame_time_us * 1000LL; // Convert us to ns

    if (elapsed_ns < target_ns) {
      long long sleep_ns = target_ns - elapsed_ns;
#ifdef _WIN32
      Sleep((DWORD)(sleep_ns / 1000000LL));
#else
      struct timespec req, rem;
      req.tv_sec = sleep_ns / 1000000000LL;
      req.tv_nsec = sleep_ns % 1000000000LL;
      while (nanosleep(&req, &rem) == -1 && errno == EINTR) {
        req = rem; // Resume sleep if interrupted by signal
      }
#endif
    }
  }
}

// Clears the entire drawing surface with the specified color.
static inline void TR_ClearBackground(Color color) {
  if (!__tr_window_open) return;
  __tr_current_bg_color = color; // Store the background color
  for (int i = 0; i < __tr_buffer_width * __tr_buffer_height; ++i) {
    __tr_screen_buffer[i] = (__TR_Cell){' ', color, color};
  }
}

// Draws a single character (pixel) at (x, y) with the specified color.
// This function always draws a solid block of the given color.
static inline void TR_DrawPixel(int x, int y, Color color) {
  if (!__tr_window_open || x < 0 || x >= __tr_buffer_width || y < 0 || y >= __tr_buffer_height) return;
  int index = y * __tr_buffer_width + x;
  __tr_screen_buffer[index].character = ' '; // A single space is the "pixel"
  __tr_screen_buffer[index].fg_color = color;
  __tr_screen_buffer[index].bg_color = color; // Pixel fills the background of its cell
}

// Draws text at (x, y) with the specified font size (ignored), foreground color, and background color.
// Pass BLANK for bg_color to use the current TR_ClearBackground color as background.
static inline void TR_DrawText(const char* text, int x, int y, int fontSize, Color fg_color, Color bg_color) {
  (void)fontSize; // Suppress unused parameter warning
  if (!__tr_window_open || !text || y < 0 || y >= __tr_buffer_height) return;

  Color final_bg_color = bg_color;
  if (__tr_colors_equal(bg_color, BLANK)) {
    final_bg_color = __tr_current_bg_color;
  }

  for (int i = 0; text[i] != '\0'; ++i) {
    int current_x = x + i;
    if (current_x >= 0 && current_x < __tr_buffer_width) {
      int index = y * __tr_buffer_width + current_x;
      __tr_screen_buffer[index].character = text[i];
      __tr_screen_buffer[index].fg_color = fg_color;
      __tr_screen_buffer[index].bg_color = final_bg_color;
    }
  }
}

// Draws a filled rectangle with the specified foreground and background colors.
// Pass BLANK for bg_color to use the current TR_ClearBackground color as background.
static inline void TR_DrawRectangle(int x, int y, int width, int height, Color fg_color, Color bg_color) {
  if (!__tr_window_open) return;

  Color final_bg_color = bg_color;
  if (__tr_colors_equal(bg_color, BLANK)) {
    final_bg_color = __tr_current_bg_color;
  }

  for (int j = 0; j < height; ++j) {
    for (int i = 0; i < width; ++i) {
      int current_x = x + i;
      int current_y = y + j;
      if (current_x >= 0 && current_x < __tr_buffer_width &&
        current_y >= 0 && current_y < __tr_buffer_height)
      {
        int index = current_y * __tr_buffer_width + current_x;
        __tr_screen_buffer[index].character = ' ';
        __tr_screen_buffer[index].fg_color = fg_color;
        __tr_screen_buffer[index].bg_color = final_bg_color;
      }
    }
  }
}

// Draws an empty rectangle (border) with the specified foreground and background colors.
// Pass BLANK for bg_color to use the current TR_ClearBackground color as background.
static inline void TR_DrawRectangleLines(int x, int y, int width, int height, Color fg_color, Color bg_color) {
  if (!__tr_window_open) return;

  Color final_bg_color = bg_color;
  if (__tr_colors_equal(bg_color, BLANK)) {
    final_bg_color = __tr_current_bg_color;
  }

  // Draw top and bottom lines
  for (int i = 0; i < width; ++i) {
    int top_x = x + i;
    int bottom_x = x + i;
    int top_y = y;
    int bottom_y = y + height - 1;

    if (top_x >= 0 && top_x < __tr_buffer_width && top_y >= 0 && top_y < __tr_buffer_height) {
      int index = top_y * __tr_buffer_width + top_x;
      __tr_screen_buffer[index].character = '#';
      __tr_screen_buffer[index].fg_color = fg_color;
      __tr_screen_buffer[index].bg_color = final_bg_color;
    }
    if (bottom_x >= 0 && bottom_x < __tr_buffer_width && bottom_y >= 0 && bottom_y < __tr_buffer_height) {
      int index = bottom_y * __tr_buffer_width + bottom_x;
      __tr_screen_buffer[index].character = '#';
      __tr_screen_buffer[index].fg_color = fg_color;
      __tr_screen_buffer[index].bg_color = final_bg_color;
    }
  }

  // Draw left and right lines (avoid corners already drawn)
  for (int j = 1; j < height - 1; ++j) {
    int left_y = y + j;
    int right_y = y + j;
    int left_x = x;
    int right_x = x + width - 1;

    if (left_x >= 0 && left_x < __tr_buffer_width && left_y >= 0 && left_y < __tr_buffer_height) {
      int index = left_y * __tr_buffer_width + left_x;
      __tr_screen_buffer[index].character = '#';
      __tr_screen_buffer[index].fg_color = fg_color;
      __tr_screen_buffer[index].bg_color = final_bg_color;
    }
    if (right_x >= 0 && right_x < __tr_buffer_width && right_y >= 0 && right_y < __tr_buffer_height) {
      int index = right_y * __tr_buffer_width + right_x;
      __tr_screen_buffer[index].character = '#';
      __tr_screen_buffer[index].fg_color = fg_color;
      __tr_screen_buffer[index].bg_color = final_bg_color;
    }
  }
}

// Checks if a key is currently down.
static inline bool TR_IsKeyDown(int key) {
  // This is a simplified check. For continuous key presses,
  // a more robust input system would be needed.
  // For now, it just checks the last key pressed.
  return __tr_key_buffer == key;
}

// Checks if a key has been pressed once.
static inline bool TR_IsKeyPressed(int key) {
  // Same as IsKeyDown in this simple implementation.
  return __tr_key_buffer == key;
}

// Get the last key pressed (and clears the buffer).
static inline int TR_GetKeyPressed() {
  int key = __tr_key_buffer;
  __tr_key_buffer = 0; // Clear buffer after reading
  return key;
}

// Gets the current width of the terminal screen in characters.
static inline int TR_GetScreenWidth() {
  // Ensure stdout handle is initialized on Windows before querying
#ifdef _WIN32
  if (__tr_h_stdout == NULL) {
    __tr_h_stdout = GetStdHandle(STD_OUTPUT_HANDLE);
  }
#endif
  return __tr_get_screen_width_platform();
}

// Gets the current height of the terminal screen in characters.
static inline int TR_GetScreenHeight() {
  // Ensure stdout handle is initialized on Windows before querying
#ifdef _WIN32
  if (__tr_h_stdout == NULL) {
    __tr_h_stdout = GetStdHandle(STD_OUTPUT_HANDLE);
  }
#endif
  return __tr_get_screen_height_platform();
}


// 3D stuff only:
#ifdef TR_3D

// --- 3D Data Structures ---
typedef struct { float x, y, z; } TR_Vector3;
typedef struct { float m[4][4]; } TR_Matrix4x4;
typedef struct { int v[3]; } TR_Triangle; // Indices into a vertex array


// --- 3D Math Functions ---

// Matrix Identity
static inline TR_Matrix4x4 TR_MatrixIdentity() {
  TR_Matrix4x4 mat = {0};
  mat.m[0][0] = 1.0f; mat.m[1][1] = 1.0f; mat.m[2][2] = 1.0f; mat.m[3][3] = 1.0f;
  return mat;
}

// Matrix Multiplication (A * B)
static inline TR_Matrix4x4 TR_MatrixMultiply(TR_Matrix4x4 mat1, TR_Matrix4x4 mat2) {
  TR_Matrix4x4 result = {0};
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      for (int k = 0; k < 4; k++) {
        result.m[i][j] += mat1.m[i][k] * mat2.m[k][j];
      }
    }
  }
  return result;
}

// Vector3 Transform (v * m)
static inline TR_Vector3 TR_Vector3Transform(TR_Vector3 v, TR_Matrix4x4 mat) {
  TR_Vector3 result;
  float x = v.x * mat.m[0][0] + v.y * mat.m[1][0] + v.z * mat.m[2][0] + 1.0f * mat.m[3][0];
  float y = v.x * mat.m[0][1] + v.y * mat.m[1][1] + v.z * mat.m[2][1] + 1.0f * mat.m[3][1];
  float z = v.x * mat.m[0][2] + v.y * mat.m[1][2] + v.z * mat.m[2][2] + 1.0f * mat.m[3][2];
  float w = v.x * mat.m[0][3] + v.y * mat.m[1][3] + v.z * mat.m[2][3] + 1.0f * mat.m[3][3];

  // Perspective divide
  if (w != 0.0f) {
    result.x = x / w;
    result.y = y / w;
    result.z = z / w;
  } else {
    result.x = x;
    result.y = y;
    result.z = z;
  }
  return result;
}

// Matrix Translation
static inline TR_Matrix4x4 TR_MatrixTranslate(float x, float y, float z) {
  TR_Matrix4x4 mat = TR_MatrixIdentity();
  mat.m[3][0] = x;
  mat.m[3][1] = y;
  mat.m[3][2] = z;
  return mat;
}

// Matrix Rotation around X axis (radians)
static inline TR_Matrix4x4 TR_MatrixRotateX(float angle) {
  TR_Matrix4x4 mat = TR_MatrixIdentity();
  float c = (float)cos(angle);
  float s = (float)sin(angle);
  mat.m[1][1] = c; mat.m[1][2] = s;
  mat.m[2][1] = -s; mat.m[2][2] = c;
  return mat;
}

// Matrix Rotation around Y axis (radians)
static inline TR_Matrix4x4 TR_MatrixRotateY(float angle) {
  TR_Matrix4x4 mat = TR_MatrixIdentity();
  float c = (float)cos(angle);
  float s = (float)sin(angle);
  mat.m[0][0] = c; mat.m[0][2] = -s;
  mat.m[2][0] = s; mat.m[2][2] = c;
  return mat;
}

// Matrix Rotation around Z axis (radians)
static inline TR_Matrix4x4 TR_MatrixRotateZ(float angle) {
  TR_Matrix4x4 mat = TR_MatrixIdentity();
  float c = (float)cos(angle);
  float s = (float)sin(angle);
  mat.m[0][0] = c; mat.m[0][1] = s;
  mat.m[1][0] = -s; mat.m[1][1] = c;
  return mat;
}

// Matrix Scaling
static inline TR_Matrix4x4 TR_MatrixScale(float x, float y, float z) {
  TR_Matrix4x4 mat = TR_MatrixIdentity();
  mat.m[0][0] = x;
  mat.m[1][1] = y;
  mat.m[2][2] = z;
  return mat;
}

// Basic Perspective Projection Matrix
static inline TR_Matrix4x4 TR_MatrixPerspective(float fovY, float aspect, float nearPlane, float farPlane) {
  TR_Matrix4x4 mat = {0};
  float tanHalfFovY = (float)tan(fovY / 2.0f);
  mat.m[0][0] = 1.0f / (aspect * tanHalfFovY);
  mat.m[1][1] = 1.0f / tanHalfFovY;
  mat.m[2][2] = -(farPlane + nearPlane) / (farPlane - nearPlane);
  mat.m[2][3] = -1.0f;
  mat.m[3][2] = -(2.0f * farPlane * nearPlane) / (farPlane - nearPlane);
  return mat;
}

// --- 2D Drawing Helper for 3D (Bresenham's Line Algorithm) ---
static inline void __tr_draw_line_2d(int x1, int y1, int x2, int y2, Color color) {
  int dx = abs(x2 - x1);
  int dy = abs(y2 - y1);
  int sx = (x1 < x2) ? 1 : -1;
  int sy = (y1 < y2) ? 1 : -1;
  int err = dx - dy;

  while (true) {
    TR_DrawPixel(x1, y1, color);
    if (x1 == x2 && y1 == y2) break;
    int e2 = 2 * err;
    if (e2 > -dy) { err -= dy; x1 += sx; }
    if (e2 < dx) { err += dx; y1 += sy; }
  }
}

// --- 3D Rendering Functions ---

// Projects a 3D point to 2D screen coordinates
static inline TR_Vector3 __tr_project_vertex(TR_Vector3 vertex, TR_Matrix4x4 mvp_matrix) {
  TR_Vector3 transformed_v = TR_Vector3Transform(vertex, mvp_matrix);

  // Convert from NDC (-1 to 1) to screen coordinates (0 to width/height)
  TR_Vector3 screen_v;
  screen_v.x = (transformed_v.x + 1.0f) * 0.5f * __tr_buffer_width;
  screen_v.y = (1.0f - transformed_v.y) * 0.5f * __tr_buffer_height; // Y-axis inverted for screen
  screen_v.z = transformed_v.z; // Keep Z for depth testing
  return screen_v;
}

// Draws a 3D triangle wireframe
static inline void TR_DrawTriangle3DWireframe(TR_Vector3 v1, TR_Vector3 v2, TR_Vector3 v3, TR_Matrix4x4 mvp_matrix, Color color) {
  TR_Vector3 pv1 = __tr_project_vertex(v1, mvp_matrix);
  TR_Vector3 pv2 = __tr_project_vertex(v2, mvp_matrix);
  TR_Vector3 pv3 = __tr_project_vertex(v3, mvp_matrix);

  // Simple view frustum culling (if any point is outside NDC range, skip triangle)
  // This is a very rough check and doesn't handle partial visibility.
  // For now, let's just draw if all points are within bounds for simplicity.
  // A proper implementation would clip lines.
  // We draw lines regardless of whether the points are within bounds, as the line drawing
  // function will handle out-of-bounds pixels by not drawing them.
  __tr_draw_line_2d((int)pv1.x, (int)pv1.y, (int)pv2.x, (int)pv2.y, color);
  __tr_draw_line_2d((int)pv2.x, (int)pv2.y, (int)pv3.x, (int)pv3.y, color);
  __tr_draw_line_2d((int)pv3.x, (int)pv3.y, (int)pv1.x, (int)pv1.y, color);
}

// Draws a 3D triangle filled (with Z-buffering)
static inline void TR_DrawTriangle3DFilled(TR_Vector3 v1, TR_Vector3 v2, TR_Vector3 v3, TR_Matrix4x4 mvp_matrix, Color color) {
  // Project vertices to screen space
  TR_Vector3 p[3];
  p[0] = __tr_project_vertex(v1, mvp_matrix);
  p[1] = __tr_project_vertex(v2, mvp_matrix);
  p[2] = __tr_project_vertex(v3, mvp_matrix);

  // Sort vertices by Y-coordinate (top to bottom) for scanline fill
  // This is a simplified approach, a more robust one would use barycentric coordinates
  // or edge functions for pixel-perfect fill.
  if (p[0].y > p[1].y) { TR_Vector3 temp = p[0]; p[0] = p[1]; p[1] = temp; }
  if (p[0].y > p[2].y) { TR_Vector3 temp = p[0]; p[0] = p[2]; p[2] = temp; }
  if (p[1].y > p[2].y) { TR_Vector3 temp = p[1]; p[1] = p[2]; p[2] = temp; }

  int y_start = (int)p[0].y;
  int y_end = (int)p[2].y;

  // Clamp to screen bounds
  if (y_start < 0) y_start = 0;
  if (y_end >= __tr_buffer_height) y_end = __tr_buffer_height - 1;

  for (int y = y_start; y <= y_end; ++y) {
    float x_left = (float)__tr_buffer_width;
    float x_right = 0.0f;
    float z_left = 1.0f; // Max Z (far)
    float z_right = 0.0f; // Min Z (near)

    // Find intersections with edges for current scanline
    for (int i = 0; i < 3; ++i) {
      TR_Vector3 p_curr = p[i];
      TR_Vector3 p_next = p[(i + 1) % 3];

      if ((y >= (int)p_curr.y && y < (int)p_next.y) || (y >= (int)p_next.y && y < (int)p_curr.y)) {
        if (p_curr.y == p_next.y) continue; // Horizontal line, skip

        float t = (float)(y - p_curr.y) / (p_next.y - p_curr.y);
        float intersect_x = p_curr.x + t * (p_next.x - p_curr.x);
        float intersect_z = p_curr.z + t * (p_next.z - p_curr.z);

        if (intersect_x < x_left) { x_left = intersect_x; z_left = intersect_z; }
        if (intersect_x > x_right) { x_right = intersect_x; z_right = intersect_z; }
      }
    }

    int x_start = (int)x_left;
    int x_end = (int)x_right;

    // Clamp to screen bounds
    if (x_start < 0) x_start = 0;
    if (x_end >= __tr_buffer_width) x_end = __tr_buffer_width - 1;

    // Draw scanline
    for (int x = x_start; x <= x_end; ++x) {
      // Interpolate Z for current pixel
      float pixel_z = z_left;
      if (x_end != x_start) {
        float t_x = (float)(x - x_start) / (x_end - x_start);
        pixel_z = z_left + t_x * (z_right - z_left);
      }

      int index = y * __tr_buffer_width + x;
      if (index >= 0 && index < __tr_z_buffer_size) {
        // Z-buffering check
        if (pixel_z < __tr_z_buffer[index]) { // Smaller Z means closer
          __tr_z_buffer[index] = pixel_z;
          TR_DrawPixel(x, y, color); // Draw the pixel
        }
      }
    }
  }
}


// --- Cube Data ---
// Vertices of a unit cube centered at origin
static const TR_Vector3 __tr_cube_vertices[] = {
  {-0.5f, -0.5f, -0.5f}, // 0
  { 0.5f, -0.5f, -0.5f}, // 1
  { 0.5f,  0.5f, -0.5f}, // 2
  {-0.5f,  0.5f, -0.5f}, // 3
  {-0.5f, -0.5f,  0.5f}, // 4
  { 0.5f, -0.5f,  0.5f}, // 5
  { 0.5f,  0.5f,  0.5f}, // 6
  {-0.5f,  0.5f,  0.5f}  // 7
};

// Faces of the cube (indices into __tr_cube_vertices)
// Each face is defined by two triangles
static const TR_Triangle __tr_cube_faces[] = {
  // Front face
  {{0, 1, 2}}, {{0, 2, 3}},
  // Back face
  {{4, 6, 5}}, {{4, 7, 6}},
  // Right face
  {{1, 5, 6}}, {{1, 6, 2}},
  // Left face
  {{4, 0, 3}}, {{4, 3, 7}},
  // Top face
  {{3, 2, 6}}, {{3, 6, 7}},
  // Bottom face
  {{0, 4, 5}}, {{0, 5, 1}}
};
static const int __tr_num_cube_faces = sizeof(__tr_cube_faces) / sizeof(TR_Triangle);


// --- Public 3D Drawing Functions ---

// Draws a 3D wireframe cube
static inline void TR_DrawCubeWireframe3D(TR_Vector3 position, TR_Vector3 size, TR_Vector3 rotation_radians, Color color) {
  TR_Matrix4x4 model = TR_MatrixIdentity();
  model = TR_MatrixMultiply(model, TR_MatrixScale(size.x, size.y, size.z));
  model = TR_MatrixMultiply(model, TR_MatrixRotateX(rotation_radians.x));
  model = TR_MatrixMultiply(model, TR_MatrixRotateY(rotation_radians.y));
  model = TR_MatrixMultiply(model, TR_MatrixRotateZ(rotation_radians.z));
  model = TR_MatrixMultiply(model, TR_MatrixTranslate(position.x, position.y, position.z));

  // Simple fixed camera (view matrix)
  TR_Matrix4x4 view = TR_MatrixTranslate(0.0f, 0.0f, -5.0f); // Move camera back

  // Perspective projection matrix (adjust FOV and aspect ratio)
  float aspect_ratio = (float)__tr_buffer_width / __tr_buffer_height;
  // Adjust aspect ratio for terminal characters (usually taller than wide)
  // A common ratio for terminal chars is 0.5 (width is half of height).
  // This is a heuristic and might need fine-tuning for different terminals.
  aspect_ratio *= 0.5f; // Compensate for character aspect ratio

  TR_Matrix4x4 projection = TR_MatrixPerspective(45.0f * (float)M_PI / 180.0f, aspect_ratio, 0.1f, 100.0f);

  TR_Matrix4x4 mvp_matrix = TR_MatrixMultiply(TR_MatrixMultiply(model, view), projection);

  for (int i = 0; i < __tr_num_cube_faces; ++i) {
    TR_Triangle face = __tr_cube_faces[i];
    TR_DrawTriangle3DWireframe(
      __tr_cube_vertices[face.v[0]],
      __tr_cube_vertices[face.v[1]],
      __tr_cube_vertices[face.v[2]],
      mvp_matrix,
      color
    );
  }
}

// Draws a 3D filled cube
static inline void TR_DrawCubeFilled3D(TR_Vector3 position, TR_Vector3 size, TR_Vector3 rotation_radians, Color color) {
  // Reset Z-buffer for each filled frame
  for (int i = 0; i < __tr_z_buffer_size; ++i) {
    __tr_z_buffer[i] = 1.0f; // Far plane value (normalized device coordinates)
  }

  TR_Matrix4x4 model = TR_MatrixIdentity();
  model = TR_MatrixMultiply(model, TR_MatrixScale(size.x, size.y, size.z));
  model = TR_MatrixMultiply(model, TR_MatrixRotateX(rotation_radians.x));
  model = TR_MatrixMultiply(model, TR_MatrixRotateY(rotation_radians.y));
  model = TR_MatrixMultiply(model, TR_MatrixRotateZ(rotation_radians.z));
  model = TR_MatrixMultiply(model, TR_MatrixTranslate(position.x, position.y, position.z));

  // Simple fixed camera (view matrix)
  TR_Matrix4x4 view = TR_MatrixTranslate(0.0f, 0.0f, -5.0f); // Move camera back

  // Perspective projection matrix (adjust FOV and aspect ratio)
  float aspect_ratio = (float)__tr_buffer_width / __tr_buffer_height;
  aspect_ratio *= 0.5f; // Compensate for character aspect ratio

  TR_Matrix4x4 projection = TR_MatrixPerspective(45.0f * (float)M_PI / 180.0f, aspect_ratio, 0.1f, 100.0f);

  TR_Matrix4x4 mvp_matrix = TR_MatrixMultiply(TR_MatrixMultiply(model, view), projection);

  for (int i = 0; i < __tr_num_cube_faces; ++i) {
    TR_Triangle face = __tr_cube_faces[i];
    TR_DrawTriangle3DFilled(
      __tr_cube_vertices[face.v[0]],
      __tr_cube_vertices[face.v[1]],
      __tr_cube_vertices[face.v[2]],
      mvp_matrix,
      color
    );
  }
}

#endif // TR_3D

#endif // TREAD_H
