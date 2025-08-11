# Tread
- A simple but advanced terminal graphics library for C.

---

Tread is a header-only C library designed to provide a lightweight API for creating simple "graphics" with extra advanced features only using system API's in the terminal. It aims to hide away low-level terminal complexities while still offering an experience similar to Raylib but in the terminal and functionality of NCurses with a lot of configuration.

It has recently been tested and made to fully work on Windows using either GCC or Clang from Mingw64 (latest version from [winlibs](https://winlibs.com)). While designed for cross-platform compatibility, specific POSIX environments will require further testing. So for now, **don't** use POSIX environments for running Tread.

---

## Examples
This repo contains the main `tread.h` file for making Tread apps but we also have examples of using it. Here are some:

### Games
- [`tgame.c`](./src/games/2D/tgame.c): A basic "terminal game" demonstrating player movement using WASD/arrows, simple rectangles, and text drawing.
- [`pacman.c`](./src/games/2D/pacman.c): Quite litterally a fully playable Pac-Man clone just without the cherries that showcases character movement, map rendering, collision detection, and score tracking all made with C and Tread.
- [`snake.c`](./src/games/2D/snake.c): A classic game of Snake written in C with Tread. Showcasing dynamic snake growth, food placement, and self-collision.
- [`selector.c`](./src/games/3D/selector.c): A 3D character selector without the actual selecting bit that shows rotating shapes. This showcases 3D stuff in Tread. It is possible to do, just means you have to know a lot about maths and coding with it to work with it.

### Tools
- [`animator.c`](./src/seperate/animator/animator.c): A text-based simple animation program written in C using Tread. It actually exports usable binary data which can be loaded, saved, played and created all inside this one [`animator.c`](./src/seperate/animator/animator.c) program.
- [`libloader.c`](./src/seperate/libloader/libloader.c): A program to load libs (`.dll` or `.so`) and then assign them a keybind so when ever the user presses that keybind while in the [`libloader.c`](./src/seperate/libloader/libloader.c) program in that same session it will run the contents of that library from the function: `void run_lib_app() {}` in C before compiling it into a usable library file to then be ran in [`libloader.c`](./src/seperate/libloader/libloader.c). Confusing? You'll get used to it if you use it. ***Be warned*** [`libloader.c`](./src/seperate/libloader/libloader.c) runs any thing inside the `void run_lib_app() {}` in C before compiling it into a usable library file without checking it first. Check your file your going to load with an antivirus before running it otherwise you will get viruses and stuff from the library you loaded. Not libloader. Libloader itself doesn't contain the viruses. The library you ran does. So check them.

## Getting Started
### Prequisites
  - A C compiler (e.g., MinGW-w64 GCC or Clang on Windows, GCC or Clang on Linux/macOS).
  - For Windows, MinGW-w64 from [winlibs](https://winlibs.com) is recommended and tested. Do not use MSVC.

### Installation
Since Tread is a header-only library there's no installation step required.
Simply place `tread.h` in your project's include path or in the same directory as your source files.

### Compilation
To compile your C app into an executable, ensuring that `tread.h` is accessible in your project, compile it like this using GCC:
```bash
# On Windows (Mingw64 required):
gcc ./src/main.c -o ./dist/main -lkernel32 -lm

# On POSIX systems:
gcc ./src/main.c -o ./dist/main -lm
```
Or you can do it using Clang like this (more recommended):
```bash
# On Windows (Mingw64 required):
clang ./src/main.c -o ./dist/main -lkernel32 -lm

# On POSIX systems:
clang ./src/main.c -o ./dist/main -lm
```

And if you want to compile a shared library (for using in another program or with libloader in the group of examples) you can like this using GCC:
```bash
# On Windows (Mingw64 required):
gcc -shared ./src/main.c -o ./dist/main.dll -lkernel32 -lm

# On POSIX systems:
gcc -shared -fPIC ./src/main.c -o ./dist/main.so -lm
```
Or you can do it using Clang like this (more recommended):
```bash
# On Windows (Mingw64 required):
clang -shared ./src/main.c -o ./dist/main.dll -lkernel32 -lm

# On POSIX systems:
clang -shared -fPIC ./src/main.c -o ./dist/main.so -lm
```

We also have a group of examples that are specifically only made for Windows at the moment but POSIX support is coming soon for them. You can compile them using: `build.bat -clang` for the Clang compiler or `build.bat -gcc` for the GCC compiler.

## Features
- **Header-Only**: integrate quickly into new or existing C projects just by including `#include <tread.h>` and linking it to your compiler of choice.
- **3D (optional) and 2D Support**: 2D is built in to Tread by default so no changes needed there. For 3D to be enabled you need to define `TR_3D` before including `tread.h` like this:
```c
#define TR_3D
#include <tread.h>
```
- **Character-Based Drawing**: All rendering is done using characters and terminal colors.
- **Double Buffering**: Reduces screen flickering for smoother animations.
- **Fixed FPS Control**: Allows setting a target frame rate for consistent application speed. This may increase how fast reactive elements in your app move if you have a higher frame rate. 60fps is recommended for the 3D side of Tread but ***be warned*** for the 2D stuff with high frame rate.
- **Basic Input Handling**: Detects single key presses, including standard ASCII characters and common special keys (arrows, ESC, F-keys).
- **Terminal Resize Detection**: Automatically stops the running program if the terminal is resized at all. This prevents your program from looking all messed up when a user accidentally resizes it and breaks your program.
- **Customizable colors**: Provides a `Color` struct and predefined Raylib-like color macros, mapped to basic 8/16 terminal colors. ***Be warned*** some colors may not look correct like `BEIGE` for example. `BEIGE` looks white in the terminal because Tread maps it to the closest supported terminal color for multi-terminal support.
- **`SIGINT` Handling (`CTRL+C`)**: Disables default `CTRL+C` termination to give applications more control over how they exit when they do.

## Contributing
Contributions are welcome! If you find bugs, have suggestions for improvements, or want to add new features, please consider submitting a pull request or make an issue.

## Licensing
Tread is open-source. That means that to keep the library updated we rely on issues, pull requests and people using the library to grow this. Tread goes under the GPL-3.0 license so follow the license rules when using the library.

## API Reference (reference for `tread.h`)
> [!WARNING]
> This is very big so I'm including it at the bottom of the README.md file on purpose. There is a lot of API stuff. I mean like 1,267 lines long of it in the actual `tread.h`. It gets complicated. And your brain might hurt like mine did without using AI to understand the maths at some point. Please take breaks from coding the 3D stuff specifically if your coding for a long period of time. Don't say I didn't warn you.

### Core Functions
- `void TR_InitWindow(int width, int height, const char* title)`: Initializes the terminal window. `width` and `height` are logical dimensions; the library adapts to the actual terminal size. `title` sets the terminal window title. Disables Ctrl+C.
- `void TR_CloseWindow()`: Closes the terminal window and restores original terminal settings.
- `bool TR_WindowShouldClose()`: Checks if the window should close (e.g., if ESC or 'q' is pressed).
- `void TR_SetTargetFPS(int fps)`: Sets the target frames per second.
- `void TR_BeginDrawing()`: Begins the drawing phase. Reads input and prepares the buffer. Also checks for terminal resize and exits if detected.
- `void TR_EndDrawing()`: Ends the drawing phase. Compares buffers, draws only changed cells, flushes output, and handles frame timing.
- `void TR_ClearBackground(Color color)`: Clears the entire drawing surface with the specified `color`.
- `int TR_GetScreenWidth()`: Returns the current width of the terminal screen in characters.
- `int TR_GetScreenHeight()`: Returns the current height of the terminal screen in characters.

### Drawing Functions
- `void TR_DrawPixel(int x, int y, Color color)`: Draws a single character "pixel" at (x,y) with the specified `color`. This fills the cell with the color.
- `void TR_DrawText(const char* text, int x, int y, int fontSize, Color fg_color, Color bg_color)`: Draws `text` at (x,y). `fontSize` is ignored. `fg_color` is the foreground color, `bg_color` is the background color for the text characters. Pass `BLANK` for `bg_color` to use the current background color set by `TR_ClearBackground`.
- `void TR_DrawRectangle(int x, int y, int width, int height, Color fg_color, Color bg_color)`: Draws a filled rectangle. `fg_color` is the character color (usually space), `bg_color` fills the cells. Pass `BLANK` for `bg_color` to use the current background.
- `void TR_DrawRectangleLines(int x, int y, int width, int height, Color fg_color, Color bg_color)`: Draws an empty rectangle (border) using `#` characters. `fg_color` is for the border characters, `bg_color` for the character's cell background. Pass `BLANK` for `bg_color` to use the current background.

### Input Functions
- `bool TR_IsKeyDown(int key)`: Checks if a `key` is currently "down" (i.e., was the last key pressed).
- `bool TR_IsKeyPressed(int key)`: Checks if a `key` has been pressed once. (Currently behaves the same as `TR_IsKeyDown` in this implementation).
- `int TR_GetKeyPressed()`: Returns the ASCII value or custom key code of the last key pressed and clears the internal key buffer.

### Custom Key Codes
Special keys are mapped to integer values above ASCII range:
- `TR_KEY_UP`, `TR_KEY_DOWN`, `TR_KEY_LEFT`, `TR_KEY_RIGHT`
- `TR_KEY_ENTER`, `TR_KEY_BACKSPACE`, `TR_KEY_DELETE`, `TR_KEY_ESCAPE`
- `TR_KEY_F1` to `TR_KEY_F12`

### color Macros
Tread provides Raylib-like color macros for convenience:
- `BLANK`, `RAYWHITE` (from Raylib), `TREADGRAY` (custom color specifically for Tread), `LIGHTGRAY`, `GRAY`, `DARKGRAY`
- `YELLOW`, `GOLD`, `ORANGE`, `PINK`, `RED`, `MAROON`
- `GREEN`, `LIME`, `DARKGREEN`
- `SKYBLUE`, `BLUE`, `DARKBLUE`
- `PURPLE`, `VIOLET`, `DARKPURPLE`
- `BEIGE`, `BROWN`, `DARKBROWN`
- `WHITE`, `BLACK`
- `MAGENTA`, `CYAN`

### 3D Functionality (`TR_3D` Macro)
To enable 3D features, define `TR_3D` before including `tread.h`:
```c
#define TR_3D
#include <tread.h>
```
When `TR_3D` is defined, the following are available:

#### 3D Data Structures
- `typedef struct { float x, y, z; } TR_Vector3;`
- `typedef struct { float m[4][4]; } TR_Matrix4x4;`
- `typedef struct { int v[3]; } TR_Triangle;` (Indices into a vertex array)

#### 3D Math Functions
- `TR_Matrix4x4 TR_MatrixIdentity()`: Returns an identity 4times4 matrix.
- `TR_Matrix4x4 TR_MatrixMultiply(TR_Matrix4x4 mat1, TR_Matrix4x4 mat2)`: Multiplies two 4times4 matrices (mat1timesmat2).
- `TR_Vector3 TR_Vector3Transform(TR_Vector3 v, TR_Matrix4x4 mat)`: Transforms a 3D vector by a 4times4 matrix.
- `TR_Matrix4x4 TR_MatrixTranslate(float x, float y, float z)`: Creates a translation matrix.
- `TR_Matrix4x4 TR_MatrixRotateX(float angle)`: Creates a rotation matrix around the X-axis (in radians).
- `TR_Matrix4x4 TR_MatrixRotateY(float angle)`: Creates a rotation matrix around the Y-axis (in radians).
- `TR_Matrix4x4 TR_MatrixRotateZ(float angle)`: Creates a rotation matrix around the Z-axis (in radians).
- `TR_Matrix4x4 TR_MatrixScale(float x, float y, float z)`: Creates a scaling matrix.
- `TR_Matrix4x4 TR_MatrixPerspective(float fovY, float aspect, float nearPlane, float farPlane)`: Creates a basic perspective projection matrix.

#### 3D Drawing Functions
- `void TR_DrawTriangle3DWireframe(TR_Vector3 v1, TR_Vector3 v2, TR_Vector3 v3, TR_Matrix4x4 mvp_matrix, Color color)`: Draws a wireframe triangle in 3D space, transformed by the provided model-view-projection `mvp_matrix`.
- `void TR_DrawTriangle3DFilled(TR_Vector3 v1, TR_Vector3 v2, TR_Vector3 v3, TR_Matrix4x4 mvp_matrix, Color color)`: Draws a filled triangle in 3D space with Z-buffering, transformed by the provided model-view-projection `mvp_matrix`.
- `void TR_DrawCubeWireframe3D(TR_Vector3 position, TR_Vector3 size, TR_Vector3 rotation_radians, Color color)`: Helper function to draw a wireframe cube.
- `void TR_DrawCubeFilled3D(TR_Vector3 position, TR_Vector3 size, TR_Vector3 rotation_radians, Color color)`: Helper function to draw a filled cube with Z-buffering.

---

You made it to the end without dying in the process. Good job.
