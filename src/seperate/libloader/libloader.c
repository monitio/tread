#include "../../tread.h"

#include <ctype.h>  // For tolower
#include <errno.h>  // For errno in nanosleep (used by tread.h internally, and for safety)

// --- Platform-Specific Includes and Definitions for Dynamic Loading and File I/O ---

#ifdef _WIN32
  #include <windows.h> // For LoadLibrary, GetProcAddress, FreeLibrary
  #include <direct.h>  // For _getcwd, _chdir
  #include <io.h>    // For _findfirst, _findnext, _findclose
  #define PATH_SEP '\\'
  #define DLL_EXT ".dll"
  #define GET_CURRENT_DIR _getcwd
  #define CHDIR _chdir
  #define DLOPEN LoadLibraryA
  #define DLSYM GetProcAddress
  #define DLCLOSE FreeLibrary
  #define MAX_PATH_LENGTH _MAX_PATH // Windows max path length
  typedef HMODULE lib_handle_t;
  #define snprintf _snprintf // Microsoft's snprintf is _snprintf
#else
  #include <dlfcn.h>   // For dlopen, dlsym, dlsym_error, dlclose
  #include <unistd.h>  // For getcwd, chdir
  #include <dirent.h>  // For opendir, readdir, closedir
  #include <limits.h>  // For PATH_MAX
  #define PATH_SEP '/'
  #define DLL_EXT ".so"
  #define GET_CURRENT_DIR getcwd
  #define CHDIR chdir
  #define DLOPEN(path) dlopen(path, RTLD_LAZY) // RTLD_LAZY for lazy binding
  #define DLSYM dlsym
  #define DLCLOSE dlclose
  #define MAX_PATH_LENGTH PATH_MAX // POSIX max path length
  typedef void* lib_handle_t;
#endif

// --- Configuration ---
#define MAX_LIBS 10 // Maximum number of libraries that can be loaded simultaneously
#define MAX_FILE_ENTRIES 100 // Maximum number of files/directories to list
#define RUN_FUNCTION_NAME "run_lib_app" // The exact name of the function to call in DLLs/SOs
// The following SCREEN_WIDTH and SCREEN_HEIGHT are now primarily for initial window sizing,
// but the UI drawing logic will dynamically adapt to actual terminal size.
#define INITIAL_SCREEN_WIDTH 100 // Initial logical screen width for the loader TUI
#define INITIAL_SCREEN_HEIGHT 30 // Initial logical screen height for the loader TUI
#define LOADER_FPS 10 // FPS for the file manager UI

// --- Data Structures ---

// Represents an entry in the file manager
typedef struct {
  char name[MAX_PATH_LENGTH]; // Name of the file/directory
  bool is_directory;     // True if it's a directory
  bool is_loadable_lib;    // True if it's a .dll/.so
} FileEntry;

// Represents a dynamically loaded library
typedef struct {
  char name[MAX_PATH_LENGTH]; // Full path to the library file
  lib_handle_t handle;    // OS-specific handle to the loaded library
  void (*run_function)();  // Pointer to the 'run_lib_app' function
  char hotkey;         // Hotkey assigned to run this library
  bool is_loaded;      // True if the library is currently loaded
} LoadedLibrary;

// --- Global Variables ---
static char current_path[MAX_PATH_LENGTH];
static FileEntry current_dir_entries[MAX_FILE_ENTRIES];
static int num_dir_entries = 0;
static int selected_entry_index = 0;

static LoadedLibrary loaded_libs[MAX_LIBS];
static int num_loaded_libs = 0;

static bool running = true; // Main loop flag

// --- Function Prototypes ---
// File Manager Functions
static void InitFileManager();
static void UpdateFileManager(); // Not used in current main loop, but kept for consistency
static void DrawFileManager();
static void RefreshCurrentDirectory();
static int CompareFileEntries(const void* a, const void* b); // For sorting file entries

// Dynamic Library Loader Functions
static void LoadDynamicLibrary(const char* path);
static void RunLoadedLibrary(char hotkey);
static void UnloadAllLibraries();

