# entities audit (in progress)
# Аудит: команды чата с аргументами от клиента (mister/ \entities)

Файлы: ddracechat.cpp, ddracecommands.cpp, teams.cpp, player.cpp
Ищем: индексы массивов без bounds-check, деление на 0, nullptr deref, краши от str_toint/atoi.

