# netconn audit (in progress)

## network.cpp

### network.cpp:376 — Decompress длины сжатого пакета управляется атакующим
Проблема: при NET_PACKETFLAG_COMPRESSION `pPacket->m_DataSize` берётся напрямую из размера полученного пакета и целиком передаётся в Huffman.Decompress без верхнего ограничения на коэффициент расширения. Хаффман-бомба (маленький входной пакет -> максимальный выход) заставляет декомпрессор пройти весь буфер вывода на каждый пакет; при флуде компрессированными пакетами это CPU-DoS. Декомпрессия при этом происходит ДО валидации токена/соединения (см. network_server AllowDecompression), т.е. spoofed-адрес может грузить CPU.
Фикс: ограничить обработку компрессированных пакетов до установленного соединения и добавить rate-limit на распаковку. Как минимум не декомпрессить пакеты от неизвестных адресов:
old:
```
if((pPacket->m_Flags & NET_PACKETFLAG_COMPRESSION) != 0)
{
	if(!AllowDecompression)
	{
		return -1;
	}
```
new:
```
if((pPacket->m_Flags & NET_PACKETFLAG_COMPRESSION) != 0)
{
	if(!AllowDecompression)
	{
		return -1;
	}
	// анти-DoS: компрессированный вход не может быть меньше минимального осмысленного размера
	if(pPacket->m_DataSize <= 0)
	{
		return -1;
	}
```

### network.cpp:332,384 — dbg_assert на размере пакета из сети
Проблема: `dbg_assert((size_t)pPacket->m_DataSize <= sizeof(pPacket->m_aChunkData), ...)`. Сейчас перед этим Size ограничен NET_MAX_PACKETSIZE (UnpackPacketFlags), а DataSize=Size-Offset, так что assert математически не срабатывает от прямого входа. НО это защитный assert на данных, производных от сети: любой рефактор Offset/размера буфера превратит его в удалённый аборт (в release dbg_assert обычно активен в DDNet -> мгновенный краш всех). Опасный паттерн.
Фикс: заменить аборт на мягкий возврат ошибки:
old:
```
dbg_assert((size_t)pPacket->m_DataSize <= sizeof(pPacket->m_aChunkData), "invalid packet size reached mem_copy, size=%d", pPacket->m_DataSize);
mem_copy(pPacket->m_aChunkData, pBuffer + Offset, pPacket->m_DataSize);
```
new:
```
if((size_t)pPacket->m_DataSize > sizeof(pPacket->m_aChunkData))
	return -1;
mem_copy(pPacket->m_aChunkData, pBuffer + Offset, pPacket->m_DataSize);
```
(аналогично для строки 384)

### network.cpp:82-92 — разбор chunk header (VITAL 3 байта) проверяет только 2 байта
Проблема: перед распаковкой заголовка проверяется `m_CurrentOffset + 2 > m_DataSize` (строка 75), но для VITAL-чанка Unpack читает 3-й байт (`pData[2]`, строка 465). Если чанк помечен VITAL и лежит в самом конце пакета так, что 3-й байт заголовка = последний+1 байт, чтение pData[2] выходит за m_DataSize (в пределах буфера m_aChunkData, но за валидными данными -> чтение мусора для m_Sequence). Проверка `pData + Header.m_Size > pEnd` (строка 87) идёт УЖЕ ПОСЛЕ чтения 3-го байта.
Фикс: проверять 3 байта для потенциально vital-заголовка до Unpack:
old:
```
// the chunk header is two bytes, three for vital chunks
if(m_CurrentOffset + 2 > m_Data.m_DataSize)
{
	m_Valid = false;
	return false;
}

// unpack the header
const int HeaderSplit = m_pConnection->m_Sixup ? 6 : 4;
CNetChunkHeader Header;
unsigned char *pData = Header.Unpack(&m_Data.m_aChunkData[m_CurrentOffset], HeaderSplit);
```
new:
```
// the chunk header is two bytes, three for vital chunks
if(m_CurrentOffset + 2 > m_Data.m_DataSize)
{
	m_Valid = false;
	return false;
}

// если чанк vital (флаг в первом байте), заголовок 3 байта — проверяем наличие 3-го байта
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

### network.cpp:95-116 — resend-запрос без rate-limit (усилитель трафика)
Проблема: при каждом out-of-sequence vital-чанке вызывается `m_pConnection->SignalResend()`. Атакующий с валидным (или spoofed после handshake) соединением может напихать в один пакет NET_MAX_PACKET_CHUNKS чанков с "будущими" последовательностями -> сервер на каждый шлёт resend-ответ. Нет ограничения количества SignalResend на пакет/на секунду -> усиление исходящего трафика.
Фикс: ограничить один SignalResend на обработанный пакет (флаг) и/или троттлинг по времени в CNetConnection::SignalResend. Минимум — выходить из цикла после первого запроса ресенда в пакете.
