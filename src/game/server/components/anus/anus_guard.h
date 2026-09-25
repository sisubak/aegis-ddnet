#ifndef GAME_SERVER_COMPONENTS_ANUS_ANUS_GUARD_H
#define GAME_SERVER_COMPONENTS_ANUS_ANUS_GUARD_H

#include <cstdint>
#include <cstdlib>

class CAnusGuard
{
	int64_t m_WindowStart;
	int m_PacketCount;
	int m_LastPort;

public:
	void by_utf8xbot_4712(int64_t Now, int64_t WindowFreq);
	bool by_utf8xbot_8391(int64_t Now, int64_t WindowFreq, int Threshold);
	int by_utf8xbot_2056(int PortMin, int PortMax, int Exclude);
	void by_utf8xbot_6743(int Port);
	int by_utf8xbot_1928() const;
};

#endif
