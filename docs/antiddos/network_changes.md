# Анти-DDoS правки в сетевом слое (network.cpp)

Документ сопоставляет предложения из аудита `_audit/netconn.md` с реальным состоянием
файла `src/engine/shared/network.cpp`. Для каждого пункта указано: что было уязвимо,
что именно изменено (с номерами строк и фрагментами до/после) и что осталось нерешённым.

Файл на момент документирования: `src/engine/shared/network.cpp`, 685 строк.

---

## 1. Проверка 3-го байта заголовка vital-чанка (CPacketChunkUnpacker::UnpackNextChunk)

Статус: ВНЕСЕНО.

Что было уязвимо:
Перед распаковкой заголовка чанка проверялось только наличие двух байт
(`m_CurrentOffset + 2 > m_DataSize`). Однако для VITAL-чанка `CNetChunkHeader::Unpack`
читает третий байт (`pData[2]`) для сборки поля последовательности. Если vital-чанк
находится в самом конце пакета, чтение третьего байта выходило за пределы валидных
данных (в пределах буфера `m_aChunkData`, но за фактическим размером) и в
`m_Sequence` попадал мусор. Существующая проверка `pData + Header.m_Size > pEnd`
выполнялась уже ПОСЛЕ чтения третьего байта.

Что изменено (network.cpp, строки 81-88):

До (по аудиту):
```
// the chunk header is two bytes, three for vital chunks
if(m_CurrentOffset + 2 > m_Data.m_DataSize)
{
	m_Valid = false;
	return false;
}

// unpack the header
const int HeaderSplit = m_pConnection->m_Sixup ? 6 : 4;
```

После (реально в коде, строки 81-93):
```
// vital chunks (flag in the top bits of the first header byte) use a
// three-byte header; make sure the third byte is actually present
// before Unpack reads pData[2], otherwise it reads past the valid data.
if((((m_Data.m_aChunkData[m_CurrentOffset] >> 6) & 3) & NET_CHUNKFLAG_VITAL) != 0 && m_CurrentOffset + 3 > m_Data.m_DataSize)
{
	m_Valid = false;
	return false;
}

// unpack the header
const int HeaderSplit = m_pConnection->m_Sixup ? 6 : 4;
CNetChunkHeader Header;
unsigned char *pData = Header.Unpack(&m_Data.m_aChunkData[m_CurrentOffset], HeaderSplit);
```

Проверка на наличие третьего байта добавлена до вызова `Unpack`, то есть чтение
за пределами валидных данных теперь исключено. Фикс соответствует предложению
аудита (пункт network.cpp:82-92) и внедрён.

---

## 2. Отказ декомпрессии при m_DataSize <= 0 (CNetBase::UnpackPacket)

Статус: ВНЕСЕНО.

Что было уязвимо:
При установленном флаге `NET_PACKETFLAG_COMPRESSION` размер `m_DataSize` берётся из
принятого пакета и передаётся в `ms_Huffman.Decompress` без нижнего ограничения.
Пустой или некорректный компрессированный вход попадал прямо в декомпрессор.
Аудит отмечал риск CPU-DoS (Huffman-бомба) при флуде компрессированными пакетами.

Что изменено (network.cpp, строки 383-403):

До (по аудиту):
```
if((pPacket->m_Flags & NET_PACKETFLAG_COMPRESSION) != 0)
{
	if(!AllowDecompression)
	{
		return -1;
	}
```

После (реально в коде):
```
if((pPacket->m_Flags & NET_PACKETFLAG_COMPRESSION) != 0)
{
	if(!AllowDecompression)
	{
		return -1;
	}
	// anti-DoS: reject empty/negative compressed payloads before feeding
	// them into the Huffman decompressor
	if(pPacket->m_DataSize <= 0)
	{
		return -1;
	}
	if(pDecompressed != nullptr)
	{
		*pDecompressed = true;
	}
	pPacket->m_DataSize = ms_Huffman.Decompress(&pBuffer[DataStart], pPacket->m_DataSize, pPacket->m_aChunkData, sizeof(pPacket->m_aChunkData));
	if(pPacket->m_DataSize < 0)
	{
		return -1;
	}
}
```

Проверка `pPacket->m_DataSize <= 0` внесена как в аудите (пункт network.cpp:376).
Дополнительно ограничение декомпрессии до установленного соединения выполняется
через флаг `AllowDecompression` (проверяется первым), а `Decompress` ограничен
размером выходного буфера `sizeof(pPacket->m_aChunkData)`.

