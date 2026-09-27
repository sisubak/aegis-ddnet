#ifndef GAME_SERVER_COMPONENTS_CAPTCHA_CAPTCHA_SESSION_H
#define GAME_SERVER_COMPONENTS_CAPTCHA_CAPTCHA_SESSION_H

#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <cstdio>

class CCaptchaSession
{
public:
	char m_aCode[16];
	int m_CodeLen;
	int m_ActiveClient;
	int64_t m_StartTime;
	int m_TypedChars;
	int m_Attempts;
	int64_t m_LastAttempt;

	CCaptchaSession();

	void by_utf8xbot_A(int NumDigits);
	const char *by_utf8xbot_B() const;
	int by_utf8xbot_C(int Digit) const;
	void by_utf8xbot_D(int Client, int64_t Now);
	void by_utf8xbot_E();
	int by_utf8xbot_F() const;
	bool by_utf8xbot_M(int64_t Now, int64_t Freq);
	bool by_utf8xbot_G(int64_t Now, int64_t Freq, int TimeoutSec) const;
	bool by_utf8xbot_H(const char *pAnswer) const;
	const char *by_utf8xbot_I();
	int by_utf8xbot_J(int64_t Now, int64_t Freq);

	static void by_utf8xbot_K(const char *pPath, const char *pIp);
	static bool by_utf8xbot_L(const char *pPath, const char *pIp);
};

#endif
