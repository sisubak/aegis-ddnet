# Аудит #6909 — sv_banned_versions не работает с версией -1

Автор: mister/ \banver
Репо: C:\Users\WWWWWUeHaA\Desktop\antiddos

## СТАТУС (после сверки с реальным кодом): УЖЕ ИСПРАВЛЕНО В КОДЕ — правок не вносил

Проверка реального дерева показала: фикс issue #6909 **уже присутствует** в этом
репозитории. Ранний разбор ниже (Причины A/B, кандидаты-фиксы) писался по устаревшему
слепку и БОЛЬШЕ НЕ АКТУАЛЕН как «баг». Оставлен как история; ниже — фактическое
положение дел.

### Что реально в коде сейчас
`src\game\server\gamecontext.cpp`, `OnClientEnter`, строки 1746-1762:
```cpp
IServer::CClientInfo Info;
if(Server()->GetClientInfo(ClientId, &Info))
{
    // 0.7 clients can send ddnet message, F-Client does that for example
    if(Info.m_GotDDNetVersion)
    {
        if(OnClientDDNetVersionKnown(ClientId))
        {
            return; // kicked
        }
    }
    else if(g_Config.m_SvBannedVersions[0] != '\0' && IsVersionBanned(VERSION_NONE))
    {
        Server()->Kick(ClientId, "unsupported client");
        return; // kicked
    }
}
```

Ключевое: появилась ветка `else if`, которой не было в исходном разборе. Для клиента
**без** DDNet-версии (`!Info.m_GotDDNetVersion`, т.е. чистый vanilla) теперь напрямую
вызывается `IsVersionBanned(VERSION_NONE)`, где `VERSION_NONE == -1`
(`src\engine\shared\protocol.h:144`). Это решает обе исходные причины сразу:

- **Причина A снята**: в ban-check передаётся жёсткая константа `VERSION_NONE` (-1),
  а НЕ `Info.m_DDNetVersion`. Поэтому клампинг `-1 → VERSION_VANILLA` в
  `GetClientInfo` (`src\engine\server\server.cpp:738`) на этот путь не влияет.
  `IsVersionBanned(-1)` форматирует строку "-1" и ищет её в `sv_banned_versions`
  (`gamecontext.cpp:5161-5167`).
- **Причина B снята**: проверка теперь достижима и для не-DDNet клиентов — она вынесена
  из-под условия `m_GotDDNetVersion` в ветку `else if`.

### Итог проверки
Чтобы забанить клиентов без DDNet-версии, админ добавляет `-1` в `sv_banned_versions` —
и это РАБОТАЕТ в текущем коде. Ветка `if(Info.m_GotDDNetVersion)` по-прежнему обслуживает
клиентов с известной версией через `OnClientDDNetVersionKnown` → `IsVersionBanned(ClientVersion)`
(`gamecontext.cpp:2011`). Регрессий/дыр в логике не обнаружено.

**Кода не менял** — фикс уже на месте, безопасного изменения вносить не требуется.

---

## (Архив) Первоначальный разбор — писался по устаревшему коду, НЕ актуален

Автор: mister/ \banver

Репо: C:\Users\WWWWWUeHaA\Desktop\antiddos

## Issue (github)
#6909 "sv_banned_versions does not work with version -1 anymore" (Robyt3, 24 Jul 2023).
Reported by Kaya on Discord. Работало в 2018. Сейчас логика **не доходит до проверки
sv_banned_versions, когда у клиента нет DDNet-версии** (версия неизвестна).

## Где живёт логика

### 1. Конфиг
`src\engine\shared\config_variables.h:715`
```
MACRO_CONFIG_STR(SvBannedVersions, sv_banned_versions, 128, "", CFGFLAG_SERVER, "Comma-separated list of banned clients to be kicked on join")
```

### 2. Проверка бана по версии
`src\game\server\gamecontext.cpp:5149-5155`
```cpp
bool CGameContext::IsVersionBanned(int Version)
{
	char aVersion[16];
	str_format(aVersion, sizeof(aVersion), "%d", Version);

	return str_in_list(g_Config.m_SvBannedVersions, ",", aVersion);
}
```
Т.е. чтобы забанить неизвестную версию, админ пишет в список `-1`. Функция сравнивает
переданный `Version` со строками из списка. Значит в неё должно приходить именно `-1`.

### 3. Вызов проверки
`src\game\server\gamecontext.cpp:1979-2003`
```cpp
bool CGameContext::OnClientDDNetVersionKnown(int ClientId)
{
	IServer::CClientInfo Info;
	dbg_assert(Server()->GetClientInfo(ClientId, &Info), "failed to get client info");
	int ClientVersion = Info.m_DDNetVersion;
	...
	// Autoban known bot versions.
	if(g_Config.m_SvBannedVersions[0] != '\0' && IsVersionBanned(ClientVersion))
	{
		Server()->Kick(ClientId, "unsupported client");
		return true;
	}
```

### 4. Откуда берётся ClientVersion (корень бага)
`src\engine\server\server.cpp:712-736` (функция `CServer::GetClientInfo`)
```cpp
pInfo->m_GotDDNetVersion = m_aClients[ClientId].m_DDNetVersionSettled;
pInfo->m_DDNetVersion = m_aClients[ClientId].m_DDNetVersion >= 0 ? m_aClients[ClientId].m_DDNetVersion : VERSION_VANILLA;
```

