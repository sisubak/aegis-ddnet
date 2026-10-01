#include "captcha_controller.h"

#include "captcha_box.h"
#include "captcha_map.h"
#include "captcha_session.h"

#include <engine/server.h>
#include <engine/shared/config.h>

#include <base/time.h>

#include <game/server/entities/character.h>
#include <game/server/gamecontext.h>
#include <game/server/gameworld.h>
#include <game/server/player.h>

CCaptchaController::CCaptchaController()
{
	m_Init = false;
	m_Role = 0;
	m_ActiveClient = -1;
	m_pBox = nullptr;
}

void CCaptchaController::by_utf8xbot_8810_clear_box()
{
	if(m_pBox)
	{
		m_pBox->Reset();
		m_pBox = nullptr;
	}
}

void CCaptchaController::by_utf8xbot_8801_init(CGameContext *pGameServer)
{
	if(!g_Config.m_SvCaptchaSrvMode)
		return;

	m_Role = g_Config.m_SvCaptchaSrvRole;
	m_Gate.by_utf8xbot_7413();
	m_Gate.m_NumDigits = g_Config.m_SvCaptchaSrvDigits;
	m_ActiveClient = -1;
	by_utf8xbot_8810_clear_box();

	if(m_Role == 0)
		m_Ipc.by_utf8xbot_5501_open_receiver(g_Config.m_SvCaptchaSrvIpcPort);

	m_Init = true;
}

void CCaptchaController::by_utf8xbot_8811_begin_session(CGameContext *pGameServer, int ClientId)
{
	by_utf8xbot_8810_clear_box();

	float SpawnX = 0.0f;
	float SpawnY = 0.0f;
	CCaptchaMap::by_utf8xbot_5729_spawn(&SpawnX, &SpawnY);
	vec2 Pos = vec2(SpawnX, SpawnY);

	const char *pCode = m_Gate.by_utf8xbot_8734();
	m_pBox = new CCaptchaBox(&pGameServer->m_World, Pos, pCode);
	m_pBox->by_utf8xbot_2044(ClientId);

	m_ActiveClient = ClientId;
}


void CCaptchaController::by_utf8xbot_8812_pass(CGameContext *pGameServer, int ClientId)
{
	char aAddr[NETADDR_MAXSTRSIZE];
	net_addr_str(pGameServer->Server()->ClientAddr(ClientId), aAddr, sizeof(aAddr), false);

	if(!CCaptchaIpc::by_utf8xbot_5504_send(g_Config.m_SvCaptchaSrvIpcPort, aAddr, g_Config.m_SvCaptchaSrvIpcSecret))
	{
		pGameServer->Server()->Kick(ClientId, "Капча: временная ошибка, попробуй переподключиться");
		m_Gate.by_utf8xbot_9046(ClientId);
		m_ActiveClient = -1;
		by_utf8xbot_8810_clear_box();
		return;
	}

	pGameServer->SendChatTarget(ClientId, "Капча пройдена. Переход на игровой сервер.");
	pGameServer->Server()->RedirectClient(ClientId, g_Config.m_SvCaptchaSrvGamePort);

	m_Gate.by_utf8xbot_9046(ClientId);
	m_ActiveClient = -1;
	by_utf8xbot_8810_clear_box();
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
			CCaptchaSession::by_utf8xbot_K(g_Config.m_SvCaptchaSrvIpcPath, aIp, time_timestamp(), g_Config.m_SvCaptchaSrvWhitelistMax, g_Config.m_SvCaptchaSrvWhitelistTtlSec);
		return;
	}

	int64_t Now = pGameServer->Server()->Tick();
	int64_t Freq = pGameServer->Server()->TickSpeed();

	int TimedOut = m_Gate.by_utf8xbot_6605(Now, Freq, g_Config.m_SvCaptchaSrvAnswerSec);
	if(TimedOut != -1)
	{
		pGameServer->Server()->Kick(TimedOut, "Капча не пройдена вовремя");
		m_Gate.by_utf8xbot_9046(TimedOut);
		m_ActiveClient = -1;
		by_utf8xbot_8810_clear_box();
	}

	if(m_Gate.by_utf8xbot_2158() == -1)
	{
		int Next = m_Gate.by_utf8xbot_3271(Now);
		if(Next != -1)
			by_utf8xbot_8811_begin_session(pGameServer, Next);
	}

	int Active = m_Gate.by_utf8xbot_2158();
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(!pGameServer->m_apPlayers[i])
			continue;

		if(i == Active)
		{

			char aBroadcast[256];
			m_Hud.by_utf8xbot_7412(m_Gate.m_Session.m_StartTime, Now, Freq, aBroadcast, sizeof(aBroadcast));
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

	pGameServer->GlobalTuning()->Set("gravity", 0.0f);
	pGameServer->SendTuningParams(ClientId);

	m_Gate.by_utf8xbot_5182(ClientId);
}


void CCaptchaController::by_utf8xbot_8804_on_drop(CGameContext *pGameServer, int ClientId)
{
	if(!g_Config.m_SvCaptchaSrvMode || m_Role != 1)
		return;
	bool WasActive = m_Gate.by_utf8xbot_2158() == ClientId;
	m_Gate.by_utf8xbot_9046(ClientId);
	if(WasActive)
	{
		m_ActiveClient = -1;
		by_utf8xbot_8810_clear_box();
	}
}

bool CCaptchaController::by_utf8xbot_8805_on_chat(CGameContext *pGameServer, int ClientId, const char *pMessage)
{
	if(!g_Config.m_SvCaptchaSrvMode || m_Role != 1)
		return false;

	if(m_Gate.by_utf8xbot_2158() != ClientId)
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
		pGameServer->Server()->Kick(ClientId, "Капча не пройдена");
		m_Gate.by_utf8xbot_9046(ClientId);
		if(m_ActiveClient == ClientId)
			m_ActiveClient = -1;
		by_utf8xbot_8810_clear_box();
	}

	return true;
}
