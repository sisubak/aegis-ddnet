#ifndef GAME_SERVER_COMPONENTS_CAPTCHA_CAPTCHA_GLYPHS_H
#define GAME_SERVER_COMPONENTS_CAPTCHA_CAPTCHA_GLYPHS_H

struct CGlyphSeg
{
	float m_X0, m_Y0, m_X1, m_Y1;
};

const CGlyphSeg *by_utf8xbot_31007(int Digit, int *pCount);

#endif
