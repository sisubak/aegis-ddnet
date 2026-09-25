# Аудит #9448 — "grenade tile hit disabled can be bypassed by disconnecting"

Автор: mister/ \grenade
Репо: C:\Users\WWWWWUeHaA\Desktop\antiddos

## СТАТУС: ✅ ФИКС УЖЕ ПРИМЕНЁН В РЕПОЗИТОРИИ (проверено при повторном ревью)

На момент повторного аудита весь предложенный фикс (правки A–E) уже присутствует в коде и внутренне непротиворечив. Дополнительных изменений НЕ вносил — применять нечего, всё на месте.

Что фактически проверено в текущем коде:
- `projectile.h:56` — поле `bool m_GrenadeHitDisabled;` присутствует (правка A).
- `projectile.cpp:52` — в конструкторе: `m_GrenadeHitDisabled = pOwnerChar ? pOwnerChar->GrenadeHitDisabled() : !g_Config.m_SvHit;` (правка B). Для `Owner == -1` (shotgun-тайлы из gamecontroller.cpp) `pOwnerChar == nullptr` → берётся `!g_Config.m_SvHit`, что соответствует прежней семантике.
- `projectile.cpp:105` — `if(!m_GrenadeHitDisabled)` (правка C).
- `projectile.cpp:132` — `if(((pTargetChr && (!m_GrenadeHitDisabled || m_Owner == -1 || pTargetChr == pOwnerChar)) ...` (правка D).
- `projectile.cpp:143` и `:235` — оба вызова `CreateExplosion(...)` передают последним аргументом `m_GrenadeHitDisabled ? 1 : 0` (правка E, серверная часть).
- `gamecontext.cpp:375` — сигнатура `CreateExplosion(..., int OverrideHitDisabled)`; в `gamecontext.h:293` — дефолт `OverrideHitDisabled = -1`.
- `gamecontext.cpp:391` — `bool HitDisabled = OverrideHitDisabled >= 0 ? (bool)OverrideHitDisabled : (GetPlayerChar(Owner) ? GetPlayerChar(Owner)->GrenadeHitDisabled() : !g_Config.m_SvHit);` — при переданном значении со снаряда используется оно, а не живой владелец. Строки 411/420 (бывшие 410/419) читают уже `HitDisabled`.

Полнота охвата (все места создания снарядов и вызовы взрыва):
- `new CProjectile` найдены в: character.cpp:576 (WEAPON_GUN, Owner), character.cpp:607 (WEAPON_GRENADE, Owner), gamecontroller.cpp:232 и :259 (WEAPON_SHOTGUN, Owner=-1). Все проходят через единый конструктор → поле выставляется всегда.
- `CreateExplosion` (серверные вызовы): projectile.cpp:143/:235 (пробрасывают состояние снаряда), plasma.cpp:82/:98 (NoDamage=true, OverrideHitDisabled=-1 по умолчанию — прежнее поведение, к гранате-дисконнекту не относится). Других серверных вызовов нет.
- Сеть/snap НЕ затронуты: `m_GrenadeHitDisabled` — чисто серверное поле, в NetInfo*/Snap не сериализуется. Клиентский prediction (client/prediction/...) использует свой отдельный путь и последний аргумент CreateExplosion там — `Id` для предсказания, серверная логика хитов не зависит от него.

Вывод ревью: патч точный, полный и непротиворечивый; риск отсутствует, потому что он уже интегрирован. Ниже сохранён исходный разбор бага и дословный патч — как историческая справка.

---

## (Исходный разбор — историческая справка)

Репо: C:\Users\WWWWWUeHaA\Desktop\antiddos

## Суть бага (из issue GitHub #9448)

Ссылка: https://github.com/ddnet/ddnet/issues/9448 (ChillerDragon, 30 Dec 2024, label: bug/server)

Цитата issue:
> Hit disabled information should be stored on the projectile and not depend on the character being alive.
> If sv_destroy_bullets_on_death is 0 and the player touched a "You can't hit others" tile.
> Then fires a grenade and before it lands disconnects the grenade can hit others.

