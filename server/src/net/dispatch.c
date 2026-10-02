#include "net/dispatch.h"

#include <string.h>

static int reply_error(const server_config *cfg, const mm_frame *f, uint32_t code, const char *text,
                       mm_buf *out) {
    mm_buf p;
    mm_buf_init(&p);
    int rc = mm_tlv_put_u32(&p, MM_TAG_ERROR_CODE, code) || mm_tlv_put_str(&p, MM_TAG_ERROR_TEXT, text) ||
             mm_frame_encode(out, MM_MSG_ERROR, 0, f->request_id, p.data, p.len, cfg->max_frame_bytes);
    mm_buf_free(&p);
    return rc ? -1 : 0;
}

int dispatch_frame(const server_config *cfg, const mm_frame *f, mm_buf *out) {
    switch (f->type) {
    case MM_MSG_PING:
        return mm_frame_encode(out, MM_MSG_PONG, 0, f->request_id, f->payload, f->payload_len,
                               cfg->max_frame_bytes) == MM_OK ? 0 : -1;
    case MM_MSG_HELLO: {
        mm_buf p;
        mm_buf_init(&p);
        int rc = mm_tlv_put_str(&p, MM_TAG_SERVER_NAME, cfg->name) ||
                 mm_tlv_put_str(&p, MM_TAG_SERVER_VERSION, MINIMAX_VERSION) ||
                 mm_tlv_put_u32(&p, MM_TAG_MAX_FRAME, cfg->max_frame_bytes) ||
                 mm_frame_encode(out, MM_MSG_HELLO, 0, f->request_id, p.data, p.len, cfg->max_frame_bytes);
        mm_buf_free(&p);
        return rc ? -1 : 0;
    }
    default:
        return reply_error(cfg, f, MM_ERR_UNKNOWN_TYPE, "unknown message type", out);
    }
}
