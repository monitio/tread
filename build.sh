#!/bin/bash
set -e # Exit immediately if a command exits with a non-zero status.

GCC_FLAG=0
CLANG_FLAG=0

# Setup logging path
TEMP_LOG="/tmp/build_temp.log" # Using /tmp for temporary files on Unix-like systems
LOG_FILE="build.sh.log"

# Check if the existing log file is too large (1GB = 1073741824 bytes)
if [ -f "$LOG_FILE" ]; then
  FILE_SIZE=$(stat -c%s "$LOG_FILE")
  if [ "$FILE_SIZE" -gt 1073741824 ]; then
    echo "Error: Log file is too big to save (>1GB)"
    exit 1
  fi
fi

# Function to display usage
show_usage() {
  echo "Usage: build.sh [-gcc|-clang]"
  echo "Options:"
  echo "  -gcc    Build using GCC compiler"
  echo "  -clang  Build using Clang compiler"
  exit 1
}

# Parse arguments
while [[ "$#" -gt 0 ]]; do
  case "$1" in
    -gcc|--gcc|-GCC|--GCC)
      GCC_FLAG=$((GCC_FLAG + 1))
      ;;
    -clang|--clang|-Clang|--Clang)
      CLANG_FLAG=$((CLANG_FLAG + 1))
      ;;
    *)
      echo "Error: Unknown argument \"$1\""
      show_usage
      ;;
  esac
  shift
done

TOTAL_FLAGS=$((GCC_FLAG + CLANG_FLAG))

if [ "$TOTAL_FLAGS" -gt 1 ]; then
  echo "Error: Cannot specify both -gcc and -clang at the same time"
  show_usage
fi

if [ "$TOTAL_FLAGS" -eq 0 ]; then
  echo "Error: No compiler specified"
  show_usage
fi

# Create temporary log file
> "$TEMP_LOG"

# Start logging all output
(
  # Delete the dist folder for a clean output and recreate it
  echo "Cleaning and recreating 'dist' directory..."
  rm -rf dist
  mkdir -p dist dist/2D dist/3D dist/gha dist/libs

  # Copy necessary files to the dist directory
  echo "Copying essential files to 'dist'..."
  if [ -f ./src/tread.h ]; then
    cp ./src/tread.h ./dist/
  else
    echo "Error: ./src/tread.h not found!"
    exit 1
  fi

  if [ -f ./.gitignore ]; then
    cp ./.gitignore ./dist/
  else
    echo "Warning: ./.gitignore not found, skipping copy."
  fi

  if [ -f ./README.md ]; then
    cp ./README.md ./dist/
  else
    echo "Warning: ./README.md not found, skipping copy."
  fi

  if [ -f ./LICENSE ]; then
    cp ./LICENSE ./dist/
  else
    echo "Warning: ./LICENSE not found, skipping copy."
  fi

  #clear

  if [ "$GCC_FLAG" -eq 1 ]; then
    COMPILER="gcc"

    # The notes about building. (logger functionality simplified for bash)
    $COMPILER ./src/logger.c -o ./dist/logger
    if [ -f dist/logger ]; then
      ./dist/logger -t Note -c "Building with GCC..."
    fi

    # Github Actions (ignore and keep here):
    $COMPILER ./src/gha/packagezip.c -o ./dist/gha/packagezip

    # Main working bits:
    $COMPILER ./src/seperate/launcher/launcher.c -o ./dist/Tread -lm
    $COMPILER ./src/seperate/animator/animator.c -o ./dist/anim -lm
    $COMPILER ./src/seperate/libloader/libloader.c -o ./dist/libloader -lm

    # Libs for libloader to load:
    $COMPILER -shared -fPIC ./src/seperate/libloader/libs/counter.c -o ./dist/libs/counter.so -lm
    $COMPILER -shared -fPIC ./src/seperate/libloader/libs/3Dselector.c -o ./dist/libs/3Dselector.so -lm

    # Tech demos:
    $COMPILER ./src/games/3D/selector.c -o ./dist/3D/selector -lm

    # Testing game executables:
    $COMPILER ./src/games/2D/tgame.c -o ./dist/2D/tgame -lm
    $COMPILER ./src/games/2D/pacman.c -o ./dist/2D/trpacman -lm
    $COMPILER ./src/games/2D/snake.c -o ./dist/2D/trsnake -lm

    if [ -f dist/logger ]; then
      ./dist/logger -t Note -c "All files have been built."
    fi

    # To delete the un-needed files:
    rm -f dist/logger
  elif [ "$CLANG_FLAG" -eq 1 ]; then
    COMPILER="clang"

    # The notes about building. (logger functionality simplified for bash)
    $COMPILER ./src/logger.c -o ./dist/logger
    if [ -f dist/logger ]; then
      ./dist/logger -t Note -c "Building with Clang..."
    fi

    # Github Actions (ignore and keep here):
    gcc ./src/gha/packagezip.c -o ./dist/gha/packagezip # Still using gcc as in original

    # Main working bits:
    $COMPILER ./src/seperate/launcher/launcher.c -o ./dist/Tread -lm
    $COMPILER ./src/seperate/animator/animator.c -o ./dist/anim -lm
    $COMPILER ./src/seperate/libloader/libloader.c -o ./dist/libloader -lm

    # Libs for libloader to load:
    $COMPILER -shared -fPIC ./src/seperate/libloader/libs/counter.c -o ./dist/libs/counter.so -lm
    $COMPILER -shared -fPIC ./src/seperate/libloader/libs/3Dselector.c -o ./dist/libs/3Dselector.so -lm

    # Tech demos:
    $COMPILER ./src/games/3D/selector.c -o ./dist/3D/selector -lm

    # Testing game executables:
    $COMPILER ./src/games/2D/tgame.c -o ./dist/2D/tgame -lm
    $COMPILER ./src/games/2D/pacman.c -o ./dist/2D/trpacman -lm
    $COMPILER ./src/games/2D/snake.c -o ./dist/2D/trsnake -lm

    if [ -f dist/logger ]; then
      ./dist/logger -t Note -c "All files have been built."
    fi

    # To delete the un-needed files:
    rm -f dist/logger
  fi
) &> "$TEMP_LOG" # Redirect all stdout and stderr to TEMP_LOG

# Display the output that was saved
cat "$TEMP_LOG"

# Save to final log file
cp "$TEMP_LOG" "$LOG_FILE"

# Clean up temp file
rm "$TEMP_LOG"

exit 0
