#ifndef GAME_SERVER_COMPONENTS_CAPTCHA_CAPTCHA_CONTROLLER_H
#define GAME_SERVER_COMPONENTS_CAPTCHA_CAPTCHA_CONTROLLER_H

#include "captcha_gate.h"
#include "captcha_hud.h"
#include "captcha_ipc.h"

class CGameContext;
class CCaptchaBox;

class CCaptchaController
{
public:
	CCaptchaController();

	void by_utf8xbot_8801_init(CGameContext *pGameServer);
	void by_utf8xbot_8802_tick(CGameContext *pGameServer);
	void by_utf8xbot_8803_on_enter(CGameContext *pGameServer, int ClientId);
	void by_utf8xbot_8804_on_drop(CGameContext *pGameServer, int ClientId);
	bool by_utf8xbot_8805_on_chat(CGameContext *pGameServer, int ClientId, const char *pMessage);

private:
	bool m_Init;
	int m_Role;
	int m_ActiveClient;
	CCaptchaGate m_Gate;
	CCaptchaHud m_Hud;
	CCaptchaIpc m_Ipc;
	CCaptchaBox *m_pBox;

	void by_utf8xbot_8810_clear_box();
	void by_utf8xbot_8811_begin_session(CGameContext *pGameServer, int ClientId);
	void by_utf8xbot_8812_pass(CGameContext *pGameServer, int ClientId);
};

#endif
