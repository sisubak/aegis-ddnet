# Аудит: projectile owner id матчит не того игрока при reconnect (#10398 / #10366)

Проект: DDNet (C++). Автор аудита: mister/ \projowner
Статус: **НЕ ПРАВИЛ, высокий риск** (изменения затрагивают поведение снарядов/лазеров и сетевой снапшот).

## Затронутые файлы
- `src/game/server/entities/projectile.cpp` / `projectile.h`
- `src/game/server/entities/laser.cpp` / `laser.h`
- `src/game/server/gamecontext.cpp` (`OnClientDrop`, строки 1878-1925)

## Как хранится и используется m_Owner

### projectile.h
- `int m_Owner;` (строка 41) — id владельца, задаётся в конструкторе.
- `int GetOwnerId() const override { return m_Owner; }` (строка 63).

### projectile.cpp
- Конструктор: `m_Owner = Owner;` (строка 34). Из него же считаются
  `m_DDRaceTeam` (стр. 50), `m_IsSolo` (стр. 51), `m_GrenadeHitDisabled` (стр. 52),
  `m_BelongsToPracticeTeam` (стр. 49) — всё привязано к текущему владельцу m_Owner.
- Tick/коллизии: `GetPlayerChar(m_Owner)` (стр. 48, 100-101), `IntersectCharacter(..., m_Owner)` (стр. 106),
  `CanCollide(m_Owner)` (стр. 118), `CreateExplosion(..., m_Owner, ...)` (стр. 143, 235),
  `TakeDamage(..., m_Owner, ...)` (стр. 159).
- Снап-инфо:
  - `NetInfoLegacy` (стр. 284): `int Owner = m_Owner; Server()->Translate(Owner, SnappingClient)` → пакует id в `Data` (стр. 296-298).
  - `NetInfo` (стр. 318): `int Owner = m_Owner; Translate(...)` → `Result.m_Owner = Owner;` (стр. 360).
  - `Snap` (стр. 367) вызывает `SnapNewItem(...)` — клиенту уходит owner id.
- `SwapClients` (стр. 411-414): `m_Owner = m_Owner == Client1 ? Client2 : (...)` — перестановка id, используется при обмене слотов.
- `CanCollide` (стр. 418-424): опирается на `m_DDRaceTeam` и `m_Owner == ClientId`.

### laser.cpp/.h (аналогичный паттерн)
- `int m_Owner;` (laser.h стр. 34), `GetOwnerId()` (стр. 20).
- `m_Owner = Owner;` (laser.cpp стр. 23); коллизии/урон через `m_Owner` (стр. 35-100),
  `GetDDRaceTeam(m_Owner)` (стр. 320).
- `Snap` (стр. 284): `SnapNewItem(... m_Owner, LaserType ...)` (стр. 296) — заметно, что здесь m_Owner
  идёт в снап **без** `Server()->Translate(...)`, в отличие от projectile (потенциально отдельная мелкая
  несогласованность, но не корень этого бага).
- `SwapClients` (стр. 301): та же логика перестановки.

## Корень проблемы

`m_Owner` — это индекс слота клиента (`ClientId`, 0..MAX_CLIENTS-1), а не устойчивый идентификатор
конкретного игрока/сессии. Снаряд (projectile) и лазер живут во времени (grenade, shotgun bounce, laser
с energy) и продолжают существовать после того, как их владелец отключился.

В `OnClientDrop` (gamecontext.cpp, стр. 1878-1925) при выходе игрока:
- удаляется `m_apPlayers[ClientId]`, saved teams/tees, обнуляется `m_aTeamMapping`,
- **но НЕ выполняется зачистка/переназначение уже существующих сущностей (projectile/laser),
  у которых `m_Owner == ClientId`.**

Снаряды сносятся только по смерти персонажа (`m_Owner >= 0 && (m_Type != WEAPON_GRENADE ||
g_Config.m_SvDestroyBulletsOnDeath ...)` — projectile.cpp стр. 126) либо гранаты просто продолжают лететь.
Значит после дисконнекта в мире могут оставаться «осиротевшие» снаряды с `m_Owner == X`.

Когда на тот же слот `X` заходит **новый** игрок (reconnect / другой клиент занял свободный слот),
старый снаряд с `m_Owner == X` начинает ассоциироваться с новым игроком:
- в снапе (`NetInfo`/`NetInfoLegacy`) owner id = X уходит клиентам → визуально/по логике снаряд
  «принадлежит» новому игроку;
- коллизии, `CreateExplosion(..., m_Owner, ...)`, `TakeDamage(..., m_Owner, ...)`, начисление kill/урона
  привязываются к новому игроку в слоте X.

Это и есть симптом из #10398/#10366: **owner id снаряда матчит не того игрока после reconnect**.
Проблема усугубляется тем, что производные поля (`m_DDRaceTeam`, `m_IsSolo`, `m_GrenadeHitDisabled`,
`m_BelongsToPracticeTeam`) были посчитаны от СТАРОГО владельца в конструкторе и не пересчитываются.

## Кандидат-фикс (НЕ применён)

Возможные направления (каждое меняет поведение снарядов → требует ревью мейнтейнеров и тестов):

1. Зачистка сущностей владельца при дисконнекте.
   В `OnClientDrop` до/после удаления игрока пройтись по `ENTTYPE_PROJECTILE` и `ENTTYPE_LASER`
   (аналогично логике `SvDestroyBulletsOnDeath`) и удалить/сбросить те, у кого `GetOwnerId() == ClientId`,
   или выставить им `m_Owner = -1` (owner-less), чтобы они не приписывались новому игроку.
   - Плюс: точечно, локализовано в одном месте.
   - Минус: меняет геймплей (гранаты/лазеры внезапно исчезают при выходе владельца); нужен конфиг-флаг.

2. Пометка «осиротевших» снарядов: при дисконнекте выставлять `m_Owner = -1` всем сущностям слота
   (сохранить полёт, но отвязать от нового игрока). Тогда `CreateExplosion`/`TakeDamage` идут как no-owner
   (`m_Owner == -1` уже обрабатывается по всему коду).
   - Плюс: не ломает физику снаряда, устраняет ложный матч владельца.
   - Минус: нужно пересчитать/зафиксировать `m_DDRaceTeam`, чтобы коллизии в команде не сломались.

3. Ввести устойчивый идентификатор сессии (unique owner token) отдельно от ClientId и матчить по нему;
   ClientId использовать только для снапа с `Translate`.
   - Плюс: концептуально правильно.
   - Минус: крупное изменение протокола/сущностей, самый высокий риск.

Рекомендуемый минимально-инвазивный вариант для обсуждения с мейнтейнерами: **вариант 2** (отвязка
`m_Owner = -1` осиротевших projectile/laser в `OnClientDrop`) с корректным сбросом производных полей.

## Дополнительная находка (побочная)
`CLaser::Snap` (laser.cpp стр. 296) пишет `m_Owner` в снап без `Server()->Translate(m_Owner, SnappingClient)`,
тогда как `CProjectile::NetInfo/NetInfoLegacy` используют Translate. Стоит проверить отдельно — не корень
этого бага, но потенциальная несогласованность owner id в снапе лазера.

---
**Вывод: НЕ ПРАВИЛ. Высокий риск — изменения затрагивают физику/снап снарядов и требуют ревью.**

— mister/ \projowner
