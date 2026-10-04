#include "captcha_controller.h"

#include "captcha_box.h"
#include "captcha_map.h"
#include "captcha_session.h"

#include <base/time.h>

#include <engine/server.h>
#include <engine/shared/config.h>

#include <game/server/entities/character.h>
#include <game/server/gamecontext.h>
#include <game/server/gameworld.h>
#include <game/server/player.h>

namespace
{
	void by_utf8xbot_8820_addr(CGameContext *pGs, int ClientId, char *pOut, int Size)
	{
		pOut[0] = 0;
		net_addr_str(pGs->Server()->ClientAddr(ClientId), pOut, Size, false);
	}
}

CCaptchaController::CCaptchaController()
{
	m_Init = false;
	m_Role = 0;
	for(CCaptchaBox *&pBox : m_apBox)
		pBox = nullptr;
}

void CCaptchaController::by_utf8xbot_8810_clear_box(int ClientId)
{
	if(ClientId < 0 || ClientId >= MAX_CLIENTS)
		return;
	if(m_apBox[ClientId])
	{
		m_apBox[ClientId]->Reset();
		m_apBox[ClientId] = nullptr;
	}
}

void CCaptchaController::by_utf8xbot_8813_note_fail(CGameContext *pGameServer, int ClientId)
{
	char aAddr[NETADDR_MAXSTRSIZE];
	by_utf8xbot_8820_addr(pGameServer, ClientId, aAddr, sizeof(aAddr));
	if(!aAddr[0])
		return;

	int64_t Now = time_timestamp();
	int Window = g_Config.m_SvCaptchaSrvFailWindowSec;

	if(m_FailByIp.size() > 50000)
	{
		for(auto It = m_FailByIp.begin(); It != m_FailByIp.end();)
		{
			if((Now - It->second.m_WindowStart) >= (int64_t)Window)
				It = m_FailByIp.erase(It);
			else
				++It;
		}
		if(m_FailByIp.size() > 50000)
			m_FailByIp.clear();
	}

	SCaptchaFail &F = m_FailByIp[aAddr];
	if(F.m_Count == 0 || (Now - F.m_WindowStart) >= (int64_t)Window)
	{
		F.m_WindowStart = Now;
		F.m_Count = 0;
	}
	F.m_Count++;
}

bool CCaptchaController::by_utf8xbot_8814_in_cooldown(CGameContext *pGameServer, int ClientId)
{
	char aAddr[NETADDR_MAXSTRSIZE];
	by_utf8xbot_8820_addr(pGameServer, ClientId, aAddr, sizeof(aAddr));
	if(!aAddr[0])
		return false;

	auto It = m_FailByIp.find(aAddr);
	if(It == m_FailByIp.end())
		return false;

	int64_t Now = time_timestamp();
	int Window = g_Config.m_SvCaptchaSrvFailWindowSec;
	if((Now - It->second.m_WindowStart) >= (int64_t)Window)
	{
		m_FailByIp.erase(It);
		return false;
	}
	return It->second.m_Count >= g_Config.m_SvCaptchaSrvMaxFails;
}

void CCaptchaController::by_utf8xbot_8815_clear_fail(CGameContext *pGameServer, int ClientId)
{
	char aAddr[NETADDR_MAXSTRSIZE];
	by_utf8xbot_8820_addr(pGameServer, ClientId, aAddr, sizeof(aAddr));
	if(!aAddr[0])
		return;
	m_FailByIp.erase(aAddr);
}

void CCaptchaController::by_utf8xbot_8801_init(CGameContext *pGameServer)
{
	if(!g_Config.m_SvCaptchaSrvMode)
		return;

	m_Role = g_Config.m_SvCaptchaSrvRole;
	m_Gate.by_utf8xbot_7413();
	m_Gate.m_NumDigits = g_Config.m_SvCaptchaSrvDigits;
	for(int i = 0; i < MAX_CLIENTS; i++)
		by_utf8xbot_8810_clear_box(i);

	if(m_Role == 0)
		m_Ipc.by_utf8xbot_5501_open_receiver(g_Config.m_SvCaptchaSrvIpcPort);

	m_Init = true;
}

void CCaptchaController::by_utf8xbot_8811_begin_session(CGameContext *pGameServer, int ClientId)
{
	by_utf8xbot_8810_clear_box(ClientId);

	float SpawnX = 0.0f;
	float SpawnY = 0.0f;
	CCaptchaMap::Spawn(&SpawnX, &SpawnY);
	vec2 Pos = vec2(SpawnX, SpawnY);

	const char *pCode = m_Gate.by_utf8xbot_8734(ClientId);
	m_apBox[ClientId] = new CCaptchaBox(&pGameServer->m_World, Pos, pCode);
	m_apBox[ClientId]->by_utf8xbot_2044(ClientId);
}

void CCaptchaController::by_utf8xbot_8812_pass(CGameContext *pGameServer, int ClientId)
{
	char aAddr[NETADDR_MAXSTRSIZE];
	net_addr_str(pGameServer->Server()->ClientAddr(ClientId), aAddr, sizeof(aAddr), false);

	if(!CCaptchaIpc::Send(g_Config.m_SvCaptchaSrvIpcPort, aAddr, g_Config.m_SvCaptchaSrvIpcSecret))
	{
		pGameServer->Server()->Kick(ClientId, "Капча: временная ошибка, попробуй переподключиться");
		m_Gate.by_utf8xbot_9046(ClientId);
		by_utf8xbot_8810_clear_box(ClientId);
		return;
	}

	pGameServer->SendChatTarget(ClientId, "Капча пройдена. Переход на игровой сервер.");
	pGameServer->Server()->RedirectClient(ClientId, g_Config.m_SvCaptchaSrvGamePort);

	by_utf8xbot_8815_clear_fail(pGameServer, ClientId);
	m_Gate.by_utf8xbot_9046(ClientId);
	by_utf8xbot_8810_clear_box(ClientId);
}

