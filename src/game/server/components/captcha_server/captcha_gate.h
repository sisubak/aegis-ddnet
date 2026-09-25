#ifndef GAME_SERVER_COMPONENTS_CAPTCHA_SERVER_CAPTCHA_GATE_H
#define GAME_SERVER_COMPONENTS_CAPTCHA_SERVER_CAPTCHA_GATE_H

#include <base/types.h>
#include <engine/shared/protocol.h>

class CCaptchaGate
{
 public:
	CCaptchaGate();

	void by_utf8xbot_4001_reset();
	void by_utf8xbot_4002_on_join(int ClientId, int64_t Now);
	void by_utf8xbot_4003_on_leave(int ClientId);
	int  by_utf8xbot_4004_active() const { return m_ActiveId; }
	int  by_utf8xbot_4005_queue_pos(int ClientId) const;
	void by_utf8xbot_4006_promote_next(int64_t Now);

 private:
	struct SEntry { int m_ClientId; int64_t m_JoinTime; };
	SEntry m_aQueue[MAX_CLIENTS];
	int m_QueueSize;
	int m_ActiveId;
};

#endif
