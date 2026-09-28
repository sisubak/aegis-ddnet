#include "captcha_ipc.h"

#include <base/mem.h>
#include <base/str.h>

#include <cstring>

CCaptchaIpc::CCaptchaIpc()
{
	m_Socket = nullptr;
	m_Open = false;
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

	NETADDR From;
	unsigned char *pData = nullptr;
	int Size = net_udp_recv(m_Socket, &From, &pData);
	if(Size <= 0 || !pData)
		return false;

	if(!(From.ip[0] == 127 && From.ip[1] == 0 && From.ip[2] == 0 && From.ip[3] == 1))
		return false;

	char aBuf[512];
	int Copy = Size < (int)sizeof(aBuf) - 1 ? Size : (int)sizeof(aBuf) - 1;
	for(int i = 0; i < Copy; i++)
		aBuf[i] = (char)pData[i];
	aBuf[Copy] = 0;

	int Len = Copy;
	while(Len > 0 && (aBuf[Len - 1] == '\n' || aBuf[Len - 1] == '\r' || aBuf[Len - 1] == ' ' || aBuf[Len - 1] == '\t'))
		aBuf[--Len] = 0;

	const char *pSep = strchr(aBuf, ':');
	if(!pSep)
		return false;

	int SecretLen = (int)(pSep - aBuf);
	const char *pExpected = pSecret ? pSecret : "";
	if(pExpected[0] == '\0')
		return false;
	if((int)str_length(pExpected) != SecretLen || str_comp_num(aBuf, pExpected, SecretLen) != 0)
		return false;

	const char *pIp = pSep + 1;
	int IpLen = 0;
	while(pIp[IpLen] && IpLen < OutSize - 1)
	{
		pOutIp[IpLen] = pIp[IpLen];
		IpLen++;
	}
	pOutIp[IpLen] = 0;

	return IpLen > 0;
}

bool CCaptchaIpc::by_utf8xbot_5504_send(int Port, const char *pIp, const char *pSecret)
{
	if(!pIp)
		return false;

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
	str_format(aBuf, sizeof(aBuf), "%s:%s", pSecret ? pSecret : "", pIp);
	int Len = (int)str_length(aBuf);
	int Sent = net_udp_send(Socket, &Addr, aBuf, Len);
	net_udp_close(Socket);

	return Sent == Len;
}
