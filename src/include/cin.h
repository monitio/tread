// cin.h - A header-only library for simplified Windows API interactions
//
// A mix of "win" and "C" for easy use.

#ifndef CIN_H
#define CIN_H

#include <windows.h>

#include <commdlg.h> // For common dialogs (Open/Save File)

#include <stdarg.h> // For variadic arguments (va_list, va_start, va_end)
#include <stdio.h>  // For fprintf, vsnprintf
#include <time.h>   // For time_t, struct tm, time, localtime, strftime
#include <string.h> // For strftime, strcmp, wcslen
#include <stdlib.h> // For malloc, free, _strdup (for string duplication)

// --- Error Handling Macros (Optional but useful for debugging) ---
// These can be used to check for API call failures and print error messages.
#ifdef _DEBUG
#define CIN_CHECK_ERROR(result, msg) \
  if (!(result)) { \
    DWORD _cin_last_error_code = GetLastError(); \
    fprintf(stderr, "CIN Error: %s failed with code %lu at %s:%d\n", msg, _cin_last_error_code, __FILE__, __LINE__); \
  }
#else
  #define CIN_CHECK_ERROR(result, msg) (void)0 // No-op in release builds
#endif

// --- Simplified Message Box Type Aliases ---
// These macros map common MessageBoxA flags to more readable names.
// You can combine these using the bitwise OR operator (|).

// Button Types
#define cin_ok                MB_OK
#define cin_okcancel          MB_OKCANCEL
#define cin_abortretryignore  MB_ABORTRETRYIGNORE
#define cin_canceltrycontinue MB_CANCELTRYCONTINUE
#define cin_help              MB_HELP
#define cin_yesno             MB_YESNO
#define cin_yesnocancel       MB_YESNOCANCEL
#define cin_retrycancel       MB_RETRYCANCEL

// Icon Types
#define cin_info         MB_ICONINFORMATION
#define cin_questionmark MB_ICONQUESTION
#define cin_warning      MB_ICONWARNING
#define cin_error        MB_ICONERROR
#define cin_asterisk     MB_ICONASTERISK // Same as info
#define cin_exclamation  MB_ICONEXCLAMATION // Same as warning
#define cin_hand         MB_ICONHAND // Same as error
#define cin_stop         MB_ICONSTOP // Same as error

// Default Button (less common to specify, first button is default)
#define cin_defbutton1  MB_DEFBUTTON1
#define cin_defbutton2  MB_DEFBUTTON2
#define cin_defbutton3  MB_DEFBUTTON3
#define cin_defbutton4  MB_DEFBUTTON4

// Modality (less common for simple use, typically application modal)
#define cin_appmodal    MB_APPLMODAL
#define cin_systemmodal MB_SYSTEMMODAL
#define cin_taskmodal   MB_TASKMODAL

// --- Logging Function (lprintf) ---

// Define a maximum buffer size for the formatted log message.
// This is a common practice to prevent buffer overflows.
#define cin_LOG_BUFFER_SIZE 1024

/**
 * @brief A custom logging function similar to printf, but with
 * timestamp and a log type prefix.
 *
 * It's defined as static inline to allow it to be included in multiple
 * compilation units (C files) without causing multiple definition errors,
 * and to potentially allow the compiler to optimize calls.
 *
 * @param type A string indicating the type of log message (e.g., "INFO", "WARN", "ERROR").
 * @param format The format string for the log message (like printf).
 * @param ... Variable arguments for the format string.
 */
static inline void lprintf(const char* type, const char* format, ...) {
  char timeBuffer[64];
  time_t rawtime;
  struct tm *timeinfo;

  // Get current time and format it.
  time(&rawtime);
  timeinfo = localtime(&rawtime);
  // Format: [DD/MM/YY | HH:MI:SS]
  strftime(timeBuffer, sizeof(timeBuffer), "[%d/%m/%y | %H:%M:%S]", timeinfo);

  // Prepare for variadic arguments.
  va_list args;
  va_start(args, format);

  char msgBuffer[cin_LOG_BUFFER_SIZE];
  // Format the user's message into msgBuffer.
  // vsnprintf is used for safety to prevent buffer overflows.
  vsnprintf(msgBuffer, sizeof(msgBuffer), format, args);

  va_end(args); // Clean up va_list.

  // Print the final formatted log message to standard output.
  // Format: [TIMESTAMP] [LOG] [TYPE] MESSAGE
  fprintf(stdout, "%s [LOG] [%s] %s", timeBuffer, type, msgBuffer);
}

