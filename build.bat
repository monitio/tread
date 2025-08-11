@echo off
setlocal enabledelayedexpansion

set GCC_FLAG=0
set CLANG_FLAG=0

REM Setup logging path
set "TEMP_LOG=%TEMP%\build_temp.log"
set "LOG_FILE=build.bat.log"

REM Check if the existing log file is too large (1GB = 1073741824 bytes)
if exist "%LOG_FILE%" (
  for %%F in ("%LOG_FILE%") do (
    if %%~zF gtr 1073741824 (
      echo Error: Log file is too big to save ^(>1GB^)
      exit /b 1
    )
  )
)

:parse_args
if "%1"=="-gcc" (
  set /a GCC_FLAG+=1
  shift
  goto parse_args
)
if "%1"=="--gcc" (
  set /a GCC_FLAG+=1
  shift
  goto parse_args
)
if "%1"=="-GCC" (
  set /a GCC_FLAG+=1
  shift
  goto parse_args
)
if "%1"=="--GCC" (
  set /a GCC_FLAG+=1
  shift
  goto parse_args
)
if "%1"=="-clang" (
  set /a CLANG_FLAG+=1
  shift
  goto parse_args
)
if "%1"=="--clang" (
  set /a CLANG_FLAG+=1
  shift
  goto parse_args
)
if "%1"=="-Clang" (
  set /a CLANG_FLAG+=1
  shift
  goto parse_args
)
if "%1"=="--Clang" (
  set /a CLANG_FLAG+=1
  shift
  goto parse_args
)
if not "%1"=="" (
  echo Error: Unknown argument "%1"
  goto show_usage
)

set /a TOTAL_FLAGS=%GCC_FLAG%+%CLANG_FLAG%

if %TOTAL_FLAGS% GTR 1 (
  echo Error: Cannot specify both -gcc and -clang at the same time
  goto show_usage
)

if %TOTAL_FLAGS% EQU 0 (
  echo Error: No compiler specified
  goto show_usage
)

REM Create temporary log file
type nul > "%TEMP_LOG%"

REM Start logging all output
(
  REM Delete the dist folder for a clean output
  del dist\* /f /q /s

  REM Create the necessary output directories
  if not exist dist md dist
  if not exist dist\2D md dist\2D
  if not exist dist\3D md dist\3D
  if not exist dist\libs md dist\libs

  clear

  if %GCC_FLAG% EQU 1 (
    REM The notes about building.
    gcc ./src/logger.c -o ./dist/logger.exe -luser32 -lgdi32
    if exist dist\logger.exe (
      dist\logger.exe -t Note -c "Building with GCC..."
    )

    REM Main working bits:
    gcc ./src/seperate/launcher/launcher.c -o ./dist/Tread -lkernel32 -lm
    gcc ./src/seperate/animator/animator.c -o ./dist/anim -lkernel32 -lm
    gcc ./src/seperate/libloader/libloader.c -o ./dist/libloader.exe -lkernel32 -lm

    REM Libs for libloader to load:
    gcc -shared ./src/seperate/libloader/libs/counter.c -o ./dist/libs/counter.dll -lm
    gcc -shared ./src/seperate/libloader/libs/3Dselector.c -o ./dist/libs/3Dselector.dll -lm

    REM Tech demos:
    gcc ./src/games/3D/selector.c -o ./dist/3D/selector -lkernel32 -lm

    REM Testing game executables:
    gcc ./src/games/2D/tgame.c -o ./dist/2D/tgame -lkernel32 -lm
    gcc ./src/games/2D/pacman.c -o ./dist/2D/trpacman -lkernel32 -lm
    gcc ./src/games/2D/snake.c -o ./dist/2D/trsnake -lkernel32 -lm

    if exist dist\logger.exe (
      dist\logger.exe -t Note -c "All files have been built."
    )

    REM To delete the un-needed files:
    del dist\logger.exe
  ) else if %CLANG_FLAG% EQU 1 (
    REM The notes about building.
    clang ./src/logger.c -o ./dist/logger.exe -luser32 -lgdi32
    if exist dist\logger.exe (
      dist\logger.exe -t Note -c "Building with Clang..."
    )

    REM Main working bits:
    clang ./src/seperate/launcher/launcher.c -o ./dist/Tread -lkernel32 -lm
    clang ./src/seperate/animator/animator.c -o ./dist/anim -lkernel32 -lm
    clang ./src/seperate/libloader/libloader.c -o ./dist/libloader.exe -lkernel32 -lm

    REM Libs for libloader to load:
    clang -shared ./src/seperate/libloader/libs/counter.c -o ./dist/libs/counter.dll -lm
    clang -shared ./src/seperate/libloader/libs/3Dselector.c -o ./dist/libs/3Dselector.dll -lm

    REM Tech demos:
    clang ./src/games/3D/selector.c -o ./dist/3D/selector -lkernel32 -lm

    REM Testing game executables:
    clang ./src/games/2D/tgame.c -o ./dist/2D/tgame -lkernel32 -lm
    clang ./src/games/2D/pacman.c -o ./dist/2D/trpacman -lkernel32 -lm
    clang ./src/games/2D/snake.c -o ./dist/2D/trsnake -lkernel32 -lm

    if exist dist\logger.exe (
      dist\logger.exe -t Note -c "All files have been built."
    )

    REM To delete the un-needed files:
    del dist\logger.exe
  )
) > "%TEMP_LOG%" 2>&1

REM Display the output that was saved
type "%TEMP_LOG%"

REM Save to final log file
type nul > "%LOG_FILE%"
type "%TEMP_LOG%" > "%LOG_FILE%"

REM Clean up temp file
del "%TEMP_LOG%"

exit /b 0

:show_usage
echo Usage: build.bat [-gcc^|-clang]
echo Options:
echo   -gcc   Build using GCC compiler
echo   -clang   Build using Clang compiler
exit /b 1
