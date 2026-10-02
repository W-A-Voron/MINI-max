#ifndef MINIMAX_DISPATCH_H
#define MINIMAX_DISPATCH_H

#include "config.h"
#include "minimax/mmproto.h"

/* Handles one decoded frame and appends the reply frame(s) to out. Returns 0 or -1 on OOM. */
int dispatch_frame(const server_config *cfg, const mm_frame *f, mm_buf *out);

#endif
