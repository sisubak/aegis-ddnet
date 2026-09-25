#ifndef GAME_SERVER_COMPONENTS_ANUS_SOB_ANUS_SOB_IPC_H
#define GAME_SERVER_COMPONENTS_ANUS_SOB_ANUS_SOB_IPC_H

#include <base/detect.h>
#include <base/types.h>
#include <vector>

class CAnusSobIpc
{
 public:
	static int  by_utf8xbot_2100_listen_unix(const char *pPath);
	static int  by_utf8xbot_2101_accept(int ListenFd, int TimeoutMs);
	static int  by_utf8xbot_2102_connect_unix(const char *pPath, int TimeoutMs);
	static bool by_utf8xbot_2103_send_all(int Fd, const void *pBuf, unsigned Len);
	static bool by_utf8xbot_2104_recv_all(int Fd, void *pBuf, unsigned Len);
	static bool by_utf8xbot_2105_send_frame(int Fd, const std::vector<unsigned char> &Frame);
	static bool by_utf8xbot_2106_recv_frame(int Fd, std::vector<unsigned char> &Out, unsigned MaxLen);
	static void by_utf8xbot_2107_close(int Fd);
	static void by_utf8xbot_2108_unlink(const char *pPath);
};

#endif
