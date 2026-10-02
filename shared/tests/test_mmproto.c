#include "minimax/mmproto.h"

#include <stdio.h>
#include <string.h>

#define CHECK(c) do { if (!(c)) { fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); return 1; } } while (0)

int main(void) {
    mm_buf p, f;
    mm_buf_init(&p); mm_buf_init(&f);
    CHECK(mm_tlv_put_str(&p, MM_TAG_SERVER_NAME, "hello") == 0);
    CHECK(mm_tlv_put_u64(&p, MM_TAG_MAX_FRAME, 0x1122334455667788ull) == 0);
    CHECK(mm_frame_encode(&f, MM_MSG_HELLO, 0, 42, p.data, p.len, 1024) == MM_OK);

    mm_frame fr; size_t used = 0;
    /* partial data */
    CHECK(mm_frame_decode(f.data, f.len - 1, 1024, &fr, &used) == MM_NEED_MORE);
    CHECK(mm_frame_decode(f.data, 3, 1024, &fr, &used) == MM_NEED_MORE);
    /* full */
    CHECK(mm_frame_decode(f.data, f.len, 1024, &fr, &used) == MM_OK);
    CHECK(used == f.len && fr.type == MM_MSG_HELLO && fr.request_id == 42);

    const uint8_t *v; size_t vl; uint64_t u;
    CHECK(mm_tlv_find(fr.payload, fr.payload_len, MM_TAG_SERVER_NAME, &v, &vl) == 0);
    CHECK(vl == 5 && memcmp(v, "hello", 5) == 0);
    CHECK(mm_tlv_get_u64(fr.payload, fr.payload_len, MM_TAG_MAX_FRAME, &u) == 0);
    CHECK(u == 0x1122334455667788ull);
    CHECK(mm_tlv_find(fr.payload, fr.payload_len, 999, &v, &vl) == -1);

    /* limits and corruption */
    CHECK(mm_frame_decode(f.data, f.len, 4, &fr, &used) == MM_E_TOO_BIG);
    CHECK(mm_frame_encode(&f, 1, 0, 0, p.data, p.len, 4) == MM_E_TOO_BIG);
    f.data[0] ^= 0xFF;
    CHECK(mm_frame_decode(f.data, f.len, 1024, &fr, &used) == MM_E_MAGIC);
    f.data[0] ^= 0xFF; f.data[2] = 99;
    CHECK(mm_frame_decode(f.data, f.len, 1024, &fr, &used) == MM_E_VERSION);

    /* malformed TLV (declared length exceeds data) */
    uint8_t bad[] = {0, 1, 0, 0, 0, 50, 1};
    mm_tlv_reader r; uint16_t tag;
    mm_tlv_reader_init(&r, bad, sizeof bad);
    CHECK(mm_tlv_next(&r, &tag, &v, &vl) == -1);

    mm_buf_free(&p); mm_buf_free(&f);
    puts("mmproto: OK");
    return 0;
}
