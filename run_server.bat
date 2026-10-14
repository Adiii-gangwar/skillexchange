@echo off
if not exist "server.exe" (
    echo server.exe not found! Compiling backend first...
    call build.bat
)
echo Starting SkillSwap C++ HTTP Server on http://127.0.0.1:8080/ ...
server.exe
pause
