#ifndef GAME_SERVER_COMPONENTS_ANUS_SOB_ANUS_SOB_STATE_H
#define GAME_SERVER_COMPONENTS_ANUS_SOB_ANUS_SOB_STATE_H

#include <base/types.h>
#include <vector>

struct SAnusSobClientState
{
	int m_ClientId;
	char m_aAddr[64];
	char m_aName[32];
	char m_aClan[16];
	int m_Country;
	int m_Team;
	int m_Alive;
	float m_PosX, m_PosY;
	float m_VelX, m_VelY;
	int m_Direction;
	int m_Jumped;
	int m_HookState;
	int m_HookedPlayer;
	float m_HookX, m_HookY;
	int m_FreezeEndTick;
	int m_DeepFrozen;
	int m_LiveFrozen;
	int m_Solo;
	int m_Super;
	int m_Endless;
	int m_Jetpack;
	int m_ActiveWeapon;
	int m_LastWeapon;
	int m_aGotWeapon[6];
	int m_aAmmo[6];
	char m_aSaveCode[256];
};

class CAnusSobStateCodec
{
 public:
	static const unsigned MAGIC = 0x53534E41u;
	static const unsigned VERSION = 1u;

	static void by_utf8xbot_2200_pack(const std::vector<SAnusSobClientState> &In, std::vector<unsigned char> &Out);
	static bool by_utf8xbot_2201_unpack(const std::vector<unsigned char> &In, std::vector<SAnusSobClientState> &Out);
};

#endif
