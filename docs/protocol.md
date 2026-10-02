# MMProto

Бинарный протокол MINI max. Все числа — big-endian.

| Смещение | Размер | Поле | Описание |
|---|---|---|---|
| 0 | 2 | magic | `0x4D4D` («MM») |
| 2 | 1 | version | сейчас `1` |
| 3 | 1 | flags | `0x01` — payload это JSON (режим отладки) |
| 4 | 2 | type | тип сообщения |
| 6 | 4 | request_id | связывает ответ с запросом; `0` — push от сервера |
| 10 | 4 | length | размер payload |
| 14 | N | payload | последовательность TLV |

**TLV:** `u16 tag`, `u32 length`, `value`.

Максимальный размер payload задаётся сервером (`[limits].max_frame_bytes`) и сообщается клиенту в ответе HELLO.

## Сообщения (реализовано)

| type | Имя | Направление | Payload |
|---|---|---|---|
| 1 | PING | C→S | произвольные данные |
| 2 | PONG | S→C | эхо данных из PING |
| 3 | HELLO | C↔S | C→S: `CLIENT_NAME`; S→C: `SERVER_NAME`, `SERVER_VERSION`, `MAX_FRAME` |
| 4 | ERROR | S→C | `ERROR_CODE` (u32), `ERROR_TEXT` |

Теги: 1 CLIENT_NAME, 2 SERVER_NAME, 3 SERVER_VERSION, 4 MAX_FRAME (u32), 5 ERROR_CODE, 6 ERROR_TEXT.
Код ошибки 1 — неизвестный тип, 2 — некорректный запрос.

Нарушение формата (неверный magic/версия, превышение лимита) — сервер закрывает соединение.
Реализация: `shared/src/mmproto.c`, тесты: `shared/tests`, `server/tests/test_server.py`.
