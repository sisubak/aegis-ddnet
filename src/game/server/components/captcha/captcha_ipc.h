#ifndef GAME_SERVER_COMPONENTS_CAPTCHA_CAPTCHA_IPC_H
#define GAME_SERVER_COMPONENTS_CAPTCHA_CAPTCHA_IPC_H

#include <base/net.h>

#include <cstdint>
#include <deque>
#include <string>
#include <unordered_set>

class CCaptchaIpc
{
public:
	CCaptchaIpc();
	~CCaptchaIpc();

	bool by_utf8xbot_5501_open_receiver(int Port);
	void by_utf8xbot_5502_close();
	bool by_utf8xbot_5503_poll(char *pOutIp, int OutSize, const char *pSecret);
	static bool by_utf8xbot_5504_send(int Port, const char *pIp, const char *pSecret);

private:
	NETSOCKET m_Socket;
	bool m_Open;
	int64_t m_RateWindowStart;
	int m_RateCount;
	std::deque<std::string> m_vNonceOrder;
	std::unordered_set<std::string> m_NonceSeen;
};

#endif
