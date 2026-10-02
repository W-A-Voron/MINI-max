#include <signal.h>
#include <stdio.h>
#include <string.h>

#include "config.h"
#include "net/server.h"
#include "net/sock.h"
#include "util/log.h"

static volatile sig_atomic_t g_stop = 0;
static void on_signal(int sig) { (void)sig; g_stop = 1; }

int main(int argc, char **argv) {
    const char *path = NULL;
    for (int i = 1; i < argc; i++) {
        if ((strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--config") == 0) && i + 1 < argc) path = argv[++i];
        else { fprintf(stderr, "usage: %s -c <server.toml>\n", argv[0]); return 2; }
    }
    if (!path) { fprintf(stderr, "usage: %s -c <server.toml>\n", argv[0]); return 2; }

    server_config cfg;
    char err[256];
    if (config_load(path, &cfg, err, (int)sizeof err)) { fprintf(stderr, "%s\n", err); return 1; }
    if (log_set_level(cfg.log_level)) {
        fprintf(stderr, "config: invalid [log].level '%s'\n", cfg.log_level);
        config_free(&cfg);
        return 1;
    }
    if (sock_startup() != 0) { fprintf(stderr, "network stack initialisation failed\n"); config_free(&cfg); return 1; }

    signal(SIGINT, on_signal);
    signal(SIGTERM, on_signal);
#ifdef SIGBREAK
    signal(SIGBREAK, on_signal);
#endif
#ifdef SIGPIPE
    signal(SIGPIPE, SIG_IGN);
#endif

    LOGI("%s server %s starting", cfg.name, MINIMAX_VERSION);
    int rc = server_run(&cfg, &g_stop);
    sock_shutdown();
    config_free(&cfg);
    return rc ? 1 : 0;
}
