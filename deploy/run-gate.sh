#!/bin/sh
# Coded by ueha
# Запуск gate-сервера (role 1). Должен лежать рядом с бинарём DDNet-Server и data/.
cd "$(dirname "$0")" || exit 1
BIN=./DDNet-Server
[ -x "$BIN" ] || BIN=./DDNet-Server.exe
exec "$BIN" "exec gate.cfg"
