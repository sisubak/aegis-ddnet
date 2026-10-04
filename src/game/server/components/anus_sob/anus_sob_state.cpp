#include "anus_sob_state.h"

#include <cstring>

static void by_utf8xbot_2210_put_u32(std::vector<unsigned char> &Buf, unsigned V)
{
	Buf.push_back((unsigned char)(V & 0xff));
	Buf.push_back((unsigned char)((V >> 8) & 0xff));
	Buf.push_back((unsigned char)((V >> 16) & 0xff));
	Buf.push_back((unsigned char)((V >> 24) & 0xff));
}

static void by_utf8xbot_2211_put_i32(std::vector<unsigned char> &Buf, int V)
{
	by_utf8xbot_2210_put_u32(Buf, (unsigned)V);
}

static void by_utf8xbot_2212_put_f32(std::vector<unsigned char> &Buf, float V)
{
	unsigned U;
	std::memcpy(&U, &V, sizeof(U));
	by_utf8xbot_2210_put_u32(Buf, U);
}

static void by_utf8xbot_2213_put_bytes(std::vector<unsigned char> &Buf, const void *p, unsigned N)
{
	const unsigned char *pB = (const unsigned char *)p;
	Buf.insert(Buf.end(), pB, pB + N);
}

static bool by_utf8xbot_2220_get_u32(const std::vector<unsigned char> &In, unsigned &Off, unsigned &V)
{
	if(Off + 4 > In.size())
		return false;
	V = (unsigned)In[Off] | ((unsigned)In[Off + 1] << 8) | ((unsigned)In[Off + 2] << 16) | ((unsigned)In[Off + 3] << 24);
	Off += 4;
	return true;
}

static bool by_utf8xbot_2221_get_i32(const std::vector<unsigned char> &In, unsigned &Off, int &V)
{
	unsigned U;
	if(!by_utf8xbot_2220_get_u32(In, Off, U))
		return false;
	V = (int)U;
	return true;
}

static bool by_utf8xbot_2222_get_f32(const std::vector<unsigned char> &In, unsigned &Off, float &V)
{
	unsigned U;
	if(!by_utf8xbot_2220_get_u32(In, Off, U))
		return false;
	std::memcpy(&V, &U, sizeof(V));
	return true;
}

static bool by_utf8xbot_2223_get_bytes(const std::vector<unsigned char> &In, unsigned &Off, void *p, unsigned N)
{
	if(Off + N > In.size())
		return false;
	std::memcpy(p, In.data() + Off, N);
	Off += N;
	return true;
}

void CAnusSobStateCodec::Pack(const std::vector<SAnusSobClientState> &In, std::vector<unsigned char> &Out)
{
	Out.clear();
	by_utf8xbot_2210_put_u32(Out, MAGIC);
	by_utf8xbot_2210_put_u32(Out, VERSION);
	by_utf8xbot_2211_put_i32(Out, (int)In.size());
	for(unsigned i = 0; i < In.size(); ++i)
	{
		const SAnusSobClientState &S = In[i];
		by_utf8xbot_2211_put_i32(Out, S.m_ClientId);
		by_utf8xbot_2213_put_bytes(Out, S.m_aAddr, sizeof(S.m_aAddr));
		by_utf8xbot_2213_put_bytes(Out, S.m_aName, sizeof(S.m_aName));
		by_utf8xbot_2213_put_bytes(Out, S.m_aClan, sizeof(S.m_aClan));
		by_utf8xbot_2211_put_i32(Out, S.m_Country);
		by_utf8xbot_2211_put_i32(Out, S.m_Team);
		by_utf8xbot_2211_put_i32(Out, S.m_Alive);
		by_utf8xbot_2212_put_f32(Out, S.m_PosX);
		by_utf8xbot_2212_put_f32(Out, S.m_PosY);
		by_utf8xbot_2212_put_f32(Out, S.m_VelX);
		by_utf8xbot_2212_put_f32(Out, S.m_VelY);
		by_utf8xbot_2211_put_i32(Out, S.m_Direction);
		by_utf8xbot_2211_put_i32(Out, S.m_Jumped);
		by_utf8xbot_2211_put_i32(Out, S.m_HookState);
		by_utf8xbot_2211_put_i32(Out, S.m_HookedPlayer);
		by_utf8xbot_2212_put_f32(Out, S.m_HookX);
		by_utf8xbot_2212_put_f32(Out, S.m_HookY);
		by_utf8xbot_2211_put_i32(Out, S.m_FreezeEndTick);
		by_utf8xbot_2211_put_i32(Out, S.m_DeepFrozen);
		by_utf8xbot_2211_put_i32(Out, S.m_LiveFrozen);
		by_utf8xbot_2211_put_i32(Out, S.m_Solo);
		by_utf8xbot_2211_put_i32(Out, S.m_Super);
		by_utf8xbot_2211_put_i32(Out, S.m_Endless);
		by_utf8xbot_2211_put_i32(Out, S.m_Jetpack);
		by_utf8xbot_2211_put_i32(Out, S.m_ActiveWeapon);
		by_utf8xbot_2211_put_i32(Out, S.m_LastWeapon);
		for(int w = 0; w < 6; ++w)
			by_utf8xbot_2211_put_i32(Out, S.m_aGotWeapon[w]);
		for(int w = 0; w < 6; ++w)
			by_utf8xbot_2211_put_i32(Out, S.m_aAmmo[w]);
		by_utf8xbot_2213_put_bytes(Out, S.m_aSaveCode, sizeof(S.m_aSaveCode));
	}
}

