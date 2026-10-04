#include "captcha_ipc.h"

#include <base/hash.h>
#include <base/hash_ctxt.h>
#include <base/mem.h>
#include <base/secure.h>
#include <base/str.h>
#include <base/time.h>

#include <cstring>

namespace
{
	SHA256_DIGEST by_utf8xbot_5510_hmac(const char *pKey, const char *pMsg, size_t MsgLen)
	{
		const size_t Block = 64;
		unsigned char aKey[64];
		mem_zero(aKey, sizeof(aKey));
		size_t KeyLen = pKey ? strlen(pKey) : 0;
		if(KeyLen > Block)
		{
			SHA256_DIGEST KeyDig = sha256(pKey, KeyLen);
			mem_copy(aKey, KeyDig.data, SHA256_DIGEST_LENGTH);
		}
		else if(KeyLen > 0)
		{
			mem_copy(aKey, pKey, KeyLen);
		}

		unsigned char aIpad[64];
		unsigned char aOpad[64];
		for(size_t i = 0; i < Block; i++)
		{
			aIpad[i] = (unsigned char)(aKey[i] ^ 0x36);
			aOpad[i] = (unsigned char)(aKey[i] ^ 0x5c);
		}

		SHA256_CTX Inner;
		sha256_init(&Inner);
		sha256_update(&Inner, aIpad, Block);
		sha256_update(&Inner, pMsg, MsgLen);
		SHA256_DIGEST InnerDig = sha256_finish(&Inner);

		SHA256_CTX Outer;
		sha256_init(&Outer);
		sha256_update(&Outer, aOpad, Block);
		sha256_update(&Outer, InnerDig.data, SHA256_DIGEST_LENGTH);
		return sha256_finish(&Outer);
	}
}

CCaptchaIpc::CCaptchaIpc()
{
	m_Socket = nullptr;
	m_Open = false;
	m_RateWindowStart = 0;
	m_RateCount = 0;
}

CCaptchaIpc::~CCaptchaIpc()
{
	by_utf8xbot_5502_close();
}

bool CCaptchaIpc::by_utf8xbot_5501_open_receiver(int Port)
{
	by_utf8xbot_5502_close();

	NETADDR BindAddr;
	mem_zero(&BindAddr, sizeof(BindAddr));
	BindAddr.type = NETTYPE_IPV4;
	BindAddr.ip[0] = 127;
	BindAddr.ip[1] = 0;
	BindAddr.ip[2] = 0;
	BindAddr.ip[3] = 1;
	BindAddr.port = Port;

	m_Socket = net_udp_create(BindAddr);
	if(!m_Socket)
		return false;

	m_Open = true;
	return true;
}

void CCaptchaIpc::by_utf8xbot_5502_close()
{
	if(m_Open && m_Socket)
		net_udp_close(m_Socket);
	m_Socket = nullptr;
	m_Open = false;
}

