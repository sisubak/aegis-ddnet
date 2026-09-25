# Аудит разбора датафайла карты (mister/ \map)

Область: `src/engine/shared/datafile.cpp`, `datafile.h`, `map.cpp`.
Вектор: злонамеренно сформированный .map передаётся в `CDataFileReader::Open` / `CMap::Load`.

Замечание: этот форк основан на уже сильно захардненной ветке upstream DDNet — есть
полноценная `CDatafile::Validate()`, проверки заголовка, лимит `MaxAllocSize` и int64
арифметика при расчёте размеров. Классические переполнения (num items/size, отрицательные
размеры, integer overflow при аллокации структуры) уже закрыты. Ниже — реальные остаточные
проблемы и точечный хардненинг.

---

### datafile.cpp:264-265 (CDatafile::GetData, ветка v4)
Проблема: `uncompress` без верхнего лимита распакованного размера — decompression bomb / DoS.
`OriginalUncompressedSize` берётся напрямую из файла (`m_Info.m_pDataSizes[Index]`) и в
`Validate()` проверяется только на `>= 0` (строки 486-497). Сжатый блок `DataSize` ограничен
размером файла, но заявленный распакованный размер атакующий может выставить вплоть до
`INT_MAX` (~2 ГиБ). Крошечная карта (сотня байт) может заставить сделать `malloc(~2 ГБ)`
на строке 256 и распаковать в него — исчерпание памяти / краш сервера. Общий `MaxAllocSize`
(строка 656) покрывает только служебную структуру, но НЕ пер-дата распаковку.

Фикс: добавить абсолютный лимит и лимит коэффициента сжатия сразу после проверки
`OriginalUncompressedSize == 0`.

old:
```cpp
			const unsigned OriginalUncompressedSize = m_Info.m_pDataSizes[Index];
			log_trace("datafile", "loading data. index=%d size=%d uncompressed=%d", Index, DataSize, OriginalUncompressedSize);
			if(OriginalUncompressedSize == 0)
			{
				log_error("datafile", "data size invalid. data will be ignored. index=%d size=%d uncompressed=%d", Index, DataSize, OriginalUncompressedSize);
				m_ppDataPtrs[Index] = nullptr;
				m_pDataSizes[Index] = -1;
				return nullptr;
			}
```
new:
```cpp
			const unsigned OriginalUncompressedSize = m_Info.m_pDataSizes[Index];
			log_trace("datafile", "loading data. index=%d size=%d uncompressed=%d", Index, DataSize, OriginalUncompressedSize);
			if(OriginalUncompressedSize == 0)
			{
				log_error("datafile", "data size invalid. data will be ignored. index=%d size=%d uncompressed=%d", Index, DataSize, OriginalUncompressedSize);
				m_ppDataPtrs[Index] = nullptr;
				m_pDataSizes[Index] = -1;
				return nullptr;
			}

			// Guard against decompression bombs: cap the absolute uncompressed
			// size and the compression ratio. The declared uncompressed size is
			// fully attacker-controlled and only validated to be >= 0.
			static constexpr unsigned MAX_UNCOMPRESSED_DATA_SIZE = 512u * 1024 * 1024; // 512 MiB
			static constexpr unsigned MAX_COMPRESSION_RATIO = 1024u;
			if(OriginalUncompressedSize > MAX_UNCOMPRESSED_DATA_SIZE ||
				(DataSize > 0 && OriginalUncompressedSize / DataSize > MAX_COMPRESSION_RATIO))
			{
				log_error("datafile", "uncompressed data too large (possible decompression bomb). index=%d compressed=%d uncompressed=%u", Index, DataSize, OriginalUncompressedSize);
				m_ppDataPtrs[Index] = nullptr;
				m_pDataSizes[Index] = -1;
				return nullptr;
			}
```

---

### datafile.cpp:710 (CDataFileReader::Open)
Проблема: `dbg_assert` на данных, производных из файла. `DataSwapLen` вычисляется из
`m_Header.m_Swaplen` (значение из заголовка карты). Хотя выше (строки 641-654) swaplen
сверяется с реальным размером файла, здесь на непроверяемо-выведенной величине стоит
`dbg_assert`. В сборках с активными ассертами это `abort()` (DoS) на битой карте вместо
корректного возврата ошибки; в релизе без ассертов — фактически отсутствие проверки перед
`SwapEndianInPlace` над всем буфером. Ассерт на данных файла — плохая практика: любой промах
логики валидации превращается в краш.

Фикс: заменить ассерт на аккуратный возврат ошибки с очисткой ресурсов.

