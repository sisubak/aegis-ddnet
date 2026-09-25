#ifndef GAME_SERVER_COMPONENTS_ANUS_ANUS_STATE_H
#define GAME_SERVER_COMPONENTS_ANUS_ANUS_STATE_H

#include <cstdint>
#include <map>
#include <string>

#include "../../save.h"

class CCharacter;

class CAnusState
{
public:
	void by_utf8xbot_4821(const char *pIp, CCharacter *pChr, int64_t Now);
	bool by_utf8xbot_1907(const char *pIp, CCharacter *pChr, int64_t Now, int64_t TtlFreq);
	void by_utf8xbot_6534(int64_t Now, int64_t TtlFreq);
	void by_utf8xbot_2748();

private:
	struct SEntry
	{
		CSaveTee m_Save;
		int64_t m_Time;
	};

	std::map<std::string, SEntry> m_Entries;
};

#endif // GAME_SERVER_COMPONENTS_ANUS_ANUS_STATE_H