// Utility Functions
static void GetParentPath(char* dest, const char* path);
static bool IsLoadableLibrary(const char* filename);
static char GetNextAvailableHotkey();
static void DisplayMessage(const char* message, Color color, int duration_ms); // For temporary messages
static bool ShowYesNoPrompt(const char* message, Color color); // New: Yes/No prompt

// --- Main Program ---
int main() {
  // Initialize window with initial dimensions; drawing will adapt to actual screen size
  TR_InitWindow(INITIAL_SCREEN_WIDTH, INITIAL_SCREEN_HEIGHT, "Tread.h Library Loader");
  TR_SetTargetFPS(LOADER_FPS);

  InitFileManager();

  while (running && !TR_WindowShouldClose()) {
    TR_BeginDrawing();
    TR_ClearBackground(DARKGRAY); // Dark background for the loader UI

    DrawFileManager();

    int key = TR_GetKeyPressed();
    if (key != 0) {
      switch (key) {
        case TR_KEY_UP:
          selected_entry_index = (selected_entry_index - 1 + num_dir_entries) % num_dir_entries;
          break;
        case TR_KEY_DOWN:
          selected_entry_index = (selected_entry_index + 1) % num_dir_entries;
          break;
        case 8: // Backspace key (common ASCII for backspace)
        case 127: // Another common ASCII for backspace (e.g., on Linux terminals)
          {
            char parent_path[MAX_PATH_LENGTH];
            GetParentPath(parent_path, current_path);
            if (CHDIR(parent_path) == 0) {
              strcpy(current_path, parent_path);
              RefreshCurrentDirectory();
            } else {
              DisplayMessage("Error: Cannot go up a directory.", RED, 1000);
            }
          }
          break;
        case 13: // Enter key
          if (num_dir_entries > 0) {
            FileEntry* selected_entry = &current_dir_entries[selected_entry_index];
            if (selected_entry->is_directory) {
              if (strcmp(selected_entry->name, "..") == 0) {
                // Go up one directory
                char parent_path[MAX_PATH_LENGTH];
                GetParentPath(parent_path, current_path);
                if (CHDIR(parent_path) == 0) {
                  strcpy(current_path, parent_path);
                  RefreshCurrentDirectory();
                } else {
                  DisplayMessage("Error: Cannot go up a directory.", RED, 1000);
                }
              } else if (strcmp(selected_entry->name, ".") == 0) {
                // Do nothing for current directory
              } else {
                // Enter selected directory
                char new_path[MAX_PATH_LENGTH];
                #ifdef _WIN32
                  // For Windows, append with backslash if current_path doesn't end with one
                  snprintf(new_path, sizeof(new_path), "%s%c%s", current_path, PATH_SEP, selected_entry->name);
                #else
                  // For POSIX, append with slash if current_path doesn't end with one
                  snprintf(new_path, sizeof(new_path), "%s%s%s", current_path, (current_path[strlen(current_path)-1] == PATH_SEP ? "" : "/"), selected_entry->name);
                #endif
                if (CHDIR(new_path) == 0) {
                  strcpy(current_path, new_path);
                  RefreshCurrentDirectory();
                } else {
                  DisplayMessage("Error: Cannot enter directory.", RED, 1000);
                }
              }
            } else if (selected_entry->is_loadable_lib) {
              // Display warning message and prompt
              const char* warning_msg =
                "Always make sure you have checked the source of the code if you downloaded the DLL off the internet and always also check the libraries for malware first with a responsible malware checker like your installed antivirus or (recommended more) VirusTotal (https://www.virustotal.com/).\n\nLoad this library? (Y/N)";

              bool proceed = ShowYesNoPrompt(warning_msg, YELLOW);

              RefreshCurrentDirectory(); // Refresh file list to redraw properly

              if (proceed) {
                char full_lib_path[MAX_PATH_LENGTH];
                #ifdef _WIN32
                  snprintf(full_lib_path, sizeof(full_lib_path), "%s%c%s", current_path, PATH_SEP, selected_entry->name);
                #else
                  snprintf(full_lib_path, sizeof(full_lib_path), "%s%s%s", current_path, (current_path[strlen(current_path)-1] == PATH_SEP ? "" : "/"), selected_entry->name);
                #endif
                LoadDynamicLibrary(full_lib_path);
              } else {
                DisplayMessage("Library loading cancelled.", RED, 1500);
              }
            } else {
              DisplayMessage("Not a loadable library or directory.", YELLOW, 1000);
            }
          }
          break;
        case 'q': // Quit
        case 27:  // ESC
          running = false;
          break;
        default:
          // Check for hotkeys '1' to '9', then 'a' to 'z'
          if ((key >= '1' && key <= '9') || (tolower(key) >= 'a' && tolower(key) <= 'z')) {
            RunLoadedLibrary((char)key);
          }
          break;
      }
    }

    TR_EndDrawing();
  }

  UnloadAllLibraries();
  TR_CloseWindow();
  return 0;
}

