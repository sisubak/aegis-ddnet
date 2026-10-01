#ifndef GAME_SERVER_COMPONENTS_CAPTCHA_CAPTCHA_GATE_H
#define GAME_SERVER_COMPONENTS_CAPTCHA_CAPTCHA_GATE_H

#include <cstdint>
#include <vector>

#include <engine/shared/protocol.h>

#include "captcha_session.h"

class CCaptchaGate
{
public:
	enum
	{
		ADMIT_KNOWN = 0,
		ADMIT_ACTIVATED,
		ADMIT_QUEUED,
		ADMIT_FULL,
	};

	CCaptchaSession m_aSessions[MAX_CLIENTS];
	bool m_aActive[MAX_CLIENTS];
	std::vector<int> m_Queue;
	int m_NumDigits;

	CCaptchaGate();

	void by_utf8xbot_7413();
	int by_utf8xbot_9050_admit(int ClientId, int64_t Now, int MaxConcurrent, int MaxQueue);
	void by_utf8xbot_9051_promote(int64_t Now, int MaxConcurrent, std::vector<int> &vActivated);
	void by_utf8xbot_9046(int ClientId);
	int by_utf8xbot_9052_collect_timeouts(int64_t Now, int64_t Freq, int TimeoutSec, int *pOut, int MaxOut) const;

	bool by_utf8xbot_9053_is_active(int ClientId) const;
	int by_utf8xbot_9054_active_count() const;
	int by_utf8xbot_1927(int ClientId) const;
	const char *by_utf8xbot_8734(int ClientId);
	int64_t by_utf8xbot_9055_start_time(int ClientId) const;
	bool by_utf8xbot_4890(int ClientId, const char *pAnswer);
	bool by_utf8xbot_4891(int ClientId, int64_t Now, int64_t Freq);

private:
	bool by_utf8xbot_9056_valid(int ClientId) const;
	bool by_utf8xbot_9057_in_queue(int ClientId) const;
	void by_utf8xbot_9058_start(int ClientId, int64_t Now);
};

#endif
