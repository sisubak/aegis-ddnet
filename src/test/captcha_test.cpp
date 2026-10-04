#include "test.h"

#include <base/fs.h>
#include <base/io.h>
#include <base/str.h>

#include <game/server/components/captcha/captcha_gate.h>
#include <game/server/components/captcha/captcha_session.h>

#include <gtest/gtest.h>

#include <cstring>
#include <string>

static std::string by_utf8xbot_ReadFile(const char *pPath)
{
	IOHANDLE File = io_open(pPath, IOFLAG_READ);
	if(!File)
		return std::string();
	char aBuf[4096];
	int Read = io_read(File, aBuf, sizeof(aBuf) - 1);
	io_close(File);
	if(Read < 0)
		Read = 0;
	aBuf[Read] = 0;
	return std::string(aBuf);
}

TEST(Captcha, CodeGeneration)
{
	CCaptchaSession Session;
	Session.by_utf8xbot_A(6);
	EXPECT_EQ((int)strlen(Session.by_utf8xbot_B()), 6);
	for(const char *p = Session.by_utf8xbot_B(); *p; p++)
		EXPECT_TRUE(*p >= '0' && *p <= '9');

	Session.by_utf8xbot_A(0);
	EXPECT_EQ((int)strlen(Session.by_utf8xbot_B()), 1);
	Session.by_utf8xbot_A(99);
	EXPECT_EQ((int)strlen(Session.by_utf8xbot_B()), 15);
}

TEST(Captcha, CheckAnswer)
{
	CCaptchaSession Session;
	Session.by_utf8xbot_A(6);
	char aCode[16];
	str_copy(aCode, Session.by_utf8xbot_B(), sizeof(aCode));

	EXPECT_TRUE(Session.by_utf8xbot_H(aCode));

	char aPadded[32];
	str_format(aPadded, sizeof(aPadded), "  %s  ", aCode);
	EXPECT_TRUE(Session.by_utf8xbot_H(aPadded));

	char aTooLong[32];
	str_format(aTooLong, sizeof(aTooLong), "%s7", aCode);
	EXPECT_FALSE(Session.by_utf8xbot_H(aTooLong));

	char aWrong[16];
	str_copy(aWrong, aCode, sizeof(aWrong));
	aWrong[0] = (aWrong[0] == '0') ? '1' : '0';
	EXPECT_FALSE(Session.by_utf8xbot_H(aWrong));

	EXPECT_FALSE(Session.by_utf8xbot_H(nullptr));
}

TEST(Captcha, AttemptLimit)
{
	CCaptchaSession Session;
	Session.by_utf8xbot_A(4);
	Session.by_utf8xbot_D(0, 0);
	for(int i = 0; i < 20; i++)
		EXPECT_TRUE(Session.by_utf8xbot_M((int64_t)i, 0));
	EXPECT_FALSE(Session.by_utf8xbot_M(21, 0));
}

TEST(Captcha, AttemptRateLimit)
{
	CCaptchaSession Session;
	Session.by_utf8xbot_A(4);
	Session.by_utf8xbot_D(0, 0);
	EXPECT_TRUE(Session.by_utf8xbot_M(100, 50));
	EXPECT_FALSE(Session.by_utf8xbot_M(110, 50));
	EXPECT_TRUE(Session.by_utf8xbot_M(140, 50));
}

TEST(Captcha, Timeout)
{
	CCaptchaSession Session;
	Session.by_utf8xbot_A(4);
	Session.by_utf8xbot_D(0, 0);
	EXPECT_FALSE(Session.by_utf8xbot_G(100, 50, 2));
	EXPECT_TRUE(Session.by_utf8xbot_G(101, 50, 2));
}

TEST(Captcha, WhitelistFormatAndDedup)
{
	CTestInfo Info;
	CCaptchaSession::by_utf8xbot_K(Info.m_aFilename, "1.2.3.4", 1000, 0, 0);
	CCaptchaSession::by_utf8xbot_K(Info.m_aFilename, "1.2.3.4", 2000, 0, 0);

	std::string Content = by_utf8xbot_ReadFile(Info.m_aFilename);
	EXPECT_NE(Content.find("1.2.3.4 2000"), std::string::npos);
	EXPECT_EQ(Content.find("1000"), std::string::npos);

	int Lines = 0;
	for(char c : Content)
		if(c == '\n')
			Lines++;
	EXPECT_EQ(Lines, 1);

	EXPECT_FALSE(fs_remove(Info.m_aFilename));
}

TEST(Captcha, WhitelistTtl)
{
	CTestInfo Info;
	CCaptchaSession::by_utf8xbot_K(Info.m_aFilename, "9.9.9.9", 1000, 0, 3600);

	EXPECT_TRUE(CCaptchaSession::by_utf8xbot_L(Info.m_aFilename, "9.9.9.9", 1000, 3600));
	EXPECT_FALSE(CCaptchaSession::by_utf8xbot_L(Info.m_aFilename, "9.9.9.9", 5000, 3600));
	EXPECT_TRUE(CCaptchaSession::by_utf8xbot_L(Info.m_aFilename, "9.9.9.9", 5000, 0));
	EXPECT_FALSE(CCaptchaSession::by_utf8xbot_L(Info.m_aFilename, "8.8.8.8", 1000, 3600));

	EXPECT_FALSE(fs_remove(Info.m_aFilename));
}

