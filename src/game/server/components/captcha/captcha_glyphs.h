#ifndef GAME_SERVER_COMPONENTS_CAPTCHA_CAPTCHA_GLYPHS_H
#define GAME_SERVER_COMPONENTS_CAPTCHA_CAPTCHA_GLYPHS_H

struct CGlyphSeg
{
	float x0, y0, x1, y1;
};

const CGlyphSeg *by_utf8xbot_31007(int Digit, int *pCount);

#endif
