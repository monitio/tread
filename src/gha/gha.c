#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
  #include <windows.h>
  #define PATH_SEP '\\'
  #define BUILD_SCRIPT "build.bat"
  #define PACKAGE_ZIP_EXEC "packagezip.exe"
#else
  #include <sys/stat.h>
  #include <unistd.h>
  #include <limits.h>
  #define PATH_SEP '/'
  #define BUILD_SCRIPT "build.sh"
  #define PACKAGE_ZIP_EXEC "packagezip"
#endif

// Define PATH_MAX if not available (some compilers/systems might not define it)
#ifndef PATH_MAX
  #define PATH_MAX 4096
#endif

// Function to check if a directory exists (kept for dir_exists(output_zip_name) usage if needed elsewhere, though now unused in main)
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
  // These constants are now mostly informational or for path construction,
  // as packagezip will handle the actual zip file creation and naming.
  // const char* dist_folder = "dist"; // No longer directly checked here as packagezip handles this
  // const char* output_zip_name = "dist.zip"; // packagezip handles actual creation
  const char* final_zip_name; // Declare here, define later based on platform

#ifdef _WIN32
  final_zip_name = "tread-bin-WIN.zip";
#else
  final_zip_name = "tread-bin-UNIX.zip";
#endif

  // --- Step 1: Run the build script ---
  char build_command[PATH_MAX * 2]; // Buffer to hold the command string

  // Construct the command to execute the build script with the -clang flag
#ifdef _WIN32
  snprintf(build_command, sizeof(build_command), ".%c%s -clang", PATH_SEP, BUILD_SCRIPT);
#else
  snprintf(build_command, sizeof(build_command), ".%c%s -clang", PATH_SEP, BUILD_SCRIPT);
#endif

  printf("Attempting to run build script: %s\n", build_command);

  // Execute the build script
  int build_result = system(build_command);

  // Check the return value of the system call for the build script
  if (build_result == 0) {
    printf("\nBuild script '%s' executed successfully.\n", BUILD_SCRIPT);
  } else {
    // system() returns -1 if command processor could not be invoked.
    // Otherwise, it returns the exit status of the command.
    fprintf(stderr, "\nError: Build script '%s' failed to execute or returned an error.\n", BUILD_SCRIPT);
    fprintf(stderr, "Please ensure the script exists, has execute permissions (on POSIX),\n");
    fprintf(stderr, "and is located in the same directory as this executable.\n");
    return EXIT_FAILURE;
  }

  // --- Step 2: Run the packagezip executable ---
  char packagezip_path[PATH_MAX]; // Buffer for the full path to packagezip
  char packagezip_command[PATH_MAX + 10]; // Buffer for the command to execute packagezip

  // Construct the path to the packagezip executable
  // This assumes packagezip will be located at ./dist/gha/packagezip(.exe)
  snprintf(packagezip_path, sizeof(packagezip_path), ".%cdist%cgha%c%s", PATH_SEP, PATH_SEP, PATH_SEP, PACKAGE_ZIP_EXEC);

  // For executing, we just need the path.
  // The `system()` function will execute it, and its output will also be shown.
  snprintf(packagezip_command, sizeof(packagezip_command), "%s", packagezip_path);

  printf("\nAttempting to run packaging tool: %s\n", packagezip_command);

  int packagezip_result = system(packagezip_command);

  // Check the return value of the system call for packagezip
  if (packagezip_result == 0) {
    printf("\nPackaging tool '%s' executed successfully.\n", PACKAGE_ZIP_EXEC);
    printf("Operation complete.\n");
    return EXIT_SUCCESS;
  } else {
    fprintf(stderr, "\nError: Packaging tool '%s' failed to execute or returned an error.\n", PACKAGE_ZIP_EXEC);
    fprintf(stderr, "Please ensure the '%s' executable exists at '%s',\n", PACKAGE_ZIP_EXEC, packagezip_path);
    fprintf(stderr, "and has execute permissions (on POSIX).\n");
    return EXIT_FAILURE;
  }
}
