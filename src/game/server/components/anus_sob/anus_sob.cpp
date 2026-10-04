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

void CAnusSob::by_utf8xbot_2008_note_source(uint64_t SourceHash)
{
	if(!by_utf8xbot_2005_is_active())
		return;
	if(m_UniqueSources.size() < 200000)
		m_UniqueSources.insert(SourceHash);
}

bool CAnusSob::by_utf8xbot_2003_should_migrate(int64_t Now, int64_t Freq)
{
	if(!by_utf8xbot_2005_is_active())
		return false;
	int64_t WindowLen = (Freq * g_Config.m_SvAnusSobWindowMs) / 1000;
	if(WindowLen <= 0)
		WindowLen = Freq;
	if(Now - m_WindowStart >= WindowLen)
		return false;
	if(m_PacketsInWindow < g_Config.m_SvAnusSobThreshold)
		return false;
	if((int)m_UniqueSources.size() < g_Config.m_SvAnusSobMinUniqueSources)
		return false;
	return true;
}

void CAnusSob::by_utf8xbot_2004_tick(int64_t Now, int64_t Freq)
{
	if(!by_utf8xbot_2005_is_active())
	{
		m_PacketsInWindow = 0;
		m_WindowStart = Now;
		m_UniqueSources.clear();
		return;
	}
	int64_t WindowLen = (Freq * g_Config.m_SvAnusSobWindowMs) / 1000;
	if(WindowLen <= 0)
		WindowLen = Freq;
	if(Now - m_WindowStart >= WindowLen)
	{
		if(m_PacketsInWindow >= g_Config.m_SvAnusSobThreshold && Now - m_LastMigrationLog >= Freq * 5)
		{
			log_info("anus_sob", "threshold hit: %d pkts/window (limit %d), unique sources %d (min %d) - migration %s here",
				m_PacketsInWindow, g_Config.m_SvAnusSobThreshold,
				(int)m_UniqueSources.size(), g_Config.m_SvAnusSobMinUniqueSources,
				(int)m_UniqueSources.size() >= g_Config.m_SvAnusSobMinUniqueSources ? "would trigger" : "suppressed (too few sources)");
			m_LastMigrationLog = Now;
		}
		m_WindowStart = Now;
		m_PacketsInWindow = 0;
		m_UniqueSources.clear();
	}
}
