#include "captcha_session.h"

#include <base/fs.h>
#include <base/secure.h>

#include <string>
#include <unordered_map>
#include <vector>

CCaptchaSession::CCaptchaSession()
{
	m_aCode[0] = 0;
	m_CodeLen = 0;

	m_ActiveClient = -1;
	m_StartTime = 0;
	m_TypedChars = 0;
	m_Attempts = 0;
	m_LastAttempt = 0;
}


void CCaptchaSession::by_utf8xbot_A(int NumDigits)
{
	if(NumDigits < 1)
		NumDigits = 1;
	if(NumDigits > 15)
		NumDigits = 15;
	for(int i = 0; i < NumDigits; i++)
	{
		unsigned char Rnd = 0;
		secure_random_fill(&Rnd, sizeof(Rnd));
		m_aCode[i] = (char)('0' + (Rnd % 10));
	}
	m_aCode[NumDigits] = 0;
	m_CodeLen = NumDigits;
}


const char *CCaptchaSession::by_utf8xbot_B() const
{
	return m_aCode;
}

int CCaptchaSession::by_utf8xbot_C(int Digit) const
{
	if(Digit < 0 || Digit >= m_CodeLen)
		return -1;
	return m_aCode[Digit] - '0';
}

void CCaptchaSession::by_utf8xbot_D(int Client, int64_t Now)
{
	m_ActiveClient = Client;
	m_StartTime = Now;
	m_TypedChars = 0;
	m_Attempts = 0;
	m_LastAttempt = 0;
}


void CCaptchaSession::by_utf8xbot_E()
{
	m_ActiveClient = -1;
}

int CCaptchaSession::by_utf8xbot_F() const
{
	return m_ActiveClient;
}

bool CCaptchaSession::by_utf8xbot_M(int64_t Now, int64_t Freq)
{
	if(Freq > 0 && m_LastAttempt != 0 && (Now - m_LastAttempt) < (Freq / 2))
		return false;
	m_LastAttempt = Now;
	m_Attempts++;
	if(m_Attempts > 20)
		return false;
	return true;
}


bool CCaptchaSession::by_utf8xbot_G(int64_t Now, int64_t Freq, int TimeoutSec) const
{
	return (Now - m_StartTime) > (Freq * (int64_t)TimeoutSec);
}

bool CCaptchaSession::by_utf8xbot_H(const char *pAnswer) const
{
	if(!pAnswer)
		return false;
	const char *pStart = pAnswer;
	while(*pStart == ' ' || *pStart == '\t')
		pStart++;
	int Len = (int)strlen(pStart);
	while(Len > 0 && (pStart[Len - 1] == ' ' || pStart[Len - 1] == '\t' || pStart[Len - 1] == '\r' || pStart[Len - 1] == '\n'))
		Len--;
	char aTrimmed[16];
	if(Len < 0)
		Len = 0;
	if(Len > 15)
		return false;
	for(int i = 0; i < Len; i++)
		aTrimmed[i] = pStart[i];
	aTrimmed[Len] = 0;
	if(Len != m_CodeLen)
		return false;
	return strcmp(aTrimmed, m_aCode) == 0;
}

const char *CCaptchaSession::by_utf8xbot_I()
{
	return "Введи цифры из лазеров в чат что бы пройти капчу";
}

int CCaptchaSession::by_utf8xbot_J(int64_t Now, int64_t Freq)
{
	const char *pPhrase = by_utf8xbot_I();
	int Len = (int)strlen(pPhrase);
	if(Freq <= 0)
		return Len;
	int64_t ElapsedMs = ((Now - m_StartTime) * (int64_t)1000) / Freq;
	if(ElapsedMs < 0)
		ElapsedMs = 0;
	int64_t Shown = ElapsedMs / 50;
	if(Shown > (int64_t)Len)
		Shown = Len;
	m_TypedChars = (int)Shown;
	return (int)Shown;
}

