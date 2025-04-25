@echo off
setlocal enabledelayedexpansion
REM Build ILI9488 Pico example programs

echo Creating build directory...
if not exist build mkdir build

echo Entering build directory...
cd build

echo Running CMake...
cmake -G "MinGW Makefiles" ..

echo Starting compilation...
mingw32-make

echo Compilation complete!

echo Generated files:
echo ------------------------------
for %%f in (*.uf2) do (
    echo - %%f
)
echo ------------------------------

cd ..

echo Build process completed.
pause
