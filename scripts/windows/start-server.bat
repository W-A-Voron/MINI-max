@echo off
cd /d "%~dp0"
minimax-server.exe -c configs\server.toml
pause
