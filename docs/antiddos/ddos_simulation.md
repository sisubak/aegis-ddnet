# Симуляция DDoS / crash-пакетов на localhost

Область: локальный стенд для проверки антифлуд/анти-DDoS защиты DDNet-сервера.
Всё крутится на 127.0.0.1, никакого внешнего трафика. Спуфинг IP только визуальный
(в консоли атаки печатаются случайные source-IP), реальные пакеты уходят с localhost,
потому что ядро Windows не даёт слать с чужого адреса без raw-сокетов и драйвера.

## Состав стенда (папка ddos_sim)

- x7f3k9q2.cfg / build\z9k2m4p.cfg — конфиг сервера-мишени (без описаний, рандомное имя):
  порт 8303, dm1, sv_register 0, sv_connlimit 10 / sv_connlimit_time 1.
- by_utf8xbot_attack.py — консоль АТАКИ. Многопоточный флуд с эмуляцией разных source-IP
  и смесью типов пакетов. Аргументы: threads pps_per_thread duration_sec.
- by_utf8xbot_defense.py — консоль ЗАЩИТЫ. Тейлит live-лог сервера и подсвечивает
  ban / connlimit / rate-limit / drop / invalid / new_client / error, ведёт счётчики.
- run_sim.bat — поднимает сервер + две консоли (ATTACK и DEFENSE) в отдельных окнах.

## Типы пакетов в атаке (вес = частота)

- getinfo (30)          — SERVERBROWSE_GETINFO, проверка амплификации serverinfo.
- getinfo64 (15)        — SERVERBROWSE_GETINFO_64_LEGACY, «жирный» multi-chunk ответ.
- extended (15)         — connless xe-заголовок + GETINFO, самый большой ответ.
- connect_flood (15)    — NET_CTRLMSG_CONNECT с TKEN magic, флуд полуоткрытых соединений.
- crash_malformed (8)   — битые control/connless-пакеты, обрезанные заголовки, кривой
                          NumChunks, compression-флаг с мусором — проверка путей разбора.
- huffman_bomb (6)      — маленький compressed-пакет из повторяющихся байтов (расширение
                          при декомпрессии) — проверка CPU-DoS на Huffman.
- resend_flood (5)      — RESEND-флаг + NumChunks=0xff — попытка выжать resend-усиление.
- random_garbage (4)    — чистый рандом 1..40 байт.
- oversize (2)          — 1200..1400 байт мусора, близко к NET_MAX_PACKETSIZE.

## Прогоны и результат

Проведены волны:
- 8 потоков x 50 pps, 6 c
- 10 потоков x 60 pps, 6 c
- 20 потоков x 100 pps, 5 c (burst)
- 6 потоков x 30 pps, 5 c (~708 пакетов за прогон по счётчику)

Итог: процесс DDNet-Server.exe (PID стабилен всё время) НЕ упал ни на одной волне,
включая crash_malformed, huffman_bomb, oversize и resend_flood. Это подтверждает,
что ранее внесённые правки в сетевом пути (см. network_changes.md, serverinfo_changes.md,
ban_changes.md) держат удар по крашам разбора и dbg_assert-абортам.

## Что стенд НЕ покрывает (ограничения)

- Реальный спуфинг source-IP невозможен без raw-сокетов/драйвера, поэтому per-IP
  rate-limit и connlimit по факту видят один адрес 127.0.0.1 — их эффективность против
  распределённой атаки этим стендом не измеряется, только визуализируется.
- Амплификация (reflection) не измеряется по коэффициенту усиления: нужен захват
  исходящего трафика. Проверяется лишь устойчивость к самому потоку запросов.
- Huffman-бомба ограничена тем, что декомпрессия compressed-пакета идёт только для
  установленных соединений (AllowDecompression) — от неизвестного адреса путь короткий.

## Как запустить

    cd C:\Users\WWWWWUeHaA\Desktop\antiddos\ddos_sim
    run_sim.bat

либо вручную:

    (окно 1) build\DDNet-Server.exe "exec z9k2m4p.cfg" "logfile ddos_sim/server_live.log"
    (окно 2) python by_utf8xbot_defense.py server_live.log
    (окно 3) python by_utf8xbot_attack.py 8 40 0

Документировал: mister/ \day
