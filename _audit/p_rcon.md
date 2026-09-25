# Аудит бага #9580 — Incorrect IP address redaction with `show_ips 0`

**Автор:** mister/ \rcon
**Репозиторий:** `C:\Users\WWWWWUeHaA\Desktop\antiddos` (DDNet)
**Исходники НЕ редактировались** — это только аудит.

---

## 1. Описание бага (из issue)

Источник: https://github.com/ddnet/ddnet/issues/9580
Открыт: Robyt3, 25 Jan 2025. Метки: `bug`, `rcon`, `server`.

Текст дословно:

> Naming yourself `<{1.1.1.1:1}>` will cause the name to be redacted to `XXX`
> in console output for authenticated users with `show_ips 0` in all messages
> where this name appears first before other IP addresses. Only the first IP
> address is redacted for every log line, so the name is printed correctly in
> status output because the IP address comes first, but messages like
> `*** '<{1.1.1.1:1}>' has left the game` will incorrectly be changed to
> `*** 'XXX' has left the game`.

Суть: сервер оборачивает реальные IP в лог-строках маркерами `<{ ... }>`, а
функция редакции при `show_ips 0` заменяет содержимое между этими маркерами
на `XXX`. Игрок может задать себе имя, содержащее `<{` и `}>`, и тогда его имя
ошибочно принимается за IP и редактируется. Плюс редактируется только ПЕРВОЕ
вхождение `<{...}>` в строке.

---

## 2. Где применяется редакция IP

### Единственное место редакции — функция `CServer::StrHideIps`

**Путь:** `src\engine\server\server.cpp`
**Строки:** 633–655

Механизм редакции построен не на `str_sanitize`, регэкспе IP или переменной
`m_SvShowIps`, а на текстовых маркерах `<{` / `}>`, которыми сервер оборачивает
адреса (см. `ClientAddrString(..., true)` и формат `addr=<{%s}>`). Символов
`show_ips`/`m_SvShowIps`/`RedactIp` в коде НЕТ — используется поле клиента
`m_ShowIps` и функция `StrHideIps`.

Содержимое функции (строки 633–655) дословно:

```cpp
bool CServer::StrHideIps(const char *pInput, char *pOutputWithIps, size_t OutputWithIpsSize, char *pOutputWithoutIps, size_t OutputWithoutIpsSize)
{
	const char *pStart = str_find(pInput, "<{");
	const char *pEnd = pStart == nullptr ? nullptr : str_find(pStart + 2, "}>");
	pOutputWithIps[0] = '\0';
	pOutputWithoutIps[0] = '\0';

	if(pStart == nullptr || pEnd == nullptr)
	{
		str_copy(pOutputWithIps, pInput, OutputWithIpsSize);
		str_copy(pOutputWithoutIps, pInput, OutputWithoutIpsSize);
		return false;
	}

	str_append(pOutputWithIps, pInput, std::min((size_t)(pStart - pInput + 1), OutputWithIpsSize));
	str_append(pOutputWithIps, pStart + 2, std::min((size_t)(pEnd - pInput - 1), OutputWithIpsSize));
	str_append(pOutputWithIps, pEnd + 2, OutputWithIpsSize);

	str_append(pOutputWithoutIps, pInput, std::min((size_t)(pStart - pInput + 1), OutputWithoutIpsSize));
	str_append(pOutputWithoutIps, "XXX", OutputWithoutIpsSize);
	str_append(pOutputWithoutIps, pEnd + 2, OutputWithoutIpsSize);
	return true;
}
```

**Корень бага в этой функции:** она находит только ОДНО (первое) вхождение
`<{...}>` через одиночный `str_find` и заменяет его на `XXX`. Отсюда оба
симптома из issue:
1. редактируется только первый маркер в строке;
2. если имя игрока содержит `<{...}>`, оно принимается за IP-маркер и редактируется.

### Место вызова (для контекста, не для правки)

