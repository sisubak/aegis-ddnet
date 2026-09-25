# Аудит packer / compression / huffman / snapshot / protocol_ex

Аудитор: mister/ \packer. Форк: C:\Users\WWWWWUeHaA\Desktop\antiddos
Цель: баги, эксплуатируемые от недоверенного клиента (OOB чтение/запись, отсутствие
проверок размеров, отрицательные размеры, целочисленные переполнения, dbg_assert на
клиентских данных).

Важно про `dbg_assert`: в этом форке (src/base/dbg.h:27-34, 59-60) `dbg_assert_imp`
помечен `[[noreturn]]` и в комментарии сказано "Also works in release mode". Значит любое
срабатывание assert на данных, которые может подсунуть клиент, = гарантированный краш
(abort) сервера, а не просто debug-проверка. Поэтому все assert'ы ниже рассматриваются как
потенциальный DoS, если аргумент управляется клиентом.

---

### packer.cpp:78 — CAbstractPacker::AddString (запись за границей буфера)

Проблема:
Завершающий нуль пишется без проверки границы. Если буфер уже заполнен ровно до конца
(`m_pCurrent == m_pEnd`, что законно достижимо через `AddRaw`, у которого проверка
`m_pCurrent + Size > m_pEnd` пропускает случай точного совпадения), а затем вызывается
`AddString("")` (или строка обрезается по `break` при `AllowTruncation`), цикл `while(*pStr)`
не выполняется, и строка сразу доходит до `*m_pCurrent++ = '\0';`. Это запись 1 байта за
концом буфера (`CPacker::m_aBuffer[PACKER_BUFFER_SIZE]`). Внутрицикловый guard
`m_pCurrent + Length + 1 > m_pEnd` защищает только путь с непустой строкой, но не пустую
строку / вход в функцию на полном буфере. m_Error при этом не выставляется.

Фикс:
```cpp
// old
	*m_pCurrent++ = '\0';
}
```
```cpp
// new
	if(m_pCurrent >= m_pEnd)
	{
		m_Error = true;
		m_pCurrent = pPrevCurrent;
		return;
	}
	*m_pCurrent++ = '\0';
}
```

---

### huffman.cpp:251 — CHuffman::Decompress (знаковый сдвиг в signed int, UB)

Проблема:
`Bits |= (*pSrc++) << Bitcount;` — `*pSrc` (unsigned char) промотится в `int`, а `Bitcount`
на этой ветке доходит до 24 (цикл `while(Bitcount < 24 ...)`). При байте >= 0x80 и сдвиге
на 24 результат `0x80..0xFF << 24` выходит за пределы `INT_MAX` — знаковое переполнение,
это undefined behaviour (значение `InputSize`/содержимое буфера полностью управляются
клиентом — Decompress вызывается на входящих сетевых пакетах). На практике на x86 обычно
"работает", но формально UB и зависит от флагов оптимизации/UBSan (может привести к abort
при сборке с -fsanitize или неожиданному значению).

Фикс:
```cpp
// old
			Bits |= (*pSrc++) << Bitcount;
```
```cpp
// new
			Bits |= (unsigned)(*pSrc++) << Bitcount;
```

---

### compression.cpp:39 — CVariableInt::Decompress (dbg_assert на размере)

Проблема:
`dbg_assert(DstSize % sizeof(int) == 0, "invalid bounds");` абортит в release. `SrcSize`
(размер сжатых данных от клиента) через проверки в `Unpack`/цикл защищён, но здесь важно,
что assert стоит на `DstSize`. Если где-то в вызывающем коде `DstSize` вычисляется из
клиентского размера и не выравнен на 4 — это краш. Сам по себе файл не даёт клиенту прямого
контроля над `DstSize` (обычно это фиксированный буфер), поэтому помечаю как условно-опасное:
проверить в вызывающих (net/snap-код), что `DstSize` всегда trusted-константа. Функция
`Unpack` (compression.cpp:9-35) корректно возвращает nullptr при нехватке байт и не читает
за границей — здесь OOB нет.

Фикс (defensive, чтобы не абортить, а возвращать ошибку):
```cpp
// old
	dbg_assert(DstSize % sizeof(int) == 0, "invalid bounds");
```
```cpp
// new
	if(DstSize % sizeof(int) != 0)
		return -1;
```

---

## Проверено и признано безопасным (OOB/переполнений не найдено)

- packer.cpp CUnpacker::GetInt / GetUncompressedInt / GetString / GetRaw — все проверяют
  `m_pCurrent >= m_pEnd`, `m_pCurrent + Size > m_pEnd`, `Size < 0`; GetString ищет нуль строго
  в пределах буфера и валидирует UTF-8. Чтения за границей нет.
- compression.cpp CVariableInt::Unpack — корректный контроль `SrcSize` на каждом байте,
  возвращает nullptr при нехватке. Сдвиги ограничены таблицами масок/сдвигов (макс 27 бит,
  маска последнего байта 0x0F) — переполнения int в результат нет.
- huffman.cpp: обход дерева (строки 284-300) всегда останавливается на листе (m_NumBits>0),
  внутренние узлы фикс-дерева имеют оба валидных потомка, индекс m_aNodes не выходит за
  HUFFMAN_MAX_NODES; клиент влияет только на биты, не на структуру дерева. Записи в pDst
  проверены (`pDst == pDstEnd`).
- snapshot.cpp CSnapshot::IsValid (140-177) — полная валидация: total size, m_NumItems в
  [0..MAX_ITEMS], m_DataSize>=0, ActualSize==TotalSize(), каждый offset в [0..m_DataSize] и
  кратен 4, каждый ItemSize>=0 и кратен 4. Отрицательные/переполненные размеры отсекаются.
- snapshot.cpp CSnapshotDelta::UnpackDelta (517-631) — проверяет m_NumDeletedItems>=0 и что
  он влезает (530-531), `pData + 2 > pEnd` (562), Type в [0..MAX_TYPE] (566), Id в [0..MAX_ID]
  (570), размер элемента `*pData < 0 || > INT_MAX/4` (580) — исключает переполнение при `*4`,
  затем `ItemSize < 0 || остаток < ItemSize` (585), сверка размеров при апдейте/существующем
  элементе (595, 612). OOB не найдено. CSnapshotBuilder::NewItemRaw проверяет ёмкость перед
  записью (907-917).
- protocol_ex.cpp UnpackMessageId — проверяет `pUnpacker->Error()`, диапазон `*pId`
  (`< 0 || >= OFFSET_UUID`), UUID_INVALID/UNKNOWN; все чтения через защищённый CUnpacker.

## Замечание про dbg_assert на клиентских путях
Прямых assert'ов, аргумент которых напрямую = размер из сети, в этих 6 файлах не найдено:
все клиентские размеры проходят через `if(...) return -1/-2xx` (snapshot/compression),
а не через dbg_assert. Assert'ы в CSnapshotBuilder (NewItemRaw, Finish) и CSnapshotStorage::Add
срабатывают на данных, которые сервер строит сам из уже провалидированного (IsValid/UnpackDelta)
ввода, поэтому не считаются напрямую эксплуатируемыми — при условии, что вызывающий код всегда
делает IsValid перед доверием снапшоту. Рекомендуется отдельно проверить net-слой (вне этих
файлов), что IsValid вызывается до любого использования принятого от клиента снапшота.
