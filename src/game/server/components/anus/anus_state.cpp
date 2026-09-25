#include "anus_state.h"

#include "../../entities/character.h"

void CAnusState::by_utf8xbot_4821(const char *pIp, CCharacter *pChr, int64_t Now)
{
	if(pIp == nullptr || pIp[0] == '\0' || pChr == nullptr)
		return;

	SEntry Entry;
	Entry.m_Save.Save(pChr, false);
	Entry.m_Time = Now;
	m_Entries[std::string(pIp)] = Entry;
}

bool CAnusState::by_utf8xbot_1907(const char *pIp, CCharacter *pChr, int64_t Now, int64_t TtlFreq)
{
	if(pIp == nullptr || pIp[0] == '\0' || pChr == nullptr)
		return false;

	auto It = m_Entries.find(std::string(pIp));
	if(It == m_Entries.end())
		return false;

	if(Now - It->second.m_Time > TtlFreq)
	{
		m_Entries.erase(It);
		return false;
	}

	bool Ok = It->second.m_Save.Load(pChr);
	m_Entries.erase(It);
	return Ok;
}

void CAnusState::by_utf8xbot_6534(int64_t Now, int64_t TtlFreq)
{
	for(auto It = m_Entries.begin(); It != m_Entries.end();)
	{
		if(Now - It->second.m_Time > TtlFreq)
			It = m_Entries.erase(It);
		else
			++It;
	}
}

void CAnusState::by_utf8xbot_2748()
{
	m_Entries.clear();
}