### 5. Константы версий
`src\engine\shared\protocol.h:142-146`
```cpp
enum
{
	VERSION_NONE = -1,
	VERSION_VANILLA = 0,
	VERSION_DDRACE = 1,
	VERSION_DDNET_OLD = 2,
```
Внутреннее `m_DDNetVersion` по дефолту = `VERSION_NONE` (-1), см. server.cpp:1198,1235,1269
(инициализация при подключении) и server.h:203-204.

## Почему -1 больше не матчится — ДВЕ причины

**Причина A (маскировка -1 → 0).**
В `GetClientInfo` (server.cpp:722) значение `m_DDNetVersion` пропускается через тернарник
`>= 0 ? ... : VERSION_VANILLA`. Когда реальная версия клиента неизвестна и равна
`VERSION_NONE` (-1), наружу отдаётся `VERSION_VANILLA` (0), а не -1.
Поэтому в `IsVersionBanned` попадает `0`, а не `-1` — строка "-1" из sv_banned_versions
никогда не совпадёт. Чтобы забанить неизвестную версию, пришлось бы писать "0", но это
заодно забанит и настоящих ванильных клиентов.

**Причина B (проверка вообще не достигается для клиентов без DDNet-версии).**
`OnClientDDNetVersionKnown` вызывается только:
- `gamecontext.cpp:1745` внутри `OnClientEnter`, и только под условием
  `if(Info.m_GotDDNetVersion)` (строка 1743);
- `gamecontext.cpp:2754` в `OnIsDDNetLegacyNetMessage`, т.е. когда клиент прислал DDNet-сообщение.

`Info.m_GotDDNetVersion` = `m_DDNetVersionSettled`, который остаётся `false`, пока клиент
не пришлёт DDNet version message (server.cpp:1200,1237,1271 сбрасывают в false).
Чистый vanilla-клиент никогда его не присылает → условие на строке 1743 ложно →
`OnClientDDNetVersionKnown` не зовётся → ban-check из строки 1999 не выполняется вообще.
Это ровно то, что описано в issue: "logic now does not seem to reach the sv_banned_versions
check when there is no DDNet version for a client".

## Итог

Задача была: НЕ редактировать код, а записать точные old_text -> new_text для фикса.
Однозначного минимального однострочного фикса нет: `GetClientInfo` (строка 722) намеренно
клампит -1 в 0, и от этого зависят другие потребители `m_DDNetVersion`, поэтому просто
убрать клампинг нельзя (сломает прочую логику). Плюс требуется отдельно чинить
достижимость (Причина B). Ниже — предлагаемые изменения (кандидаты), НЕ применённые.

### Кандидат-фикс 1 — не маскировать -1 при отдаче версии
Файл: `src\engine\server\server.cpp`, строка 722

old_text (дословно):
```
		pInfo->m_DDNetVersion = m_aClients[ClientId].m_DDNetVersion >= 0 ? m_aClients[ClientId].m_DDNetVersion : VERSION_VANILLA;
```
new_text (дословно):
```
		pInfo->m_DDNetVersion = m_aClients[ClientId].m_DDNetVersionSettled ? m_aClients[ClientId].m_DDNetVersion : (m_aClients[ClientId].m_DDNetVersion >= 0 ? m_aClients[ClientId].m_DDNetVersion : VERSION_VANILLA);
```
ПРИМЕЧАНИЕ: этот вариант рискован — меняет контракт `m_DDNetVersion` для всех вызывающих.
Точная семантика фикса требует проверки остальных потребителей `Info.m_DDNetVersion`.
Поэтому дословный безопасный old->new здесь дать НЕЛЬЗЯ без изменения архитектуры
(нужно отдавать «сырую» версию отдельным полем или прокидывать VERSION_NONE только в
ban-check). Точечный однострочный дословный патч, гарантированно не ломающий прочее — НЕЛЬЗЯ.

### Кандидат-фикс 2 — достижимость проверки для клиентов без DDNet-версии
Файл: `src\game\server\gamecontext.cpp`, `OnClientEnter`, строки 1739-1750

old_text (дословно):
```
	IServer::CClientInfo Info;
	if(Server()->GetClientInfo(ClientId, &Info))
	{
		// 0.7 clients can send ddnet message, F-Client does that for example
		if(Info.m_GotDDNetVersion)
		{
			if(OnClientDDNetVersionKnown(ClientId))
			{
				return; // kicked
			}
		}
	}
```
Здесь нужно вызывать ban-check и для клиентов без DDNet-версии (когда
`!Info.m_GotDDNetVersion`), т.е. вынести проверку `IsVersionBanned` из-под условия
`m_GotDDNetVersion`. Конкретный дословный new_text зависит от решения по Причине A
(что именно передавать в `IsVersionBanned` для неизвестной версии: -1 или отдельный путь),
поэтому единственно-верный дословный new_text без согласования дизайна дать НЕЛЬЗЯ.

## Вывод по формату задачи
Корень бага локализован точно (server.cpp:722 маскирует -1→0; и/или проверка недостижима
для не-DDNet клиентов из-за условия gamecontext.cpp:1743). Дать дословный
безопасный old_text -> new_text одной правкой — **НЕЛЬЗЯ**: корректный фикс затрагивает
контракт `m_DDNetVersion` и достижимость вызова, что требует дизайн-решения, а не
механической замены строки.

— mister/ \banver
