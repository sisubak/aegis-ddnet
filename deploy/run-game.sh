#!/bin/sh
# Coded by ueha
# Запуск игрового сервера (role 0). Должен лежать рядом с бинарём DDNet-Server и data/.
cd "$(dirname "$0")"
BIN=./DDNet-Server
[ -x "$BIN" ] || BIN=./DDNet-Server.exe
exec "$BIN" "exec server.cfg"
