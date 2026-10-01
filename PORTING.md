# PORTING — перенос анти-DDoS/капчи aegis в другой DDNet-сервер

Цель: взять функционал aegis (капча-гейт, анти-флуд-харденинг, аварийная
миграция) и вставить в СВОЙ форк DDNet, сделав **минимум правок в оригинальных
исходниках** — чтобы функционал твоего сервера не ломался.

> Coded by ueha

## Философия

Весь фичевый код живёт в самодостаточных модулях, которые копируются как есть:

- `src/game/server/components/captcha/` — капча-гейт (игровая часть)
- `src/game/server/components/anus_sob/` — аварийная миграция портов (опционально, EXPERIMENTAL)

Остальное — это **тонкие хуки** в ядре. В стоковом DDNet нет plugin-системы, а
защита от DDoS по своей природе живёт на пути разбора пакетов (движок), поэтому
несколько хуков в движке неизбежны. Ниже — ПОЛНЫЙ и минимальный их список,
сгруппированный по фичам: бери только то, что нужно. Каждая группа независима.

Правки сгруппированы так:
- **A. Капча-гейт** — основная фича (модуль captcha + хуки).
- **B. Независимый харденинг** — serverinfo rate-limit, ping-лимит, SSRF,
  net/huffman-защиты, проверка версии. Каждый патч самостоятелен, капчу не требует.
- **C. Аварийная миграция** — модуль anus_sob + хуки (EXPERIMENTAL, по желанию).

## Шаг 0. Общее для любой группы

CMake (`CMakeLists.txt`, блок исходников `game-server`, рядом со строкой
`set_src(GAME_SERVER ...)`): добавь файлы копируемых модулей (см. текущий блок
строк ~3133–3159 как образец — пути `components/captcha/*` и
`components/anus_sob/*`).

Конфиги (`src/engine/shared/config_variables.h`): скопируй блок переменных
`sv_captcha_srv_*` (и `sv_anus_sob_*` для группы C). Это один непрерывный блок
`MACRO_CONFIG_*` — просто вставь его среди остальных.

## A. Капча-гейт

Скопировать: `src/game/server/components/captcha/` целиком.

Игровая часть (`src/game/server/gamecontext.h` / `.cpp`) — хуки жизненного цикла:

- `gamecontext.h`: `#include "components/captcha/captcha_controller.h"` и член
  класса `CCaptchaController m_CaptchaController;`.
- `OnTick()`: `m_CaptchaController.by_utf8xbot_8802_tick(this);`
- `OnClientEnter()`: `m_CaptchaController.by_utf8xbot_8803_on_enter(this, ClientId);`
- `OnClientDrop()`: `m_CaptchaController.by_utf8xbot_8804_on_drop(this, ClientId);`
- в обработке чата (`OnSayNetMessage`/`OnMessage`): если
  `m_CaptchaController.by_utf8xbot_8805_on_chat(this, ClientId, pMessage)` вернул
  true — проглотить сообщение (это был ответ капчи).
- в начале `OnMessage()` сразу после `if(!pRawMsg) return;` — фильтр гейта:
  `if(g_Config.m_SvCaptchaSrvMode && g_Config.m_SvCaptchaSrvRole == 1 && MsgId != NETMSGTYPE_CL_SAY && MsgId != NETMSGTYPE_CL_STARTINFO) return;`

Движок (`src/engine/server/server.h` / `.cpp`) — вайтлист-гейт на пути пакетов:

- `server.h`, в `struct CClient`: поле `int64_t m_CaptchaGraceSince;` и его сброс
  в `0` там же, где сбрасываются прочие поля клиента (`CClient::Reset`-путь).
- `server.cpp`, `#include <game/server/components/captcha/captcha_session.h>`.
- `ProcessClientPacket()` в самом начале — проверка вайтлиста для role 0:
  скопируй блок `if(g_Config.m_SvCaptchaSrvMode && g_Config.m_SvCaptchaSrvRole == 0){ ... by_utf8xbot_L ... grace ... m_NetServer.Drop ... }` (текущие строки ~1791–1813).
