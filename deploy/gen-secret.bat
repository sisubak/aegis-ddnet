@echo off
rem Coded by ueha
rem Генерирует общий ipc_secret и rcon-пароль и прописывает их в server.cfg и gate.cfg.
setlocal
cd /d "%~dp0"
for /f %%i in ('powershell -NoProfile -Command "[guid]::NewGuid().ToString('N')+[guid]::NewGuid().ToString('N').Substring(0,16)"') do set SECRET=%%i
for /f %%i in ('powershell -NoProfile -Command "[guid]::NewGuid().ToString('N').Substring(0,24)"') do set RCON=%%i
powershell -NoProfile -Command "(Get-Content server.cfg) -replace 'CHANGE_ME_SECRET','%SECRET%' -replace 'CHANGE_ME_RCON','%RCON%' | Set-Content server.cfg"
powershell -NoProfile -Command "(Get-Content gate.cfg) -replace 'CHANGE_ME_SECRET','%SECRET%' -replace 'CHANGE_ME_RCON','%RCON%' | Set-Content gate.cfg"
echo ipc_secret и rcon-пароль прописаны. rcon: %RCON%
endlocal
