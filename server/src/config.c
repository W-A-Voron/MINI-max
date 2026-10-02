#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "toml.h"

typedef struct {
    toml_table_t *tab;
    const char *section;
    char *err;
    int errlen;
    int failed;
} sec_t;

static void fail(sec_t *s, const char *what, const char *key) {
    if (!s->failed) snprintf(s->err, (size_t)s->errlen, "config: %s [%s].%s", what, s->section, key);
    s->failed = 1;
}

static char *get_str(sec_t *s, const char *key) {
    toml_datum_t d = toml_string_in(s->tab, key);
    if (!d.ok) { fail(s, "missing or non-string key", key); return NULL; }
    return d.u.s;
}

static int64_t get_int(sec_t *s, const char *key, int64_t min, int64_t max) {
    toml_datum_t d = toml_int_in(s->tab, key);
    if (!d.ok) { fail(s, "missing or non-integer key", key); return 0; }
    if (d.u.i < min || d.u.i > max) { fail(s, "value out of range for", key); return 0; }
    return d.u.i;
}

static int get_bool(sec_t *s, const char *key) {
    toml_datum_t d = toml_bool_in(s->tab, key);
    if (!d.ok) { fail(s, "missing or non-bool key", key); return 0; }
    return d.u.b;
}

static sec_t open_sec(toml_table_t *root, const char *name, char *err, int errlen, int *failed) {
    sec_t s = {toml_table_in(root, name), name, err, errlen, 0};
    if (!s.tab && !*failed) {
        snprintf(err, (size_t)errlen, "config: missing section [%s]", name);
        *failed = 1;
    }
    if (!s.tab) s.failed = 1;
    return s;
}

int config_load(const char *path, server_config *cfg, char *err, int errlen) {
    memset(cfg, 0, sizeof *cfg);
    FILE *fp = fopen(path, "r");
    if (!fp) { snprintf(err, (size_t)errlen, "cannot open %s", path); return -1; }
    char perr[200];
    toml_table_t *root = toml_parse_file(fp, perr, (int)sizeof perr);
    fclose(fp);
    if (!root) { snprintf(err, (size_t)errlen, "%s: %s", path, perr); return -1; }

    int failed = 0;
    sec_t s = open_sec(root, "server", err, errlen, &failed);
    if (s.tab) {
        cfg->name = get_str(&s, "name");
        cfg->host = get_str(&s, "host");
        cfg->port = (int)get_int(&s, "port", 0, 65535);
        cfg->backlog = (int)get_int(&s, "backlog", 1, 65535);
    }
    failed |= s.failed;

    s = open_sec(root, "limits", err, errlen, &failed);
    if (s.tab) {
        cfg->max_connections = (int)get_int(&s, "max_connections", 1, 1000000);
        cfg->max_frame_bytes = (uint32_t)get_int(&s, "max_frame_bytes", 64, 1 << 30);
    }
    failed |= s.failed;

    s = open_sec(root, "log", err, errlen, &failed);
    if (s.tab) cfg->log_level = get_str(&s, "level");
    failed |= s.failed;

    s = open_sec(root, "database", err, errlen, &failed);
    if (s.tab) {
        cfg->db_url = get_str(&s, "url");
        cfg->db_pool_size = (int)get_int(&s, "pool_size", 1, 1024);
    }
    failed |= s.failed;

    s = open_sec(root, "redis", err, errlen, &failed);
    if (s.tab) {
        cfg->redis_host = get_str(&s, "host");
        cfg->redis_port = (int)get_int(&s, "port", 1, 65535);
    }
    failed |= s.failed;

    s = open_sec(root, "minio", err, errlen, &failed);
    if (s.tab) {
        cfg->minio_endpoint = get_str(&s, "endpoint");
        cfg->minio_bucket = get_str(&s, "bucket");
        cfg->minio_access_key = get_str(&s, "access_key");
        cfg->minio_secret_key = get_str(&s, "secret_key");
    }
    failed |= s.failed;

    s = open_sec(root, "tls", err, errlen, &failed);
    if (s.tab) {
        cfg->tls_enabled = get_bool(&s, "enabled");
        cfg->tls_cert_file = get_str(&s, "cert_file");
        cfg->tls_key_file = get_str(&s, "key_file");
    }
    failed |= s.failed;

    toml_free(root);
    if (failed) { config_free(cfg); return -1; }
    return 0;
}

void config_free(server_config *c) {
    free(c->name); free(c->host); free(c->log_level); free(c->db_url); free(c->redis_host);
    free(c->minio_endpoint); free(c->minio_bucket); free(c->minio_access_key);
    free(c->minio_secret_key); free(c->tls_cert_file); free(c->tls_key_file);
    memset(c, 0, sizeof *c);
}
