# Audit: DDNet Issue #5707 — "Practice finish message is broken"

СТАТУС: НЕ ИСПРАВЛЕНО (требует изменения логики финиша/рангов — вне безопасного скоупа задачи)

## Что за баг
Issue https://github.com/ddnet/ddnet/issues/5707 (label: bug, server).
Сообщение о финише в practice-режиме отображается/обрабатывается некорректно.
В обсуждении самого issue прямо сказано, что фикс требует «special eye on
the new logic» и, возможно, установки cheat-состояния игрока при входе в
practice — то есть это НЕ правка текста, а правка игровой логики.

## Разбор кода (src/game/server/teams.cpp)

Точка входа финиша — `OnCharacterFinish(int ClientId)` (строка 203):

```
if(((Team == TEAM_FLOCK || m_aTeamFlock[Team]) && SvTeam != FORCED_SOLO) || Team == TEAM_SUPER)
    -> OnFinish(pPlayer, TimeTicks, aTimestamp);   // ветка A (соло/flock)
else
    -> CheckTeamFinished(m_Core.Team(ClientId));   // ветка B (командный финиш)
```

### Ветка A — `OnFinish()` (строка 827)
Формирует публичное сообщение:
`"%s finished in: %d minute(s) %5.2f second(s)"` (строка 841-844),
шлёт `SendChat(-1, TEAM_ALL, ...)`, считает рекорды, вызывает
`SaveScore()` / `SendFinish()` / `SendRecord()`.

Ключевая проблема: **в `OnFinish()` нет никакой проверки practice-режима.**
Функция всегда объявляет время публично и всегда пытается сохранить ранг.
Проверки на practice здесь отсутствуют (`m_aPractice` в теле `OnFinish`
не встречается).

### Ветка B — `CheckTeamFinished()` (строка 328)
Здесь practice учитывается (строки 359-378):

```
if(m_aPractice[Team])
{
    ChangeTeamState(Team, ETeamState::FINISHED);
    ...
    str_format(aBuf, ...,
        "Your team would've finished in: %d minute(s) %5.2f second(s). "
        "Since you had practice mode enabled your rank doesn't count.", ...);
    GameServer()->SendChatTeam(Team, aBuf);
    for(...) SetDDRaceState(apTeamPlayers[i], ERaceState::FINISHED);
    return;   // ранг НЕ сохраняется, OnFinish НЕ вызывается — корректно
}
```

## Суть поломки
Обработка финиша в practice-режиме размазана и непоследовательна между двумя
ветками:
- Ветка B (командный путь) корректно перехватывает practice ДО вызова
  `OnFinish` и шлёт спец-сообщение «would've finished… rank doesn't count».
- Ветка A (`OnFinish`, соло/flock/super) вообще не знает про practice.

Practice включается только в залоченной команде (Team >= 1), поэтому в норме
финиш такой команды идёт через ветку B. Однако состояние practice и
переходы состояний команды/игрока (`ETeamState`, `ERaceState`, cheat-флаги)
управляются раздельно от места формирования сообщения, из-за чего в ряде
сценариев (perma-practice команды, вход в practice во время рана) сообщение
и/или сохранение ранга отрабатывают некорректно. Мейнтейнеры в issue
предлагают унифицировать: помечать игрока как cheated сразу при входе в
practice и обрабатывать все читы одинаково.

## Почему НЕ трогал
Правка касается:
- логики сохранения/подсчёта рангов (`SaveScore`, `m_BestTime`, `SendRecord`);
- состояний `ETeamState` / `ERaceState` и cheat-флагов;
- ветвления `OnCharacterFinish` между соло- и командным путём.

Это игровая логика финиша, а не текст сообщения. По условию задачи такие
изменения не применяю, только документирую. Грамматических/форматных ошибок
в самих строках сообщений (`teams.cpp:841`, `teams.cpp:367`) не обнаружено —
проблема именно в потоке логики.

## Рекомендация для реального фикса (не применялось)
Добавить в `OnFinish()` ранний перехват practice-режима команды игрока
(по аналогии с `CheckTeamFinished`), либо централизовать обработку practice
в `OnCharacterFinish` до разветвления, помечая игроков cheated при входе в
practice. Требует ревью мейнтейнера и тестов на perma-practice командах.

## Проверенные файлы
- src/game/server/teams.cpp (OnCharacterFinish:203, CheckTeamFinished:328,
  OnFinish:827, m_aPractice usages)
- src/game/server/teams.h (m_aPractice[NUM_DDRACE_TEAMS]:37, OnFinish decl:62)
- src/game/server/gamemodes/DDRace.cpp — совпадений по OnFinish/practice/rank
  НЕТ (логика финиша полностью в teams.cpp)

-- mister/ \practice
