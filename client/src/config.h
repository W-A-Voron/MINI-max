#pragma once
#include <QString>
#include <stdexcept>

struct ConfigError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

struct ClientConfig {
    QString host;
    quint16 port = 0;
    bool tlsEnabled = false;
    bool tlsVerifyPeer = true;
    QString tlsCaFile;
    int keepaliveSeconds = 0;
    int reconnectMinMs = 0;
    int reconnectMaxMs = 0;
    QString theme;     // dark | light | system
    QString cacheDir;  // '~' already expanded

    // Parses the TOML file; every key is required. Throws ConfigError.
    static ClientConfig load(const QString &path);
    // Looks for client.toml next to the executable, then in ./configs, ../configs, ../../configs.
    static QString findDefaultPath(const QString &appDir);
};
