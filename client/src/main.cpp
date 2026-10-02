#include <QApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QMessageBox>
#include <QTimer>

#include "config.h"
#include "ui/main_window.h"
#include "ui/theme.h"

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    QApplication::setApplicationName("MINI max");
    QApplication::setApplicationVersion(MINIMAX_VERSION);

    QCommandLineParser cli;
    cli.addHelpOption();
    cli.addVersionOption();
    QCommandLineOption cfgOpt({"c", "config"}, "Path to client.toml", "file");
    QCommandLineOption shotOpt("screenshot", "Save a window screenshot to <file> and exit", "file");
    QCommandLineOption quitOpt("quit-after", "Quit after <ms> milliseconds", "ms");
    cli.addOptions({cfgOpt, shotOpt, quitOpt});
    cli.process(app);

    QString path = cli.value(cfgOpt);
    if (path.isEmpty()) path = ClientConfig::findDefaultPath(QApplication::applicationDirPath());

    ClientConfig cfg;
    try {
        if (path.isEmpty()) throw ConfigError("client.toml not found (use --config <file>)");
        cfg = ClientConfig::load(path);
    } catch (const ConfigError &e) {
        QMessageBox::critical(nullptr, "MINI max", QString("Configuration error:\n%1").arg(e.what()));
        return 1;
    }
    QDir().mkpath(cfg.cacheDir);

    QFont f = app.font();
    f.setPointSizeF(10.5);
    app.setFont(f);
    ThemeManager::instance().setMode(cfg.theme);

    MainWindow w(cfg);
    w.show();

    if (cli.isSet(shotOpt)) {
        QTimer::singleShot(1200, &app, [&] {
            w.grab().save(cli.value(shotOpt));
            app.quit();
        });
    } else if (cli.isSet(quitOpt)) {
        QTimer::singleShot(cli.value(quitOpt).toInt(), &app, &QApplication::quit);
    }
    return app.exec();
}
