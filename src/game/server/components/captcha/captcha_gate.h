#ifndef GAME_SERVER_COMPONENTS_CAPTCHA_CAPTCHA_GATE_H
#define GAME_SERVER_COMPONENTS_CAPTCHA_CAPTCHA_GATE_H

#include <cstdint>
#include <vector>

#include "captcha_session.h"

class CCaptchaGate
{
public:
	CCaptchaSession m_Session;
	std::vector<int> m_Queue;
	int m_Active;
	int m_NumDigits;

	CCaptchaGate();

	void by_utf8xbot_7413();
	void by_utf8xbot_5182(int ClientId);
	void by_utf8xbot_9046(int ClientId);
	int by_utf8xbot_3271(int64_t Now);
	int by_utf8xbot_6605(int64_t Now, int64_t Freq, int TimeoutSec) const;
	bool by_utf8xbot_4890(int ClientId, const char *pAnswer);
	bool by_utf8xbot_4891(int ClientId, int64_t Now, int64_t Freq);
	int by_utf8xbot_2158() const;
	const char *by_utf8xbot_8734();
	int by_utf8xbot_1927(int ClientId) const;
	int by_utf8xbot_5061(int64_t Now, int64_t Freq);
	const char *by_utf8xbot_3348();
};

#endif
