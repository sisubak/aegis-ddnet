# Деплой и перенос aegis-ddnet

Портативный запуск связки **gate + game** на любом сервере. Всё состояние
(вайтлист, базы очков, settings) держится в каталоге деплоя — перенос
сводится к копированию папки или образа.

> Coded by ueha

## Архитектура

Два процесса одного и того же бинаря `DDNet-Server`:

- **gate** (`sv_captcha_srv_role 1`, порт **8304**) — публичный вход. Игрок
  подключается сюда, проходит капчу.
- **game** (`sv_captcha_srv_role 0`, порт **8303**) — игровой сервер. После
  капчи gate редиректит игрока сюда; его IP уходит на game по loopback-UDP
  (порт 8305, пакет с HMAC-подписью) и попадает в вайтлист.

Оба порта (8303 и 8304) должны быть открыты наружу. IPC gate→game работает
только в пределах одной машины — это архитектурное ограничение (см. ниже про
Docker, где это решается общим сетевым namespace).

`sv_captcha_srv_ipc_secret` **обязан совпадать** у gate и game, и быть
непустым — иначе сервер в captcha-режиме не стартует.

## Вариант A: Docker (рекомендуется)

Нужен только Docker + docker compose. Из корня репозитория:

```sh
cp deploy/.env.example deploy/.env
# впиши секрет и rcon-пароль:
#   AEGIS_IPC_SECRET=$(openssl rand -hex 24)
#   AEGIS_RCON_PASSWORD=<свой пароль>
cd deploy
docker compose up -d --build
```

Поднимутся два контейнера: `game` (публикует 8303/udp и 8304/udp) и `gate`
(делит сетевой namespace с game, поэтому loopback-IPC работает). Один и тот
же секрет из `.env` уходит обоим.

Логи: `docker compose logs -f`. Остановить: `docker compose down`.

## Вариант B: bare-metal из релизного архива

Без Docker, из готового архива релиза (вкладка Releases, напр. `v5`):

1. Распакуй архив своей ОС так, чтобы рядом оказались `DDNet-Server` и `data/`:
   - Linux: `aegis-server-linux-x86_64.tar.gz`
   - Windows: `aegis-server-windows-x64.zip`
   - macOS: `aegis-server-macos-arm64.tar.gz`
2. Скопируй в ту же папку файлы из `deploy/`: `storage.cfg`, `server.cfg`,
   `gate.cfg`, скрипты запуска и `gen-secret`.
3. Сгенерируй общий секрет и rcon-пароль (пропишутся в оба конфига):
   - Linux/macOS: `sh gen-secret.sh`
   - Windows: `gen-secret.bat`
4. Запусти ОБА процесса (в разных окнах/юнитах, на одной машине):
   - Linux/macOS: `sh run-game.sh` и `sh run-gate.sh`
   - Windows: `run-game.bat` и `run-gate.bat`

Итоговая раскладка каталога:

```
<папка>/
  DDNet-Server(.exe)        бинарь из релиза
  data/                     из релиза
  storage.cfg server.cfg gate.cfg
  run-game.* run-gate.* gen-secret.*
  captcha_whitelist.txt     создаётся game в рантайме
  *_scores.db               создаётся в рантайме
```

`storage.cfg` ставит `$CURRENTDIR` первым путём, поэтому всё состояние
пишется в эту папку.

## Перенос на другой сервер

- **Docker:** скопируй репозиторий (или только `deploy/` + исходники) и
  `deploy/.env` на новую машину → `docker compose up -d --build`. Либо
  `docker save`/`docker load` готового образа.
- **Bare-metal:** просто скопируй всю папку деплоя (вместе с `*.cfg`,
  базами и вайтлистом) на новый сервер и запусти те же два скрипта. Абсолютных
  путей внутри нет — всё относительно каталога.

Проверь, что на новом сервере открыты **UDP 8303 и 8304** (и что порты в
`server.cfg`/`gate.cfg`/compose совпадают). Пример для ufw:

```sh
ufw allow 8303/udp
ufw allow 8304/udp
```

## Заметки

- Образ собирается рецептом из зелёного CI (`release-servers.yml`,
  `CLIENT=OFF`). Локально в этом окружении Docker-сборку проверить нельзя —
  при первом `docker build` на твоей машине убедись, что стадия build
  проходит; рантайм-зависимости (`libsqlite3`, `libssl`, `libcurl`, `zlib`)
  уже прописаны.
- Вайтлист не требует переноса (восстанавливается по мере прохождения капчи,
  TTL по умолчанию 1 час). Базы очков (`*_scores.db`) переноси, если нужна
  история.
- Это userspace-защита: держит ботов/флаг-флуд, но от волюметрического
  UDP-флуда нужен сетевой уровень (провайдер/anti-DDoS перед сервером).

