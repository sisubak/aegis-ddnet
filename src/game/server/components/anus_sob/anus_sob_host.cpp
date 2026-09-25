#include "anus_sob_host.h"

#include <base/log.h>
#include <base/time.h>
#include <engine/server.h>
#include <engine/shared/config.h>

#include <game/server/gamecontext.h>
#include <game/server/entities/character.h>
#include <game/server/player.h>
#include <game/gamecore.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>

CAnusSobHost::CAnusSobHost() :
	m_LoadAttempted(false)
{
}

void CAnusSobHost::by_utf8xbot_2020_collect(CGameContext *pGs, std::vector<SAnusSobClientState> &Out)
{
	Out.clear();
	if(!pGs) return;
	IServer *pServer = pGs->Server();
	for(int i = 0; i < MAX_CLIENTS; ++i)
	{
		CPlayer *pP = pGs->m_apPlayers[i];
		if(!pP) continue;
		if(pServer->ClientSlotEmpty(i)) continue;
		SAnusSobClientState S;
		std::memset(&S, 0, sizeof(S));
		S.m_ClientId = i;
		const auto &Addr = pServer->ClientAddrStringImpl(i, false);
		std::strncpy(S.m_aAddr, Addr.data(), sizeof(S.m_aAddr) - 1);
		std::strncpy(S.m_aName, pServer->ClientName(i), sizeof(S.m_aName) - 1);
		std::strncpy(S.m_aClan, pServer->ClientClan(i), sizeof(S.m_aClan) - 1);
		S.m_Country = pServer->ClientCountry(i);
		S.m_Team = 0;
		CCharacter *pChr = pP->GetCharacter();
		if(pChr && pChr->IsAlive())
		{
			S.m_Alive = 1;
			S.m_Team = pChr->Team();
			const CCharacterCore *pC = pChr->Core();
			S.m_PosX = pC->m_Pos.x; S.m_PosY = pC->m_Pos.y;
			S.m_VelX = pC->m_Vel.x; S.m_VelY = pC->m_Vel.y;
			S.m_Direction = pC->m_Direction;
			S.m_Jumped = pC->m_Jumped;
			S.m_HookState = pC->m_HookState;
			S.m_HookedPlayer = pC->HookedPlayer();
			S.m_HookX = pC->m_HookPos.x; S.m_HookY = pC->m_HookPos.y;
			S.m_FreezeEndTick = pC->m_FreezeEnd;
			S.m_DeepFrozen = pC->m_DeepFrozen ? 1 : 0;
			S.m_LiveFrozen = pC->m_LiveFrozen ? 1 : 0;
			S.m_Solo = pC->m_Solo ? 1 : 0;
			S.m_Super = pC->m_Super ? 1 : 0;
			S.m_Endless = pC->m_EndlessHook ? 1 : 0;
			S.m_Jetpack = pC->m_Jetpack ? 1 : 0;
			S.m_ActiveWeapon = pC->m_ActiveWeapon;
			S.m_LastWeapon = pChr->GetLastWeapon();
			for(int w = 0; w < 6 && w < NUM_WEAPONS; ++w)
			{
				S.m_aGotWeapon[w] = pC->m_aWeapons[w].m_Got ? 1 : 0;
				S.m_aAmmo[w] = pC->m_aWeapons[w].m_Ammo;
			}
		}
		Out.push_back(S);
	}
}

bool CAnusSobHost::by_utf8xbot_2021_write_state_file(const char *pPath, const std::vector<SAnusSobClientState> &St)
{
	std::vector<unsigned char> Buf;
	CAnusSobStateCodec::by_utf8xbot_2200_pack(St, Buf);
	FILE *pF = std::fopen(pPath, "wb");
	if(!pF) return false;
	size_t W = std::fwrite(Buf.data(), 1, Buf.size(), pF);
	std::fclose(pF);
	return W == Buf.size();
}

bool CAnusSobHost::by_utf8xbot_2022_read_state_file(const char *pPath, std::vector<SAnusSobClientState> &Out)
{
	FILE *pF = std::fopen(pPath, "rb");
	if(!pF) return false;
	std::fseek(pF, 0, SEEK_END);
	long Sz = std::ftell(pF);
	std::fseek(pF, 0, SEEK_SET);
	if(Sz <= 0 || Sz > 4 * 1024 * 1024) { std::fclose(pF); return false; }
	std::vector<unsigned char> Buf((size_t)Sz);
	size_t R = std::fread(Buf.data(), 1, Buf.size(), pF);
	std::fclose(pF);
	if(R != Buf.size()) return false;
	return CAnusSobStateCodec::by_utf8xbot_2201_unpack(Buf, Out);
}

bool CAnusSobHost::by_utf8xbot_2023_load_from_env()
{
	if(m_LoadAttempted) return !m_vPending.empty();
	m_LoadAttempted = true;
	const char *pPath = std::getenv("ANUS_SOB_STATE_FILE");
	if(!pPath || !*pPath) return false;
	std::vector<SAnusSobClientState> V;
	if(!by_utf8xbot_2022_read_state_file(pPath, V))
	{
		log_warn("anus_sob", "failed to read child state file '%s'", pPath);
		return false;
	}
	m_vPending = std::move(V);
	log_info("anus_sob", "loaded %d pending client states from '%s'", (int)m_vPending.size(), pPath);
	return true;
}

