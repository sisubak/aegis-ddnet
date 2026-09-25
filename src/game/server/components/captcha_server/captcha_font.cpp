#include "captcha_font.h"

static const unsigned char PIXELS_5x7[10][7] = {
	{ 0b01110, 0b10001, 0b10011, 0b10101, 0b11001, 0b10001, 0b01110 },
	{ 0b00100, 0b01100, 0b00100, 0b00100, 0b00100, 0b00100, 0b01110 },
	{ 0b01110, 0b10001, 0b00001, 0b00110, 0b01000, 0b10000, 0b11111 },
	{ 0b11110, 0b00001, 0b00001, 0b01110, 0b00001, 0b00001, 0b11110 },
	{ 0b00010, 0b00110, 0b01010, 0b10010, 0b11111, 0b00010, 0b00010 },
	{ 0b11111, 0b10000, 0b11110, 0b00001, 0b00001, 0b10001, 0b01110 },
	{ 0b00110, 0b01000, 0b10000, 0b11110, 0b10001, 0b10001, 0b01110 },
	{ 0b11111, 0b00001, 0b00010, 0b00100, 0b01000, 0b01000, 0b01000 },
	{ 0b01110, 0b10001, 0b10001, 0b01110, 0b10001, 0b10001, 0b01110 },
	{ 0b01110, 0b10001, 0b10001, 0b01111, 0b00001, 0b00010, 0b01100 },
};

const unsigned char *CCaptchaFont::by_utf8xbot_3001_pixels(int Digit)
{
	if(Digit < 0 || Digit > 9)
		Digit = 0;
	return PIXELS_5x7[Digit];
}

void CCaptchaFont::by_utf8xbot_3002_render(int Digit, vec2 Origin, float CellPx, int JitterSeed, std::vector<SLaserSegment> &Out)
{
	const unsigned char *pRows = by_utf8xbot_3001_pixels(Digit);
	for(int y = 0; y < GLYPH_H; ++y)
	{
		int x = 0;
		while(x < GLYPH_W)
		{
			if(!(pRows[y] & (1 << x))) { ++x; continue; }
			int start = x;
			while(x < GLYPH_W && (pRows[y] & (1 << x))) ++x;
			int end = x - 1;
			float jx = ((JitterSeed * (y + 1) * 131u) & 0x7) - 3.5f;
			float jy = ((JitterSeed * (start + 1) * 197u) & 0x7) - 3.5f;
			SLaserSegment seg;
			seg.m_From = vec2(Origin.x + start * CellPx + jx, Origin.y + y * CellPx + jy);
			seg.m_To = vec2(Origin.x + (end + 1) * CellPx + jx, Origin.y + y * CellPx + jy);
			Out.push_back(seg);
		}
	}
}
