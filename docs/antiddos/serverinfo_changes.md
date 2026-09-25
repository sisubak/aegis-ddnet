# Анти-DDoS правки в отдаче serverinfo и rate-limit

Файлы: src/engine/server/server.cpp, src/engine/server/server.h,
src/engine/shared/config_variables.h.
Основа анализа: _audit/netban.md (пункты 1, 2, 4).
Ниже: уязвимость -> статус -> детали с номерами строк.

---

## 1. Амплификация EXTENDED / 64_LEGACY на неверифицированном 0.6-пути

Статус: ПОФИКШЕНО.

Детали. В PumpNetwork при разборе SERVERBROWSE_GETINFO для протокола 0.6
(адрес источника не верифицируется) теперь отвечаем ТОЛЬКО одиночным пакетом
SERVERINFO_VANILLA. Тяжёлые многочанковые форматы (SERVERINFO_EXTENDED и
SERVERINFO_64_LEGACY), которые из одного маленького спуфнутого запроса рождали
несколько крупных UDP-ответов, на этом пути отклоняются.

- server.cpp:3135-3146 — ветка `else if(Type != -1)`; при `Type != SERVERINFO_VANILLA`
  выполняется `continue` (ранний выход), см. комментарий "Amplification hardening"
  на строках 3137-3142. То есть EXTENDED/64_LEGACY на невериф. connless-запрос
  больше не обслуживаются.
- server.cpp:3097-3111 — определение Type (EXTENDED при NETSENDFLAG_EXTENDED,
  иначе VANILLA; 64_LEGACY по своей сигнатуре) осталось, но результат для не-VANILLA
  теперь глушится выше.
- Верифицированный 0.7 (sixup) путь не затронут: server.cpp:3112-3134 — он идёт
  только при `ResponseToken != NET_SECURITY_TOKEN_UNKNOWN`, там источник доказан
  токеном, ответ уходит через SendPacketConnlessWithToken7.

Соответствует пунктам 1 и 2 из netban.md (server.cpp:2710/3034-3086 в старой нумерации).

---

## 2. Небезопасное чтение токена запроса (риск OOB / хрупкость)

Статус: ЧАСТИЧНО ПОФИКШЕНО.

Детали.
- Sixup-ветка (0.7): токен читается безопасно через CUnpacker с проверкой Error()
  и ранним `continue` при ошибке — server.cpp:3114-3120. Это ровно то, что
  рекомендовал netban.md (читать через Unpacker, а не по прямому индексу).
- VANILLA-ветка (0.6): токен по-прежнему берётся прямым индексом
  `((unsigned char *)Packet.m_pData)[sizeof(SERVERBROWSE_GETINFO)]` — server.cpp:3160.
  Индекс защищён проверкой размера `m_DataSize >= sizeof(SERVERBROWSE_GETINFO) + 1`
  (server.cpp:3094) и продублирован ассертом на строке 3159. OOB сейчас нет, но
  паттерн остался «по индексу», а не через Unpacker — то есть рекомендация netml
  выполнена не полностью.

Осталось: перевести чтение токена VANILLA-пути на CUnpacker для единообразия и
устойчивости к будущим изменениям условий размера.

---

## 3. dbg_assert в сетевом пути отдачи serverinfo (риск краша)

Статус: ПОФИКШЕНО (для неверного типа) / ОСТАЛОСЬ (ассерт размера токена).

Детали.
- Неверный тип serverinfo: раньше был `dbg_assert_failed("Invalid serverinfo Type")`
  (abort сервера из сетевого пути). Теперь в SendServerInfo это безопасный
  early-return с логом: `log_error("server", "invalid serverinfo type: %d", Type)`
  и `return` — server.cpp:2809-2815.
- Ассерт границы токена: на server.cpp:3159 остался
  `dbg_assert(Packet.m_DataSize >= (int)sizeof(SERVERBROWSE_GETINFO) + 1, ...)`.
  В release со включёнными ассертами это по-прежнему потенциальный abort из
  сетевого кода. Условие всегда истинно из-за проверки на 3094, поэтому на практике
  не срабатывает, но как «страховка» это всё ещё assert, а не early-return.
