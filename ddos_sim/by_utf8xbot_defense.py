import sys
import os
import time
import re

LOG_PATH = sys.argv[1] if len(sys.argv) > 1 else "server_live.log"

C_RESET = "\033[0m"
C_RED = "\033[91m"
C_YEL = "\033[93m"
C_CYN = "\033[96m"
C_GRN = "\033[92m"
C_MAG = "\033[95m"
C_GRY = "\033[90m"
C_BLU = "\033[94m"

counters = {
    "drop": 0,
    "ban": 0,
    "connlimit": 0,
    "rate_limit": 0,
    "invalid": 0,
    "new_client": 0,
    "error": 0,
}


def by_utf8xbot_5560(line):
    low = line.lower()
    color = C_GRY
    tag = None
    if "banned" in low or "ban added" in low or "'ban" in low:
        color = C_RED
        tag = "ban"
    elif "connlimit" in low or "conn limit" in low or "too many" in low:
        color = C_MAG
        tag = "connlimit"
    elif "rate" in low and "limit" in low:
        color = C_YEL
        tag = "rate_limit"
    elif "dropped" in low or "drop" in low or "disconnect" in low or "closed" in low or "timeout" in low:
        color = C_YEL
        tag = "drop"
    elif "invalid" in low or "malformed" in low or "wrong" in low or "corrupt" in low or "bad" in low:
        color = C_RED
        tag = "invalid"
    elif "new client" in low or "player has entered" in low or "connect" in low:
        color = C_CYN
        tag = "new_client"
    elif " e " in line or "error" in low or "failed" in low:
        color = C_RED
        tag = "error"
    elif " i " in line:
        color = C_GRN
    if tag:
        counters[tag] += 1
    return color


def by_utf8xbot_8104():
    print("%s#############################################################%s" % (C_BLU, C_RESET))
    print("%s#   DEFENSE CONSOLE  -  SERVER LIVE LOG (anti-DDoS view)     #%s" % (C_BLU, C_RESET))
    print("%s#   tailing: %-44s#%s" % (C_BLU, os.path.basename(LOG_PATH), C_RESET))
    print("%s#############################################################%s" % (C_BLU, C_RESET))
    sys.stdout.flush()

    while not os.path.exists(LOG_PATH):
        print("%s   waiting for server log...%s" % (C_GRY, C_RESET))
        sys.stdout.flush()
        time.sleep(1)

    last_stat = time.time()
    with open(LOG_PATH, "r", encoding="utf-8", errors="replace") as f:
        f.seek(0, os.SEEK_END)
        while True:
            line = f.readline()
            if line:
                line = line.rstrip("\n")
                if line.strip():
                    color = by_utf8xbot_5560(line)
                    print("%s%s%s" % (color, line, C_RESET))
                    sys.stdout.flush()
            else:
                time.sleep(0.2)
            now = time.time()
            if now - last_stat > 3:
                last_stat = now
                print("%s---- DEFENSE STATS: ban=%d connlimit=%d rate=%d drop=%d invalid=%d new=%d err=%d ----%s" % (
                    C_BLU, counters["ban"], counters["connlimit"], counters["rate_limit"],
                    counters["drop"], counters["invalid"], counters["new_client"], counters["error"], C_RESET))
                sys.stdout.flush()


if __name__ == "__main__":
    try:
        by_utf8xbot_8104()
    except KeyboardInterrupt:
        pass