// --- Message Box Function ---

/**
 * @brief Displays a message box with the specified text, title, and type.
 *
 * This function provides a simplified interface to the Windows API's MessageBoxA function.
 * It uses ASCII characters, which are generally simpler for basic C applications.
 *
 * @param message The text to display in the message box.
 * @param title The title of the message box window.
 * @param type An integer representing the type of message box (e.g., buttons, icon).
 * Common values can now be used with the simplified aliases (e.g., `ok | info`).
 * @return The integer ID of the button pressed (e.g., IDOK, IDCANCEL, IDYES, IDNO).
 * Returns 0 if the function fails.
 */
static inline int cin_message_box(const char* message, const char* title, unsigned int type) {
  // Get a handle to the desktop window.
  // NULL indicates the message box is owned by the desktop.
  HWND hWnd = NULL;

  // Call the Windows API MessageBoxA function.
  // MessageBoxA is the ASCII version; MessageBoxW is the wide-character version.
  int result = MessageBoxA(hWnd, message, title, type);

  // Check for errors (optional, but good practice)
  CIN_CHECK_ERROR(result != 0, "MessageBoxA");

  return result;
}

// --- File Dialog Functions ---

/**
 * @brief Displays a standard Windows "Open File" dialog.
 *
 * @param out_filepath_buffer A character array buffer where the selected file path will be stored.
 * It should be large enough to hold the full path (e.g., MAX_PATH, which is 260).
 * @param buffer_size The size of the out_filepath_buffer in bytes.
 * @param filter A null-terminated string specifying the file filters (e.g., "Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0").
 * Each filter pair (description and pattern) is separated by a null character. The entire string ends with two null characters.
 * @param default_ext The default file extension to append if the user doesn't specify one (e.g., "txt"). Can be NULL.
 * @param initial_dir The initial directory to open the dialog in. Can be NULL for default.
 * @return TRUE if the user selected a file and clicked OK, FALSE otherwise (e.g., cancelled, error).
 */
static inline BOOL cin_open_file_dialog(char* out_filepath_buffer, DWORD buffer_size, const char* filter, const char* default_ext, const char* initial_dir) {
  OPENFILENAMEA ofn;     // Common dialog box structure
  ZeroMemory(&ofn, sizeof(ofn)); // Initialize structure to all zeros

  ofn.lStructSize = sizeof(ofn);
  ofn.hwndOwner = NULL; // Owner window, NULL for desktop
  ofn.lpstrFile = out_filepath_buffer;
  ofn.nMaxFile = buffer_size;
  ofn.lpstrFilter = filter;
  ofn.nFilterIndex = 1; // Default to the first filter
  ofn.lpstrFileTitle = NULL; // Can be used to get just the file name
  ofn.nMaxFileTitle = 0;
  ofn.lpstrInitialDir = initial_dir;
  ofn.lpstrTitle = "Open File"; // Dialog title
  ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR; // Standard flags

  // Set default extension if provided
  ofn.lpstrDefExt = default_ext;

  // Clear the buffer before use
  out_filepath_buffer[0] = '\0';

  // Display the Open dialog box.
  BOOL result = GetOpenFileNameA(&ofn);

  // Check for errors (0 means user cancelled, non-zero is an actual error code)
  if (!result) {
    DWORD error_code = CommDlgExtendedError(); // Get extended error for common dialogs
    if (error_code != 0) {
      lprintf("ERROR", "cin_open_file_dialog failed with extended error code %lu.\n", error_code);
    } else {
      lprintf("INFO", "cin_open_file_dialog cancelled by user.\n");
    }
  } else {
    lprintf("INFO", "cin_open_file_dialog selected file: %s\n", out_filepath_buffer);
  }

  return result;
}

