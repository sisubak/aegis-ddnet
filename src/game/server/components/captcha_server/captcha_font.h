#ifndef GAME_SERVER_COMPONENTS_CAPTCHA_SERVER_CAPTCHA_FONT_H
#define GAME_SERVER_COMPONENTS_CAPTCHA_SERVER_CAPTCHA_FONT_H

#include <base/vmath.h>
#include <vector>

class CCaptchaFont
{
 public:
	static const int GLYPH_W = 5;
	static const int GLYPH_H = 7;

	struct SLaserSegment { vec2 m_From; vec2 m_To; };

	static const unsigned char *by_utf8xbot_3001_pixels(int Digit);

	static void by_utf8xbot_3002_render(int Digit, vec2 Origin, float CellPx, int JitterSeed, std::vector<SLaserSegment> &Out);
};

#endif