**Путь:** `src\engine\server\server.cpp`
**Строки:** 1507–1526 — `CServer::SendRconLogLine`

```cpp
void CServer::SendRconLogLine(int ClientId, const CLogMessage *pMessage)
{
	char aLine[sizeof(CLogMessage().m_aLine)];
	char aLineWithoutIps[sizeof(CLogMessage().m_aLine)];
	StrHideIps(pMessage->m_aLine, aLine, sizeof(aLine), aLineWithoutIps, sizeof(aLineWithoutIps));
	...
			SendRconLine(i, m_aClients[i].m_ShowIps ? aLine : aLineWithoutIps);
	...
}
```

Именно тут выбор `m_ShowIps ? aLine : aLineWithoutIps` определяет, отдавать ли
редактированную версию клиенту.

### Объявление

**Путь:** `src\engine\server\server.h`, строка 339:

```cpp
static bool StrHideIps(const char *pInput, char *pOutputWithIps, size_t OutputWithIpsSize, char *pOutputWithoutIps, size_t OutputWithoutIpsSize);
```

### gamecontext.cpp

**В `src\game\server\gamecontext.cpp` кода редакции IP НЕТ.** Поиск по
`show_ips`, `m_SvShowIps`, `redact`, `RedactIp`, `str_sanitize`, IP-регэкспам
дал 0 совпадений. Вся логика редакции сосредоточена в `server.cpp`.

---

## 3. Предлагаемая правка (old_text -> new_text, дословно)

Правка минимальная: заставить `StrHideIps` обрабатывать ВСЕ вхождения `<{...}>`
в строке в цикле, а не только первое. Это устраняет оба симптома (частичная
редакция + утечка последующих IP), при этом маркеры имени игрока всё равно
попадают под редакцию — но это уже вопрос отдельной санитизации имён
(маркеры `<{`/`}>` в именах должны экранироваться на входе). Изменение логики
редакции строк — минимальное и безопасное.

**Файл:** `src\engine\server\server.cpp`, строки 633–655.

### old_text

```cpp
bool CServer::StrHideIps(const char *pInput, char *pOutputWithIps, size_t OutputWithIpsSize, char *pOutputWithoutIps, size_t OutputWithoutIpsSize)
{
	const char *pStart = str_find(pInput, "<{");
	const char *pEnd = pStart == nullptr ? nullptr : str_find(pStart + 2, "}>");
	pOutputWithIps[0] = '\0';
	pOutputWithoutIps[0] = '\0';

	if(pStart == nullptr || pEnd == nullptr)
	{
		str_copy(pOutputWithIps, pInput, OutputWithIpsSize);
		str_copy(pOutputWithoutIps, pInput, OutputWithoutIpsSize);
		return false;
	}

	str_append(pOutputWithIps, pInput, std::min((size_t)(pStart - pInput + 1), OutputWithIpsSize));
	str_append(pOutputWithIps, pStart + 2, std::min((size_t)(pEnd - pInput - 1), OutputWithIpsSize));
	str_append(pOutputWithIps, pEnd + 2, OutputWithIpsSize);

	str_append(pOutputWithoutIps, pInput, std::min((size_t)(pStart - pInput + 1), OutputWithoutIpsSize));
	str_append(pOutputWithoutIps, "XXX", OutputWithoutIpsSize);
	str_append(pOutputWithoutIps, pEnd + 2, OutputWithoutIpsSize);
	return true;
}
```

### new_text