// --- File Manager Functions ---

// Initializes the file manager, gets current working directory
static void InitFileManager() {
  if (GET_CURRENT_DIR(current_path, sizeof(current_path)) == NULL) {
    perror("Error getting current directory");
    strcpy(current_path, "."); // Default to current directory if error
  }
  RefreshCurrentDirectory();
}

// This function is not explicitly used in the main loop, but could be for future updates.
static void UpdateFileManager() {
  // Logic for updating file manager state (e.g., polling directory changes)
  // For now, RefreshCurrentDirectory is called only on navigation events.
}

// Refreshes the list of files and directories in the current_path
static void RefreshCurrentDirectory() {
  num_dir_entries = 0;
  selected_entry_index = 0; // Reset selection

  // Add ".." for parent directory navigation
  // This handles Windows drive roots (e.g., C:\) correctly as their parent is themselves
  // and POSIX root (/) parent is itself.
  char temp_parent_path[MAX_PATH_LENGTH];
  GetParentPath(temp_parent_path, current_path);
  if (strcmp(temp_parent_path, current_path) != 0) { // If parent is different, add ".."
    if (num_dir_entries < MAX_FILE_ENTRIES) {
      strcpy(current_dir_entries[num_dir_entries].name, "..");
      current_dir_entries[num_dir_entries].is_directory = true;
      current_dir_entries[num_dir_entries].is_loadable_lib = false;
      num_dir_entries++;
    }
  }


#ifdef _WIN32
  WIN32_FIND_DATAA find_data;
  HANDLE h_find;
  char search_path[MAX_PATH_LENGTH];
  snprintf(search_path, sizeof(search_path), "%s%c*", current_path, PATH_SEP);

  h_find = FindFirstFileA(search_path, &find_data);
  if (h_find == INVALID_HANDLE_VALUE) {
    DisplayMessage("Error: Could not list directory contents.", RED, 2000);
    return;
  }

  do {
    if (strcmp(find_data.cFileName, ".") == 0 || strcmp(find_data.cFileName, "..") == 0) {
      continue; // Skip "." and ".." (already added ".." if applicable)
    }

    if (num_dir_entries < MAX_FILE_ENTRIES) {
      strcpy(current_dir_entries[num_dir_entries].name, find_data.cFileName);
      current_dir_entries[num_dir_entries].is_directory = (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
      current_dir_entries[num_dir_entries].is_loadable_lib = IsLoadableLibrary(find_data.cFileName);
      num_dir_entries++;
    }
  } while (FindNextFileA(h_find, &find_data) != 0);

  FindClose(h_find);

#else // POSIX
  DIR* dir = opendir(current_path);
  if (dir == NULL) {
    perror("Error opening directory");
    DisplayMessage("Error: Could not list directory contents.", RED, 2000);
    return;
  }

  struct dirent* entry;
  while ((entry = readdir(dir)) != NULL) {
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
      continue; // Skip "." and ".." (already added ".." if applicable)
    }

    if (num_dir_entries < MAX_FILE_ENTRIES) {
      strcpy(current_dir_entries[num_dir_entries].name, entry->d_name);
      current_dir_entries[num_dir_entries].is_directory = (entry->d_type == DT_DIR);
      current_dir_entries[num_dir_entries].is_loadable_lib = IsLoadableLibrary(entry->d_name);
      num_dir_entries++;
    }
  }
  closedir(dir);
#endif

  // Sort entries: directories first, then regular files, alphabetically
  qsort(current_dir_entries, num_dir_entries, sizeof(FileEntry), CompareFileEntries);
}