- Ассерт размера пакета (`Packer.Size() <= MaxPacketSize`) из netban.md п.4
  (старая строка 2701) в текущем SendServerInfo (2767-2822) отсутствует — код
  просто шлёт чанки, отдельного ассерта на размер здесь нет.

Осталось: при желании заменить dbg_assert на 3159 на явный `if(...) continue;`.

---

## 4. Rate-limit: глобальный лимит без разбивки по IP

Статус: ПОФИКШЕНО (добавлен per-IP лимит) / ЧАСТИЧНО ОСТАЛОСЬ (окно, учёт по числу).

Детали.
- Добавлен per-source-IP токен-лимит: RateLimitServerInfoConnlessAddr —
  server.cpp:2344-2378. Ключ — адрес без порта (`Key.port = 0`, server.cpp:2352,
  порт игнорируется намеренно, т.к. цели отражения варьируют порт). Бакет
  выбирается хэшем FNV-1a по байтам адреса (server.cpp:2354-2362) — это устраняет
  слабый хэш из netban.md. Кольцо на SERVERINFO_IP_BUCKETS = 1024 бакетов
  (server.h:293, 301). Лимит — MAX_REPLIES_PER_IP_PER_SECOND = 10 ответов на IP
  в секунду (server.cpp:2376-2377).
- Оба лимита вызываются последовательно на обоих путях отдачи:
  server.cpp:3122-3126 (sixup) и server.cpp:3148-3152 (vanilla) — при провале
  любого из них `continue`. То есть одиночный (в т.ч. спуфнутый) источник больше
  не может ни исчерпать глобальный лимит, ни быть эффективной целью отражения.
- Глобальный лимит сохранён: RateLimitServerInfoConnless — server.cpp:2323-2342,
  счётчики m_ServerInfoNumRequests / m_ServerInfoFirstRequest, конфиги
  sv_server_info_per_second (дефолт 50) и sv_server_info_replies_per_second
  (дефолт 500) — config_variables.h:502-503.

Осталось:
- Окно лимита — жёсткий сброс раз в секунду: глобальное на server.cpp:2326
  (`Now > m_ServerInfoFirstRequest + time_freq()`) и per-IP на server.cpp:2365.
  На стыке двух окон возможен всплеск до 2x лимита (burst). Скользящее окно /
  плавно пополняемый токен-бакет не внедрены.
- Учёт ведётся по ЧИСЛУ ответов, а не по БАЙТАМ (netml п.2). Размер ответа в
  лимит не входит.
- Дефолты sv_server_info_replies_per_second = 500 не снижены
  (config_variables.h:503).
- 1024 бакета без цепочек: при коллизии разные IP делят один бакет и вытесняют
  друг друга (сброс счётчика в server.cpp:2365-2371 при несовпадении адреса).
  Флуд со множества спуфнутых адресов всё ещё способен нагружать глобальный
  лимит, хотя per-IP резко снижает эффективность отражения на конкретную цель.

---

## Сводка

Пофикшено:
- Амплификация EXTENDED/64_LEGACY на невериф. 0.6 (server.cpp:3143-3146).
- Per-IP rate-limit с FNV-1a хэшем и 1024 бакетами (server.cpp:2344-2378, server.h:288-301).
- Безопасное чтение sixup-токена через CUnpacker (server.cpp:3114-3120).
- Замена dbg_assert_failed на log_error + return при неверном типе (server.cpp:2809-2815).

Осталось уязвимым/хрупким:
- VANILLA-токен читается прямым индексом, не через Unpacker (server.cpp:3160).
- dbg_assert в сетевом пути на границе токена (server.cpp:3159).
- Жёсткое окно rate-limit -> burst 2x на стыке окон (server.cpp:2326, 2365).
- Лимит по числу ответов, не по байтам; дефолт replies_per_second = 500 не снижен
  (config_variables.h:503).
- Коллизии бакетов per-IP (1024 без цепочек) -> флуд со спуфнутых адресов частично
  проходит через глобальный лимит.

Документировал: mister/ \info
