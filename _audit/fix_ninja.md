# Аудит фикса Issue #11706 — ninja expire damage indicators в server demos

Проверял: mister/ \ninja
Проект: DDNet (C++)
Файл: `src/game/server/entities/character.cpp`, метод `CCharacter::HandleNinja()` (~296–313)

## Суть issue
Индикаторы урона от истечения ninja (`CreateDamageInd`) попадают в server demos и ломают старые клиенты. Фикс маскирует эти индикаторы так, чтобы их получали только клиенты старее `VERSION_DDNET_NEW_HUD`.

## Шаг 1. Проверка API `ClientsMaskExcludeClientVersionAndHigher`

Результат: **МЕТОД СУЩЕСТВУЕТ** — это не выдуманный API.

- Объявление: `src/game/server/gamecontext.h:424`
  ```cpp
  CClientMask ClientsMaskExcludeClientVersionAndHigher(int Version) const;
  ```
- Реализация: `src/game/server/gamecontext.cpp:5215`
  ```cpp
  CClientMask CGameContext::ClientsMaskExcludeClientVersionAndHigher(int Version) const
  {
      CClientMask Mask;
      for(int i = 0; i < MAX_CLIENTS; ++i)
      {
          if(GetClientVersion(i) >= Version)
              continue;
          Mask.set(i);
      }
      return Mask;
  }
  ```
- Константа `VERSION_DDNET_NEW_HUD = 16020` объявлена в `src/engine/shared/protocol.h:156` — тоже реальная.

## Шаг 2. Проверка корректности вызова

Строка 312 (текущая, оставлена без изменений):
```cpp
GameServer()->CreateDamageInd(m_Pos, 0, NinjaTime / Server()->TickSpeed(), TeamMask() & GameServer()->ClientsMaskExcludeClientVersionAndHigher(VERSION_DDNET_NEW_HUD));
```

Идентичный, уже принятый в проекте паттерн используется для freeze-индикаторов в том же файле — `character.cpp:2245`:
```cpp
GameServer()->CreateDamageInd(m_Pos, 0, (m_FreezeTime + 1) / Server()->TickSpeed(), TeamMask() & GameServer()->ClientsMaskExcludeClientVersionAndHigher(VERSION_DDNET_NEW_HUD));
```

Сигнатура `CreateDamageInd(pos, angle, seconds, mask)` соблюдена, `TeamMask()` комбинируется с маской версий через `&` — логика корректна и консистентна с остальным кодом.

## Вывод

- Метод существует: **ДА**
- Old -> New: **изменений не требуется** (строка уже в корректном виде, откат НЕ нужен)
- Статус: **ПРИМЕНЕНО / оставлено как есть** — фикс валиден.

— mister/ \ninja
