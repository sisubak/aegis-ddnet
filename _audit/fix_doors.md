# Аудит багов дверей (doors) — ТОЛЬКО АНАЛИЗ, КОД НЕ ПРАВЛЕН

Автор: mister/ \doors. Физика/коллизия = высокий риск, поэтому по обоим багам
даны кандидат-фиксы, но НЕ применены.

Изученные файлы/строки:
- `src/game/collision.cpp`
  - `CCollision::Init` — строки 51–120 (аллокация `m_pDoor`, инициализация из switch-слоя)
  - `CCollision::GetMoveRestrictions(callback...)` — строки 262–309 (применение door-рестрикций)
  - `CCollision::TileExists` / `TileExistsNext` — строки 845–907
  - `CCollision::SetDoorCollisionAt` — строки 1091–1101 (запись door-тайла)
  - `CCollision::GetDoorTile` — строки 1103–1113 (чтение door-тайла)
- `src/game/server/entities/door.cpp` — весь файл (1–83): `CDoor::CDoor`, `ResetCollision` (28–41), `Snap` (48–83)
- `src/game/server/gamecontroller.cpp` — строки 204–219 (создание `CDoor` из `ENTITY_DOOR`)
- `src/game/gamecore.cpp` — `CCharacterCore::IsSwitchActiveCb` строки 740–747, `InitSwitchers` 749+
- `src/game/server/entities/character.cpp` — `IsSwitchActiveCb` строки 1597–1602, вызов 1642
- `src/game/client/prediction/entities/character.cpp` — `IsSwitchActiveCb` 772, вызов 784
- `_audit/issues.json` — #11068 (строки 108–118), #2526 (строки 529–539)

Ключевая структура: `m_pDoor` — это массив `CDoorTile[m_Width*m_Height]`,
т.е. РОВНО ОДИН door-тайл (`m_Index`, `m_Flags`, `m_Number`) на клетку карты.

---

## Баг A — #2526 "Doors without having switch layer are always opened"

### Корень бага
В `CCollision::Init` (collision.cpp:71–76) массив `m_pDoor` выделяется ТОЛЬКО
внутри `if(m_pLayers->SwitchLayer())`:

```cpp
if(m_pLayers->SwitchLayer())
{
    m_pSwitch = ...;
    m_pDoor = new CDoorTile[m_Width * m_Height];
    mem_zero(...);
}
```

Если на карте нет switch-слоя, `m_pDoor` остаётся `nullptr`.

Цепочка последствий на карте без switch-слоя:
1. `CDoor::ResetCollision()` (door.cpp:39) вызывает
   `Collision()->SetDoorCollisionAt(...)`, но в `SetDoorCollisionAt`
   (collision.cpp:1093) стоит `if(!m_pDoor) return;` → коллизия двери НЕ ставится.
2. `GetDoorTile` (collision.cpp:1105) при `!m_pDoor` возвращает пустой тайл, а в
   `GetMoveRestrictions` (302) `pfnSwitchActive` при пустых `Switchers()` вернёт
   `false` → door-рестрикция не применяется.

Итог: дверь без switch-слоя не имеет коллизии вообще → сквозь неё можно пройти,
т.е. она «always opened» (хотя по смыслу дверь без переключателя = номер 0 =
должна быть постоянно ЗАКРЫТА).

### Кандидат-фикс (old -> new)
Выделять `m_pDoor` независимо от наличия switch-слоя, чтобы door-сущности могли
регистрировать коллизию.

old (collision.cpp:71–76):
```cpp
	if(m_pLayers->SwitchLayer())
	{
		m_pSwitch = static_cast<CSwitchTile *>(m_pLayers->Map()->GetData(m_pLayers->SwitchLayer()->m_Switch));
		m_pDoor = new CDoorTile[m_Width * m_Height];
		mem_zero(m_pDoor, (size_t)m_Width * m_Height * sizeof(CDoorTile));
	}
```

new (вариант):
```cpp
	if(m_pLayers->SwitchLayer())
	{
		m_pSwitch = static_cast<CSwitchTile *>(m_pLayers->Map()->GetData(m_pLayers->SwitchLayer()->m_Switch));
	}
	// m_pDoor нужен для любых ENTITY_DOOR, даже без switch-слоя
	m_pDoor = new CDoorTile[m_Width * m_Height];
	mem_zero(m_pDoor, (size_t)m_Width * m_Height * sizeof(CDoorTile));
```