/**
 * @brief Displays a standard Windows "Save File" dialog.
 *
 * @param out_filepath_buffer A character array buffer where the selected file path will be stored.
 * It should be large enough to hold the full path (e.g., MAX_PATH, which is 260).
 * @param buffer_size The size of the out_filepath_buffer in bytes.
 * @param filter A null-terminated string specifying the file filters (e.g., "Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0").
 * Each filter pair (description and pattern) is separated by a null character. The entire string ends with two null characters.
 * @param default_ext The default file extension to append if the user doesn't specify one (e.g., "txt"). Can be NULL.
 * @param initial_dir The initial directory to open the dialog in. Can be NULL for default.
 * @return TRUE if the user specified a file and clicked OK, FALSE otherwise (e.g., cancelled, error).
 */
static inline BOOL cin_save_file_dialog(char* out_filepath_buffer, DWORD buffer_size, const char* filter, const char* default_ext, const char* initial_dir) {
  OPENFILENAMEA ofn;     // Common dialog box structure
  ZeroMemory(&ofn, sizeof(ofn)); // Initialize structure to all zeros

  ofn.lStructSize = sizeof(ofn);
  ofn.hwndOwner = NULL; // Owner window, NULL for desktop
  ofn.lpstrFile = out_filepath_buffer;
  ofn.nMaxFile = buffer_size;
  ofn.lpstrFilter = filter;
  ofn.nFilterIndex = 1; // Default to the first filter
  ofn.lpstrFileTitle = NULL; // Can be used to get just the file name
  ofn.nMaxFileTitle = 0;
  ofn.lpstrInitialDir = initial_dir;
  ofn.lpstrTitle = "Save File As"; // Dialog title
  ofn.Flags = OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR; // Prompt if file exists, don't change current directory

  // Set default extension if provided
  ofn.lpstrDefExt = default_ext;

  // Clear the buffer before use
  out_filepath_buffer[0] = '\0';

  // Display the Save dialog box.
  BOOL result = GetSaveFileNameA(&ofn);

  // Check for errors (0 means user cancelled, non-zero is an actual error code)
  if (!result) {
    DWORD error_code = CommDlgExtendedError(); // Get extended error for common dialogs
    if (error_code != 0) {
      lprintf("ERROR", "cin_save_file_dialog failed with extended error code %lu.\n", error_code);
    } else {
      lprintf("INFO", "cin_save_file_dialog cancelled by user.\n");
    }
  } else {
    lprintf("INFO", "cin_save_file_dialog selected file: %s\n", out_filepath_buffer);
  }

  return result;
}

// --- Simplified Toast Notification (Window-based) ---
// This provides a non-blocking, auto-closing pop-up notification without COM/WinRT complexity.

// Define a unique window class name for our toast notifications
#define CIN_TOAST_CLASS_NAME "CinSimpleToastNotificationClass"
#define CIN_TOAST_TIMER_ID 1001 // Unique timer ID

// Structure to pass data to the notification thread
typedef struct {
  char* message; // Dynamically allocated
  char* title;   // Dynamically allocated
  DWORD duration_ms;
} CinToastParams;

