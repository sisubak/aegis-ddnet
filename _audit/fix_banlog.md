# Аудит: Issue #11095 — пропадают логи net_ban / kill / "leave" при бане через rcon

Автор: mister/ \banlog · Режим: ТОЛЬКО РАЗБОР (правки не применялись)

## Как устроен log-scoping

DDNet использует thread-local «активный логгер» (scope logger). См. `src/base/log.cpp`:

- `scope_logger` — thread_local указатель (`log.cpp:28`).
- `log_get_scope_logger()` (`log.cpp:59`) / `log_set_scope_logger()` (`log.cpp:68`).
- Все вызовы `log_info/log_error/...` в итоге идут в `log_log_impl()` (`log.cpp:77`), который в самом конце делает **`scope_logger->Log(&Msg);`** (`log.cpp:111`) — то есть сообщение получает ТОЛЬКО текущий scope-логгер, а не глобальный.
- `CLogScope` (RAII, `logger.h:293`) на входе подменяет scope-логгер через `log_set_scope_logger`, на выходе возвращает прежний.

В штатном режиме scope-логгером сервера является `CServerLogger` (`server_logger.cpp`), чей `Log()` вызывает `m_pServer->SendLogLine()` → `SendRconLogLine(-1, ...)` (всем админам) + `m_Econ.Send()` (`server.cpp:673`). Именно так строки попадают в stdout/econ/лог сервера.

## Где теряются логи (корень бага)

Обработчик rcon-команды: `CServer::OnNetMsgRconCmd` (`server.cpp:2218`):

```cpp
m_RconClientId = ClientId;
m_RconAuthLevel = GetAuthedState(ClientId);
{
    CRconClientLogger Logger(this, ClientId, log_get_scope_logger());
    CLogScope Scope(&Logger);                       // <-- подмена scope-логгера
    Console()->ExecuteLineFlag(pCmd, CFGFLAG_SERVER, ClientId);
}
```

На время исполнения команды scope-логгер подменяется на `CRconClientLogger`. Значит ВСЕ `log_*` вызовы, порождённые командой `ban` (внутри: `net_ban`-логи, `kill`, drop клиента, "has left the game" / "leave player"), уходят в `CRconClientLogger::Log`.

Классический (upstream) `CRconClientLogger::Log` выглядит так:

```cpp
void CRconClientLogger::Log(const CLogMessage *pMessage)
{
    if(m_Filter.Filters(pMessage))
        return;
    m_pServer->SendRconLogLine(m_ClientId, pMessage);   // только инициатору rcon
}
```

То есть строки уходят **только rcon-клиенту-инициатору** (`SendRconLogLine(m_ClientId, ...)`) и НЕ попадают в `CServerLogger` → значит не идут ни в stdout/файл-лог сервера, ни в econ, ни другим админам. Это и есть корень #11095: log-scoping «захватывает» вывод команды и не пробрасывает его в собственный логгер сервера.

## Текущее состояние кода в этом репозитории

ВНИМАНИЕ: в этом дереве `CRconClientLogger::Log` (`server.cpp:214-228`) уже содержит проброс в родительский логгер (комментарий прямо ссылается на #11095):

```cpp
void CRconClientLogger::Log(const CLogMessage *pMessage)
{
    if(m_pParentLogger)          // parent = log_get_scope_logger() на момент создания
        m_pParentLogger->Log(pMessage);
    if(m_Filter.Filters(pMessage))
        return;
    m_pServer->SendRconLogLine(m_ClientId, pMessage);
}
```

Родитель передаётся при создании: `CRconClientLogger Logger(this, ClientId, log_get_scope_logger())` (`server.cpp:2237`). Т.е. фикс-кандидат фактически УЖЕ применён кем-то ранее. Поэтому в текущем состоянии логи net_ban/kill/leave должны доходить и до `CServerLogger` (stdout/файл/econ).

## Кандидат-фикс (безопасный)

Если исходить из «чистого» upstream-состояния (без проброса) — минимальный и безопасный фикс: пробрасывать сообщение в родительский scope-логгер, как и сделано выше. Ключевые моменты для безопасности:

1. Родитель захватывается ОДИН раз в конструкторе (`log_get_scope_logger()` ДО `CLogScope`), а не берётся динамически внутри `Log()` — иначе внутри scope `log_get_scope_logger()` вернул бы сам `CRconClientLogger` и получилась бы бесконечная рекурсия. В текущем коде это соблюдено.
2. Проброс в родителя делать ДО собственной фильтрации `m_Filter` — чтобы фильтр rcon-клиента не глотал строки серверного лога. В текущем коде соблюдено.
3. Рекурсия по `log_log_impl` защищена флагом `in_logger` (`log.cpp:80`), так что вложенные `log_*` в самом логгере безопасны.

### Потенциальный побочный эффект (стоит проверить)
Инициатор-админ получит строки дважды: один раз от `m_pParentLogger` → `CServerLogger::SendLogLine` → `SendRconLogLine(-1)` (рассылка ВСЕМ админам, включая инициатора), и второй раз от `SendRconLogLine(m_ClientId, ...)` в самом `CRconClientLogger::Log`. Это дубликат вывода у того, кто выполнил `ban`.

Безопасный вариант устранения дубля (НЕ применял): при наличии проброса в родителя убрать/сделать условным собственный `SendRconLogLine(m_ClientId, ...)`, либо в `CServerLogger`/`SendRconLogLine(-1)` пропускать `m_RconClientId`. Но это уже косметика — на потерю логов (#11095) не влияет.

## Файлы, задействованные в механизме
- `src/base/log.cpp` — scope_logger, `log_log_impl` (одиночная доставка в scope).
- `src/base/logger.h` — `CLogScope`, `log_get/set_scope_logger`.
- `src/engine/server/server.cpp:198-228` — `CRconClientLogger` (место потери/проброса).
- `src/engine/server/server.cpp:2218-2245` — `OnNetMsgRconCmd` (установка scope на время rcon).
- `src/engine/server/server.cpp:673-683` / `1523-1542` — `SendLogLine` / `SendRconLogLine`.
- `src/engine/server/server_logger.cpp` — штатный `CServerLogger` (stdout/econ).

## СТАТУС
- Корень бага: ИДЕНТИФИЦИРОВАН — log-scoping подменяет scope-логгер на `CRconClientLogger`, который в upstream отправляет вывод только rcon-инициатору и не пробрасывает в `CServerLogger` (stdout/файл/econ).
- Кандидат-фикс: ОПИСАН (проброс в родительский scope-логгер, захваченный до `CLogScope`).
- В ТЕКУЩЕМ дереве: фикс-проброс УЖЕ ПРИСУТСТВУЕТ в `server.cpp:214-228` — требуется лишь верификация сборкой/тестом и проверка возможного дубля вывода у инициатора.
- Правки НЕ вносились (задача — только разбор).
