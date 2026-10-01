@echo off
rem Coded by ueha
rem Запуск игрового сервера (role 0) на Windows. Рядом должен лежать DDNet-Server.exe и data\.
cd /d "%~dp0"
DDNet-Server.exe "exec server.cfg"
