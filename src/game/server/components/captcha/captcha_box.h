#ifndef GAME_SERVER_COMPONENTS_CAPTCHA_CAPTCHA_BOX_H
#define GAME_SERVER_COMPONENTS_CAPTCHA_CAPTCHA_BOX_H

#include <vector>

#include <game/server/entity.h>

struct CSnapContext;

class CCaptchaBox : public CEntity
{
public:
	CCaptchaBox(CGameWorld *pGameWorld, vec2 Pos, const char *pCode);
	virtual ~CCaptchaBox();

	void Reset() override;
	void Tick() override;
	void TickPaused() override;
	void Snap(int SnappingClient) override;
	void SwapClients(int Client1, int Client2) override;

	void by_utf8xbot_2043(vec2 Pos);

private:
	char m_aCode[9];
	int m_NumDigits;
	std::vector<int> m_vIds;

	int by_utf8xbot_5521() const;
	float by_utf8xbot_8842(int Salt) const;
	void by_utf8xbot_6193(const CSnapContext &Context, int &IdIndex, vec2 From, vec2 To);
};

#endif
