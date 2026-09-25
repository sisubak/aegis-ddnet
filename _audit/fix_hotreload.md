# fix_hotreload.md — Issues #10344 и #9203 (hot_reload)

Автор: mister/ \hotreload
Проект: DDNet (C++), C:\Users\WWWWWUeHaA\Desktop\antiddos

## Механизм hot_reload
CGameContext::ConHotReload (gamecontext.cpp:3434-3461):
- Для каждого живого чара сохраняет CSaveHotReloadTee в m_apSavedTees[i]
  (3445-3447) и состояние команды в m_apSavedTeams через m_aTeamMapping[i]
  (3450-3458).
- Вызывает Server()->ReloadMap() (3460) — карта перезагружается, все чары
  пересоздаются.
- При спавне чара после релоуда CCharacter::Spawn (character.cpp:122-142)
  читает m_aTeamMapping[i], восстанавливает команду через SetForceCharacterTeam,
  затем Load из m_apSavedTeams[Team] и m_apSavedTees[i].

## Issue #10344 — dummy остаётся, игрок дисконнектится
Корень: ConHotReload сохраняет только чаров, у которых GetPlayerChar(i) != null
(3439). После ReloadMap клиенты переустанавливают соединение. Если у игрока есть
dummy (второй клиент того же владельца), при пересоздании клиентов основной
игрок и dummy обрабатываются как независимые слоты. Восстановление завязано на
CCharacter::Spawn, который срабатывает только для тех, кто успешно
переподключился и заспавнился. Рассинхрон между реконнектом основного клиента и
dummy приводит к тому, что один из них выпадает.

Кандидат-фикс (НЕ применён): при hot_reload хранить связь main<->dummy
(DummyMapping) и восстанавливать/дропать их атомарно, либо форсить одинаковый
путь реконнекта. Требует правки логики реконнекта в server.cpp
(m_IngameBeforeRejoin, см. server.cpp:2144) и dummy-обработки в gamecontext.
Риск высокий — затрагивает сетевой реконнект.

## Issue #9203 — теряется weak/strong hook
Корень: CSaveHotReloadTee::Save/Load (save.cpp:536-545) сохраняет состояние тича,
но порядок пересоздания чаров после релоуда определяет weak/strong (hook
collision priority) по индексу в m_World.m_Core.m_apCharacters. Если игрок был в
/spec (не имел живого чара) на момент hot_reload, он не попадает в m_apSavedTees
(условие GetPlayerChar на 3439), и после релоуда спавнится как новый — с
дефолтным (strong) приоритетом, теряя прежний weak/strong-порядок.

Кандидат-фикс (НЕ применён): сохранять и восстанавливать порядок/приоритет
weak-strong явно (индекс в core-массиве или флаг), независимо от того, был ли чар
живым. Требует расширения CSaveHotReloadTee и аккуратной работы с порядком
InsertEntity. Риск средний-высокий — влияет на физику hook collision.

## СТАТУС
- #10344: ANALYZED — НЕ ПРИМЕНЕНО (реконнект main/dummy, высокий риск).
- #9203: ANALYZED — НЕ ПРИМЕНЕНО (weak/strong порядок, влияет на физику).
Код не менялся. Ключевые места: gamecontext.cpp:3434-3461,
character.cpp:122-142, save.cpp:536-545, server.cpp:2144.
