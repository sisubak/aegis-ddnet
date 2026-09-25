#include "captcha_box.h"

#include "captcha_glyphs.h"

#include <cstring>

#include <engine/shared/protocol.h>

#include <generated/protocol.h>

#include <game/mapitems.h>
#include <game/server/gamecontext.h>
#include <game/server/gameworld.h>

CCaptchaBox::CCaptchaBox(CGameWorld *pGameWorld, vec2 Pos, const char *pCode) :
	CEntity(pGameWorld, CGameWorld::ENTTYPE_LASER, false)
{
	m_Pos = Pos;
	m_Number = 0;
	m_Layer = LAYER_GAME;

	m_NumDigits = 0;
	m_aCode[0] = '\0';
	if(pCode)
	{
		while(m_NumDigits < 8 && pCode[m_NumDigits] != '\0')
		{
			m_aCode[m_NumDigits] = pCode[m_NumDigits];
			m_NumDigits++;
		}
	}
	m_aCode[m_NumDigits] = '\0';

	int SegmentCount = 4;
	for(int i = 0; i < m_NumDigits; i++)
	{
		int Digit = m_aCode[i] - '0';
		if(Digit < 0 || Digit > 9)
			continue;
		int Count = 0;
		by_utf8xbot_31007(Digit, &Count);
		SegmentCount += Count;
	}

	for(int i = 0; i < SegmentCount; i++)
	{
		std::optional<int> Id = Server()->SnapNewId();
		if(Id.has_value())
			m_vIds.push_back(Id.value());
	}

	GameWorld()->InsertEntity(this);
}

CCaptchaBox::~CCaptchaBox()
{
	for(int Id : m_vIds)
		Server()->SnapFreeId(Id);
	m_vIds.clear();
}

void CCaptchaBox::Reset()
{
	m_MarkedForDestroy = true;
}

void CCaptchaBox::Tick()
{
}

void CCaptchaBox::TickPaused()
{
}

int CCaptchaBox::by_utf8xbot_5521() const
{
	return m_NumDigits;
}

float CCaptchaBox::by_utf8xbot_8842(int Salt) const
{
	unsigned int x = (unsigned int)(Salt * 2654435761u + 40503u);
	x ^= x >> 13;
	x *= 1274126177u;
	x ^= x >> 16;
	float t = (float)(x % 1000u) / 1000.0f;
	return (t - 0.5f) * 6.0f;
}

void CCaptchaBox::by_utf8xbot_6193(const CSnapContext &Context, int &IdIndex, vec2 From, vec2 To)
{
	if(IdIndex >= (int)m_vIds.size())
		return;
	GameServer()->SnapLaserObject(Context, m_vIds[IdIndex], To, From, Server()->Tick(), -1, LASERTYPE_FREEZE, 0, -1);
	IdIndex++;
}

void CCaptchaBox::Snap(int SnappingClient)
{
	if(m_vIds.empty())
		return;
	if(NetworkClipped(SnappingClient))
		return;

	int SnappingClientVersion = GameServer()->GetClientVersion(SnappingClient);
	CSnapContext Context = CSnapContext(SnappingClientVersion, Server()->IsSixup(SnappingClient), SnappingClient);

	int IdIndex = 0;

	const float HalfW = 120.0f;
	const float HalfH = 90.0f;

	vec2 TopLeft = m_Pos + vec2(-HalfW, -HalfH);
	vec2 TopRight = m_Pos + vec2(HalfW, -HalfH);
	vec2 BottomLeft = m_Pos + vec2(-HalfW, HalfH);
	vec2 BottomRight = m_Pos + vec2(HalfW, HalfH);

	by_utf8xbot_6193(Context, IdIndex, TopLeft, TopRight);
	by_utf8xbot_6193(Context, IdIndex, TopRight, BottomRight);
	by_utf8xbot_6193(Context, IdIndex, BottomRight, BottomLeft);
	by_utf8xbot_6193(Context, IdIndex, BottomLeft, TopLeft);

	const float GlyphW = 48.0f;
	const float GlyphH = 64.0f;
	const float Step = 60.0f;

	float TotalWidth = (m_NumDigits > 0) ? (Step * (m_NumDigits - 1) + GlyphW) : 0.0f;
	float StartX = m_Pos.x - TotalWidth * 0.5f;
	float BaseY = m_Pos.y - HalfH - 40.0f - GlyphH;

	for(int i = 0; i < m_NumDigits; i++)
	{
		int Digit = m_aCode[i] - '0';
		if(Digit < 0 || Digit > 9)
			continue;

		int Count = 0;
		const CGlyphSeg *pSegs = by_utf8xbot_31007(Digit, &Count);
		if(!pSegs)
			continue;

		float OriginX = StartX + Step * i;

		for(int s = 0; s < Count; s++)
		{
			int Salt = (i * 733 + s * 97 + Digit * 17);

			float Jx0 = by_utf8xbot_8842(Salt + 1);
			float Jy0 = by_utf8xbot_8842(Salt + 2);
			float Jx1 = by_utf8xbot_8842(Salt + 3);
			float Jy1 = by_utf8xbot_8842(Salt + 4);

			vec2 From = vec2(
				OriginX + pSegs[s].x0 * GlyphW + Jx0,
				BaseY + pSegs[s].y0 * GlyphH + Jy0);
			vec2 To = vec2(
				OriginX + pSegs[s].x1 * GlyphW + Jx1,
				BaseY + pSegs[s].y1 * GlyphH + Jy1);

			by_utf8xbot_6193(Context, IdIndex, From, To);
		}
	}
}

void CCaptchaBox::SwapClients(int Client1, int Client2)
{
}