// Comparison function for qsort to sort file entries
static int CompareFileEntries(const void* a, const void* b) {
  const FileEntry* entryA = (const FileEntry*)a;
  const FileEntry* entryB = (const FileEntry*)b;

  // ".." should always be first if present
  if (strcmp(entryA->name, "..") == 0) return -1;
  if (strcmp(entryB->name, "..") == 0) return 1;

  // Directories come before files
  if (entryA->is_directory && !entryB->is_directory) return -1;
  if (!entryA->is_directory && entryB->is_directory) return 1;

  // Otherwise, sort alphabetically
  return strcmp(entryA->name, entryB->name);
}


// Draws the file manager UI
static void DrawFileManager() {
  int screen_width = TR_GetScreenWidth();
  int screen_height = TR_GetScreenHeight();

  // Use padding for the main UI box, adapting to actual screen size
  const int PADDING_X = 2; // Horizontal padding
  const int PADDING_Y = 2; // Vertical padding

  int ui_width = screen_width - (PADDING_X * 2);
  int ui_height = screen_height - (PADDING_Y * 2);

  // Ensure minimum dimensions if terminal is too small
  if (ui_width < 60) ui_width = 60; // Minimum width
  if (ui_height < 20) ui_height = 20; // Minimum height

  int start_x = PADDING_X;
  int start_y = PADDING_Y;

  // Draw border
  TR_DrawRectangleLines(start_x, start_y, ui_width, ui_height, RAYWHITE, BLANK);
  TR_DrawText(" Dynamic Library Loader ", start_x + (ui_width - strlen(" Dynamic Library Loader ")) / 2, start_y, 10, GREEN, DARKGRAY);

  // Draw current path
  char path_display[MAX_PATH_LENGTH + 20];
  snprintf(path_display, sizeof(path_display), "Path: %s", current_path);
  TR_DrawText(path_display, start_x + 2, start_y + 2, 10, LIGHTGRAY, DARKGRAY);
  TR_DrawRectangle(start_x + 1, start_y + 3, ui_width - 2, 1, DARKGRAY, DARKGRAY); // Separator line

  // Draw file entries
  int list_start_y = start_y + 5;
  int max_list_items = ui_height - 10; // Reserve space for header, path, loaded libs, footer

  for (int i = 0; i < num_dir_entries && i < max_list_items; ++i) {
    int draw_y = list_start_y + i;
    Color fg_color = LIGHTGRAY;
    Color bg_color = DARKGRAY;
    char prefix = ' ';

    if (i == selected_entry_index) {
      fg_color = YELLOW;
      bg_color = BLUE;
    }

    if (current_dir_entries[i].is_directory) {
      fg_color = CYAN; // Directories in Cyan
      prefix = '/';
    } else if (current_dir_entries[i].is_loadable_lib) {
      fg_color = LIME; // Loadable libraries in Lime Green
      prefix = '*';
    }

    char entry_text[MAX_PATH_LENGTH + 5];
    // Ensure text does not exceed available width
    snprintf(entry_text, sizeof(entry_text), "%c %s", prefix, current_dir_entries[i].name);
    int entry_text_len = strlen(entry_text);
    int available_text_width = ui_width - 4; // 2 for prefix/padding, 2 for right padding
    if (entry_text_len > available_text_width) {
      entry_text[available_text_width - 3] = '.'; // Truncate and add ellipsis
      entry_text[available_text_width - 2] = '.';
      entry_text[available_text_width - 1] = '.';
      entry_text[available_text_width] = '\0';
    }
    TR_DrawText(entry_text, start_x + 2, draw_y, 10, fg_color, bg_color);
  }

  // Calculate position for loaded libraries section based on dynamic height
  int loaded_libs_start_y = start_y + ui_height - 5;
  // Draw loaded libraries section
  TR_DrawRectangle(start_x + 1, loaded_libs_start_y - 1, ui_width - 2, 1, DARKGRAY, DARKGRAY); // Separator
  TR_DrawText("Loaded Libraries:", start_x + 2, loaded_libs_start_y, 10, RAYWHITE, DARKGRAY);

  for (int i = 0; i < MAX_LIBS; ++i) {
    if (loaded_libs[i].is_loaded) {
      char lib_info[MAX_PATH_LENGTH + 50];
      snprintf(lib_info, sizeof(lib_info), "[%c] %s", loaded_libs[i].hotkey, loaded_libs[i].name);
      int lib_info_len = strlen(lib_info);
      int available_lib_info_width = ui_width - 4;
      if (lib_info_len > available_lib_info_width) {
        lib_info[available_lib_info_width - 3] = '.';
        lib_info[available_lib_info_width - 2] = '.';
        lib_info[available_lib_info_width - 1] = '.';
        lib_info[available_lib_info_width] = '\0';
      }
      TR_DrawText(lib_info, start_x + 2, loaded_libs_start_y + 1 + i, 10, GOLD, DARKGRAY);
    }
  }

  // Draw instructions based on dynamic height
  TR_DrawText("Arrows: Navigate | Enter: Open/Load | Backspace: Up | Hotkey: Run | Q/ESC: Quit",
        start_x + 2, start_y + ui_height - 2, 10, WHITE, DARKGRAY);
}