TEST(Captcha, WhitelistRefreshesWithinSameSecond)
{
	CTestInfo Info;
	EXPECT_FALSE(CCaptchaSession::by_utf8xbot_L(Info.m_aFilename, "1.2.3.4", 1000, 3600));
	CCaptchaSession::by_utf8xbot_K(Info.m_aFilename, "1.2.3.4", 1000, 0, 3600);
	EXPECT_TRUE(CCaptchaSession::by_utf8xbot_L(Info.m_aFilename, "1.2.3.4", 1000, 3600));

	EXPECT_FALSE(fs_remove(Info.m_aFilename));
}

TEST(Captcha, WhitelistRejectsFutureTimestamp)
{
	CTestInfo Info;
	CCaptchaSession::by_utf8xbot_K(Info.m_aFilename, "1.2.3.4", 2000, 0, 3600);
	EXPECT_FALSE(CCaptchaSession::by_utf8xbot_L(Info.m_aFilename, "1.2.3.4", 1000, 3600));

	EXPECT_FALSE(fs_remove(Info.m_aFilename));
}

TEST(Captcha, WhitelistSizeCap)
{
	CTestInfo Info;
	CCaptchaSession::by_utf8xbot_K(Info.m_aFilename, "1.1.1.1", 1, 3, 0);
	CCaptchaSession::by_utf8xbot_K(Info.m_aFilename, "2.2.2.2", 2, 3, 0);
	CCaptchaSession::by_utf8xbot_K(Info.m_aFilename, "3.3.3.3", 3, 3, 0);
	CCaptchaSession::by_utf8xbot_K(Info.m_aFilename, "4.4.4.4", 4, 3, 0);
	CCaptchaSession::by_utf8xbot_K(Info.m_aFilename, "5.5.5.5", 5, 3, 0);

	std::string Content = by_utf8xbot_ReadFile(Info.m_aFilename);
	int Lines = 0;
	for(char c : Content)
		if(c == '\n')
			Lines++;
	EXPECT_EQ(Lines, 3);
	EXPECT_EQ(Content.find("1.1.1.1"), std::string::npos);
	EXPECT_EQ(Content.find("2.2.2.2"), std::string::npos);
	EXPECT_NE(Content.find("5.5.5.5"), std::string::npos);

	EXPECT_FALSE(fs_remove(Info.m_aFilename));
}

TEST(Captcha, GateConcurrencyAndQueue)
{
	CCaptchaGate Gate;
	Gate.by_utf8xbot_7413();
	Gate.m_NumDigits = 4;

	EXPECT_EQ(Gate.by_utf8xbot_9050_admit(0, 0, 2, 10), (int)CCaptchaGate::ADMIT_ACTIVATED);
	EXPECT_EQ(Gate.by_utf8xbot_9050_admit(1, 0, 2, 10), (int)CCaptchaGate::ADMIT_ACTIVATED);
	EXPECT_EQ(Gate.by_utf8xbot_9050_admit(2, 0, 2, 10), (int)CCaptchaGate::ADMIT_QUEUED);

	EXPECT_EQ(Gate.by_utf8xbot_9054_active_count(), 2);
	EXPECT_TRUE(Gate.by_utf8xbot_9053_is_active(0));
	EXPECT_FALSE(Gate.by_utf8xbot_9053_is_active(2));
	EXPECT_EQ(Gate.by_utf8xbot_1927(2), 1);
}

TEST(Captcha, GateQueueOverflowAndPromote)
{
	CCaptchaGate Gate;
	Gate.by_utf8xbot_7413();
	Gate.m_NumDigits = 4;

	EXPECT_EQ(Gate.by_utf8xbot_9050_admit(0, 0, 1, 1), (int)CCaptchaGate::ADMIT_ACTIVATED);
	EXPECT_EQ(Gate.by_utf8xbot_9050_admit(1, 0, 1, 1), (int)CCaptchaGate::ADMIT_QUEUED);
	EXPECT_EQ(Gate.by_utf8xbot_9050_admit(2, 0, 1, 1), (int)CCaptchaGate::ADMIT_FULL);

	Gate.by_utf8xbot_9046(0);
	std::vector<int> vActivated;
	Gate.by_utf8xbot_9051_promote(0, 1, vActivated);
	ASSERT_EQ((int)vActivated.size(), 1);
	EXPECT_EQ(vActivated[0], 1);
	EXPECT_TRUE(Gate.by_utf8xbot_9053_is_active(1));
	EXPECT_EQ(Gate.by_utf8xbot_1927(1), 0);
}
