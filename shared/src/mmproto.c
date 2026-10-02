#include "minimax/mmproto.h"

#include <stdlib.h>
#include <string.h>

static void put16(uint8_t *p, uint16_t v) { p[0] = (uint8_t)(v >> 8); p[1] = (uint8_t)v; }
static void put32(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)(v >> 24); p[1] = (uint8_t)(v >> 16); p[2] = (uint8_t)(v >> 8); p[3] = (uint8_t)v;
}
static void put64(uint8_t *p, uint64_t v) { put32(p, (uint32_t)(v >> 32)); put32(p + 4, (uint32_t)v); }
static uint16_t get16(const uint8_t *p) { return (uint16_t)((p[0] << 8) | p[1]); }
static uint32_t get32(const uint8_t *p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | p[3];
}
static uint64_t get64(const uint8_t *p) { return ((uint64_t)get32(p) << 32) | get32(p + 4); }

void mm_buf_init(mm_buf *b) { b->data = NULL; b->len = 0; b->cap = 0; }
void mm_buf_free(mm_buf *b) { free(b->data); mm_buf_init(b); }

static int reserve(mm_buf *b, size_t extra) {
    if (extra > SIZE_MAX - b->len) return -1;
    size_t need = b->len + extra;
    if (need <= b->cap) return 0;
    size_t cap = b->cap ? b->cap : 256;
    while (cap < need) {
        if (cap > SIZE_MAX / 2) return -1;
        cap *= 2;
    }
    uint8_t *n = realloc(b->data, cap);
    if (!n) return -1;
    b->data = n;
    b->cap = cap;
    return 0;
}

int mm_buf_append(mm_buf *b, const void *data, size_t n) {
    if (reserve(b, n)) return -1;
    if (n) memcpy(b->data + b->len, data, n);
    b->len += n;
    return 0;
}

void mm_buf_consume(mm_buf *b, size_t n) {
    if (n >= b->len) { b->len = 0; return; }
    memmove(b->data, b->data + n, b->len - n);
    b->len -= n;
}

mm_status mm_frame_encode(mm_buf *out, uint16_t type, uint8_t flags, uint32_t request_id,
                          const void *payload, size_t payload_len, uint32_t max_payload) {
    if (payload_len > max_payload || payload_len > UINT32_MAX) return MM_E_TOO_BIG;
    uint8_t h[MM_HEADER_SIZE];
    put16(h, MM_MAGIC);
    h[2] = MM_VERSION;
    h[3] = flags;
    put16(h + 4, type);
    put32(h + 6, request_id);
    put32(h + 10, (uint32_t)payload_len);
    if (mm_buf_append(out, h, sizeof h)) return MM_E_NOMEM;
    if (mm_buf_append(out, payload, payload_len)) return MM_E_NOMEM;
    return MM_OK;
}

mm_status mm_frame_decode(const uint8_t *buf, size_t len, uint32_t max_payload, mm_frame *out,
                          size_t *consumed) {
    if (len < MM_HEADER_SIZE) return MM_NEED_MORE;
    if (get16(buf) != MM_MAGIC) return MM_E_MAGIC;
    if (buf[2] != MM_VERSION) return MM_E_VERSION;
    uint32_t pl = get32(buf + 10);
    if (pl > max_payload) return MM_E_TOO_BIG;
    if (len - MM_HEADER_SIZE < pl) return MM_NEED_MORE;
    out->version = buf[2];
    out->flags = buf[3];
    out->type = get16(buf + 4);
    out->request_id = get32(buf + 6);
    out->payload = buf + MM_HEADER_SIZE;
    out->payload_len = pl;
    *consumed = MM_HEADER_SIZE + (size_t)pl;
    return MM_OK;
}

int mm_tlv_put(mm_buf *b, uint16_t tag, const void *data, size_t len) {
    if (len > UINT32_MAX) return -1;
    uint8_t h[MM_TLV_HEADER_SIZE];
    put16(h, tag);
    put32(h + 2, (uint32_t)len);
    if (mm_buf_append(b, h, sizeof h)) return -1;
    return mm_buf_append(b, data, len);
}

int mm_tlv_put_u32(mm_buf *b, uint16_t tag, uint32_t v) {
    uint8_t x[4];
    put32(x, v);
    return mm_tlv_put(b, tag, x, sizeof x);
}

int mm_tlv_put_u64(mm_buf *b, uint16_t tag, uint64_t v) {
    uint8_t x[8];
    put64(x, v);
    return mm_tlv_put(b, tag, x, sizeof x);
}

int mm_tlv_put_str(mm_buf *b, uint16_t tag, const char *s) { return mm_tlv_put(b, tag, s, strlen(s)); }

void mm_tlv_reader_init(mm_tlv_reader *r, const uint8_t *payload, size_t len) { r->p = payload; r->left = len; }

int mm_tlv_next(mm_tlv_reader *r, uint16_t *tag, const uint8_t **val, size_t *len) {
    if (r->left == 0) return 0;
    if (r->left < MM_TLV_HEADER_SIZE) return -1;
    uint32_t l = get32(r->p + 2);
    if (l > r->left - MM_TLV_HEADER_SIZE) return -1;
    *tag = get16(r->p);
    *val = r->p + MM_TLV_HEADER_SIZE;
    *len = l;
    r->p += MM_TLV_HEADER_SIZE + l;
    r->left -= MM_TLV_HEADER_SIZE + l;
    return 1;
}

int mm_tlv_find(const uint8_t *payload, size_t len, uint16_t tag, const uint8_t **val, size_t *vlen) {
    mm_tlv_reader r;
    mm_tlv_reader_init(&r, payload, len);
    uint16_t t;
    int rc;
    while ((rc = mm_tlv_next(&r, &t, val, vlen)) == 1)
        if (t == tag) return 0;
    return -1;
}

int mm_tlv_get_u32(const uint8_t *payload, size_t len, uint16_t tag, uint32_t *out) {
    const uint8_t *v; size_t vl;
    if (mm_tlv_find(payload, len, tag, &v, &vl) || vl != 4) return -1;
    *out = get32(v);
    return 0;
}

int mm_tlv_get_u64(const uint8_t *payload, size_t len, uint16_t tag, uint64_t *out) {
    const uint8_t *v; size_t vl;
    if (mm_tlv_find(payload, len, tag, &v, &vl) || vl != 8) return -1;
    *out = get64(v);
    return 0;
}
