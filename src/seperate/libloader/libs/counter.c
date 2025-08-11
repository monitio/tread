#include "../../../tread.h"

// Function to increment a string representation of a number by 1.
// Returns a new, dynamically allocated string.
// The caller is responsible for freeing the returned string.
char* increment_string_number(const char* num_str) {
  int len = strlen(num_str);
  // Allocate enough space for a potential new leading digit + null terminator
  char* temp_buf = (char*)malloc(len + 2);
  if (temp_buf == NULL) {
    perror("Failed to allocate memory for temporary buffer");
    exit(EXIT_FAILURE); // Abort on critical memory allocation failure
  }
  // Ensure the entire buffer is clean before use, especially for `memcpy`
  memset(temp_buf, '0', len + 1);
  temp_buf[len + 1] = '\0'; // Explicitly null-terminate at the end

  int carry = 1; // Start with a carry of 1 to increment the number

  // Iterate from the rightmost digit to the left
  for (int i = len - 1; i >= 0; i--) {
    int digit = (num_str[i] - '0') + carry; // Get digit value and add carry
    temp_buf[i + 1] = (digit % 10) + '0';   // Store the new digit in the result buffer (offset by 1)
    carry = digit / 10;           // Calculate new carry

    if (carry == 0) {
      // If no more carry, copy the unchanged prefix of the original number
      // to the beginning of temp_buf. This avoids processing digits that won't change.
      memcpy(temp_buf, num_str, i + 1); // Copy up to and including 'i'
      break; // No more carry, so we're done processing digits
    }
  }

  char* final_result;
  if (carry > 0) {
    // If there's still a carry after the loop (e.g., "99" becomes "100"),
    // it means the number's length has increased.
    temp_buf[0] = carry + '0'; // The carry becomes the new leading digit
    final_result = temp_buf; // The allocated `temp_buf` is now the final string
  } else {
    // If no carry remains, the length of the number hasn't changed.
    // The effective number in `temp_buf` starts from index 1.
    // We need to create a new string of correct size and copy the content.
    final_result = (char*)malloc(len + 1); // Allocate exactly for the original length + null terminator
    if (final_result == NULL) {
      perror("Failed to allocate memory for final number string");
      free(temp_buf); // Free the temporary buffer
      exit(EXIT_FAILURE);
    }
    strcpy(final_result, temp_buf + 1); // Copy the content from temp_buf+1 to final_num_str
    free(temp_buf); // Free the larger temporary buffer
  }

  return final_result;
}


// This is the function the loader will look for and call
void run_lib_app() {
  // Get the actual screen dimensions from the terminal, which tread.h can now query.
  // This allows the DLL to adapt to the size of the terminal it's launched in.
  int app_target_width = TR_GetScreenWidth();
  int app_target_height = TR_GetScreenHeight();

  // Initialize the TUI window with the actual dimensions.
  TR_InitWindow(app_target_width, app_target_height, "Infinite Counter TUI App");
  TR_SetTargetFPS(60); // Adjust FPS if needed for faster/slower counting (Increased for faster update)

  // Start with "0", dynamically allocated.
  // This string will grow as the number increments.
  char* count_str = strdup("0");
  if (!count_str) {
    perror("Failed to allocate initial count string");
    TR_CloseWindow();
    return;
  }

  // Main application loop: runs until 'Q' or 'ESC' is pressed
  while (!TR_WindowShouldClose()) {
    TR_BeginDrawing();
    TR_ClearBackground(BLUE);

    // Re-query screen width (for robustness, though tread.h exits on resize)
    int current_display_width = TR_GetScreenWidth();

    char display_msg[current_display_width + 1]; // Buffer for the entire display line
    const char* prefix = "Infinite Count: ";
    int prefix_len = strlen(prefix);

    // Calculate available width for the number part, leaving 2 characters for padding
    int available_width_for_number = current_display_width - prefix_len - 2;
    if (available_width_for_number < 0) available_width_for_number = 0;

    // The number is truly infinite. If it's too long for the current terminal width,
    // it will be truncated with "..." to show the most recent digits.
    if ((int)strlen(count_str) > available_width_for_number) {
      // If the number is too long, display a truncated version with "..."
      const char* ellipsis = "...";
      int ellipsis_len = strlen(ellipsis);
      // Ensure there's space for ellipsis + at least one digit
      int num_chars_to_show = available_width_for_number - ellipsis_len;
      if (num_chars_to_show < 1) num_chars_to_show = 1; // Always show at least one digit if space allows

      // Display ellipsis and the last 'num_chars_to_show' digits
      // Ensure the start index for count_str is valid
      int start_idx_int = (int)strlen(count_str) - num_chars_to_show;
      if (start_idx_int < 0) start_idx_int = 0; // Defensive check for start index

      snprintf(display_msg, sizeof(display_msg), "%s%s%s", prefix, ellipsis,
           count_str + start_idx_int);
    } else {
      // If the number fits, display it fully
      snprintf(display_msg, sizeof(display_msg), "%s%s", prefix, count_str);
    }

    TR_DrawText(display_msg, 5, 5, 10, RAYWHITE, BLUE); // Font size 10 (ignored by tread.h)

    TR_DrawText("Press Q or ESC to exit this app.", 5, 7, 10, LIGHTGRAY, BLUE);
    TR_EndDrawing();

    // Increment the number string and manage memory
    char* next_count_str = increment_string_number(count_str);
    free(count_str); // Free the old string to prevent memory leaks
    count_str = next_count_str; // Point to the new, incremented string
  }

  free(count_str); // Free the final count string before the app exits
  TR_CloseWindow(); // Close the TUI window
}

// A `main` function is typically not needed when this code is compiled as a dynamic library,
// as the `run_lib_app` function is called directly by the loader.
// int main() {
//   run_lib_app();
//   return 0;
// }
