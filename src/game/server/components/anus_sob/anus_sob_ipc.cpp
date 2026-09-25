#include "anus_sob_ipc.h"

#include <base/detect.h>
#include <base/log.h>
#include <cstring>

#if defined(CONF_FAMILY_UNIX)
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/select.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#endif

int CAnusSobIpc::by_utf8xbot_2100_listen_unix(const char *pPath)
{
#if defined(CONF_FAMILY_UNIX)
	int Fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if(Fd < 0) return -1;
	struct sockaddr_un Addr;
	std::memset(&Addr, 0, sizeof(Addr));
	Addr.sun_family = AF_UNIX;
	std::strncpy(Addr.sun_path, pPath, sizeof(Addr.sun_path) - 1);
	unlink(pPath);
	if(bind(Fd, (struct sockaddr *)&Addr, sizeof(Addr)) < 0) { close(Fd); return -1; }
	if(listen(Fd, 1) < 0) { close(Fd); return -1; }
	return Fd;
#else
	(void)pPath; return -1;
#endif
}

int CAnusSobIpc::by_utf8xbot_2101_accept(int ListenFd, int TimeoutMs)
{
#if defined(CONF_FAMILY_UNIX)
	if(ListenFd < 0) return -1;
	fd_set Set; FD_ZERO(&Set); FD_SET(ListenFd, &Set);
	struct timeval Tv; Tv.tv_sec = TimeoutMs / 1000; Tv.tv_usec = (TimeoutMs % 1000) * 1000;
	int R = select(ListenFd + 1, &Set, nullptr, nullptr, &Tv);
	if(R <= 0) return -1;
	return accept(ListenFd, nullptr, nullptr);
#else
	(void)ListenFd; (void)TimeoutMs; return -1;
#endif
}

int CAnusSobIpc::by_utf8xbot_2102_connect_unix(const char *pPath, int TimeoutMs)
{
#if defined(CONF_FAMILY_UNIX)
	int Deadline = TimeoutMs;
	while(Deadline > 0)
	{
		int Fd = socket(AF_UNIX, SOCK_STREAM, 0);
		if(Fd < 0) return -1;
		struct sockaddr_un Addr;
		std::memset(&Addr, 0, sizeof(Addr));
		Addr.sun_family = AF_UNIX;
		std::strncpy(Addr.sun_path, pPath, sizeof(Addr.sun_path) - 1);
		if(connect(Fd, (struct sockaddr *)&Addr, sizeof(Addr)) == 0) return Fd;
		close(Fd);
		struct timespec Ts; Ts.tv_sec = 0; Ts.tv_nsec = 50 * 1000 * 1000;
		nanosleep(&Ts, nullptr);
		Deadline -= 50;
	}
	return -1;
#else
	(void)pPath; (void)TimeoutMs; return -1;
#endif
}

bool CAnusSobIpc::by_utf8xbot_2103_send_all(int Fd, const void *pBuf, unsigned Len)
{
#if defined(CONF_FAMILY_UNIX)
	if(Fd < 0) return false;
	const unsigned char *p = (const unsigned char *)pBuf;
	unsigned Off = 0;
	while(Off < Len)
	{
		ssize_t W = send(Fd, p + Off, Len - Off, 0);
		if(W <= 0) { if(errno == EINTR) continue; return false; }
		Off += (unsigned)W;
	}
	return true;
#else
	(void)Fd; (void)pBuf; (void)Len; return false;
#endif
}

bool CAnusSobIpc::by_utf8xbot_2104_recv_all(int Fd, void *pBuf, unsigned Len)
{
#if defined(CONF_FAMILY_UNIX)
	if(Fd < 0) return false;
	unsigned char *p = (unsigned char *)pBuf;
	unsigned Off = 0;
	while(Off < Len)
	{
		ssize_t R = recv(Fd, p + Off, Len - Off, 0);
		if(R == 0) return false;
		if(R < 0) { if(errno == EINTR) continue; return false; }
		Off += (unsigned)R;
	}
	return true;
#else
	(void)Fd; (void)pBuf; (void)Len; return false;
#endif
}

bool CAnusSobIpc::by_utf8xbot_2105_send_frame(int Fd, const std::vector<unsigned char> &Frame)
{
	unsigned Len = (unsigned)Frame.size();
	unsigned char Hdr[4];
	Hdr[0] = (unsigned char)( Len        & 0xff);
	Hdr[1] = (unsigned char)((Len >>  8) & 0xff);
	Hdr[2] = (unsigned char)((Len >> 16) & 0xff);
	Hdr[3] = (unsigned char)((Len >> 24) & 0xff);
	if(!by_utf8xbot_2103_send_all(Fd, Hdr, 4)) return false;
	if(Len == 0) return true;
	return by_utf8xbot_2103_send_all(Fd, Frame.data(), Len);
}

bool CAnusSobIpc::by_utf8xbot_2106_recv_frame(int Fd, std::vector<unsigned char> &Out, unsigned MaxLen)
{
	unsigned char Hdr[4];
	if(!by_utf8xbot_2104_recv_all(Fd, Hdr, 4)) return false;
	unsigned Len = (unsigned)Hdr[0] | ((unsigned)Hdr[1] << 8) | ((unsigned)Hdr[2] << 16) | ((unsigned)Hdr[3] << 24);
	if(Len > MaxLen) return false;
	Out.resize(Len);
	if(Len == 0) return true;
	return by_utf8xbot_2104_recv_all(Fd, Out.data(), Len);
}

void CAnusSobIpc::by_utf8xbot_2107_close(int Fd)
{
#if defined(CONF_FAMILY_UNIX)
	if(Fd >= 0) close(Fd);
#else
	(void)Fd;
#endif
}

void CAnusSobIpc::by_utf8xbot_2108_unlink(const char *pPath)
{
#if defined(CONF_FAMILY_UNIX)
	unlink(pPath);
#else
	(void)pPath;
#endif
}
