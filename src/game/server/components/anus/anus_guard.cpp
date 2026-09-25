#include "anus_guard.h"

void CAnusGuard::by_utf8xbot_4712(int64_t Now, int64_t WindowFreq)
{
	(void)WindowFreq;
	m_WindowStart = Now;
	m_PacketCount = 0;
	m_LastPort = 0;
}

bool CAnusGuard::by_utf8xbot_8391(int64_t Now, int64_t WindowFreq, int Threshold)
{
	m_PacketCount++;

	if(Now - m_WindowStart >= WindowFreq)
	{
		bool NeedMove = m_PacketCount > Threshold;
		m_WindowStart = Now;
		m_PacketCount = 0;
		return NeedMove;
	}

	return false;
}

int CAnusGuard::by_utf8xbot_2056(int PortMin, int PortMax, int Exclude)
{
	int Range = PortMax - PortMin + 1;
	if(Range <= 0)
		return PortMin;

	int Port = PortMin + (rand() % Range);

	if(Range == 1)
		return Port;

	while(Port == Exclude)
		Port = PortMin + (rand() % Range);

	return Port;
}

void CAnusGuard::by_utf8xbot_6743(int Port)
{
	m_LastPort = Port;
}

int CAnusGuard::by_utf8xbot_1928() const
{
	return m_PacketCount;
}
