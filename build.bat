@echo off
setlocal
echo ========================================================
echo   Compiling SkillSwap C++ Backend Server
echo   Compiler: MinGW GCC (g++)
echo   Database: SQLite 3 (sqlite3.o)
echo ========================================================

REM Check if sqlite3.o exists, if not compile it
if not exist "backend\sqlite\sqlite3.o" (
    echo [Step 1/2] Compiling SQLite 3 amalgamation...
    gcc -c backend\sqlite\sqlite3.c -o backend\sqlite\sqlite3.o -include backend\sqlite\mingw_compat.h
    if %ERRORLEVEL% neq 0 (
        echo [ERROR] Failed to compile sqlite3.c. Make sure gcc is in PATH.
        exit /b %ERRORLEVEL%
    )
)

echo [Step 2/2] Compiling server.exe from backend\main.cpp...
g++ -std=c++14 backend\main.cpp backend\sqlite\sqlite3.o -o server.exe -lws2_32

if %ERRORLEVEL% neq 0 (
    echo [ERROR] Build failed! Check compiler errors above.
    exit /b %ERRORLEVEL%
)

echo.
echo ========================================================
echo   [SUCCESS] Build Complete! Output: server.exe
echo   To launch the server, run: .\server.exe
echo   Or double click run_server.bat
echo ========================================================
