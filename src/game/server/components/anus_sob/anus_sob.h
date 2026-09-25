#ifndef GAME_SERVER_COMPONENTS_ANUS_SOB_ANUS_SOB_H
#define GAME_SERVER_COMPONENTS_ANUS_SOB_ANUS_SOB_H

#include <base/types.h>

class CServer;

class CAnusSob
{
 public:
	CAnusSob();

	void by_utf8xbot_2001_attach(CServer *pServer);
	void by_utf8xbot_2002_note_packet();
	bool by_utf8xbot_2003_should_migrate(int64_t Now, int64_t Freq);
	void by_utf8xbot_2004_tick(int64_t Now, int64_t Freq);
	bool by_utf8xbot_2005_is_active() const;
	int  by_utf8xbot_2006_packets_in_window() const { return m_PacketsInWindow; }

 private:
	CServer *m_pServer;
	int64_t m_WindowStart;
	int m_PacketsInWindow;
	int64_t m_LastMigrationLog;
};

#endif
