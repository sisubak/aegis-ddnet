# fix_mapchange.md — Issue #5834 (NETMSG_MAP_CHANGE шлётся дважды)

Автор: mister/ \mapchange
Проект: DDNet (C++), C:\Users\WWWWWUeHaA\Desktop\antiddos

## Симптом
При первом подключении клиент получает два NETMSG_MAP_CHANGE, из-за чего
инициируется два HTTP-запроса карты (лишний download).

## Анализ реального кода

Путь 1 — antispoof/flood-защита на уровне сети:
src/engine/shared/network_server.cpp:405-460. При коннекте (vanilla antispoof)
сервер сразу шлёт в одном пакете фейковый MAP_CHANGE + MAP_DATA + CON_READY +
3x SNAPEMPTY. Это либо "dummy" карта, либо fallback "dm1" при флуде
(строки 408-421). Нужно чтобы клиент загрузил хоть какую-то карту до валидации
токена и не крашнулся на SNAPEMPTY.

Путь 2 — штатная отправка настоящей карты:
src/engine/server/server.cpp:1410-1445 (CServer::SendMap) шлёт NETMSG_MAP_DETAILS
и NETMSG_MAP_CHANGE с реальными именем/crc/размером. Вызывается из
OnNetMsgInfo (server.cpp:2146) при переходе клиента в STATE_CONNECTING.

## Корень бага
Клиент 0.7/vanilla-совместимого пути получает MAP_CHANGE дважды: сначала dummy
из network_server.cpp (нужен для antispoof), затем реальный из SendMap. Оба
имеют разные имя/crc, поэтому клиент дважды запускает загрузку карты (для
HTTP-map-download это два GET). Для штатного DDNet-клиента dummy-карта
"dummy"/"dm1" не совпадает с реальной, и клиент честно перезапрашивает.

Важно: NETMSG_MAP_DETAILS (server.cpp:1414), добавленный позже, содержит
URL и sha256 для HTTP-download. Клиенты, умеющие MAP_DETAILS, должны
дожидаться финального MAP_CHANGE и не стартовать HTTP по dummy. Но старая
логика клиента реагирует HTTP-запросом на каждый MAP_CHANGE.

## Кандидат-фикс (НЕ применён)
Проблема двойного запроса — на стыке протокола, чинить нужно согласованно
клиент+сервер:
- Вариант A (клиент): не инициировать HTTP-download по MAP_CHANGE до получения
  MAP_DETAILS, либо игнорировать MAP_CHANGE если имя == "dummy"/"dm1"
  (antispoof-заглушки). Это правка на клиенте, вне серверной части.
- Вариант B (сервер): не слать dummy MAP_CHANGE клиентам, поддерживающим
  MAP_DETAILS. Но на момент connless-пакета (network_server.cpp) сервер ещё
  не знает возможностей клиента (capabilities приходят позже), поэтому надёжно
  отличить нельзя — dummy обязателен для antispoof.

Оба варианта затрагивают тонкую сетевую/антиспуф-логику и совместимость с
0.6.4/0.7 клиентами. Риск регресса высок.

## СТАТУС
ANALYZED — НЕ ПРИМЕНЕНО. Корень: dummy MAP_CHANGE (antispoof,
network_server.cpp:405-460) + реальный MAP_CHANGE (server.cpp SendMap 1410-1445).
Безопасный фикс требует согласованного изменения клиента (игнор dummy-имён при
HTTP-download) — это вне серверной части и вне безопасного минимального патча.
Код не менялся.
