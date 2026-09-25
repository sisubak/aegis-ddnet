#include "captcha_gate.h"

CCaptchaGate::CCaptchaGate() :
	m_QueueSize(0),
	m_ActiveId(-1)
{
	for(int i = 0; i < MAX_CLIENTS; ++i)
	{
		m_aQueue[i].m_ClientId = -1;
		m_aQueue[i].m_JoinTime = 0;
	}
}

void CCaptchaGate::by_utf8xbot_4001_reset()
{
	m_QueueSize = 0;
	m_ActiveId = -1;
	for(int i = 0; i < MAX_CLIENTS; ++i)
	{
		m_aQueue[i].m_ClientId = -1;
		m_aQueue[i].m_JoinTime = 0;
	}
}

void CCaptchaGate::by_utf8xbot_4002_on_join(int ClientId, int64_t Now)
{
	if(ClientId < 0 || ClientId >= MAX_CLIENTS) return;
	if(m_ActiveId == ClientId) return;
	for(int i = 0; i < m_QueueSize; ++i)
		if(m_aQueue[i].m_ClientId == ClientId) return;
	if(m_QueueSize >= MAX_CLIENTS) return;
	m_aQueue[m_QueueSize].m_ClientId = ClientId;
	m_aQueue[m_QueueSize].m_JoinTime = Now;
	++m_QueueSize;
	if(m_ActiveId < 0)
		by_utf8xbot_4006_promote_next(Now);
}

void CCaptchaGate::by_utf8xbot_4003_on_leave(int ClientId)
{
	if(m_ActiveId == ClientId)
		m_ActiveId = -1;
	int w = 0;
	for(int r = 0; r < m_QueueSize; ++r)
	{
		if(m_aQueue[r].m_ClientId == ClientId) continue;
		m_aQueue[w++] = m_aQueue[r];
	}
	m_QueueSize = w;
}

int CCaptchaGate::by_utf8xbot_4005_queue_pos(int ClientId) const
{
	if(m_ActiveId == ClientId) return 0;
	for(int i = 0; i < m_QueueSize; ++i)
		if(m_aQueue[i].m_ClientId == ClientId) return i + 1;
	return -1;
}

void CCaptchaGate::by_utf8xbot_4006_promote_next(int64_t Now)
{
	if(m_ActiveId >= 0) return;
	if(m_QueueSize == 0) return;
	m_ActiveId = m_aQueue[0].m_ClientId;
	for(int i = 1; i < m_QueueSize; ++i)
		m_aQueue[i - 1] = m_aQueue[i];
	--m_QueueSize;
	(void)Now;
}
