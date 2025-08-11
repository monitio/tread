#include "./include/cin.h"

#include <stdio.h>  // For printf, fprintf
#include <stdlib.h> // For exit
#include <string.h> // For strcmp

// The main entry point of the application.
// argc: argument count (number of strings in argv)
// argv: argument vector (array of strings, where argv[0] is the program name)
int main(int argc, char *argv[]) {
  // Initialize type and content pointers to NULL or empty strings
  const char* log_type = NULL;
  const char* log_content = NULL;

  // Iterate through command-line arguments to find -t and -c
  for (int i = 1; i < argc; ++i) {
    if (strcmp(argv[i], "-t") == 0) {
      // Check if there's a next argument for the type value
      if (i + 1 < argc) {
        log_type = argv[i+1];
        i++; // Skip the next argument as it's been consumed
      } else {
        fprintf(stderr, "Error: -t requires an argument.\n");
        lprintf("ERROR", "Missing argument for -t. Usage: %s -t <type> -c <content>\n", argv[0]);
        return 1;
      }
    } else if (strcmp(argv[i], "-c") == 0) {
      // Check if there's a next argument for the content value
      if (i + 1 < argc) {
        log_content = argv[i+1];
        i++; // Skip the next argument as it's been consumed
      } else {
        fprintf(stderr, "Error: -c requires an argument.\n");
        lprintf("ERROR", "Missing argument for -c. Usage: %s -t <type> -c <content>\n", argv[0]);
        return 1;
      }
    } else {
      fprintf(stderr, "Error: Unrecognized argument '%s'.\n", argv[i]);
      lprintf("ERROR", "Unrecognized argument '%s'. Usage: %s -t <type> -c <content>\n", argv[i], argv[0]);
      return 1;
    }
  }

  // Check if both type and content were provided
  if (log_type == NULL || log_content == NULL) {
    fprintf(stderr, "Error: Both -t (type) and -c (content) arguments are required.\n");
    lprintf("ERROR", "Both -t and -c arguments are required. Usage: %s -t <type> -c <content>\n", argv[0]);
    return 1;
  }

  // Use lprintf with the provided arguments
  lprintf(log_type, "%s\n", log_content);

  return 0;
}
