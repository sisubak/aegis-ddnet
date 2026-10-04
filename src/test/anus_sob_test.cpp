#include "test.h"

#include <base/str.h>

#include <game/server/components/anus_sob/anus_sob_state.h>

#include <gtest/gtest.h>

#include <cstring>
#include <vector>

TEST(AnusSobState, PackUnpackRoundtrip)
{
	std::vector<SAnusSobClientState> In;

	SAnusSobClientState A;
	std::memset(&A, 0, sizeof(A));
	A.m_ClientId = 3;
	str_copy(A.m_aAddr, "1.2.3.4:8303", sizeof(A.m_aAddr));
	str_copy(A.m_aName, "tester", sizeof(A.m_aName));
	A.m_Alive = 1;
	A.m_PosX = 123.5f;
	A.m_PosY = -42.25f;
	A.m_ActiveWeapon = 2;
	A.m_aAmmo[2] = 7;
	In.push_back(A);

	SAnusSobClientState B;
	std::memset(&B, 0, sizeof(B));
	B.m_ClientId = 10;
	str_copy(B.m_aAddr, "5.6.7.8:9999", sizeof(B.m_aAddr));
	In.push_back(B);

	std::vector<unsigned char> Buf;
	CAnusSobStateCodec::Pack(In, Buf);

	std::vector<SAnusSobClientState> Out;
	ASSERT_TRUE(CAnusSobStateCodec::Unpack(Buf, Out));
	ASSERT_EQ(Out.size(), In.size());

	EXPECT_EQ(Out[0].m_ClientId, 3);
	EXPECT_STREQ(Out[0].m_aAddr, "1.2.3.4:8303");
	EXPECT_STREQ(Out[0].m_aName, "tester");
	EXPECT_EQ(Out[0].m_Alive, 1);
	EXPECT_FLOAT_EQ(Out[0].m_PosX, 123.5f);
	EXPECT_FLOAT_EQ(Out[0].m_PosY, -42.25f);
	EXPECT_EQ(Out[0].m_ActiveWeapon, 2);
	EXPECT_EQ(Out[0].m_aAmmo[2], 7);
	EXPECT_EQ(Out[1].m_ClientId, 10);
	EXPECT_STREQ(Out[1].m_aAddr, "5.6.7.8:9999");
}

TEST(AnusSobState, EmptyRoundtrip)
{
	std::vector<SAnusSobClientState> In;
	std::vector<unsigned char> Buf;
	CAnusSobStateCodec::Pack(In, Buf);

	std::vector<SAnusSobClientState> Out;
	ASSERT_TRUE(CAnusSobStateCodec::Unpack(Buf, Out));
	EXPECT_TRUE(Out.empty());
}

TEST(AnusSobState, RejectsCorruptMagic)
{
	std::vector<unsigned char> Buf(16, 0);
	std::vector<SAnusSobClientState> Out;
	EXPECT_FALSE(CAnusSobStateCodec::Unpack(Buf, Out));

	std::vector<unsigned char> Empty;
	EXPECT_FALSE(CAnusSobStateCodec::Unpack(Empty, Out));
}

TEST(AnusSobState, RejectsTrailingData)
{
	std::vector<SAnusSobClientState> In;
	std::vector<unsigned char> Buf;
	CAnusSobStateCodec::Pack(In, Buf);
	Buf.push_back(0);

	std::vector<SAnusSobClientState> Out;
	EXPECT_FALSE(CAnusSobStateCodec::Unpack(Buf, Out));
}