// Window procedure for the toast notification window
LRESULT CALLBACK CinToastWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  static CinToastParams* s_params = NULL; // Static to hold parameters

  switch (uMsg) {
    case WM_CREATE: {
      // Get the parameters passed during window creation
      CREATESTRUCT* cs = (CREATESTRUCT*)lParam;
      s_params = (CinToastParams*)cs->lpCreateParams;

      // Set a timer to destroy the window after the specified duration
      SetTimer(hwnd, CIN_TOAST_TIMER_ID, s_params->duration_ms, NULL);

      // Position the window (e.g., bottom-right corner)
      // Get screen dimensions
      int screenWidth = GetSystemMetrics(SM_CXSCREEN);
      int screenHeight = GetSystemMetrics(SM_CYSCREEN);

      // Define notification window size (adjust as needed)
      int toastWidth = 350;
      int toastHeight = 120;
      int margin = 20;

      // Calculate position: bottom-right
      int x = screenWidth - toastWidth - margin;
      int y = screenHeight - toastHeight - margin;

      // Set window position and make it always on top
      SetWindowPos(hwnd, HWND_TOPMOST, x, y, toastWidth, toastHeight, SWP_SHOWWINDOW | SWP_NOACTIVATE);

      lprintf("INFO", "Toast notification window created: '%s' - '%s'\n", s_params->title, s_params->message);
      break;
    }
    case WM_PAINT: {
      PAINTSTRUCT ps;
      HDC hdc = BeginPaint(hwnd, &ps);

      // Fill background with a light color
      HBRUSH hBrush = CreateSolidBrush(RGB(240, 240, 240)); // Light gray
      FillRect(hdc, &ps.rcPaint, hBrush);
      DeleteObject(hBrush);

      // Draw border
      HPEN hPen = CreatePen(PS_SOLID, 1, RGB(180, 180, 180)); // Gray border
      HGDIOBJ hOldPen = SelectObject(hdc, hPen);
      Rectangle(hdc, ps.rcPaint.left, ps.rcPaint.top, ps.rcPaint.right, ps.rcPaint.bottom);
      SelectObject(hdc, hOldPen);
      DeleteObject(hPen);

      // Set text color and background mode
      SetTextColor(hdc, RGB(0, 0, 0)); // Black text
      SetBkMode(hdc, TRANSPARENT); // Transparent background for text

      // Draw title (bold)
      HFONT hFontTitle = CreateFontA(
        20, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
        OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        VARIABLE_PITCH, "Segoe UI"
      );
      HGDIOBJ hOldFont = SelectObject(hdc, hFontTitle);
      RECT titleRect = {10, 5, ps.rcPaint.right - 10, 30};
      DrawTextA(hdc, s_params->title, -1, &titleRect, DT_SINGLELINE | DT_VCENTER | DT_LEFT | DT_END_ELLIPSIS);
      SelectObject(hdc, hOldFont);
      DeleteObject(hFontTitle);

      // Draw message
      HFONT hFontMessage = CreateFontA(
        16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
        OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        VARIABLE_PITCH, "Segoe UI"
      );
      hOldFont = SelectObject(hdc, hFontMessage);
      RECT messageRect = {10, 35, ps.rcPaint.right - 10, ps.rcPaint.bottom - 10};
      DrawTextA(hdc, s_params->message, -1, &messageRect, DT_WORDBREAK | DT_LEFT | DT_TOP | DT_END_ELLIPSIS);
      SelectObject(hdc, hOldFont);
      DeleteObject(hFontMessage);

      EndPaint(hwnd, &ps);
      break;
    }
    case WM_TIMER:
      // Timer expired, destroy the window
      DestroyWindow(hwnd);
      break;
    case WM_DESTROY:
      // Free the dynamically allocated strings
      if (s_params) {
        if (s_params->message) free(s_params->message);
        if (s_params->title) free(s_params->title);
        free(s_params);
        s_params = NULL; // Clear static pointer
      }
      // Post quit message to end the thread's message loop
      PostQuitMessage(0);
      lprintf("INFO", "Toast notification window destroyed.\n");
      break;
  }
  return DefWindowProcA(hwnd, uMsg, wParam, lParam);
}

