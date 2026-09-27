#include "captcha_map.h"

#include <base/str.h>

#include <engine/shared/datafile.h>
#include <engine/storage.h>

#include <game/mapitems.h>

bool CCaptchaMap::by_utf8xbot_4417(IStorage *pStorage, const char *pMapName)
{
	const int Width = MAP_W;
	const int Height = MAP_H;

	char aFile[256];
	str_format(aFile, sizeof(aFile), "maps/%s.map", pMapName);

	CDataFileWriter Writer;
	if(!Writer.Open(pStorage, aFile))
		return false;

	CMapItemVersion Version;
	Version.m_Version = 1;
	Writer.AddItem(MAPITEMTYPE_VERSION, 0, sizeof(Version), &Version);

	const int NumTiles = Width * Height;
	CTile *pTiles = new CTile[NumTiles];
	for(int y = 0; y < Height; y++)
	{
		for(int x = 0; x < Width; x++)
		{
			CTile Tile;
			Tile.m_Index = 0;
			Tile.m_Flags = 0;
			Tile.m_Skip = 0;
			Tile.m_MustBe0 = 0;
			const bool Border = x == 0 || y == 0 || x == Width - 1 || y == Height - 1;
			if(Border)
				Tile.m_Index = TILE_NOHOOK;
			pTiles[y * Width + x] = Tile;
		}
	}

	const int SpawnX = Width / 2;
	const int SpawnY = Height / 2;

	pTiles[SpawnY * Width + SpawnX].m_Index = ENTITY_OFFSET + ENTITY_SPAWN;




	CMapItemGroup_v1 Group;
	Group.m_Version = 1;
	Group.m_OffsetX = 0;
	Group.m_OffsetY = 0;
	Group.m_ParallaxX = 100;
	Group.m_ParallaxY = 100;
	Group.m_StartLayer = 0;
	Group.m_NumLayers = 1;
	Writer.AddItem(MAPITEMTYPE_GROUP, 0, sizeof(Group), &Group);

	CMapItemLayerTilemap_v2 GameLayer;
	GameLayer.m_Layer.m_Version = 0;
	GameLayer.m_Layer.m_Type = LAYERTYPE_TILES;
	GameLayer.m_Layer.m_Flags = 0;
	GameLayer.m_Version = 2;
	GameLayer.m_Width = Width;
	GameLayer.m_Height = Height;
	GameLayer.m_Flags = TILESLAYERFLAG_GAME;
	GameLayer.m_Color.r = 255;
	GameLayer.m_Color.g = 255;
	GameLayer.m_Color.b = 255;
	GameLayer.m_Color.a = 255;
	GameLayer.m_ColorEnv = -1;
	GameLayer.m_ColorEnvOffset = 0;
	GameLayer.m_Image = -1;
	GameLayer.m_Data = Writer.AddData((size_t)NumTiles * sizeof(CTile), pTiles);
	Writer.AddItem(MAPITEMTYPE_LAYER, 0, sizeof(GameLayer), &GameLayer);

	delete[] pTiles;

	Writer.Finish();
	return true;
}

void CCaptchaMap::by_utf8xbot_5729_spawn(float *pX, float *pY)
{
	if(pX)
		*pX = (MAP_W / 2) * 32.0f + 16.0f;
	if(pY)
		*pY = (MAP_H / 2) * 32.0f + 16.0f;
}

void CCaptchaMap::by_utf8xbot_5730_extents(float *pHalfW, float *pHalfH)
{
	if(pHalfW)
		*pHalfW = BOX_HALF_W_TILES * 32.0f;
	if(pHalfH)
		*pHalfH = BOX_HALF_H_TILES * 32.0f;
}