old:
```cpp
	// The swap len also includes the size of the header (without the size offset), but the header was already swapped above.
	const int64_t DataSwapLen = pTmpDataFile->m_Header.m_Swaplen - (int)(sizeof(Header) - Header.SizeOffset());
	dbg_assert(DataSwapLen == Size, "Swap len and file size mismatch");
	SwapEndianInPlace(pTmpDataFile->m_pData, DataSwapLen);
```
new:
```cpp
	// The swap len also includes the size of the header (without the size offset), but the header was already swapped above.
	const int64_t DataSwapLen = pTmpDataFile->m_Header.m_Swaplen - (int)(sizeof(Header) - Header.SizeOffset());
	if(DataSwapLen != Size)
	{
		io_close(pTmpDataFile->m_File);
		free(pTmpDataFile);
		log_error("datafile", "swap len and file size mismatch. swaplen=%" PRId64 " size=%" PRId64, DataSwapLen, Size);
		return false;
	}
	SwapEndianInPlace(pTmpDataFile->m_pData, DataSwapLen);
```

---

### datafile.cpp:305 (CDatafile::GetData, SwapEndian после загрузки)
Проблема (низкая, только big-endian): `SwapEndianInPlace(m_ppDataPtrs[Index], m_pDataSizes[Index])`
принимает `size_t Size`, а `m_pDataSizes[Index]` — `int`. На данном этапе значение неотрицательно
(равно `OriginalUncompressedSize` или `DataSize`), поэтому реального переполнения нет, но неявный
`int -> size_t` в паре с DataProcessor, который позже может выставить `NewSize`, лучше сделать явным.
На little-endian (Windows x64 владельца) `SwapEndianInPlace` — no-op, практического влияния нет.

Фикс: явное приведение и защита от отрицательного значения (defensive).

old:
```cpp
		if(Swap)
		{
			SwapEndianInPlace(m_ppDataPtrs[Index], m_pDataSizes[Index]);
		}
```
new:
```cpp
		if(Swap && m_pDataSizes[Index] > 0)
		{
			SwapEndianInPlace(m_ppDataPtrs[Index], (size_t)m_pDataSizes[Index]);
		}
```

---

### map.cpp:650-681 (ValidateAndUnpackTilesLayerData, DataProcessor v>=4)
Проблема (пограничная, подтверждённо безопасна, оставлено как заметка): при `m_Version >= 4`
выделяется `malloc(TilemapSize)` и распаковка через `ExtractTiles`. `TilemapSize` защищён от
integer overflow проверкой на строках 630-637, а `m_Width/m_Height` валидируются в
`EnsureTileLayerProperties` (>= 2) ещё в `UpgradeAndValidateTilesLayerItem`, который гарантированно
вызывается для каждого тайл-слоя раньше (первый цикл `Load`, строка 152). `ExtractTiles` ограничивает
запись `DestSize` и чтение `SrcSize`. Переполнения нет — фикс не требуется.

Дополнительный хардненинг (опционально): `TilemapSize` для распаковки тоже стоит ограничить общим
потолком, чтобы одна карта не могла запросить много крупных `malloc` подряд. Это тот же класс
проблемы, что и фикс #1, но уже на уровне тайлов.

old:
```cpp
			CTile *pTiles = static_cast<CTile *>(malloc(TilemapSize));
			if(pTiles == nullptr)
```
new:
```cpp
			static constexpr size_t MAX_TILEMAP_SIZE = 512u * 1024 * 1024; // 512 MiB
			if(TilemapSize > MAX_TILEMAP_SIZE)
			{
				log_error("map/load", "Tile layer %d in group %d unpacked size too large (%" PRIzu ").",
					LayerIndex, GroupIndex, TilemapSize);
				free(pData);
				return std::make_pair(nullptr, 0);
			}
			CTile *pTiles = static_cast<CTile *>(malloc(TilemapSize));
			if(pTiles == nullptr)
```

---

## Проверено и признано безопасным (фикс не нужен)
- Заголовок: `m_NumItemTypes/m_NumItems/m_NumRawData/m_ItemSize/m_DataSize` проверяются на
  отрицательность и границы (datafile.cpp:584-596), выравнивание `m_ItemSize` по int.
- Аллокация служебной структуры: считается в int64 и сверяется с `MaxAllocSize` 2 ГБ
  (datafile.cpp:598-668) — нет integer overflow при аллокации.
- Соответствие суммарных размеров файлу: строки 616-654 (Size + DataSize + header == FileSize,
  m_Size и m_Swaplen сверяются, с аккуратным SizeFix для старых v4).
- `Validate()` (datafile.cpp:388-501): типы, дубли типов/ID, монотонность и границы item/data
  offsets, `FileItemSize >= sizeof(CDatafileItem)` (нет отрицательных размеров item),
  выравнивание, суммарный item size == header, data size >= 0.
- Границы data-индексов в слоях (map.cpp:617-621), уникальность data-индексов (623-628),
  integer overflow при `Width*Height*TileSize` (map.cpp:630-637), проверка что физический слой
  не меньше game-слоя (642-648).
- Все `GetItem/GetData` ограничивают `Index` по `m_NumItems/m_NumRawData` перед доступом.

## Приоритет фиксов
1. datafile.cpp:264 — decompression bomb (реальный DoS, высокий приоритет).
2. datafile.cpp:710 — ассерт на данных файла (DoS/краш, средний).
3. map.cpp:664 — лимит распакованного тайлмапа (хардненинг, средний).
4. datafile.cpp:305 — явное приведение (низкий, только BE).

— mister/ \map