- `Run()`: отказ старта при пустом секрете
  (`if(g_Config.m_SvCaptchaSrvMode && g_Config.m_SvCaptchaSrvIpcSecret[0]=='\0') return -1;`)
  и загрузка lobby-карты для гейта (`CCaptchaMap::by_utf8xbot_4417(...)`).

Это весь обязательный набор для капчи. Независимо от остального кода твоего форка.

## B. Независимый харденинг (бери патчи по отдельности)

Каждый патч самостоятелен и капчу не требует. Все живут в движке/базе.

- **serverinfo per-IP rate-limit** (`server.h`/`server.cpp`): структура
  `CServerInfoRateLimit`, массив `m_aServerInfoRateLimit[SERVERINFO_IP_BUCKETS]`
  (`enum SERVERINFO_IP_BUCKETS = 1024`), методы `RateLimitServerInfoConnless()` и
  `RateLimitServerInfoConnlessAddr()`, их `mem_zero`-инициализация и два вызова в
  обработке connless GETINFO (per-IP ПЕРЕД глобальным бюджетом).
- **ping-лимит** (`server.h`/`server.cpp`): поля `m_PingWindowSince`,
  `m_PingRepliesInWindow`, метод `by_utf8xbot_9931_ping_allowed()` и его вызовы в
  ветках ответа на ping.
- **StrHideIps** (`server.cpp`) — это апстрим-функция сокрытия IP в логах; в
  форке она починена (см. server_test.cpp). Если у тебя апстрим — уже есть.
- **проверка версии** (`server.cpp`): `by_utf8xbot_5041_cmp_version()` +
  конфиги `sv_version_*`.
- **net/huffman-защиты** (`src/engine/shared/network.cpp`, `src/base/net.cpp`):
  замена `dbg_assert` на возврат ошибки в сетевом пути, отсечение пустых
  compressed-полей, ограничение дренажа нуль-байтовых UDP (`zero_skips`).
- **SSRF-фильтр curl** (`src/engine/shared/http_curl.cpp`): фильтр open-socket,
  блок приватных/loopback-диапазонов + `MaxResponseSize` на запросах к мастеру
  (`src/engine/server/register.cpp`).
- **econ анти-брутфорс** (`src/engine/shared/econ.cpp`): бан по IP при
  исчерпании попыток (`ec_bantime` floor).

## C. Аварийная миграция портов (EXPERIMENTAL, по желанию)

Скопировать: `src/game/server/components/anus_sob/` целиком.

- `server.h`: forward `class CAnusSob;`, член `std::unique_ptr<CAnusSob> m_pAnusSob;`,
  аксессор `CAnusSob *AnusSob()`.
- `server.cpp`: attach (`by_utf8xbot_2001_attach`), счётчики пакетов/источников
  (`by_utf8xbot_2002_note_packet` / `by_utf8xbot_2008_note_source`), tick
  (`by_utf8xbot_2004_tick`), а также обработка env `ANUS_SOB_PORT`/`ANUS_SOB_READY_FD`
  и отключение `sv_register` у чайлда в `Run()`.
- `gamecontext.h`/`.cpp`: член `CAnusSobHost m_AnusSobHost;`, в `OnTick` —
  `by_utf8xbot_2032_tick` + триггер `by_utf8xbot_2003_should_migrate`/
  `by_utf8xbot_2030_execute_migration`; в `OnInit` — `by_utf8xbot_2027_cleanup_stale`
  + `by_utf8xbot_2023_load_from_env`; при входе клиента — `by_utf8xbot_2024_try_restore`.
- Только Unix. От волюметрического UDP-флуда не спасает — чисто экспериментально.

## Заметки

- Модули captcha/anus_sob не зависят ни от какой кастомной логики твоего форка —
  только от стандартных API движка/игры (IServer, CGameContext, CEntity, base/*).
- Полностью «без правок ядра» в стоковом DDNet невозможно: нет хук-системы, а
  анти-DDoS по определению на пути пакетов. Этот список — минимальный и полный.
- Если переносишь только капчу — группы B и C не нужны. Группы независимы.


