#!/bin/sh
# Coded by ueha
# Генерирует общий ipc_secret и rcon-пароль и прописывает их в server.cfg и gate.cfg.
# Запусти ОДИН раз на новом сервере перед первым стартом.
set -e
cd "$(dirname "$0")"

gen_hex() {
	if [ -r /dev/urandom ]; then
		head -c "$1" /dev/urandom | od -An -tx1 | tr -d ' \n'
	else
		date +%s%N | sha256sum | cut -c1-$(($1 * 2))
	fi
}

SECRET=$(gen_hex 24)
RCON=$(gen_hex 12)

for f in server.cfg gate.cfg; do
	sed -i.bak "s/CHANGE_ME_SECRET/$SECRET/; s/CHANGE_ME_RCON/$RCON/" "$f"
	rm -f "$f.bak"
done

echo "ipc_secret и rcon-пароль прописаны в server.cfg и gate.cfg."
echo "rcon-пароль: $RCON"
