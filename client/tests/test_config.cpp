#include <QCoreApplication>
#include <QFile>
#include <QTemporaryDir>
#include <cstdio>

#include "config.h"

#define CHECK(c) do { if (!(c)) { std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #c); return 1; } } while (0)

int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    CHECK(argc > 1);
    ClientConfig c = ClientConfig::load(argv[1]);
    CHECK(!c.host.isEmpty() && c.port > 0 && c.keepaliveSeconds > 0);
    CHECK(c.theme == "dark" || c.theme == "light" || c.theme == "system");

    QTemporaryDir dir;
    QFile f(dir.filePath("bad.toml"));
    CHECK(f.open(QIODevice::WriteOnly));
    f.write("[server]\nhost = \"x\"\n");  // port missing
    f.close();
    bool threw = false;
    try { ClientConfig::load(f.fileName()); } catch (const ConfigError &) { threw = true; }
    CHECK(threw);
    threw = false;
    try { ClientConfig::load(dir.filePath("nope.toml")); } catch (const ConfigError &) { threw = true; }
    CHECK(threw);
    std::puts("client config: OK");
    return 0;
}
