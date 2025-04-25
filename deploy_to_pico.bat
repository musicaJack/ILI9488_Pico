@echo off
setlocal enabledelayedexpansion

REM ===============================================================
REM ILI9488 Raspberry Pi Pico Deployment Tool
REM Author: S.H. Mao
REM ===============================================================

REM Check if the Raspberry Pi Pico is connected
echo Checking if the Raspberry Pi Pico is connected...
set "DRIVE_FOUND="
for %%d in (A B C D E F G H I J K L M N O P Q R S T U V W X Y Z) do (
    if exist "%%d:\INFO_UF2.TXT" (
        set "DRIVE_FOUND=%%d"
        goto :drive_found
    )
)

echo ERROR: Raspberry Pi Pico not found in bootloader mode.
echo 1. Connect your Raspberry Pi Pico to your PC
echo 2. Press and hold the BOOTSEL button on the Pico while connecting
echo 3. Release the BOOTSEL button after connecting
echo 4. The Pico should appear as a USB drive named "RPI-RP2"
goto :eof

:drive_found
echo Found Raspberry Pi Pico on drive %DRIVE_FOUND%:

REM Search for all .uf2 files in the build directory
echo Searching for available UF2 files...
set "UF2_FILES="
set "UF2_COUNT=0"

for %%f in (build\*.uf2) do (
    set /a "UF2_COUNT+=1"
    set "UF2_FILES=!UF2_FILES!%%f "
    set "LATEST_UF2=%%f"
)

if %UF2_COUNT% EQU 0 (
    echo Error: No UF2 files found.
    echo Please run build_pico.bat to compile the project first.
    goto :eof
)

echo Found %UF2_COUNT% UF2 files.
echo Will deploy the latest compiled: %LATEST_UF2%

REM Copy UF2 file to Raspberry Pi Pico
echo Deploying %LATEST_UF2% to Raspberry Pi Pico...
copy "%LATEST_UF2%" "%DRIVE_FOUND%:\" /Y
if errorlevel 1 (
    echo Error: Unable to copy UF2 file to Raspberry Pi Pico.
    goto :eof
)

echo.
echo Deployment successful!
echo Raspberry Pi Pico will automatically reset and run the program.
echo.

endlocal 