#include "config.h"

#include <QDir>
#include <QFileInfo>
#include <toml++/toml.hpp>

namespace {

template <typename T>
T require(const toml::table &root, const char *section, const char *key) {
    const toml::table *t = root[section].as_table();
    if (!t) throw ConfigError(std::string("missing section [") + section + "]");
    auto v = (*t)[key].template value<T>();
    if (!v) throw ConfigError(std::string("missing or invalid key [") + section + "]." + key);
    return *v;
}

}  // namespace

ClientConfig ClientConfig::load(const QString &path) {
    toml::table root;
    try {
        root = toml::parse_file(path.toStdString());
    } catch (const toml::parse_error &e) {
        throw ConfigError(std::string(e.description()));
    }
    ClientConfig c;
    c.host = QString::fromStdString(require<std::string>(root, "server", "host"));
    int port = require<int>(root, "server", "port");
    if (port < 1 || port > 65535) throw ConfigError("[server].port out of range");
    c.port = static_cast<quint16>(port);
    c.tlsEnabled = require<bool>(root, "tls", "enabled");
    c.tlsVerifyPeer = require<bool>(root, "tls", "verify_peer");
    c.tlsCaFile = QString::fromStdString(require<std::string>(root, "tls", "ca_file"));
    c.keepaliveSeconds = require<int>(root, "network", "keepalive_seconds");
    c.reconnectMinMs = require<int>(root, "network", "reconnect_min_ms");
    c.reconnectMaxMs = require<int>(root, "network", "reconnect_max_ms");
    if (c.keepaliveSeconds < 1 || c.reconnectMinMs < 1 || c.reconnectMaxMs < c.reconnectMinMs)
        throw ConfigError("invalid [network] values");
    c.theme = QString::fromStdString(require<std::string>(root, "ui", "theme"));
    if (c.theme != "dark" && c.theme != "light" && c.theme != "system")
        throw ConfigError("[ui].theme must be dark, light or system");
    QString cache = QString::fromStdString(require<std::string>(root, "ui", "cache_dir"));
    if (cache.startsWith("~")) cache = QDir::homePath() + cache.mid(1);
    c.cacheDir = cache;
    return c;
}

QString ClientConfig::findDefaultPath(const QString &appDir) {
    const QStringList candidates = {appDir + "/client.toml", appDir + "/configs/client.toml",
                                    appDir + "/../configs/client.toml", appDir + "/../../configs/client.toml",
                                    appDir + "/../../../configs/client.toml", "configs/client.toml"};
    for (const QString &p : candidates)
        if (QFileInfo::exists(p)) return QFileInfo(p).absoluteFilePath();
    return {};
}