void CCaptchaController::by_utf8xbot_8802_tick(CGameContext *pGameServer)
{
	if(!g_Config.m_SvCaptchaSrvMode)
		return;
	if(!m_Init)
		by_utf8xbot_8801_init(pGameServer);

	if(m_Role == 0)
	{
		char aIp[NETADDR_MAXSTRSIZE];
		while(m_Ipc.by_utf8xbot_5503_poll(aIp, sizeof(aIp), g_Config.m_SvCaptchaSrvIpcSecret))
			CCaptchaSession::WriteWhitelist(g_Config.m_SvCaptchaSrvIpcPath, aIp, time_timestamp(), g_Config.m_SvCaptchaSrvWhitelistMax, g_Config.m_SvCaptchaSrvWhitelistTtlSec);
		return;
	}

	int64_t Now = pGameServer->Server()->Tick();
	int64_t Freq = pGameServer->Server()->TickSpeed();

	int aTimedOut[MAX_CLIENTS];
	int NumTimedOut = m_Gate.by_utf8xbot_9052_collect_timeouts(Now, Freq, g_Config.m_SvCaptchaSrvAnswerSec, aTimedOut, MAX_CLIENTS);
	for(int k = 0; k < NumTimedOut; k++)
	{
		int Client = aTimedOut[k];
		by_utf8xbot_8813_note_fail(pGameServer, Client);
		pGameServer->Server()->Kick(Client, "Капча не пройдена вовремя");
		m_Gate.by_utf8xbot_9046(Client);
		by_utf8xbot_8810_clear_box(Client);
	}

	std::vector<int> vActivated;
	m_Gate.by_utf8xbot_9051_promote(Now, g_Config.m_SvCaptchaSrvConcurrent, vActivated);
	for(int Client : vActivated)
		by_utf8xbot_8811_begin_session(pGameServer, Client);

	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(!pGameServer->m_apPlayers[i])
			continue;

		if(m_Gate.by_utf8xbot_9053_is_active(i))
		{
			char aBroadcast[256];
			m_Hud.by_utf8xbot_7412(m_Gate.by_utf8xbot_9055_start_time(i), Now, Freq, aBroadcast, sizeof(aBroadcast));
			pGameServer->SendBroadcast(aBroadcast, i, false);
		}
		else
		{
			int Position = m_Gate.by_utf8xbot_1927(i);
			if(Position > 0)
			{
				char aBroadcast[128];
				m_Hud.by_utf8xbot_7834(Position, aBroadcast, sizeof(aBroadcast));
				pGameServer->SendBroadcast(aBroadcast, i, false);
			}
		}
	}
}

void CCaptchaController::by_utf8xbot_8803_on_enter(CGameContext *pGameServer, int ClientId)
{
	if(!g_Config.m_SvCaptchaSrvMode || m_Role != 1)
		return;

	if(by_utf8xbot_8814_in_cooldown(pGameServer, ClientId))
	{
		pGameServer->Server()->Kick(ClientId, "Слишком много неудачных попыток капчи, подожди немного");
		return;
	}

	pGameServer->GlobalTuning()->Set("gravity", 0.0f);
	pGameServer->SendTuningParams(ClientId);

	int Res = m_Gate.by_utf8xbot_9050_admit(ClientId, pGameServer->Server()->Tick(), g_Config.m_SvCaptchaSrvConcurrent, g_Config.m_SvCaptchaSrvQueueMax);
	if(Res == CCaptchaGate::ADMIT_ACTIVATED)
		by_utf8xbot_8811_begin_session(pGameServer, ClientId);
	else if(Res == CCaptchaGate::ADMIT_FULL)
		pGameServer->Server()->Kick(ClientId, "Очередь капчи переполнена, зайди чуть позже");
}

void CCaptchaController::by_utf8xbot_8804_on_drop(CGameContext *pGameServer, int ClientId)
{
	if(!g_Config.m_SvCaptchaSrvMode || m_Role != 1)
		return;
	m_Gate.by_utf8xbot_9046(ClientId);
	by_utf8xbot_8810_clear_box(ClientId);
}

bool CCaptchaController::by_utf8xbot_8805_on_chat(CGameContext *pGameServer, int ClientId, const char *pMessage)
{
	if(!g_Config.m_SvCaptchaSrvMode || m_Role != 1)
		return false;

	if(!m_Gate.by_utf8xbot_9053_is_active(ClientId))
		return true;

	int64_t Now = pGameServer->Server()->Tick();
	int64_t Freq = pGameServer->Server()->TickSpeed();
	if(!m_Gate.by_utf8xbot_4891(ClientId, Now, Freq))
		return true;

	if(m_Gate.by_utf8xbot_4890(ClientId, pMessage))
	{
		by_utf8xbot_8812_pass(pGameServer, ClientId);
	}
	else
	{
		by_utf8xbot_8813_note_fail(pGameServer, ClientId);
		pGameServer->Server()->Kick(ClientId, "Капча не пройдена");
		m_Gate.by_utf8xbot_9046(ClientId);
		by_utf8xbot_8810_clear_box(ClientId);
	}

	return true;
}
