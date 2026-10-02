#ifndef MINIMAX_CONFIG_H
#define MINIMAX_CONFIG_H

#include <stdint.h>

typedef struct {
    char *name, *host;
    int port, backlog;
    int max_connections;
    uint32_t max_frame_bytes;
    char *log_level;
    char *db_url;
    int db_pool_size;
    char *redis_host;
    int redis_port;
    char *minio_endpoint, *minio_bucket, *minio_access_key, *minio_secret_key;
    int tls_enabled;
    char *tls_cert_file, *tls_key_file;
} server_config;

/* Loads and validates the TOML file. Returns 0 on success; on failure writes a message to err. */
int config_load(const char *path, server_config *cfg, char *err, int errlen);
void config_free(server_config *cfg);

#endif
