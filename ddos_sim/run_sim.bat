@echo off
setlocal
cd /d "%~dp0"

set BUILD=C:\Users\WWWWWUeHaA\Desktop\antiddos\build
set SIM=C:\Users\WWWWWUeHaA\Desktop\antiddos\ddos_sim
set CFG=%SIM%\x7f3k9q2.cfg
set SRVLOG=%SIM%\server_live.log

echo Killing old server instances...
taskkill /IM DDNet-Server.exe /F >nul 2>&1

if exist "%SRVLOG%" del "%SRVLOG%"

echo Starting DDNet-Server (target for simulation)...
start "ANTIDDOS SERVER" /D "%BUILD%" cmd /c "DDNet-Server.exe "exec %CFG%" logfile "%SRVLOG%" & pause"

timeout /t 3 /nobreak >nul

echo Opening DEFENSE console (server side)...
start "DEFENSE - SERVER" cmd /c "python "%SIM%\by_utf8xbot_defense.py" "%SRVLOG%" & pause"

timeout /t 1 /nobreak >nul

echo Opening ATTACK console (attacker side)...
start "ATTACK - DDoS SIM" cmd /c "python "%SIM%\by_utf8xbot_attack.py" 8 40 0 & pause"

echo.
echo Two consoles launched:
echo   [ATTACK]  spoofed-IP flood + crash/huffman/resend/oversize packets
echo   [DEFENSE] live server log with ban/connlimit/rate/drop highlighting
echo Close the windows or Ctrl+C to stop.
endlocal
