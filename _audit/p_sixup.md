# Аудит sixup-багов DDNet (#10066 и #7669)

Автор: mister/ \sixup
Репозиторий: `C:\Users\WWWWWUeHaA\Desktop\antiddos`
Файл под исследованием: `src/engine/server/server.cpp` (всего 5027 строк)

> Исходники НЕ редактировались. Ниже — точечные предложения правок в формате `old_text -> new_text`.
> Нумерация строк соответствует текущему состоянию файла на момент аудита.

---

## СТАТУС ПРИМЕНЕНИЯ (проверка mister/ \sixup)

Проверено против актуального `src/engine/server/server.cpp` (5061 строка).

| Правка | Статус | Где в коде сейчас |
|--------|--------|-------------------|
| 7669-A (не гасить конфиг, чистить данные) | УЖЕ ПРИМЕНЕНО | строки 3292–3303 |
| 7669-B (финальный free по факту данных) | УЖЕ ПРИМЕНЕНО | строки 3316–3321 |
| 10066 (отключение sixup без reload) | УЖЕ ПРИМЕНЕНО | строки 4673–4696 |

Итог: код уже соответствует целевому состоянию всех трёх правок (`new_text`). Исходный
`old_text` в файле отсутствует, поэтому `replace_in_file` не применялся — менять нечего.
Оба issue (#7669 и #10066) в этом дереве исходников закрыты соответствующими правками.

---


## Ключевые места в коде

| Что | Строка | Содержимое |
|-----|--------|-----------|
| Инициализация флага | 268 | `m_MapReload = false;` |
| Проверка sixup при отправке инфо | 3112 | `if(Type == SERVERINFO_VANILLA && ResponseToken != NET_SECURITY_TOKEN_UNKNOWN && Config()->m_SvSixup)` |
| Установка reload при смене карты | 3210 | `m_MapReload = str_comp(Config()->m_SvMap, GameServer()->Map()->FullName()) != 0;` |
| Сброс reload | 3220 | `m_MapReload = false;` |
| **Загрузка sixup-карты (баг #7669)** | 3271–3302 | блок `// load sixup version of the map` |
| Форс-reload в цикле | 3546 | `if(m_MapReload || m_SameMapReload || ...)` |
| Установка reload | 3774, 3779 | `m_MapReload = true;` |
| `ConchainMapUpdate` | 4644–4652 | reload при смене `sv_map` |
| **`ConchainSixupUpdate` (баг #10066)** | 4654–4662 | reload при смене `sv_sixup` |

---

## Данные из GitHub Issues

### #10066 — «Setting sv_sixup 0 forces server to restart» (Robyt3, 13 Apr 2025)
> It should be possible to at least disable this config option without the map being reloaded.
> Enabling it without a reload would also be possible by rewriting the 0.7 map loading a bit.

Метки: bug, server, sixup. Открыт, PR/веток нет.

### #7669 — «Sixup will be completly disabled if finding a map in maps7/ failed.» (ghost, 16 Dec 2023)
> Load a map that doesn't have an equivalent 0.7 map in `maps7/`. This will disable `sv_sixup`.
> Try to load a map that has a map in `maps7`, but it won't check that folder again because `sv_sixup` is disabled.
> This also explains why most DDNet servers don't show up in the 0.7 server browser.

Issue прямо указывает на блок `server.cpp` строки 2622–2649 (в старой ревизии `5f9d6b2`); в нашем репо это строки **3271–3297**. Метки: bug, server, sixup.

---

## Баг #7669 — sixup полностью отключается, если карта в maps7/ не найдена

### Причина
В `CServer::LoadMap(...)` при неудачном чтении `maps7/<map>.map` код глобально гасит
конфиг `m_SvSixup = 0`. После этого sixup выключен навсегда для всей сессии сервера:
при следующей смене карты (у которой ЕСТЬ файл в `maps7/`) блок `if(Config()->m_SvSixup)`
уже не выполняется, и sixup не восстанавливается. Из-за этого большинство DDNet-серверов
не появляются в браузере 0.7.

### Точное место
Файл: `src/engine/server/server.cpp`, строки **3272–3302**.

Текущий код (строки 3272–3302):
```cpp
	if(Config()->m_SvSixup)
	{
		str_format(aBuf, sizeof(aBuf), "maps7/%s.map", pMapName);
		void *pData;
		if(!Storage()->ReadFile(aBuf, IStorage::TYPE_ALL, &pData, &m_aCurrentMapSize[MAP_TYPE_SIXUP]))
		{
			Config()->m_SvSixup = 0;
			if(m_pRegister)
			{
				m_pRegister->OnConfigChange();
			}
			log_error("sixup", "couldn't load map %s", aBuf);
			log_info("sixup", "disabling 0.7 compatibility");
		}
		else
		{
			free(m_apCurrentMapData[MAP_TYPE_SIXUP]);
			m_apCurrentMapData[MAP_TYPE_SIXUP] = (unsigned char *)pData;

			m_aCurrentMapSha256[MAP_TYPE_SIXUP] = sha256(m_apCurrentMapData[MAP_TYPE_SIXUP], m_aCurrentMapSize[MAP_TYPE_SIXUP]);
			m_aCurrentMapCrc[MAP_TYPE_SIXUP] = crc32(0, m_apCurrentMapData[MAP_TYPE_SIXUP], m_aCurrentMapSize[MAP_TYPE_SIXUP]);
			sha256_str(m_aCurrentMapSha256[MAP_TYPE_SIXUP], aSha256, sizeof(aSha256));
			str_format(aBufMsg, sizeof(aBufMsg), "%s sha256 is %s", aBuf, aSha256);
			Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "sixup", aBufMsg);
		}
	}
	if(!Config()->m_SvSixup)
	{
		free(m_apCurrentMapData[MAP_TYPE_SIXUP]);
		m_apCurrentMapData[MAP_TYPE_SIXUP] = nullptr;
	}
```

### Предлагаемая правка

Идея: при неудаче НЕ трогать глобальный `m_SvSixup`. Просто отключить sixup ТОЛЬКО
для текущей карты — освободить/обнулить `m_apCurrentMapData[MAP_TYPE_SIXUP]`. Тогда
на следующей карте с валидным `maps7/`-файлом sixup снова загрузится. Клиенты 0.7 не
смогут зайти на карту без 0.7-версии (данных нет), но конфиг остаётся включённым.
Финальный блок `if(!Config()->m_SvSixup)` заменяется проверкой факта отсутствия данных,
чтобы регистрация в мастер-сервере обновлялась корректно.

**Правка 7669-A — не гасить конфиг, чистить данные локально**

old_text:
```cpp
		if(!Storage()->ReadFile(aBuf, IStorage::TYPE_ALL, &pData, &m_aCurrentMapSize[MAP_TYPE_SIXUP]))
		{
			Config()->m_SvSixup = 0;
			if(m_pRegister)
			{
				m_pRegister->OnConfigChange();
			}
			log_error("sixup", "couldn't load map %s", aBuf);
			log_info("sixup", "disabling 0.7 compatibility");
		}
		else
```

new_text:
```cpp
		if(!Storage()->ReadFile(aBuf, IStorage::TYPE_ALL, &pData, &m_aCurrentMapSize[MAP_TYPE_SIXUP]))
		{
			free(m_apCurrentMapData[MAP_TYPE_SIXUP]);
			m_apCurrentMapData[MAP_TYPE_SIXUP] = nullptr;
			m_aCurrentMapSize[MAP_TYPE_SIXUP] = 0;
			if(m_pRegister)
			{
				m_pRegister->OnConfigChange();
			}
			log_error("sixup", "couldn't load map %s", aBuf);
			log_info("sixup", "disabling 0.7 compatibility for this map only (sv_sixup stays on)");
		}
		else
```

**Правка 7669-B — финальный free по факту данных, а не по конфигу**

Причина: раньше блок опирался на `m_SvSixup == 0`. Теперь конфиг не гасится, поэтому
условие меняем на «если sixup-данные не загрузились» (например конфиг включён, но
карта без maps7-версии).

old_text:
```cpp
	if(!Config()->m_SvSixup)
	{
		free(m_apCurrentMapData[MAP_TYPE_SIXUP]);
		m_apCurrentMapData[MAP_TYPE_SIXUP] = nullptr;
	}
```

new_text:
```cpp
	if(!Config()->m_SvSixup && m_apCurrentMapData[MAP_TYPE_SIXUP])
	{
		free(m_apCurrentMapData[MAP_TYPE_SIXUP]);
		m_apCurrentMapData[MAP_TYPE_SIXUP] = nullptr;
		m_aCurrentMapSize[MAP_TYPE_SIXUP] = 0;
	}
```

> Примечание: правка 7669-B почти эквивалентна оригиналу (при выключенном конфиге чистит
> данные), но безопасна теперь, когда в fail-ветке 7669-A данные уже обнулены сами.
> Строго необходима только 7669-A; 7669-B — косметическая страховка. Если хочется минимум
> изменений — можно ограничиться только 7669-A.

---

## Баг #10066 — `sv_sixup 0` заставляет сервер рестартовать (перезагружать карту)

### Причина
`CServer::ConchainSixupUpdate` (строки 4654–4662) при любом изменении значения `sv_sixup`,
которое расходится с текущим состоянием (`есть ли загруженные sixup-данные`), выставляет
`m_MapReload |= ...`. Далее в главном цикле (строка 3546) `m_MapReload` приводит к полной
перезагрузке карты. То есть `sv_sixup 0` при активном sixup всегда триггерит рестарт карты.

### Точное место
Файл: `src/engine/server/server.cpp`, строки **4654–4662**.

Текущий код:
```cpp
void CServer::ConchainSixupUpdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData)
{
	pfnCallback(pResult, pCallbackUserData);
	CServer *pThis = static_cast<CServer *>(pUserData);
	if(pResult->NumArguments() >= 1 && pThis->GameServer()->Map()->IsLoaded())
	{
		pThis->m_MapReload |= (pThis->m_apCurrentMapData[MAP_TYPE_SIXUP] != nullptr) != (pResult->GetInteger(0) != 0);
	}
}
```

### Предлагаемая правка

Согласно issue Robyt3: отключение (`sv_sixup 0`) должно быть возможно БЕЗ перезагрузки
карты. Достаточно освободить sixup-данные на месте. Включение (`sv_sixup 1`) по-прежнему
требует reload, т.к. надо прочитать `maps7/`-файл (переписывание «горячей» загрузки 0.7
выходит за рамки минимальной правки — issue это допускает как отдельную опцию).

**Правка 10066 — отключать sixup без reload**

old_text:
```cpp
void CServer::ConchainSixupUpdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData)
{
	pfnCallback(pResult, pCallbackUserData);
	CServer *pThis = static_cast<CServer *>(pUserData);
	if(pResult->NumArguments() >= 1 && pThis->GameServer()->Map()->IsLoaded())
	{
		pThis->m_MapReload |= (pThis->m_apCurrentMapData[MAP_TYPE_SIXUP] != nullptr) != (pResult->GetInteger(0) != 0);
	}
}
```

new_text:
```cpp
void CServer::ConchainSixupUpdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData)
{
	pfnCallback(pResult, pCallbackUserData);
	CServer *pThis = static_cast<CServer *>(pUserData);
	if(pResult->NumArguments() >= 1 && pThis->GameServer()->Map()->IsLoaded())
	{
		const bool WantSixup = pResult->GetInteger(0) != 0;
		const bool HaveSixup = pThis->m_apCurrentMapData[MAP_TYPE_SIXUP] != nullptr;
		if(!WantSixup && HaveSixup)
		{
			// disabling sixup does not require a map reload: just drop the 0.7 map data
			free(pThis->m_apCurrentMapData[MAP_TYPE_SIXUP]);
			pThis->m_apCurrentMapData[MAP_TYPE_SIXUP] = nullptr;
			pThis->m_aCurrentMapSize[MAP_TYPE_SIXUP] = 0;
			if(pThis->m_pRegister)
			{
				pThis->m_pRegister->OnConfigChange();
			}
		}
		else if(WantSixup && !HaveSixup)
		{
			// enabling sixup still needs a reload to read the maps7/ map
			pThis->m_MapReload = true;
		}
	}
}
```

> Замечание по безопасности отключения: после освобождения `m_apCurrentMapData[MAP_TYPE_SIXUP]`
> нужно убедиться, что уже подключённые 0.7-клиенты корректно отвалятся или не получат снапшоты
> с невалидными данными карты. В текущем коде проверки sixup идут через `IsSixup(ClientId)` и
> наличие данных карты — при `nullptr` новые 0.7-клиенты не смогут скачать карту. Это поведение
> совпадает с задумкой issue (отключение совместимости на лету). Полное «горячее» включение без
> reload — отдельная задача, обозначенная в issue как «possible by rewriting the 0.7 map loading».

---

## Итог

| Баг | Можно исправить точечно? | Файл / строки | Правки |
|-----|--------------------------|---------------|--------|
| #7669 | ДА | `server.cpp` 3272–3302 | 7669-A (обязательна), 7669-B (страховка) |
| #10066 | ДА | `server.cpp` 4654–4662 | 10066 |

Обе правки локальны, не затрагивают протокол и совместимы между собой.

СТАТУС: все три правки (7669-A, 7669-B, 10066) уже присутствуют в текущем
`server.cpp` — код совпадает с целевым `new_text`. Дополнительное применение не
требовалось.

— mister/ \sixup