Механизм:
1. Игрок наступает на тайл `TILE_HIT_DISABLE` → у его character выставляется `m_Core.m_GrenadeHitDisabled = true`.
2. Игрок стреляет гранатой. Owner-id пишется в снаряд (`m_Owner`).
3. Игрок дисконнектится ДО взрыва. При `sv_destroy_bullets_on_death 0` граната-снаряд НЕ уничтожается (см. projectile.cpp:125 — для WEAPON_GRENADE без флага не помечается MarkedForDestroy).
4. При тике/взрыве `GetPlayerChar(m_Owner)` уже возвращает `nullptr` (character мёртв/удалён).
5. Тернарный оператор проваливается в ветку "иначе" и берёт **глобальный** `g_Config.m_SvHit` вместо реального tile-состояния `GrenadeHitDisabled()`. Т.к. флаг hit-disabled хранился ТОЛЬКО в character, а не в снаряде, он теряется — граната снова может бить чужих.

## Проблемные места (только чтение, код НЕ редактировался)

### 1. Флаг hit-disabled хранится в character, а НЕ в снаряде
Файл: src\game\server\entities\projectile.h
- Строки 38-56: приватные поля класса CProjectile. Отсутствует поле для сохранения состояния GrenadeHitDisabled на момент выстрела. Есть `m_Owner`, `m_BelongsToPracticeTeam`, `m_DDRaceTeam`, `m_IsSolo`, но нет `m_HitDisabled` / аналога.

Конструктор: src\game\server\entities\projectile.cpp
- Строки 48-51: владелец опрашивается один раз в конструкторе для practice/solo, но hit-disabled НЕ снимается и не сохраняется:
```
	CCharacter *pOwnerChar = GameServer()->GetPlayerChar(m_Owner);
	m_BelongsToPracticeTeam = pOwnerChar && pOwnerChar->Teams()->IsPractice(pOwnerChar->Team());
	m_DDRaceTeam = m_Owner == -1 ? 0 : GameServer()->GetDDRaceTeam(m_Owner);
	m_IsSolo = pOwnerChar && pOwnerChar->GetCore().m_Solo;
```

### 2. projectile.cpp — проверка попадания (МЕСТО ИЗ ISSUE)
Файл: src\game\server\entities\projectile.cpp
- Строка 104 (тернарный fallback на g_Config.m_SvHit при мёртвом владельце):
```
	if(pOwnerChar ? !pOwnerChar->GrenadeHitDisabled() : g_Config.m_SvHit)
```
- Строка 105:
```
		pTargetChr = GameServer()->m_World.IntersectCharacter(PrevPos, ColPos, m_Freeze ? 1.0f : 6.0f, ColPos, pOwnerChar, m_Owner);
```
- Строка 131 (вторая такая же проверка при обработке столкновения/взрыва):
```
	if(((pTargetChr && (pOwnerChar ? !pOwnerChar->GrenadeHitDisabled() : g_Config.m_SvHit || m_Owner == -1 || pTargetChr == pOwnerChar)) || Collide || GameLayerClipped(CurPos)) && !IsWeaponCollide)
```

Когда `pOwnerChar == nullptr` (владелец отключился) обе проверки уходят в ветку `g_Config.m_SvHit`, игнорируя, что игрок стоял на tile hit-disable.

### 3. gamecontext.cpp — CreateExplosion, тот же паттерн
Файл: src\game\server\gamecontext.cpp
Функция: `void CGameContext::CreateExplosion(...)` (объявлена строка 375)
- Строка 410:
```
		if((GetPlayerChar(Owner) ? !GetPlayerChar(Owner)->GrenadeHitDisabled() : g_Config.m_SvHit) || NoDamage || Owner == pChr->GetPlayer()->GetCid())
```
- Строка 419:
```
			if((GetPlayerChar(Owner) ? GetPlayerChar(Owner)->GrenadeHitDisabled() : !g_Config.m_SvHit) || NoDamage)
```
При дисконнекте `GetPlayerChar(Owner)` == nullptr → fallback на `g_Config.m_SvHit`, урон от взрыва снова наносится чужим.