// Thread function to create and manage the toast notification window
DWORD WINAPI CinToastThreadProc(LPVOID lpParam) {
  CinToastParams* params = (CinToastParams*)lpParam;

  // Register the window class (only once per process)
  WNDCLASSEXA wc = {0};
  wc.cbSize = sizeof(WNDCLASSEXA);
  wc.lpfnWndProc = CinToastWndProc;
  wc.hInstance = GetModuleHandle(NULL);
  wc.lpszClassName = CIN_TOAST_CLASS_NAME;
  // Try to register the class. If it already exists, that's fine.
  if (!RegisterClassExA(&wc)) {
    // Check if the error is due to the class already being registered
    if (GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
      lprintf("ERROR", "Failed to register toast window class: %lu\n", GetLastError());
      // Free allocated memory if registration fails for other reasons
      if (params->message) free(params->message);
      if (params->title) free(params->title);
      free(params);
      return 1;
    }
  }

  // Create the notification window
  HWND hwnd = CreateWindowExA(
    WS_EX_TOPMOST | WS_EX_TOOLWINDOW, // Topmost, not in taskbar
    CIN_TOAST_CLASS_NAME,
    params->title, // Title is passed here, but drawn in WM_PAINT
    WS_POPUP | WS_VISIBLE, // Popup window, initially visible
    CW_USEDEFAULT, CW_USEDEFAULT, // Position set in WM_CREATE
    CW_USEDEFAULT, CW_USEDEFAULT, // Size set in WM_CREATE
    NULL, // No parent window
    NULL, // No menu
    GetModuleHandle(NULL),
    params // Pass parameters to WM_CREATE
  );

  if (!hwnd) {
    lprintf("ERROR", "Failed to create toast notification window: %lu\n", GetLastError());
    // Free allocated memory if window creation fails
    if (params->message) free(params->message);
    if (params->title) free(params->title);
    free(params);
    return 1;
  }

  // Message loop for this specific window/thread
  MSG msg;
  while (GetMessage(&msg, NULL, 0, 0)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  // Parameters are freed in WM_DESTROY
  return 0;
}

/**
 * @brief Displays a non-blocking, auto-closing "toast-like" notification using a custom window.
 * This is a simplified implementation using standard WinAPI, not full COM toast notifications.
 *
 * @param message The main text content of the notification.
 * @param title The title of the notification (e.g., "Success", "Warning").
 * @param duration_ms The duration in milliseconds before the notification automatically closes.
 * (e.g., 3000 for 3 seconds).
 */
static inline void cin_toast_notification(const char* message, const char* title, DWORD duration_ms) {
  // Allocate memory for parameters to pass to the new thread
  CinToastParams* params = (CinToastParams*)malloc(sizeof(CinToastParams));
  if (!params) {
    lprintf("ERROR", "Failed to allocate memory for toast notification parameters.\n");
    return;
  }

  // Duplicate the strings to ensure they are valid for the lifetime of the thread
  params->message = _strdup(message);
  params->title = _strdup(title);

  if (!params->message || !params->title) {
    lprintf("ERROR", "Failed to duplicate toast message or title strings.\n");
    if (params->message) free(params->message);
    if (params->title) free(params->title);
    free(params);
    return;
  }

  params->duration_ms = duration_ms;

  // Create a new thread to manage the notification window
  HANDLE hThread = CreateThread(
    NULL,           // Default security attributes
    0,            // Default stack size
    CinToastThreadProc,   // Thread function
    params,         // Argument to thread function
    0,            // Creation flags (0 for run immediately)
    NULL          // Optional: receives thread identifier
  );

  if (hThread == NULL) {
    lprintf("ERROR", "Failed to create toast notification thread: %lu\n", GetLastError());
    // Free params and duplicated strings if thread creation fails
    if (params->message) free(params->message);
    if (params->title) free(params->title);
    free(params);
  } else {
    // Close the thread handle immediately as we don't need to wait for it.
    // The thread will clean up its own resources.
    CloseHandle(hThread);
  }
}

// --- Custom Window Creation and Drawing ---

// Define a unique window class name for custom windows
#define CIN_CUSTOM_WINDOW_CLASS_NAME "CinCustomWindow"

// Type definition for the user's drawing callback function
typedef void (*CinDrawCallback)(HWND hwnd, HDC hdc, RECT clientRect, LPVOID user_data);

// Structure to hold parameters for the custom window
typedef struct {
  char* title;
  int width;
  int height;
  CinDrawCallback draw_callback;
  LPVOID user_data; // Generic pointer for user-defined data
} CinWindowParams;

// Window procedure for the custom window
LRESULT CALLBACK CinCustomWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  CinWindowParams* params = NULL;

  // Retrieve CinWindowParams from GWLP_USERDATA
  // This needs to be done carefully as GWLP_USERDATA is only set after WM_CREATE
  if (uMsg == WM_CREATE) {
    CREATESTRUCT* cs = (CREATESTRUCT*)lParam;
    params = (CinWindowParams*)cs->lpCreateParams;
    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)params);
  } else {
    params = (CinWindowParams*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
  }

  switch (uMsg) {
    case WM_PAINT: {
      PAINTSTRUCT ps;
      HDC hdc = BeginPaint(hwnd, &ps);
      RECT clientRect;
      GetClientRect(hwnd, &clientRect);

      // Call the user's drawing callback if it exists
      if (params && params->draw_callback) {
        params->draw_callback(hwnd, hdc, clientRect, params->user_data);
      } else {
        // Default background if no callback or params
        FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));
      }

      EndPaint(hwnd, &ps);
      break;
    }
    case WM_DESTROY:
      lprintf("INFO", "Custom window '%s' destroyed.\n", params ? params->title : "Unknown");
      // Free dynamically allocated title and params
      if (params) {
        if (params->title) free(params->title);
        free(params);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)NULL); // Clear pointer
      }
      PostQuitMessage(0); // Signal the message loop to exit
      break;
    default:
      return DefWindowProcA(hwnd, uMsg, wParam, lParam);
  }
  return 0;
}

