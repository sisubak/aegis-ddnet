# Аудит фикса #3749 «Finishing shouldn't count to spam filter»

Автор аудита: mister/ \spamfilter
Репозиторий: C:\Users\WWWWWUeHaA\Desktop\antiddos (DDNet)

## Вердикт: НЕЛЬЗЯ / НЕ НАДО править

В текущей версии кода finish-сообщения **не проходят** через флуд-фильтр чата.
Баг #3749 в этой ревизии уже неактуален — править нечего.

---

## Что такое флуд-фильтр (spam filter)

Файл: `src\game\server\gamecontext.cpp`
Функция: `CGameContext::ProcessSpamProtection(int ClientId, bool RespectChatInitialDelay)` — строки **4922–4960**.

Именно здесь начисляется штраф m_ChatScore и выдаётся мут:

```cpp
// строки 4952-4957
if(g_Config.m_SvSpamMuteDuration && (m_apPlayers[ClientId]->m_ChatScore += g_Config.m_SvChatPenalty) > g_Config.m_SvChatThreshold)
{
	MuteWithMessage(Server()->ClientAddr(ClientId), g_Config.m_SvSpamMuteDuration, "Spam protection", Server()->ClientName(ClientId));
	m_apPlayers[ClientId]->m_ChatScore = 0;
	return true;
}
```

Также в 4926 проверяется задержка `m_LastChat`, в 4934 обновляется `m_LastChat`.

Определения:
- `m_ChatScore` объявлен: `src\game\server\player.h:229`, инициализация `player.cpp:68`, декремент по тику `player.cpp:208-209`.
- `m_LastChat`: `src\game\server\player.h:97`.
- Прототип: `src\game\server\gamecontext.h:418`.

Штраф m_ChatScore начисляется **только** через `ProcessSpamProtection`.
Единственная точка входа обычного чата в фильтр — `SendChat` (gamecontext.cpp:704-710):

```cpp
// строки 708-710
if(SpamProtectionClientId >= 0 && SpamProtectionClientId < MAX_CLIENTS)
	if(ProcessSpamProtection(SpamProtectionClientId))
		return;
```

Фильтр срабатывает, ТОЛЬКО если `SpamProtectionClientId` — валидный id игрока (0..MAX_CLIENTS).

---

## Где отправляются finish-сообщения

Файл: `src\game\server\teams.cpp`
Функция: `CGameTeams::OnFinish(...)` — строки **827+**.

Формирование и отправка «finished in» (строки 840-848):

```cpp
str_format(aBuf, sizeof(aBuf),
	"%s finished in: %d minute(s) %5.2f second(s)",
	Server()->ClientName(ClientId), (int)Time / 60,
	Time - ((int)Time / 60 * 60));
if(g_Config.m_SvHideScore)
	GameServer()->SendChatTarget(ClientId, aBuf, CGameContext::FLAG_SIX);
else
	GameServer()->SendChat(-1, TEAM_ALL, aBuf, -1., CGameContext::FLAG_SIX);
```

Аналогично «New record» (строки 858-867) — тоже `SendChat(-1, TEAM_ALL, aBuf, -1, ...)`.

Ключевые моменты:
1. `ChatterClientId = -1` (это server broadcast, не игрок).
2. `SpamProtectionClientId = -1` (5-й аргумент `-1.` / `-1`).
   → в `SendChat` условие `SpamProtectionClientId >= 0` **ложно**, поэтому
     `ProcessSpamProtection` для finish-сообщения **не вызывается вообще**.
3. Ветка `SvHideScore` использует `SendChatTarget` — эта функция (gamecontext.cpp:666-695)
   вообще не содержит вызова спам-фильтра.

Вывод: ни одна ветка отправки finish-сообщения не трогает `m_ChatScore` и не
обновляет `m_LastChat` финишировавшего игрока. Финиш не «расходует» флуд-бюджет.

---

## Почему баг неактуален

Issue #3749 (def-, 30 Mar 2021): «I can't write "gg" in chat immediately after
finishing.» Помечен как `to-reproduce` (надёжно не воспроизведён), PR не привязан.

В текущей ревизии репозитория:
- finish-сообщения идут как server broadcast с `SpamProtectionClientId = -1`;
- `ProcessSpamProtection` вызывается только для сообщений/команд самого игрока
  (см. вызовы: gamecontext.cpp:709, 2054, 2856, 4980, 5138; ddracechat.cpp:573,
  655, 834, 980, 1017, 1202, 1295; player.cpp:1033).

Никакой из этих вызовов не связан с событием финиша. Место, где «finish триггерит
m_ChatScore», отсутствует.

## Правка

old_text -> new_text: **ОТСУТСТВУЕТ.** Кода для изменения нет — фикс не требуется.

Если пользователь всё же жалуется на невозможность писать сразу после финиша, причина
не во флуд-фильтре finish-сообщения, а в обычной задержке чата (`m_LastChat` +
`SvChatDelay`, строка 4926) от его собственных предыдущих сообщений — это отдельное
штатное поведение, к #3749 отношения не имеющее.

---
mister/ \spamfilter
