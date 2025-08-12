#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
  #include <windows.h>
  #define PATH_SEP '\\'
#else
  #include <sys/stat.h>
  #include <unistd.h>
  #include <limits.h>
  #define PATH_SEP '/'
#endif

// Define PATH_MAX if not available (some compilers/systems might not define it)
#ifndef PATH_MAX
  #define PATH_MAX 4096
#endif

// Function to check if a directory exists
int dir_exists(const char* path) {
  #ifdef _WIN32
    DWORD attrib = GetFileAttributesA(path);
    return (attrib != INVALID_FILE_ATTRIBUTES && (attrib & FILE_ATTRIBUTE_DIRECTORY));
  #else
    struct stat st;
    return (stat(path, &st) == 0 && S_ISDIR(st.st_mode));
  #endif
}

int main() {
  const char* dist_folder = "dist";
  const char* output_zip_name = "dist.zip";

  #ifdef _WIN32
    const char* final_zip_name = "tread-bin-WIN.zip";
  #else
    const char* final_zip_name = "tread-bin-UNIX.zip";
  #endif

  printf("Checking for '%s' folder...\n", dist_folder);

  if (!dir_exists(dist_folder)) {
    fprintf(stderr, "Error: '%s' folder not found in the current directory.\n", dist_folder);
    return 1;
  }

  printf("'%s' folder found. Creating zip archive...\n", dist_folder);

  // Buffer to hold the system command string
  char command[PATH_MAX * 2]; // Enough space for command and paths

#ifdef _WIN32
  // Windows: Use PowerShell to compress the archive
  // Command: powershell.exe -Command "Compress-Archive -Path 'dist' -DestinationPath 'dist.zip'"
  // Note: PowerShell paths can often use '/' too, but '\\' is safer for robustness.
  snprintf(command, sizeof(command),
       "powershell.exe -Command \"Compress-Archive -Path '%s' -DestinationPath '%s'\"",
       dist_folder, output_zip_name);
#else
  // POSIX: Use the 'zip' command
  // Command: zip -r dist.zip dist
  // -r : recursive (for directories)
  snprintf(command, sizeof(command), "zip -r %s %s", output_zip_name, dist_folder);
#endif

  printf("Executing command: %s\n", command);

  // Execute the compression command
  int result = system(command);

  // Check the result of the system call
  if (result != 0) {
    fprintf(stderr, "Error: Failed to create zip archive. Command exited with code %d.\n", result);
    fprintf(stderr, "Please ensure '%s' (for POSIX) or PowerShell (for Windows) is available in your PATH.\n",
        #ifdef _WIN32
        "powershell.exe"
        #else
        "zip"
        #endif
         );
    return 1;
  }

  printf("Successfully created '%s'.\n", output_zip_name);

  if (rename(output_zip_name, final_zip_name) != 0) {
    perror("Error renaming zip file"); // Prints system error message
    fprintf(stderr, "Failed to rename '%s' to '%s'.\n", output_zip_name, final_zip_name);
    return 1;
  }

  printf("Renamed '%s' to '%s'. Operation complete.\n", output_zip_name, final_zip_name);

  return 0;
}