bool CCaptchaIpc::by_utf8xbot_5503_poll(char *pOutIp, int OutSize, const char *pSecret)
{
	if(!m_Open || !m_Socket || !pOutIp || OutSize <= 0)
		return false;

	const char *pSec = pSecret ? pSecret : "";
	if(pSec[0] == '\0')
		return false;

	const int MaxPerSecond = 128;

	while(true)
	{
		NETADDR From;
		unsigned char *pData = nullptr;
		int Size = net_udp_recv(m_Socket, &From, &pData);
		if(Size <= 0 || !pData)
			return false;

		int64_t Now = time_get();
		int64_t Freq = time_freq();
		if(Now - m_RateWindowStart >= Freq)
		{
			m_RateWindowStart = Now;
			m_RateCount = 0;
		}
		if(m_RateCount >= MaxPerSecond)
			return false;
		m_RateCount++;

		if(!(From.ip[0] == 127 && From.ip[1] == 0 && From.ip[2] == 0 && From.ip[3] == 1))
			continue;

		char aBuf[512];
		int Copy = Size < (int)sizeof(aBuf) - 1 ? Size : (int)sizeof(aBuf) - 1;
		for(int i = 0; i < Copy; i++)
			aBuf[i] = (char)pData[i];
		aBuf[Copy] = 0;

		int Len = Copy;
		while(Len > 0 && (aBuf[Len - 1] == '\n' || aBuf[Len - 1] == '\r' || aBuf[Len - 1] == ' ' || aBuf[Len - 1] == '\t'))
			aBuf[--Len] = 0;

		char *pSep1 = strchr(aBuf, ':');
		if(!pSep1)
			continue;
		*pSep1 = 0;
		char *pSep2 = strchr(pSep1 + 1, ':');
		if(!pSep2)
			continue;
		*pSep2 = 0;
		const char *pNonce = aBuf;
		const char *pMacHex = pSep1 + 1;
		const char *pIp = pSep2 + 1;

		if(str_length(pNonce) != (int)SHA256_MAXSTRSIZE - 1)
			continue;
		if(str_length(pMacHex) != (int)SHA256_MAXSTRSIZE - 1)
			continue;
		if(pIp[0] == '\0')
			continue;

		char aMsg[256];
		str_format(aMsg, sizeof(aMsg), "%s|%s", pNonce, pIp);
		SHA256_DIGEST Computed = by_utf8xbot_5510_hmac(pSec, aMsg, strlen(aMsg));
		SHA256_DIGEST Received;
		if(sha256_from_str(&Received, pMacHex) != 0)
			continue;
		unsigned char Diff = 0;
		for(size_t i = 0; i < SHA256_DIGEST_LENGTH; i++)
			Diff |= (unsigned char)(Computed.data[i] ^ Received.data[i]);
		if(Diff != 0)
			continue;

		std::string Nonce(pNonce);
		if(m_NonceSeen.find(Nonce) != m_NonceSeen.end())
			continue;
		m_NonceSeen.insert(Nonce);
		m_vNonceOrder.push_back(Nonce);
		if(m_vNonceOrder.size() > 512)
		{
			m_NonceSeen.erase(m_vNonceOrder.front());
			m_vNonceOrder.pop_front();
		}

		int IpLen = 0;
		while(pIp[IpLen] && IpLen < OutSize - 1)
		{
			pOutIp[IpLen] = pIp[IpLen];
			IpLen++;
		}
		pOutIp[IpLen] = 0;

		if(IpLen > 0)
			return true;
	}
}

bool CCaptchaIpc::Send(int Port, const char *pIp, const char *pSecret)
{
	if(!pIp)
		return false;

	const char *pSec = pSecret ? pSecret : "";
	if(pSec[0] == '\0')
		return false;

	SHA256_DIGEST NonceRaw;
	secure_random_fill(NonceRaw.data, SHA256_DIGEST_LENGTH);
	char aNonceHex[SHA256_MAXSTRSIZE];
	sha256_str(NonceRaw, aNonceHex, sizeof(aNonceHex));

	char aMsg[256];
	str_format(aMsg, sizeof(aMsg), "%s|%s", aNonceHex, pIp);
	SHA256_DIGEST Mac = by_utf8xbot_5510_hmac(pSec, aMsg, strlen(aMsg));
	char aMacHex[SHA256_MAXSTRSIZE];
	sha256_str(Mac, aMacHex, sizeof(aMacHex));

	NETADDR Addr;
	mem_zero(&Addr, sizeof(Addr));
	Addr.type = NETTYPE_IPV4;
	Addr.ip[0] = 127;
	Addr.ip[1] = 0;
	Addr.ip[2] = 0;
	Addr.ip[3] = 1;
	Addr.port = Port;

	NETADDR BindAddr;
	mem_zero(&BindAddr, sizeof(BindAddr));
	BindAddr.type = NETTYPE_IPV4;

	NETSOCKET Socket = net_udp_create(BindAddr);
	if(!Socket)
		return false;

	char aBuf[512];
	str_format(aBuf, sizeof(aBuf), "%s:%s:%s", aNonceHex, aMacHex, pIp);
	int Len = (int)str_length(aBuf);
	int Sent = net_udp_send(Socket, &Addr, aBuf, Len);
	net_udp_close(Socket);

	return Sent == Len;
}