// --- Dynamic Library Loader Functions ---

// Loads a dynamic library from the specified path
static void LoadDynamicLibrary(const char* path) {
  if (num_loaded_libs >= MAX_LIBS) {
    DisplayMessage("Max loaded libraries reached!", RED, 1500);
    return;
  }

  // Check if this library is already loaded
  for (int i = 0; i < num_loaded_libs; ++i) {
    if (loaded_libs[i].is_loaded && strcmp(loaded_libs[i].name, path) == 0) {
      DisplayMessage("Library already loaded!", YELLOW, 1000);
      return;
    }
  }

  lib_handle_t handle = DLOPEN(path);
  if (handle == NULL) {
    #ifdef _WIN32
      char msg[256];
      snprintf(msg, sizeof(msg), "LoadLibrary failed: Error %lu", GetLastError());
      DisplayMessage(msg, RED, 2000);
    #else
      const char* error_msg = dlerror(); // Get the error message
      if (error_msg == NULL) error_msg = "Unknown dlopen error"; // Fallback
      DisplayMessage(error_msg, RED, 2000);
    #endif
    return;
  }

  // Cast to (void*) to silence compiler warnings before casting to function pointer
  void (*run_func)() = (void (*)())DLSYM(handle, RUN_FUNCTION_NAME);
  if (run_func == NULL) {
    DLCLOSE(handle); // Unload if function not found
    #ifdef _WIN32
      char msg[256];
      snprintf(msg, sizeof(msg), "GetProcAddress failed for '%s': Error %lu", RUN_FUNCTION_NAME, GetLastError());
      DisplayMessage(msg, RED, 2000);
    #else
      const char* error_msg = dlerror(); // Get the error message
      if (error_msg == NULL) error_msg = "Unknown dlsym error"; // Fallback
      DisplayMessage(error_msg, RED, 2000);
    #endif
    return;
  }

  // Find an empty slot
  int slot = -1;
  for (int i = 0; i < MAX_LIBS; ++i) {
    if (!loaded_libs[i].is_loaded) {
      slot = i;
      break;
    }
  }

  if (slot == -1) { // Should not happen if max_libs check is correct, but as a safeguard
    DLCLOSE(handle);
    DisplayMessage("Internal error: No empty slot for library.", RED, 1500);
    return;
  }

  // Store library info
  strncpy(loaded_libs[slot].name, path, sizeof(loaded_libs[slot].name) - 1);
  loaded_libs[slot].name[sizeof(loaded_libs[slot].name) - 1] = '\0';
  loaded_libs[slot].handle = handle;
  loaded_libs[slot].run_function = run_func;
  loaded_libs[slot].hotkey = GetNextAvailableHotkey();
  loaded_libs[slot].is_loaded = true;
  num_loaded_libs++;

  char msg[256];
  snprintf(msg, sizeof(msg), "Loaded '%s' with hotkey '%c'", path, loaded_libs[slot].hotkey);
  DisplayMessage(msg, GREEN, 2000);
}