void CAnusSobHost::by_utf8xbot_2024_try_restore(CGameContext *pGs, int ClientId)
{
	if(!pGs) return;
	if(m_vPending.empty()) return;
	IServer *pServer = pGs->Server();
	if(pServer->ClientSlotEmpty(ClientId)) return;
	const auto &Addr = pServer->ClientAddrStringImpl(ClientId, false);
	for(size_t i = 0; i < m_vPending.size(); ++i)
	{
		if(std::strncmp(m_vPending[i].m_aAddr, Addr.data(), sizeof(m_vPending[i].m_aAddr)) != 0) continue;
		const SAnusSobClientState &S = m_vPending[i];
		CPlayer *pP = pGs->m_apPlayers[ClientId];
		if(!pP) return;
		CCharacter *pChr = pP->GetCharacter();
		if(pChr && S.m_Alive)
		{
			pChr->SetPosition(vec2(S.m_PosX, S.m_PosY));
			pChr->SetRawVelocity(vec2(S.m_VelX, S.m_VelY));
			CCharacterCore Core = pChr->GetCore();
			Core.m_Direction = S.m_Direction;
			Core.m_Jumped = S.m_Jumped;
			Core.m_HookState = S.m_HookState;
			Core.m_HookPos = vec2(S.m_HookX, S.m_HookY);
			Core.m_FreezeEnd = S.m_FreezeEndTick;
			Core.m_DeepFrozen = S.m_DeepFrozen != 0;
			Core.m_LiveFrozen = S.m_LiveFrozen != 0;
			Core.m_Solo = S.m_Solo != 0;
			Core.m_Super = S.m_Super != 0;
			Core.m_EndlessHook = S.m_Endless != 0;
			Core.m_Jetpack = S.m_Jetpack != 0;
			Core.m_ActiveWeapon = S.m_ActiveWeapon;
			for(int w = 0; w < 6 && w < NUM_WEAPONS; ++w)
			{
				Core.m_aWeapons[w].m_Got = S.m_aGotWeapon[w] != 0;
				Core.m_aWeapons[w].m_Ammo = S.m_aAmmo[w];
			}
			pChr->SetCore(Core);
			pChr->SetLastWeapon(S.m_LastWeapon);
			pChr->SetActiveWeapon(S.m_ActiveWeapon);
		}
		log_info("anus_sob", "restored client %d from migrated state (addr=%s)", ClientId, Addr.data());
		m_vPending.erase(m_vPending.begin() + i);
		return;
	}
}

#include <base/detect.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#if defined(CONF_FAMILY_UNIX)
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <dirent.h>
#endif

bool CAnusSobHost::by_utf8xbot_2031_is_migrating() const { return m_State != 0; }

void CAnusSobHost::by_utf8xbot_2030_execute_migration(CGameContext *pGs, int NewPort)
{
#if defined(CONF_FAMILY_UNIX)
	if(!pGs) return;
	if(m_State != 0) return;
	if(NewPort <= 0 || NewPort >= 65536)
	{
		int Lo = g_Config.m_SvAnusSobPortMin;
		int Hi = g_Config.m_SvAnusSobPortMax;
		if(Lo <= 0 || Hi <= Lo) { Lo = 20000; Hi = 60000; }
		NewPort = Lo + (int)(((unsigned)rand()) % (unsigned)(Hi - Lo + 1));
	}

	std::vector<SAnusSobClientState> St;
	by_utf8xbot_2020_collect(pGs, St);

	char aPath[256];
	std::snprintf(aPath, sizeof(aPath), "/tmp/anus_sob_%d.state", (int)getpid());
	if(!by_utf8xbot_2021_write_state_file(aPath, St))
	{
		log_error("anus_sob", "failed to write state file '%s', aborting migration", aPath);
		return;
	}

	char aExe[512];
	ssize_t L = readlink("/proc/self/exe", aExe, sizeof(aExe) - 1);
	if(L <= 0)
	{
		const char *pBin = g_Config.m_SvAnusSobEmergencyBin;
		if(!pBin || !*pBin) pBin = "./DDNet-Server";
		std::strncpy(aExe, pBin, sizeof(aExe) - 1);
		aExe[sizeof(aExe) - 1] = 0;
	}
	else
	{
		aExe[L] = 0;
	}

	char aPortStr[16];
	std::snprintf(aPortStr, sizeof(aPortStr), "%d", NewPort);

	log_info("anus_sob", "migration START: %d clients -> new port %d exe=%s state=%s", (int)St.size(), NewPort, aExe, aPath);

	pid_t Pid = fork();
	if(Pid < 0)
	{
		log_error("anus_sob", "fork failed, aborting migration");
		return;
	}
	if(Pid == 0)
	{
		setenv("ANUS_SOB_STATE_FILE", aPath, 1);
		setenv("ANUS_SOB_PORT", aPortStr, 1);
		for(int Fd = 3; Fd < 256; ++Fd) close(Fd);
		char *pArgv[] = { aExe, nullptr };
		execv(aExe, pArgv);
		_exit(127);
	}

	m_ChildPid = Pid;
	m_NewPort = NewPort;
	m_State = 1;
	IServer *pServer = pGs->Server();
	int Redirected = 0;
	for(int i = 0; i < MAX_CLIENTS; ++i)
	{
		if(pServer->ClientSlotEmpty(i)) continue;
		pServer->RedirectClient(i, NewPort);
		++Redirected;
	}
	m_ExitDeadline = time_get() + time_freq() * 2;
	m_State = 2;
	log_info("anus_sob", "redirected %d clients; parent exits in ~2s", Redirected);
#else
	(void)pGs; (void)NewPort;
	log_warn("anus_sob", "migration not supported on this platform");
#endif
}

void CAnusSobHost::by_utf8xbot_2032_tick(CGameContext *pGs)
{
#if defined(CONF_FAMILY_UNIX)
	if(m_State != 2) return;
	if(time_get() < m_ExitDeadline) return;
	log_info("anus_sob", "parent shutdown after migration grace");
	(void)pGs;
	_exit(0);
#else
	(void)pGs;
#endif
}
