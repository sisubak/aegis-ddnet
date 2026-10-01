#include "captcha_gate.h"

CCaptchaGate::CCaptchaGate()
{
	m_NumDigits = 6;
	for(int i = 0; i < MAX_CLIENTS; i++)
		m_aActive[i] = false;
}

bool CCaptchaGate::by_utf8xbot_9056_valid(int ClientId) const
{
	return ClientId >= 0 && ClientId < MAX_CLIENTS;
}

bool CCaptchaGate::by_utf8xbot_9057_in_queue(int ClientId) const
{
	for(size_t i = 0; i < m_Queue.size(); i++)
	{
		if(m_Queue[i] == ClientId)
			return true;
	}
	return false;
}

void CCaptchaGate::by_utf8xbot_9058_start(int ClientId, int64_t Now)
{
	m_aSessions[ClientId].by_utf8xbot_A(m_NumDigits);
	m_aSessions[ClientId].by_utf8xbot_D(ClientId, Now);
	m_aActive[ClientId] = true;
}

void CCaptchaGate::by_utf8xbot_7413()
{
	m_Queue.clear();
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		m_aActive[i] = false;
		m_aSessions[i].by_utf8xbot_E();
	}
	m_NumDigits = 6;
}

int CCaptchaGate::by_utf8xbot_9054_active_count() const
{
	int Count = 0;
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(m_aActive[i])
			Count++;
	}
	return Count;
}

int CCaptchaGate::by_utf8xbot_9050_admit(int ClientId, int64_t Now, int MaxConcurrent, int MaxQueue)
{
	if(!by_utf8xbot_9056_valid(ClientId))
		return ADMIT_FULL;
	if(m_aActive[ClientId] || by_utf8xbot_9057_in_queue(ClientId))
		return ADMIT_KNOWN;
	if(MaxConcurrent < 1)
		MaxConcurrent = 1;
	if(by_utf8xbot_9054_active_count() < MaxConcurrent)
	{
		by_utf8xbot_9058_start(ClientId, Now);
		return ADMIT_ACTIVATED;
	}
	if(MaxQueue <= 0 || (int)m_Queue.size() < MaxQueue)
	{
		m_Queue.push_back(ClientId);
		return ADMIT_QUEUED;
	}
	return ADMIT_FULL;
}

void CCaptchaGate::by_utf8xbot_9051_promote(int64_t Now, int MaxConcurrent, std::vector<int> &vActivated)
{
	if(MaxConcurrent < 1)
		MaxConcurrent = 1;
	while(by_utf8xbot_9054_active_count() < MaxConcurrent && !m_Queue.empty())
	{
		int Next = m_Queue.front();
		m_Queue.erase(m_Queue.begin());
		if(!by_utf8xbot_9056_valid(Next))
			continue;
		by_utf8xbot_9058_start(Next, Now);
		vActivated.push_back(Next);
	}
}

void CCaptchaGate::by_utf8xbot_9046(int ClientId)
{
	if(by_utf8xbot_9056_valid(ClientId))
	{
		m_aActive[ClientId] = false;
		m_aSessions[ClientId].by_utf8xbot_E();
	}
	for(size_t i = 0; i < m_Queue.size(); i++)
	{
		if(m_Queue[i] == ClientId)
		{
			m_Queue.erase(m_Queue.begin() + i);
			break;
		}
	}
}

int CCaptchaGate::by_utf8xbot_9052_collect_timeouts(int64_t Now, int64_t Freq, int TimeoutSec, int *pOut, int MaxOut) const
{
	int Count = 0;
	for(int i = 0; i < MAX_CLIENTS && Count < MaxOut; i++)
	{
		if(m_aActive[i] && m_aSessions[i].by_utf8xbot_G(Now, Freq, TimeoutSec))
			pOut[Count++] = i;
	}
	return Count;
}

bool CCaptchaGate::by_utf8xbot_9053_is_active(int ClientId) const
{
	return by_utf8xbot_9056_valid(ClientId) && m_aActive[ClientId];
}

int CCaptchaGate::by_utf8xbot_1927(int ClientId) const
{
	if(!by_utf8xbot_9056_valid(ClientId) || m_aActive[ClientId])
		return 0;
	for(size_t i = 0; i < m_Queue.size(); i++)
	{
		if(m_Queue[i] == ClientId)
			return (int)i + 1;
	}
	return 0;
}

const char *CCaptchaGate::by_utf8xbot_8734(int ClientId)
{
	if(by_utf8xbot_9053_is_active(ClientId))
		return m_aSessions[ClientId].by_utf8xbot_B();
	return "";
}

int64_t CCaptchaGate::by_utf8xbot_9055_start_time(int ClientId) const
{
	if(by_utf8xbot_9056_valid(ClientId))
		return m_aSessions[ClientId].m_StartTime;
	return 0;
}

bool CCaptchaGate::by_utf8xbot_4890(int ClientId, const char *pAnswer)
{
	if(!by_utf8xbot_9053_is_active(ClientId))
		return false;
	return m_aSessions[ClientId].by_utf8xbot_H(pAnswer);
}

bool CCaptchaGate::by_utf8xbot_4891(int ClientId, int64_t Now, int64_t Freq)
{
	if(!by_utf8xbot_9053_is_active(ClientId))
		return false;
	return m_aSessions[ClientId].by_utf8xbot_M(Now, Freq);
}
