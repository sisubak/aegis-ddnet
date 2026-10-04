#ifndef GAME_SERVER_COMPONENTS_CAPTCHA_CAPTCHA_HUD_H
#define GAME_SERVER_COMPONENTS_CAPTCHA_CAPTCHA_HUD_H

#include <cstdint>
#include <cstdio>
#include <cstring>

class CCaptchaHud
{
public:
	bool by_utf8xbot_7412(int64_t StartTime, int64_t Now, int64_t Freq, char *pOut, int OutSize);
	void by_utf8xbot_7834(int Position, char *pOut, int OutSize);

private:
	const char *by_utf8xbot_9157();
	int by_utf8xbot_6023(const char *pStr, int NumCp, char *pOut, int OutSize);
};

#endif