bool CAnusSobStateCodec::Unpack(const std::vector<unsigned char> &In, std::vector<SAnusSobClientState> &Out)
{
	Out.clear();
	unsigned Off = 0;
	unsigned Magic, Version;
	int Count;
	if(!by_utf8xbot_2220_get_u32(In, Off, Magic))
		return false;
	if(Magic != MAGIC)
		return false;
	if(!by_utf8xbot_2220_get_u32(In, Off, Version))
		return false;
	if(Version != VERSION)
		return false;
	if(!by_utf8xbot_2221_get_i32(In, Off, Count))
		return false;
	if(Count < 0 || Count > 128)
		return false;
	Out.resize((unsigned)Count);
	for(int i = 0; i < Count; ++i)
	{
		SAnusSobClientState &S = Out[i];
		if(!by_utf8xbot_2221_get_i32(In, Off, S.m_ClientId))
			return false;
		if(!by_utf8xbot_2223_get_bytes(In, Off, S.m_aAddr, sizeof(S.m_aAddr)))
			return false;
		if(!by_utf8xbot_2223_get_bytes(In, Off, S.m_aName, sizeof(S.m_aName)))
			return false;
		if(!by_utf8xbot_2223_get_bytes(In, Off, S.m_aClan, sizeof(S.m_aClan)))
			return false;
		if(!by_utf8xbot_2221_get_i32(In, Off, S.m_Country))
			return false;
		if(!by_utf8xbot_2221_get_i32(In, Off, S.m_Team))
			return false;
		if(!by_utf8xbot_2221_get_i32(In, Off, S.m_Alive))
			return false;
		if(!by_utf8xbot_2222_get_f32(In, Off, S.m_PosX))
			return false;
		if(!by_utf8xbot_2222_get_f32(In, Off, S.m_PosY))
			return false;
		if(!by_utf8xbot_2222_get_f32(In, Off, S.m_VelX))
			return false;
		if(!by_utf8xbot_2222_get_f32(In, Off, S.m_VelY))
			return false;
		if(!by_utf8xbot_2221_get_i32(In, Off, S.m_Direction))
			return false;
		if(!by_utf8xbot_2221_get_i32(In, Off, S.m_Jumped))
			return false;
		if(!by_utf8xbot_2221_get_i32(In, Off, S.m_HookState))
			return false;
		if(!by_utf8xbot_2221_get_i32(In, Off, S.m_HookedPlayer))
			return false;
		if(!by_utf8xbot_2222_get_f32(In, Off, S.m_HookX))
			return false;
		if(!by_utf8xbot_2222_get_f32(In, Off, S.m_HookY))
			return false;
		if(!by_utf8xbot_2221_get_i32(In, Off, S.m_FreezeEndTick))
			return false;
		if(!by_utf8xbot_2221_get_i32(In, Off, S.m_DeepFrozen))
			return false;
		if(!by_utf8xbot_2221_get_i32(In, Off, S.m_LiveFrozen))
			return false;
		if(!by_utf8xbot_2221_get_i32(In, Off, S.m_Solo))
			return false;
		if(!by_utf8xbot_2221_get_i32(In, Off, S.m_Super))
			return false;
		if(!by_utf8xbot_2221_get_i32(In, Off, S.m_Endless))
			return false;
		if(!by_utf8xbot_2221_get_i32(In, Off, S.m_Jetpack))
			return false;
		if(!by_utf8xbot_2221_get_i32(In, Off, S.m_ActiveWeapon))
			return false;
		if(!by_utf8xbot_2221_get_i32(In, Off, S.m_LastWeapon))
			return false;
		for(int w = 0; w < 6; ++w)
			if(!by_utf8xbot_2221_get_i32(In, Off, S.m_aGotWeapon[w]))
				return false;
		for(int w = 0; w < 6; ++w)
			if(!by_utf8xbot_2221_get_i32(In, Off, S.m_aAmmo[w]))
				return false;
		if(!by_utf8xbot_2223_get_bytes(In, Off, S.m_aSaveCode, sizeof(S.m_aSaveCode)))
			return false;
		S.m_aAddr[sizeof(S.m_aAddr) - 1] = '\0';
		S.m_aName[sizeof(S.m_aName) - 1] = '\0';
		S.m_aClan[sizeof(S.m_aClan) - 1] = '\0';
		S.m_aSaveCode[sizeof(S.m_aSaveCode) - 1] = '\0';
	}
	return Off == In.size();
}
