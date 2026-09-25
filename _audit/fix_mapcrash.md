# Аудит: map.cpp — потенциальный краш при загрузке карты (ExtractTiles / DataProcessor)

Автор: mister/ \mapcrash
Файл: `src/engine/shared/map.cpp`
Смежный: `src/engine/shared/datafile.cpp`

## Итог: НЕ ТРЕБУЕТ ПРАВКИ — уже защищено

## Что проверялось

### 1. ExtractTiles (map.cpp:278-310+)
Функция безопасна:
- Границы обхода жёстко ограничены: `while(DestIndex < DestSize && SrcIndex < SrcSize)` (строка 282) — нет выхода за пределы ни src, ни dest.
- Внутренний цикл повтора тоже гейтится по `DestIndex < DestSize` (строка 290) — переполнение dest невозможно даже при большом `m_Skip`.
- Валидация паддинга `m_MustBe0` (строки 284-289) — отказ при некорректных данных.
- Проверка усечения `DestIndex != DestSize` (строки 300-305).
- Проверка избытка `SrcIndex != SrcSize` (строки 306-310).

### 2. Вызов ExtractTiles (map.cpp:630-689)
Перед разыменованием указателей всё проверено:
- Integer-overflow чек размеров тайлмапа: строки 633-638.
- Ограничение `MAX_TILEMAP_SIZE` (512 MiB): строки 664-671, при превышении — `free(pData)` + возврат `(nullptr, 0)`.
- Проверка `malloc` -> `pTiles == nullptr`: строки 672-679, при неудаче — `free(pData)` + возврат `(nullptr, 0)`.
- Вызов `ExtractTiles(pTiles, ..., static_cast<const CTile*>(pData), SavedTilesSize)` на строке 680 происходит только после успешного malloc; при ошибке extract — `free(pTiles)` + `free(pData)` (строки 681-687).
- Ветки для версии <4 (строки 691-732) также проверяют усечение (`SavedTilesSize < TilemapCount`) и валидируют skip/padding перед доступом к тайлам.

### 3. Источник pData — datafile.cpp GetDataImpl (datafile.cpp:255-336)
`pData`, приходящий в DataProcessor, гарантированно не nullptr:
- Сжатый путь: malloc-чек строки 271-278, ошибка uncompress строки 282-289 — во всех случаях ранний возврат `nullptr`.
- Несжатый путь: malloc-чек строки 295-300, truncation-чек строки 307-314.
- Вызов процессора строки 323-334: `m_DataProcessor(m_ppDataPtrs[Index], ...)` достигается только после успешных выделений/чтений, т.е. `m_ppDataPtrs[Index] != nullptr`.
- Результат процессора проверяется: `pNewData == nullptr` (строки 326-331) -> корректный возврат nullptr вызывающему, без разыменования.

## Вывод
Разыменований указателя без предварительной nullptr/границей-проверки не найдено. Цепочка
`GetDataImpl -> DataProcessor(pData) -> ExtractTiles(pTiles, pData)` защищена на всех звеньях.
Правки не вносились.

Строки-ключи: map.cpp 282, 290, 300-310, 633-638, 664-671, 672-679, 680-687, 691-697; datafile.cpp 271-278, 295-300, 307-314, 323-334.