Дополнительно требуется проверить семантику switch-номера 0 в
`GetMoveRestrictions`/`IsSwitchActiveCb`: дверь с `m_Number == 0` без активных
переключателей должна трактоваться как постоянно закрытая (в т.ч. корректный
вызов `InitSwitchers` при `HighestSwitchNumber == 0`, gamecore.cpp:749). Иначе
даже с выделенным `m_pDoor` callback вернёт `false` и дверь останется открытой.

### Почему НЕ применён
- Меняет инициализацию коллизии для ВСЕХ карт (в т.ч. без switch-слоя) — прямое
  влияние на физику/проходимость, высокий риск регрессий на существующих картах.
- Требует согласованного изменения семантики switch-номера 0 и поведения
  `IsSwitchActiveCb` в трёх местах (server character, client prediction,
  gamecore) — иначе фикс будет неполным и может рассинхронить предсказание
  клиента с сервером.
- Задача этой сессии — только анализ, физику/коллизию не трогаем.

---

## Баг B — #11068 "door crossings can be passable, even if one door is closed"
(лейбл fix-changes-physics — НЕ МЕНЯТЬ)

### Корень бага
`m_pDoor` хранит ровно один `CDoorTile` на клетку. Каждый `CDoor::ResetCollision`
пишет свой `m_Index/m_Flags/m_Number` в клетки, через которые проходит луч двери
(`SetDoorCollisionAt`, collision.cpp:1098–1100). Когда две двери ПЕРЕСЕКАЮТСЯ на
одной клетке, вторая `ResetCollision` ПЕРЕЗАПИСЫВАЕТ данные первой — в клетке
остаётся только номер/флаги последней записанной двери.

В сообщении из issue: у дверей на пересечении разные номера (1 и 101). В точке
пересечения хранится номер лишь одной двери. `GetMoveRestrictions` (collision.cpp:
299–305) читает единственный `DoorTile.m_Number` и проверяет только его
переключатель. Если сохранённая дверь открыта, а перезаписанная (закрытая) дверь
потеряна — рестрикция не применяется → перекрёсток проходим, хотя одна из дверей
закрыта.

### Кандидат-фикс (old -> new) — концептуально
Хранить НЕСКОЛЬКО door-тайлов на клетку и в `GetMoveRestrictions` применять
рестрикции ото ВСЕХ дверей в клетке (блокировать, если блокирует любая закрытая
дверь).

old (collision.cpp:299–305):
```cpp
			CDoorTile DoorTile;
			GetDoorTile(ModMapIndex, &DoorTile);
			if((int)DoorTile.m_Number <= m_HighestSwitchNumber &&
				pfnSwitchActive(DoorTile.m_Number, pUser))
			{
				Restrictions |= ::GetMoveRestrictions(d, DoorTile.m_Index, DoorTile.m_Flags);
			}
```

new (концепт):
```cpp
			// перебрать все двери в этой клетке (m_pDoor как список на клетку)
			for(const CDoorTile &DoorTile : GetDoorTiles(ModMapIndex))
			{
				if((int)DoorTile.m_Number <= m_HighestSwitchNumber &&
					pfnSwitchActive(DoorTile.m_Number, pUser))
				{
					Restrictions |= ::GetMoveRestrictions(d, DoorTile.m_Index, DoorTile.m_Flags);
				}
			}
```

Это требует смены типа `m_pDoor` (одиночный тайл → контейнер тайлов на клетку),
переписывания `SetDoorCollisionAt` (добавление вместо перезаписи), `GetDoorTile`,
`TileExists`/`TileExistsNext` и снапшота (`CDoor::Snap`).

### Почему НЕ применён
- Лейбл fix-changes-physics и прямое указание в задаче: #11068 НЕ трогать.
- Меняет структуру данных коллизии и напрямую меняет физику на любых картах с
  пересекающимися дверьми — очень высокий риск.
- Требует синхронного изменения серверной и клиентской (prediction) логики, иначе
  рассинхрон предсказания.

---

## Вывод
Оба бага сводятся к ограничениям модели `m_pDoor`:
- #2526: массив вообще не аллоцируется без switch-слоя → двери без переключателя
  без коллизии («открыты»).
- #11068: один door-тайл на клетку → пересекающиеся двери затирают друг друга.

Оба фикса затрагивают коллизию/физику (высокий риск) и не применены в этой сессии.
