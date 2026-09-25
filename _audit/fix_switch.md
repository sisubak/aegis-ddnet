# fix_switch.md — Issue #11485 (Switch layer "hit others" incorrect Delay indices)

Автор: mister/ \switch (доработано mister/ \day)
Проект: DDNet (C++), C:\Users\WWWWWUeHaA\Desktop\antiddos

## Корень бага

Файл: src/game/server/entities/character.cpp, функция CCharacter::HandleTiles,
блок "handle switch tiles", строки 1894-1933.

Для тайлов TILE_HIT_ENABLE / TILE_HIT_DISABLE со switch-слоем поле "Delay"
(Collision()->GetSwitchDelay(MapIndex)) переиспользуется как НОМЕР оружия и
сравнивается с константами WEAPON_HAMMER(0), WEAPON_SHOTGUN(2),
WEAPON_GRENADE(3), WEAPON_LASER(4):

- 1894/1899: SwitchDelay == WEAPON_HAMMER  -> hammer hit
- 1904/1909: SwitchDelay == WEAPON_SHOTGUN -> shotgun hit
- 1914/1919: SwitchDelay == WEAPON_GRENADE -> grenade hit
- 1924/1929: SwitchDelay == WEAPON_LASER   -> laser hit

Проблема (issue #11485): значение Delay в редакторе карт маппера трактуется
не так, как эти WEAPON_* индексы. Значение WEAPON_GUN(1) вообще не
обрабатывается ни одной веткой (пропуск), а нумерация оружий в UI редактора
и в enum рассинхронизирована, из-за чего "hit others" на switch-слое включает
не то оружие, которое выбрал маппер.

## Почему НЕ ПРАВИЛ

Это fix-changes-physics / изменение поведения существующих карт — ВЫСОКИЙ РИСК.
Любая правка сопоставления Delay->оружие изменит поведение уже
опубликованных карт, использующих эти switch-тайлы (сломает прохождения,
демки, рекорды). Корректный фикс требует согласованного изменения и в
редакторе карт (DDNet editor), и в серверной логике, и договорённости о
формате — это не локальная безопасная правка.

## Кандидат-фикс (НЕ применён)

Согласовать нумерацию: добавить обработку WEAPON_GUN и привести сравнение
Delay к тому индексу, который пишет редактор. Например через явную таблицу
соответствия Delay->WEAPON_* в одном месте, вместо разбросанных сравнений.
Но применять только вместе с правкой редактора и после решения мейнтейнеров
о совместимости со старыми картами.

## Итог
НЕ ПРАВИЛ (fix-changes-physics, высокий риск для существующих карт).
Файл/строки: src/game/server/entities/character.cpp:1894-1933.