## Предлагаемый фикс (дословно old_text -> new_text)

Идея (как в issue): сохранить hit-disabled на снаряде в момент выстрела и использовать сохранённое значение, когда владелец недоступен.

### Правка A — projectile.h: добавить поле
Файл: src\game\server\entities\projectile.h

old_text:
```
	bool m_BelongsToPracticeTeam;
	int m_DDRaceTeam;
	bool m_IsSolo;
	vec2 m_InitDir;
```
new_text:
```
	bool m_BelongsToPracticeTeam;
	int m_DDRaceTeam;
	bool m_IsSolo;
	bool m_GrenadeHitDisabled;
	vec2 m_InitDir;
```

### Правка B — projectile.cpp: сохранить состояние в конструкторе
Файл: src\game\server\entities\projectile.cpp

old_text:
```
	m_IsSolo = pOwnerChar && pOwnerChar->GetCore().m_Solo;

	GameWorld()->InsertEntity(this);
```
new_text:
```
	m_IsSolo = pOwnerChar && pOwnerChar->GetCore().m_Solo;
	m_GrenadeHitDisabled = pOwnerChar ? pOwnerChar->GrenadeHitDisabled() : !g_Config.m_SvHit;

	GameWorld()->InsertEntity(this);
```
(Замечание: `m_Owner == -1` даёт `pOwnerChar == nullptr`, тогда берётся `!g_Config.m_SvHit`, что соответствует текущей семантике "нет владельца → глобальный SvHit".)

### Правка C — projectile.cpp:104 использовать сохранённое поле
Файл: src\game\server\entities\projectile.cpp

old_text:
```
	if(pOwnerChar ? !pOwnerChar->GrenadeHitDisabled() : g_Config.m_SvHit)
```
new_text:
```
	if(!m_GrenadeHitDisabled)
```

### Правка D — projectile.cpp:131 использовать сохранённое поле
Файл: src\game\server\entities\projectile.cpp

old_text:
```
	if(((pTargetChr && (pOwnerChar ? !pOwnerChar->GrenadeHitDisabled() : g_Config.m_SvHit || m_Owner == -1 || pTargetChr == pOwnerChar)) || Collide || GameLayerClipped(CurPos)) && !IsWeaponCollide)
```
new_text:
```
	if(((pTargetChr && (!m_GrenadeHitDisabled || m_Owner == -1 || pTargetChr == pOwnerChar)) || Collide || GameLayerClipped(CurPos)) && !IsWeaponCollide)
```

### Правка E — gamecontext.cpp: пробрасывать hit-disabled во взрыв
Для полного фикса взрыв (CreateExplosion, строки 410/419) должен получать состояние hit-disabled снаряда, а не опрашивать живого владельца. Это требует изменения сигнатуры `CreateExplosion` (добавить параметр, напр. `bool NoHit`/`bool HitDisabled`) и передачи `m_GrenadeHitDisabled` из projectile.cpp:142 в вызов `GameServer()->CreateExplosion(...)`. Точные old/new здесь зависят от согласованного дизайна сигнатуры и всех остальных вызовов CreateExplosion по репо — поэтому дословные old_text->new_text для правки E: **НЕЛЬЗЯ** привести однозначно без решения по сигнатуре (есть множество вызовов CreateExplosion, не только из гранаты). Точечно проблемные строки: gamecontext.cpp:410 и gamecontext.cpp:419.

## Итог
- Корень: hit-disabled состояние хранится в CCharacter (`m_Core.m_GrenadeHitDisabled`), а снаряд его не запоминает; при дисконнекте владельца везде срабатывает fallback на `g_Config.m_SvHit`.
- Затронуто 3 файла: projectile.h, projectile.cpp (стр. 48-51, 104, 131), gamecontext.cpp (стр. 410, 419).
- Правки A-D — дословные и однозначные. Правка E (взрыв) требует изменения сигнатуры CreateExplosion — дословно однозначно НЕЛЬЗЯ.
- Код в репозитории НЕ редактировался (по условию задачи).

— mister/ \grenade