// Runs a loaded library based on its hotkey
static void RunLoadedLibrary(char hotkey) {
  int found_index = -1;
  for (int i = 0; i < MAX_LIBS; ++i) {
    // Compare hotkey case-insensitively
    if (loaded_libs[i].is_loaded && tolower((unsigned char)loaded_libs[i].hotkey) == tolower((unsigned char)hotkey)) {
      found_index = i;
      break;
    }
  }

  if (found_index != -1) {
    DisplayMessage("Launching library...", YELLOW, 500);
    // Important: Close tread.h window before calling external library to restore terminal state
    TR_CloseWindow();

    // Call the external library's run function
    loaded_libs[found_index].run_function();

    // Re-initialize tread.h window after external library exits
    // Use actual screen dimensions for re-initialization if tread.h handles this well.
    // Or revert to initial fixed size if it only works reliably that way.
    // For now, we revert to initial fixed size as a safer default if behavior is inconsistent.
    TR_InitWindow(INITIAL_SCREEN_WIDTH, INITIAL_SCREEN_HEIGHT, "Tread.h Library Loader");
    TR_SetTargetFPS(LOADER_FPS);
    DisplayMessage("Returned to loader.", GREEN, 1500);
  } else {
    char msg[64];
    snprintf(msg, sizeof(msg), "No library loaded for hotkey '%c'", hotkey);
    DisplayMessage(msg, RED, 1000);
  }
}

// Unloads all currently loaded libraries
static void UnloadAllLibraries() {
  for (int i = 0; i < MAX_LIBS; ++i) {
    if (loaded_libs[i].is_loaded) {
      DLCLOSE(loaded_libs[i].handle);
      loaded_libs[i].is_loaded = false;
    }
  }
  num_loaded_libs = 0;
}

// --- Utility Functions ---

// Gets the parent path of a given path
static void GetParentPath(char* dest, const char* path) {
  strncpy(dest, path, MAX_PATH_LENGTH);
  dest[MAX_PATH_LENGTH - 1] = '\0'; // Ensure null-termination

  // Handle special cases for root directories
  #ifdef _WIN32
    // Windows drive roots like "C:\" or "D:\"
    if (strlen(dest) >= 3 && dest[1] == ':' && dest[2] == PATH_SEP && strlen(dest) == 3) {
      return; // Parent of "C:\" is "C:\"
    }
    // UNC paths like "\\server" or "\\server\share"
    if (strlen(dest) >= 2 && dest[0] == PATH_SEP && dest[1] == PATH_SEP) {
      char* second_sep = strchr(dest + 2, PATH_SEP);
      if (second_sep == NULL) { // If no second separator, it's just \\server
        return; // Parent of "\\server" is itself
      } else if (strchr(second_sep + 1, PATH_SEP) == NULL) { // If only one more separator, it's \\server\share
        *second_sep = '\0'; // Truncate to \\server
        return;
      }
    }
  #else
    // POSIX root "/"
    if (strcmp(dest, "/") == 0) {
      return; // Parent of "/" is "/"
    }
  #endif

  // Find the last path separator
  char* last_sep = strrchr(dest, PATH_SEP);

  if (last_sep == NULL) {
    // No separator found, implies current directory.
    strcpy(dest, ".");
  } else if (last_sep == dest) {
    // Separator is at the beginning (e.g., /home/user -> /)
    // For POSIX, this means the root directory.
    strcpy(dest, "/");
  } else {
    *last_sep = '\0'; // Null-terminate at the last separator
  }
}


