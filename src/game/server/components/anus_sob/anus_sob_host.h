#ifndef GAME_SERVER_COMPONENTS_ANUS_SOB_ANUS_SOB_HOST_H
#define GAME_SERVER_COMPONENTS_ANUS_SOB_ANUS_SOB_HOST_H

#include "anus_sob_state.h"

#include <base/types.h>
#include <vector>

class CGameContext;

class CAnusSobHost
{
 public:
	CAnusSobHost();

	void by_utf8xbot_2020_collect(CGameContext *pGs, std::vector<SAnusSobClientState> &Out);
	bool by_utf8xbot_2021_write_state_file(const char *pPath, const std::vector<SAnusSobClientState> &St);
	bool by_utf8xbot_2022_read_state_file(const char *pPath, std::vector<SAnusSobClientState> &Out);
	bool by_utf8xbot_2023_load_from_env();
	void by_utf8xbot_2027_cleanup_stale();
	void by_utf8xbot_2024_try_restore(CGameContext *pGs, int ClientId);
	int  by_utf8xbot_2025_pending_count() const { return (int)m_vPending.size(); }
	void by_utf8xbot_2026_clear_pending() { m_vPending.clear(); }
	void by_utf8xbot_2030_execute_migration(CGameContext *pGs, int NewPort);
	bool by_utf8xbot_2031_is_migrating() const;
	void by_utf8xbot_2032_tick(CGameContext *pGs);

 private:
	std::vector<SAnusSobClientState> m_vPending;
	bool m_LoadAttempted;
	int m_State = 0;
	int m_NewPort = 0;
	int m_ChildPid = -1;
	int64_t m_ExitDeadline = 0;
	char m_aStatePath[256] = {0};
};

#endif
