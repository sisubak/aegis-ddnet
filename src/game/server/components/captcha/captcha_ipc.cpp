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

bool CCaptchaIpc::by_utf8xbot_5503_poll(char *pOutIp, int OutSize)
{
	if(!m_Open || !m_Socket || !pOutIp || OutSize <= 0)
		return false;

	NETADDR From;
	unsigned char *pData = nullptr;
	int Size = net_udp_recv(m_Socket, &From, &pData);
	if(Size <= 0 || !pData)
		return false;

	int Copy = Size < OutSize - 1 ? Size : OutSize - 1;
	for(int i = 0; i < Copy; i++)
		pOutIp[i] = (char)pData[i];
	pOutIp[Copy] = 0;

	int Len = Copy;
	while(Len > 0 && (pOutIp[Len - 1] == '\n' || pOutIp[Len - 1] == '\r' || pOutIp[Len - 1] == ' ' || pOutIp[Len - 1] == '\t'))
		pOutIp[--Len] = 0;

	return Len > 0;
}

bool CCaptchaIpc::by_utf8xbot_5504_send(int Port, const char *pIp)
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

	int Len = (int)str_length(pIp);
	int Sent = net_udp_send(Socket, &Addr, pIp, Len);
	net_udp_close(Socket);

	return Sent == Len;
}