// Checks if a filename has the .dll or .so extension (case-insensitive)
static bool IsLoadableLibrary(const char* filename) {
  const char* dot = strrchr(filename, '.');
  if (dot && *(dot + 1) != '\0') {
    // Compare case-insensitively for robustness across platforms
    // (even if POSIX usually expects case-sensitive, this makes it more flexible)
    #ifdef _WIN32
      return (_stricmp(dot, DLL_EXT) == 0);
    #else
      // For POSIX, use `strcasecmp` for case-insensitive comparison
      // Or manually convert and compare if strcasecmp is not available on all systems.
      // Using `strcmp` here as `dlopen` typically expects correct casing on Linux.
      return (strcmp(dot, DLL_EXT) == 0);
    #endif
  }
  return false;
}

// Gets the next available hotkey (1-9, then a-z)
static char GetNextAvailableHotkey() {
  bool used[36] = {false}; // 0-9 (first 9 slots), then a-z (next 26 slots)

  for (int i = 0; i < MAX_LIBS; ++i) {
    if (loaded_libs[i].is_loaded) {
      char hk = tolower((unsigned char)loaded_libs[i].hotkey); // Use tolower with unsigned char
      if (hk >= '1' && hk <= '9') {
        used[hk - '1'] = true;
      } else if (hk >= 'a' && hk <= 'z') {
        used[hk - 'a' + 9] = true; // Offset for 'a' to 'z' hotkeys
      }
    }
  }

  for (int i = 0; i < 9; ++i) { // Hotkeys '1' to '9'
    if (!used[i]) {
      return (char)('1' + i);
    }
  }

  for (int i = 0; i < 26; ++i) { // Hotkeys 'a' to 'z'
    if (!used[i + 9]) { // Use offset 9 for 'a'
      return (char)('a' + i);
    }
  }

  return '\0'; // No hotkey available (should not happen if MAX_LIBS is reasonable)
}

// Displays a temporary message on the screen
static void DisplayMessage(const char* message, Color color, int duration_ms) {
  int screen_width = TR_GetScreenWidth();
  int screen_height = TR_GetScreenHeight();
  int text_len = (int)strlen(message);

  // Ensure message doesn't overflow screen width
  if (text_len > screen_width - 4) { // 4 chars for padding on both sides
    text_len = screen_width - 4;
  }

  int x_pos = (screen_width - text_len) / 2;
  int y_pos = screen_height - 3; // Near bottom

  TR_BeginDrawing();
  // Clear previous message area
  // Adjust rectangle width and height to cover potential multi-line messages if they were implemented
  TR_DrawRectangle(x_pos - 1, y_pos - 1, text_len + 2, 3, DARKGRAY, DARKGRAY);
  char truncated_message[256]; // Buffer for the potentially truncated message
  strncpy(truncated_message, message, text_len);
  truncated_message[text_len] = '\0'; // Null-terminate

  TR_DrawText(truncated_message, x_pos, y_pos, 10, color, DARKGRAY);
  TR_EndDrawing();

  // Use platform-specific sleep for duration
  #ifdef _WIN32
    Sleep((DWORD)duration_ms);
  #else
    struct timespec req = { .tv_sec = duration_ms / 1000, .tv_nsec = (long)((duration_ms % 1000) * 1000000) };
    struct timespec rem; // For remaining time if interrupted
    while (nanosleep(&req, &rem) == -1 && errno == EINTR) {
      req = rem; // Resume sleep if interrupted by signal
    }
  #endif
}

