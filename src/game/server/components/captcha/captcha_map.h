#ifndef GAME_SERVER_COMPONENTS_CAPTCHA_CAPTCHA_MAP_H
#define GAME_SERVER_COMPONENTS_CAPTCHA_CAPTCHA_MAP_H

class IStorage;

class CCaptchaMap
{
public:
	static const int MAP_W = 60;
	static const int MAP_H = 40;
	static const int BOX_HALF_W_TILES = 4;
	static const int BOX_HALF_H_TILES = 3;

	static bool by_utf8xbot_4417(IStorage *pStorage, const char *pMapName);
	static void by_utf8xbot_5729_spawn(float *pX, float *pY);
	static void by_utf8xbot_5730_extents(float *pHalfW, float *pHalfH);
};

#endif
