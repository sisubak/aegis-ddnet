#include "anus_sob.h"

#include <base/log.h>
#include <engine/shared/config.h>

CAnusSob::CAnusSob() :
	m_pServer(nullptr),
	m_WindowStart(0),
	m_PacketsInWindow(0),
	m_LastMigrationLog(0)
{
}

void CAnusSob::by_utf8xbot_2001_attach(CServer *pServer)
{
	m_pServer = pServer;
}

bool CAnusSob::by_utf8xbot_2005_is_active() const
{
	return g_Config.m_SvAnusSobMode != 0;
}

void CAnusSob::by_utf8xbot_2002_note_packet()
{
	if(!by_utf8xbot_2005_is_active())
		return;
	++m_PacketsInWindow;
}

bool CAnusSob::by_utf8xbot_2003_should_migrate(int64_t Now, int64_t Freq)
{
	if(!by_utf8xbot_2005_is_active())
		return false;
	int64_t WindowLen = (Freq * g_Config.m_SvAnusSobWindowMs) / 1000;
	if(WindowLen <= 0) WindowLen = Freq;
	if(Now - m_WindowStart >= WindowLen)
		return false;
	return m_PacketsInWindow >= g_Config.m_SvAnusSobThreshold;
}

void CAnusSob::by_utf8xbot_2004_tick(int64_t Now, int64_t Freq)
{
	if(!by_utf8xbot_2005_is_active())
	{
		m_PacketsInWindow = 0;
		m_WindowStart = Now;
		return;
	}
	int64_t WindowLen = (Freq * g_Config.m_SvAnusSobWindowMs) / 1000;
	if(WindowLen <= 0) WindowLen = Freq;
	if(Now - m_WindowStart >= WindowLen)
	{
		if(m_PacketsInWindow >= g_Config.m_SvAnusSobThreshold && Now - m_LastMigrationLog >= Freq * 5)
		{
			log_info("anus_sob", "threshold hit: %d pkts/window (limit %d) - migration would trigger here", m_PacketsInWindow, g_Config.m_SvAnusSobThreshold);
			m_LastMigrationLog = Now;
		}
		m_WindowStart = Now;
		m_PacketsInWindow = 0;
	}
}