```cpp
bool CServer::StrHideIps(const char *pInput, char *pOutputWithIps, size_t OutputWithIpsSize, char *pOutputWithoutIps, size_t OutputWithoutIpsSize)
{
	pOutputWithIps[0] = '\0';
	pOutputWithoutIps[0] = '\0';

	bool Redacted = false;
	const char *pCursor = pInput;
	while(true)
	{
		const char *pStart = str_find(pCursor, "<{");
		const char *pEnd = pStart == nullptr ? nullptr : str_find(pStart + 2, "}>");
		if(pStart == nullptr || pEnd == nullptr)
		{
			str_append(pOutputWithIps, pCursor, OutputWithIpsSize);
			str_append(pOutputWithoutIps, pCursor, OutputWithoutIpsSize);
			break;
		}

		Redacted = true;

		// text before the marker
		str_append(pOutputWithIps, pCursor, std::min((size_t)(pStart - pCursor + 1), OutputWithIpsSize));
		str_append(pOutputWithoutIps, pCursor, std::min((size_t)(pStart - pCursor + 1), OutputWithoutIpsSize));

		// the address itself: keep verbatim in "with ips", replace with XXX otherwise
		str_append(pOutputWithIps, pStart + 2, std::min((size_t)(pEnd - pStart - 1), OutputWithIpsSize));
		str_append(pOutputWithoutIps, "XXX", OutputWithoutIpsSize);

		// continue scanning after the closing marker
		pCursor = pEnd + 2;
	}
	return Redacted;
}
```

Замечания к правке:
- Цикл обрабатывает все маркеры `<{...}>`, а не только первый — закрывает оба
  симптома из issue.
- В `pOutputWithIps` смещения пересчитаны относительно `pCursor` (было
  относительно `pInput`), чтобы корректно работать в цикле; для первого маркера
  результат идентичен исходной логике.
- Возвращаемое значение `Redacted` сохраняет прежний контракт (true, если было
  хоть одно вхождение).
- Существующие юнит-тесты в `src\test\server_test.cpp` (TEST(Server, StrHideIps),
  строки 5–61) остаются валидными для одиночных вхождений; многократные
  вхождения теперь редактируются полностью — тесты на этот случай стоит
  дополнить (например, `<{127.0.0.1}> <{127.0.0.1}>` -> `XXX XXX`).

**ВАЖНО:** правка приведена как предложение в отчёте. Полностью санитизировать
проблему (имя игрока `<{...}>` не должно путаться с IP-маркером) следует
экранированием/удалением подстрок `<{` и `}>` из клиентских имён на входе, но
это выходит за рамки функции редакции и требует правки логики имён.

---

## 3a. СТАТУС ПРИМЕНЕНИЯ

**СТАТУС: НЕ ПРИМЕНЕНО — правка уже присутствует в коде (no-op).**

Проверка текущего `src\engine\server\server.cpp` показала, что функция
`CServer::StrHideIps` УЖЕ реализована в циклическом виде, идентичном `new_text`
из раздела 3. Актуальные строки: **642–671**.

- `old_text` (одиночный `str_find`, строки 633–655 из аудита) в текущем коде
  ОТСУТСТВУЕТ — `replace_in_file` не нашёл бы совпадения.
- Текущая реализация (строки 642–671) построчно совпадает с предложенным
  `new_text` (цикл `while(true)`, `pCursor`, `Redacted`, обработка всех
  маркеров `<{...}>`).
- Место вызова `SendRconLogLine` не менялось.

Вывод: багфикс из issue #9580 в этой ветке уже внесён; дополнительных правок
`StrHideIps` не требуется. Открытым остаётся только вопрос экранирования
маркеров `<{`/`}>` в клиентских именах (вне рамок данной функции).

_— mister/ \rcon_

---

## 4. Итог

- Редакция IP реализована ТОЛЬКО в `src\engine\server\server.cpp`, функция
  `CServer::StrHideIps` (строки 633–655), вызывается из `SendRconLogLine`
  (строки 1507–1526). Выбор версии — по полю `m_ShowIps` клиента.
- В `src\game\server\gamecontext.cpp` редакции IP НЕТ.
- Символов `show_ips` / `m_SvShowIps` / `redact` / `RedactIp` / IP-регэкспа в
  логике редакции нет; всё построено на маркерах `<{` / `}>`.
- Корень бага: одиночный `str_find` вместо цикла + отсутствие экранирования
  маркеров в клиентских именах.

_— mister/ \rcon_
