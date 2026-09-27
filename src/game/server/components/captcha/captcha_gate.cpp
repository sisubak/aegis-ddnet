#include "captcha_gate.h"

CCaptchaGate::CCaptchaGate()
{
	m_Active = -1;
	m_NumDigits = 6;
}

void CCaptchaGate::by_utf8xbot_7413()
{
	m_Queue.clear();
	m_Active = -1;
	m_NumDigits = 6;
	m_Session.by_utf8xbot_E();
}

void CCaptchaGate::by_utf8xbot_5182(int ClientId)
{
	if(ClientId < 0)
		return;
	if(ClientId == m_Active)
		return;
	for(size_t i = 0; i < m_Queue.size(); i++)
	{
		if(m_Queue[i] == ClientId)
			return;
	}
	m_Queue.push_back(ClientId);
}

void CCaptchaGate::by_utf8xbot_9046(int ClientId)
{
	if(ClientId < 0)
		return;
	for(size_t i = 0; i < m_Queue.size(); i++)
	{
		if(m_Queue[i] == ClientId)
		{
			m_Queue.erase(m_Queue.begin() + i);
			break;
		}
	}
	if(ClientId == m_Active)
	{
		m_Active = -1;
		m_Session.by_utf8xbot_E();
	}
}

int CCaptchaGate::by_utf8xbot_3271(int64_t Now)
{
	if(m_Active != -1)
		return -1;
	if(m_Queue.empty())
		return -1;
	int Next = m_Queue.front();
	m_Queue.erase(m_Queue.begin());
	m_Active = Next;
	m_Session.by_utf8xbot_A(m_NumDigits);
	m_Session.by_utf8xbot_D(Next, Now);
	return Next;
}

int CCaptchaGate::by_utf8xbot_6605(int64_t Now, int64_t Freq, int TimeoutSec) const
{
	if(m_Active == -1)
		return -1;
	if(m_Session.by_utf8xbot_G(Now, Freq, TimeoutSec))
		return m_Active;
	return -1;
}

bool CCaptchaGate::by_utf8xbot_4890(int ClientId, const char *pAnswer)
{
	if(ClientId < 0 || ClientId != m_Active)
		return false;
	return m_Session.by_utf8xbot_H(pAnswer);
}

bool CCaptchaGate::by_utf8xbot_4891(int ClientId, int64_t Now, int64_t Freq)
{
	if(ClientId < 0 || ClientId != m_Active)
		return false;
	return m_Session.by_utf8xbot_M(Now, Freq);
}


int CCaptchaGate::by_utf8xbot_2158() const
{
	return m_Active;
}

const char *CCaptchaGate::by_utf8xbot_8734()
{
	return m_Session.by_utf8xbot_B();
}

int CCaptchaGate::by_utf8xbot_1927(int ClientId) const
{
	if(ClientId < 0)
		return 0;
	if(ClientId == m_Active)
		return 0;
	for(size_t i = 0; i < m_Queue.size(); i++)
	{
		if(m_Queue[i] == ClientId)
			return (int)i + 1;
	}
	return 0;
}

int CCaptchaGate::by_utf8xbot_5061(int64_t Now, int64_t Freq)
{
	return m_Session.by_utf8xbot_J(Now, Freq);
}

const char *CCaptchaGate::by_utf8xbot_3348()
{
	return m_Session.by_utf8xbot_I();
}

void CCaptchaGate::by_utf8xbot_7702(const char *pPath, const char *pIp)
{
	CCaptchaSession::by_utf8xbot_K(pPath, pIp);
}

bool CCaptchaGate::by_utf8xbot_9915(const char *pPath, const char *pIp)
{
	return CCaptchaSession::by_utf8xbot_L(pPath, pIp);
}