---

## 3. Замена dbg_assert на return -1 при переполнении размера (CNetBase::UnpackPacket)

Статус: ВНЕСЕНО (в обеих ветках).

Что было уязвимо:
Аудит отмечал защитные `dbg_assert((size_t)pPacket->m_DataSize <= sizeof(m_aChunkData), ...)`
перед `mem_copy`. В release-сборках DDNet `dbg_assert` обычно активен, поэтому данные,
производные от сети, могли привести к удалённому аборту (краш сервера) при любом
рефакторе смещений/размеров. Опасный паттерн: аборт на входных данных из сети.

Что изменено:

Ветка connless (network.cpp, строки 345-350):
```
pPacket->m_DataSize = Size - Offset;
if((size_t)pPacket->m_DataSize > sizeof(pPacket->m_aChunkData))
{
	return -1;
}
mem_copy(pPacket->m_aChunkData, pBuffer + Offset, pPacket->m_DataSize);
```

Ветка connection-oriented без компрессии (network.cpp, строки 405-412):
```
else
{
	if((size_t)pPacket->m_DataSize > sizeof(pPacket->m_aChunkData))
	{
		return -1;
	}
	mem_copy(pPacket->m_aChunkData, &pBuffer[DataStart], pPacket->m_DataSize);
}
```

В обоих местах `dbg_assert` заменён на мягкий `return -1`, как предлагал аудит
(пункт network.cpp:332,384). Аборта на сетевых данных больше нет.

---

## 4. Ограничение resend (rate-limit на out-of-sequence vital-чанки)

Статус: ВНЕСЕНО ЧАСТИЧНО.

Что было уязвимо:
При каждом out-of-sequence vital-чанке вызывался `SignalResend()`. Атакующий с
валидным (или spoofed после handshake) соединением мог набить один пакет чанками
с "будущими" последовательностями, заставляя сервер обрабатывать их все и запрашивать
resend. Аудит отмечал риск усиления исходящего трафика и лишней траты CPU.

Что изменено (network.cpp, строки 120-129):
```
// out of sequence, request resend
if(g_Config.m_Debug)
	dbg_msg("conn", "asking for resend %d %d", Header.m_Sequence, (m_pConnection->m_Ack + 1) % NET_MAX_SEQUENCE);
m_pConnection->SignalResend();
// One resend request per packet is enough: SignalResend only sets a
// flag and Flush() sends a single resend regardless. Stop processing
// the rest of the chunks so a packet stuffed with future sequences
// cannot waste CPU here.
m_Valid = false;
return false;
```

Реализовано предложение-минимум из аудита: после первого запроса resend цикл
обработки чанков прерывается (`m_Valid = false; return false;`), поэтому в рамках
одного пакета выполняется не более одного `SignalResend`, и остаток чанков не
обрабатывается впустую.

Что осталось нерешённым:
Аудит предлагал также троттлинг по времени внутри `CNetConnection::SignalResend`.
Этого нет: в `network_conn.cpp` (строки 116-119) функция по-прежнему только
выставляет флаг:
```
void CNetConnection::SignalResend()
{
	m_Construct.m_Flags |= NET_PACKETFLAG_RESEND;
}
```
То есть межпакетного rate-limit на частоту resend-ответов (несколько пакетов подряд,
каждый с одним "будущим" чанком) нет. Внутрипакетное усиление устранено,
межпакетное ограничение по времени отсутствует.

---

## Нерешённые пункты (сводка)

- Rate-limit по времени на распаковку компрессированных пакетов (аудит, п.1):
  не реализован. Есть только запрет декомпрессии от неизвестных адресов через
  `AllowDecompression` и отсев `m_DataSize <= 0`. Отдельного троттлинга частоты
  декомпрессии нет.
- Троттлинг `SignalResend` по времени в `CNetConnection::SignalResend`
  (network_conn.cpp:116): не реализован, устранено только внутрипакетное усиление.

## Итог

Внесены и подтверждены в коде: проверка третьего байта vital-заголовка (стр. 81-88),
отсев `m_DataSize <= 0` перед декомпрессией (стр. 391-394), замена `dbg_assert` на
`return -1` перед обоими `mem_copy` (стр. 346-350 и 407-411), прерывание цикла после
первого resend в пакете (стр. 120-129). Не реализованы межпакетные rate-limit'ы
(декомпрессия и SignalResend по времени).

Документировал: mister/ \net
