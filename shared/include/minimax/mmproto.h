/*
 * MMProto — MINI max binary protocol (C11, usable from C++).
 *
 * Frame (all integers big-endian):
 *   u16 magic      0x4D4D ("MM")
 *   u8  version    MM_VERSION
 *   u8  flags      MM_FLAG_*
 *   u16 type       message type (mm_msg_type)
 *   u32 request_id correlates responses with requests (0 = server push)
 *   u32 length     payload size in bytes
 *   u8  payload[length]   sequence of TLVs
 *
 * TLV: u16 tag, u32 length, value bytes.
 */
#ifndef MINIMAX_MMPROTO_H
#define MINIMAX_MMPROTO_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MM_MAGIC 0x4D4Du
#define MM_VERSION 1u
#define MM_HEADER_SIZE 14u
#define MM_TLV_HEADER_SIZE 6u

#define MM_FLAG_JSON 0x01u /* payload is UTF-8 JSON (debug mode) */

typedef enum {
    MM_MSG_PING = 1,
    MM_MSG_PONG = 2,
    MM_MSG_HELLO = 3,
    MM_MSG_ERROR = 4
} mm_msg_type;

typedef enum {
    MM_TAG_CLIENT_NAME = 1,
    MM_TAG_SERVER_NAME = 2,
    MM_TAG_SERVER_VERSION = 3,
    MM_TAG_MAX_FRAME = 4,
    MM_TAG_ERROR_CODE = 5,
    MM_TAG_ERROR_TEXT = 6
} mm_tag;

typedef enum {
    MM_ERR_UNKNOWN_TYPE = 1,
    MM_ERR_BAD_REQUEST = 2
} mm_error_code;

typedef enum {
    MM_OK = 0,
    MM_NEED_MORE = 1,
    MM_E_MAGIC = -1,
    MM_E_VERSION = -2,
    MM_E_TOO_BIG = -3,
    MM_E_NOMEM = -4,
    MM_E_MALFORMED = -5
} mm_status;

/* Growable byte buffer. */
typedef struct {
    uint8_t *data;
    size_t len;
    size_t cap;
} mm_buf;

void mm_buf_init(mm_buf *b);
void mm_buf_free(mm_buf *b);
int mm_buf_append(mm_buf *b, const void *data, size_t n);
void mm_buf_consume(mm_buf *b, size_t n); /* drop n bytes from the front */

typedef struct {
    uint8_t version;
    uint8_t flags;
    uint16_t type;
    uint32_t request_id;
    const uint8_t *payload; /* points into the decoded buffer */
    uint32_t payload_len;
} mm_frame;

/* Appends one frame to out. Returns MM_OK or MM_E_*. */
mm_status mm_frame_encode(mm_buf *out, uint16_t type, uint8_t flags, uint32_t request_id,
                          const void *payload, size_t payload_len, uint32_t max_payload);

/* Tries to decode one frame from buf. On MM_OK, *consumed is the frame size. */
mm_status mm_frame_decode(const uint8_t *buf, size_t len, uint32_t max_payload, mm_frame *out,
                          size_t *consumed);

/* TLV writer */
int mm_tlv_put(mm_buf *b, uint16_t tag, const void *data, size_t len);
int mm_tlv_put_u32(mm_buf *b, uint16_t tag, uint32_t v);
int mm_tlv_put_u64(mm_buf *b, uint16_t tag, uint64_t v);
int mm_tlv_put_str(mm_buf *b, uint16_t tag, const char *s);

/* TLV reader: iterate with mm_tlv_next. Returns 1 = got item, 0 = end, -1 = malformed. */
typedef struct {
    const uint8_t *p;
    size_t left;
} mm_tlv_reader;

void mm_tlv_reader_init(mm_tlv_reader *r, const uint8_t *payload, size_t len);
int mm_tlv_next(mm_tlv_reader *r, uint16_t *tag, const uint8_t **val, size_t *len);

/* Finds first TLV with tag. Returns 0 if found, -1 otherwise. */
int mm_tlv_find(const uint8_t *payload, size_t len, uint16_t tag, const uint8_t **val, size_t *vlen);
int mm_tlv_get_u32(const uint8_t *payload, size_t len, uint16_t tag, uint32_t *out);
int mm_tlv_get_u64(const uint8_t *payload, size_t len, uint16_t tag, uint64_t *out);

#ifdef __cplusplus
}
#endif
#endif
