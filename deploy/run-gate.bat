@echo off
rem Coded by ueha
rem Запуск gate-сервера (role 1) на Windows. Рядом должен лежать DDNet-Server.exe и data\.
cd /d "%~dp0"
DDNet-Server.exe "exec gate.cfg"