namespace {
struct SWlEntry
{
	std::string m_Ip;
	int64_t m_Ts;
};

void by_utf8xbot_8120_read_entries(const char *pPath, std::vector<SWlEntry> &vOut)
{
	vOut.clear();
	FILE *pFile = fopen(pPath, "r");
	if(!pFile)
		return;
	char aLine[256];
	while(fgets(aLine, sizeof(aLine), pFile))
	{
		int Len = (int)strlen(aLine);
		while(Len > 0 && (aLine[Len - 1] == '\n' || aLine[Len - 1] == '\r' || aLine[Len - 1] == ' ' || aLine[Len - 1] == '\t'))
			aLine[--Len] = 0;
		if(Len <= 0)
			continue;
		char *pSpace = strchr(aLine, ' ');
		SWlEntry Entry;
		if(pSpace)
		{
			*pSpace = 0;
			Entry.m_Ip = aLine;
			Entry.m_Ts = (int64_t)strtoll(pSpace + 1, nullptr, 10);
		}
		else
		{
			Entry.m_Ip = aLine;
			Entry.m_Ts = 0;
		}
		if(!Entry.m_Ip.empty())
			vOut.push_back(Entry);
	}
	fclose(pFile);
}
}

void CCaptchaSession::by_utf8xbot_K(const char *pPath, const char *pIp, int64_t NowUnix, int MaxEntries, int TtlSec)
{
	if(!pPath || !pIp || !*pIp)
		return;

	std::vector<SWlEntry> vEntries;
	by_utf8xbot_8120_read_entries(pPath, vEntries);

	std::vector<SWlEntry> vKept;
	vKept.reserve(vEntries.size() + 1);
	for(const SWlEntry &E : vEntries)
	{
		if(E.m_Ip == pIp)
			continue;
		if(TtlSec > 0 && (NowUnix - E.m_Ts) > (int64_t)TtlSec)
			continue;
		vKept.push_back(E);
	}

	SWlEntry New;
	New.m_Ip = pIp;
	New.m_Ts = NowUnix;
	vKept.push_back(New);

	if(MaxEntries > 0 && (int)vKept.size() > MaxEntries)
		vKept.erase(vKept.begin(), vKept.begin() + ((int)vKept.size() - MaxEntries));

	FILE *pFile = fopen(pPath, "w");
	if(!pFile)
		return;
	for(const SWlEntry &E : vKept)
		fprintf(pFile, "%s %lld\n", E.m_Ip.c_str(), (long long)E.m_Ts);
	fclose(pFile);
}

bool CCaptchaSession::by_utf8xbot_L(const char *pPath, const char *pIp, int64_t NowUnix, int TtlSec)
{
	if(!pPath || !pIp)
		return false;

	static std::unordered_map<std::string, int64_t> s_Whitelist;
	static std::string s_LoadedPath;
	static time_t s_LoadedMtime = 0;
	static bool s_Loaded = false;
	static int64_t s_LastCheck = 0;

	if(!s_Loaded || s_LoadedPath != pPath || NowUnix > s_LastCheck)
	{
		s_LastCheck = NowUnix;

		time_t Modified = 0;
		bool StatOk = fs_file_time(pPath, nullptr, &Modified) == 0;

		if(!s_Loaded || s_LoadedPath != pPath || !StatOk || Modified != s_LoadedMtime)
		{
			s_Whitelist.clear();
			std::vector<SWlEntry> vEntries;
			by_utf8xbot_8120_read_entries(pPath, vEntries);
			for(const SWlEntry &E : vEntries)
				s_Whitelist[E.m_Ip] = E.m_Ts;
			s_LoadedPath = pPath;
			s_LoadedMtime = StatOk ? Modified : 0;
			s_Loaded = true;
		}
	}

	auto It = s_Whitelist.find(std::string(pIp));
	if(It == s_Whitelist.end())
		return false;
	if(TtlSec > 0 && (NowUnix - It->second) > (int64_t)TtlSec)
		return false;
	return true;
}

int CCaptchaSession::by_utf8xbot_7731_gate_packet(const char *pPath, const char *pIp, int64_t NowUnix, int TtlSec, int64_t Now, int64_t Freq, int64_t *pGraceSince)
{
	if(by_utf8xbot_L(pPath, pIp, NowUnix, TtlSec))
	{
		if(pGraceSince)
			*pGraceSince = 0;
		return GATE_PASS;
	}
	if(!pGraceSince)
		return GATE_DROP;
	if(*pGraceSince == 0)
	{
		*pGraceSince = Now;
		return GATE_DROP;
	}
	if(Now - *pGraceSince > Freq * 3)
	{
		*pGraceSince = 0;
		return GATE_KICK;
	}
	return GATE_DROP;
}

