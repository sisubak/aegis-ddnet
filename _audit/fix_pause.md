# fix_pause.md — Issue #12727 (force_pause отрицательное время)

Автор: mister/ \pause
Проект: DDNet (C++), C:\Users\WWWWWUeHaA\Desktop\antiddos

## Анализ

Функция `CPlayer::IsPaused()` (src/game/server/player.cpp:984-987) возвращает
`m_ForcePauseTime` (абсолютный тик окончания паузы), если он ненулевой.

В `player.cpp:202-206` `m_ForcePauseTime` сбрасывается в 0 только когда
`m_ForcePauseTime < Server()->Tick()`. Есть окно: когда до конца паузы остаётся
меньше секунды (`PauseState - Tick() < TickSpeed()`, но всё ещё > 0), выражение
`(PauseState - pServ->Tick()) / pServ->TickSpeed()` даёт целочисленный 0 —
это ок. Проблема же в том, что в ddracechat.cpp сообщение печатается по условию
`PauseState > 0`, а `PauseState` — это АБСОЛЮТНЫЙ тик. Пока `m_ForcePauseTime`
ещё не обнулён в `Tick()` (порядок обработки/спек-team), но уже
`m_ForcePauseTime <= Server()->Tick()`, разность становится <= 0, и деление
даёт отрицательное число секунд в тексте "You are force-paused for %d seconds.".

Минимальный безопасный фикс: clamp отображаемого значения через
`std::max(..., 0)`, чтобы никогда не показывать отрицательное время. Физика,
имена функций и логика паузы не тронуты.

## Применённые правки

Файл: src/game/server/ddracechat.cpp

### Правка 1 — include для std::max (строки 6-7 -> 6-9)
old:
```
#include <base/log.h>
#include <base/time.h>
```
new:
```
#include <base/log.h>
#include <base/time.h>

#include <algorithm>
```

### Правка 2 — ToggleSpecPause, вывод сообщения (строка 233, после вставки include -> 235)
old:
```
		str_format(aBuf, sizeof(aBuf), "You are force-paused for %d seconds.", (PauseState - pServ->Tick()) / pServ->TickSpeed());
```
new:
```
		str_format(aBuf, sizeof(aBuf), "You are force-paused for %d seconds.", std::max((PauseState - pServ->Tick()) / pServ->TickSpeed(), 0));
```

### Правка 3 — ToggleSpecPauseVoted, вывод сообщения (строка 273, после вставки include -> 275)
old:
```
		str_format(aBuf, sizeof(aBuf), "You are force-paused for %d seconds.", (PauseState - pServ->Tick()) / pServ->TickSpeed());
```
new:
```
		str_format(aBuf, sizeof(aBuf), "You are force-paused for %d seconds.", std::max((PauseState - pServ->Tick()) / pServ->TickSpeed(), 0));
```

Правки 2 и 3 сделаны одним replace_in_file с count=0 (2 идентичных вхождения).

## Не тронуто
- player.cpp:970-982 ForcePause — без изменений
- player.cpp:984-987 IsPaused — без изменений
- player.cpp:202-206 Tick сброс таймера — без изменений (это корректная логика)

## Итог
ПРИМЕНЕНО. Файл: src/game/server/ddracechat.cpp, строки ~8 (include),
235 и 275 (clamp через std::max(...,0)).
