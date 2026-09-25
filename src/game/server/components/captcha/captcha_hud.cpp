#include "captcha_hud.h"

const char *CCaptchaHud::by_utf8xbot_9157()
{
	return "Введи цифры из лазеров в чат что бы пройти капчу";
}

int CCaptchaHud::by_utf8xbot_6023(const char *pStr, int NumCp, char *pOut, int OutSize)
{
	if(!pOut || OutSize <= 0)
		return 0;
	pOut[0] = 0;
	if(!pStr || NumCp <= 0)
		return 0;

	int SrcPos = 0;
	int DstPos = 0;
	int Cp = 0;

	while(pStr[SrcPos] != 0 && Cp < NumCp)
	{
		int CharLen = 1;
		unsigned char Lead = (unsigned char)pStr[SrcPos];
		if((Lead & 0x80) == 0x00)
			CharLen = 1;
		else if((Lead & 0xE0) == 0xC0)
			CharLen = 2;
		else if((Lead & 0xF0) == 0xE0)
			CharLen = 3;
		else if((Lead & 0xF8) == 0xF0)
			CharLen = 4;
		else
			CharLen = 1;

		int Valid = 1;
		for(int i = 1; i < CharLen; i++)
		{
			unsigned char Cont = (unsigned char)pStr[SrcPos + i];
			if(Cont == 0 || (Cont & 0xC0) != 0x80)
			{
				Valid = 0;
				break;
			}
		}
		if(!Valid)
			CharLen = 1;

		if(DstPos + CharLen >= OutSize)
			break;

		for(int i = 0; i < CharLen; i++)
			pOut[DstPos + i] = pStr[SrcPos + i];

		SrcPos += CharLen;
		DstPos += CharLen;
		Cp++;
	}

	pOut[DstPos] = 0;
	return Cp;
}

bool CCaptchaHud::by_utf8xbot_7412(int64_t StartTime, int64_t Now, int64_t Freq, char *pOut, int OutSize)
{
	if(!pOut || OutSize <= 0)
		return false;
	pOut[0] = 0;

	const char *pPhrase = by_utf8xbot_9157();

	int TotalCp = 0;
	{
		int SrcPos = 0;
		while(pPhrase[SrcPos] != 0)
		{
			unsigned char Lead = (unsigned char)pPhrase[SrcPos];
			int CharLen = 1;
			if((Lead & 0x80) == 0x00)
				CharLen = 1;
			else if((Lead & 0xE0) == 0xC0)
				CharLen = 2;
			else if((Lead & 0xF0) == 0xE0)
				CharLen = 3;
			else if((Lead & 0xF8) == 0xF0)
				CharLen = 4;
			else
				CharLen = 1;

			int Valid = 1;
			for(int i = 1; i < CharLen; i++)
			{
				unsigned char Cont = (unsigned char)pPhrase[SrcPos + i];
				if(Cont == 0 || (Cont & 0xC0) != 0x80)
				{
					Valid = 0;
					break;
				}
			}
			if(!Valid)
				CharLen = 1;

			SrcPos += CharLen;
			TotalCp++;
		}
	}

	int ShownCp = TotalCp;
	if(Freq > 0)
	{
		int64_t ElapsedMs = ((Now - StartTime) * (int64_t)1000) / Freq;
		if(ElapsedMs < 0)
			ElapsedMs = 0;
		int64_t Shown = ElapsedMs / 50;
		if(Shown < 0)
			Shown = 0;
		if(Shown > (int64_t)TotalCp)
			Shown = TotalCp;
		ShownCp = (int)Shown;
	}

	by_utf8xbot_6023(pPhrase, ShownCp, pOut, OutSize);

	return ShownCp >= TotalCp;
}

void CCaptchaHud::by_utf8xbot_7834(int Position, char *pOut, int OutSize)
{
	if(!pOut || OutSize <= 0)
		return;
	if(Position < 0)
		Position = 0;
	snprintf(pOut, (size_t)OutSize, "Ты в очереди. Позиция: %d", Position);
	pOut[OutSize - 1] = 0;
}