/**
 * @brief Creates and runs a custom Windows window with a user-defined drawing callback.
 * This function is blocking, meaning it runs its own message loop and will not return
 * until the window is closed. To create a non-blocking window, call this function
 * from a separate thread.
 *
 * @param title The title of the window.
 * @param width The width of the window in pixels.
 * @param height The height of the window in pixels.
 * @param draw_callback A pointer to a function that will be called for custom drawing.
 * Can be NULL for a default window background.
 * @param user_data An optional pointer to user-defined data that will be passed to
 * the draw_callback. Can be NULL.
 * @return TRUE on successful window creation and message loop execution, FALSE on failure.
 */
static inline BOOL cin_create_custom_window(
  const char* title,
  int width,
  int height,
  CinDrawCallback draw_callback,
  LPVOID user_data)
{
  // Register the window class (only once per process)
  static BOOL classRegistered = FALSE;
  if (!classRegistered) {
    WNDCLASSEXA wc = {0};
    wc.cbSize    = sizeof(WNDCLASSEXA);
    wc.lpfnWndProc   = CinCustomWindowProc;
    wc.hInstance   = GetModuleHandle(NULL);
    wc.lpszClassName = CIN_CUSTOM_WINDOW_CLASS_NAME;
    wc.hCursor     = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1); // Default background

    if (!RegisterClassExA(&wc)) {
      lprintf("ERROR", "Failed to register custom window class: %lu\n", GetLastError());
      return FALSE;
    }
    classRegistered = TRUE;
  }

  // Allocate parameters structure to pass to WM_CREATE
  CinWindowParams* params = (CinWindowParams*)malloc(sizeof(CinWindowParams));
  if (!params) {
    lprintf("ERROR", "Failed to allocate memory for window parameters.\n");
    return FALSE;
  }
  params->title = _strdup(title); // Duplicate title string
  if (!params->title) {
    lprintf("ERROR", "Failed to duplicate window title string.\n");
    free(params);
    return FALSE;
  }
  params->width = width;
  params->height = height;
  params->draw_callback = draw_callback;
  params->user_data = user_data;

  // Create the window
  HWND hwnd = CreateWindowExA(
    0, // Optional window styles
    CIN_CUSTOM_WINDOW_CLASS_NAME,
    params->title, // Window title
    WS_OVERLAPPEDWINDOW | WS_VISIBLE, // Standard window styles
    CW_USEDEFAULT, CW_USEDEFAULT, // Default position
    width, height, // Size
    NULL, // No parent window
    NULL, // No menu
    GetModuleHandle(NULL),
    params // Pass params to WM_CREATE
  );

  if (!hwnd) {
    lprintf("ERROR", "Failed to create custom window: %lu\n", GetLastError());
    if (params->title) free(params->title);
    free(params);
    return FALSE;
  }

  // Show and update the window
  ShowWindow(hwnd, SW_SHOWDEFAULT);
  UpdateWindow(hwnd);

  // Message loop
  MSG msg;
  BOOL bRet;
  while ((bRet = GetMessageA(&msg, NULL, 0, 0)) != 0) {
    if (bRet == -1) {
      lprintf("ERROR", "GetMessageA failed: %lu\n", GetLastError());
      return FALSE;
    }
    TranslateMessage(&msg);
    DispatchMessageA(&msg);
  }

  return TRUE; // Window closed successfully (WM_QUIT received)
}

#endif // CIN_H
