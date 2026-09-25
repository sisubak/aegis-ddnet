# Fix: Multiple registration tasks (issue #11811)

Файл: `src/engine/server/register.cpp`

## Проблема
При ошибке регистрации на мастер-сервере (например `Less than 500 bytes/sec`)
`CRegister::CProtocol::SendRegister()` вызывался каждую секунду и на каждый вызов
через `m_pEngine->AddJob(...)` создавалась новая HTTP-задача регистрации
(`CProtocol::CJob`). Задачи не завершались вовремя и накапливались параллельно,
съедая CPU.

Точки старта задач:
- `CProtocol::SendRegister()` — единственное место, где создаётся `CJob` через
  `AddJob`. Вызывается из `CProtocol::Update`, `CProtocol::OnToken`,
  `CRegister::OnConfigChange` и `CRegister::OnNewInfo`.

## Решение
Добавлен флаг «запрос уже выполняется» на уровне `CProtocol::CShared` (per-protocol),
защищённый существующим `m_Lock`:

1. В `CShared` добавлено поле:
   ```cpp
   bool m_RequestInFlight GUARDED_BY(m_Lock) = false;
   ```

2. В `SendRegister()` перед созданием задачи (внутри `CLockScope(m_pShared->m_Lock)`):
   - если `m_RequestInFlight == true`, новая задача НЕ создаётся; вместо этого
     `m_NextRegister` сдвигается на +1 сек (`Now + Freq`), чтобы перепроверить позже
     без busy-loop, и функция выходит через `return`;
   - иначе флаг выставляется в `true`, и дальше идёт обычное создание задачи.

3. В `CJob::Run()` сразу после `m_pRegister->Wait()` (то есть когда HTTP-запрос
   реально завершён — успех, ошибка или таймаут) флаг сбрасывается в `false`
   под `m_Lock`, до всех веток разбора ответа и ранних `return`. Это гарантирует,
   что флаг снимается при любом исходе и следующая попытка регистрации сможет
   стартовать.

## Что НЕ трогалось
- Логика ретраев (`m_NextRegister`/`m_PrevRegister`, `CheckChallengeStatus`,
  экспоненциальный backoff-TODO) не менялась.
- Существующие функции не переименовывались.
- Индексация запросов (`m_NumTotalRequests`, `m_LatestResponseIndex`) сохранена.

## Проверка
- Изменённые участки перечитаны (строки ~70-76, ~326-347, ~442-451).
- Флаг снимается ДО ранних `return` в `Run()` — залипания «навсегда in-flight»
  быть не должно.
- Сборку проекта не запускал (среда сборки DDNet тут не проверялась); изменения
  минимальны и синтаксически согласованы с окружающим кодом.

Автор: mister/ \register