// Displays a multi-line message and waits for 'Y' or 'N' input.
// Returns true for 'Y' (yes), false for 'N' (no).
static bool ShowYesNoPrompt(const char* message, Color color) {
  int screen_width = TR_GetScreenWidth();
  int screen_height = TR_GetScreenHeight();

  // Calculate dynamic padding and minimums for the prompt box
  const int PROMPT_HORIZONTAL_PADDING = 3;
  const int PROMPT_VERTICAL_PADDING = 2; // Above and below text
  const int PROMPT_MIN_WIDTH = 50;
  const int PROMPT_MIN_HEIGHT = 10;

  // Split message into lines for drawing
  char message_copy[2048]; // A buffer large enough for the warning message
  strncpy(message_copy, message, sizeof(message_copy) - 1);
  message_copy[sizeof(message_copy) - 1] = '\0';

  char* lines[50]; // Max 50 lines for the prompt message
  int num_lines = 0;
  char* token = strtok(message_copy, "\n");
  while (token != NULL && num_lines < (sizeof(lines) / sizeof(lines[0]))) {
    lines[num_lines++] = token;
    token = strtok(NULL, "\n");
  }

  // Determine the max line length to calculate box width
  int max_text_line_len = 0;
  for (int i = 0; i < num_lines; ++i) {
    int len = strlen(lines[i]);
    if (len > max_text_line_len) max_text_line_len = len;
  }
  const char* prompt_text = "(Y/N)";
  if (strlen(prompt_text) > max_text_line_len) max_text_line_len = strlen(prompt_text);

  int box_width = max_text_line_len + (PROMPT_HORIZONTAL_PADDING * 2);
  int box_height = num_lines + (PROMPT_VERTICAL_PADDING * 2) + 1; // +1 for the (Y/N) line

  // Clamp box dimensions to minimums and screen limits
  if (box_width < PROMPT_MIN_WIDTH) box_width = PROMPT_MIN_WIDTH;
  if (box_height < PROMPT_MIN_HEIGHT) box_height = PROMPT_MIN_HEIGHT;

  if (box_width > screen_width - 4) box_width = screen_width - 4;
  if (box_height > screen_height - 4) box_height = screen_height - 4;

  int box_x = (screen_width - box_width) / 2;
  int box_y = (screen_height - box_height) / 2;

  bool input_received = false;
  bool result = false;

  while (!input_received) {
    TR_BeginDrawing();
    // Clear the entire screen before drawing the prompt box, to hide the underlying UI
    TR_ClearBackground(DARKGRAY);

    // Draw the prompt box
    TR_DrawRectangle(box_x, box_y, box_width, box_height, DARKGRAY, DARKGRAY); // Filled background
    TR_DrawRectangleLines(box_x, box_y, box_width, box_height, RAYWHITE, DARKGRAY); // Border

    // Draw each line of the message
    for (int i = 0; i < num_lines; ++i) {
      // Center each line within the box, adapting to actual box width
      int line_x = box_x + (box_width - strlen(lines[i])) / 2;
      if (line_x < box_x + PROMPT_HORIZONTAL_PADDING) line_x = box_x + PROMPT_HORIZONTAL_PADDING; // Ensure padding
      TR_DrawText(lines[i], line_x, box_y + PROMPT_VERTICAL_PADDING + i, 10, color, DARKGRAY);
    }

    // Draw the (Y/N) prompt
    int prompt_y = box_y + PROMPT_VERTICAL_PADDING + num_lines;
    int prompt_x = box_x + (box_width - strlen(prompt_text)) / 2;
    TR_DrawText(prompt_text, prompt_x, prompt_y, 10, LIGHTGRAY, DARKGRAY);

    TR_EndDrawing();

    int key = TR_GetKeyPressed();
    if (key != 0) {
      key = tolower((unsigned char)key);
      if (key == 'y') {
        result = true;
        input_received = true;
      } else if (key == 'n') {
        result = false;
        input_received = true;
      }
    }
  }
  return result;
}
