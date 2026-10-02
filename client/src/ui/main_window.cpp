#include "ui/main_window.h"

#include <QApplication>
#include <QMenu>
#include <QMessageBox>
#include <QSplitter>
#include <QtDebug>

#include "core/chats_model.h"
#include "net/connection.h"
#include "ui/chat_view.h"
#include "ui/profile_panel.h"
#include "ui/sidebar.h"
#include "ui/theme.h"

MainWindow::MainWindow(const ClientConfig &cfg, QWidget *parent) : QMainWindow(parent), m_cfg(cfg) {
    setWindowTitle("MINI max");
    resize(1100, 720);

    m_chats = new ChatsModel(this);
    m_sidebar = new Sidebar(m_chats);
    m_chatView = new ChatView(m_chats);
    m_profile = new ProfilePanel;
    m_profile->hide();

    auto *split = new QSplitter;
    split->setHandleWidth(1);
    split->setChildrenCollapsible(false);
    split->addWidget(m_sidebar);
    split->addWidget(m_chatView);
    split->addWidget(m_profile);
    split->setStretchFactor(0, 0);
    split->setStretchFactor(1, 1);
    split->setSizes({340, 620, 300});
    setCentralWidget(split);

    QMenu *menu = m_sidebar->menu();
    menu->addAction(tr("Toggle dark / light theme"), this, [this] {
        ThemeManager::instance().setMode(ThemeManager::instance().isDark() ? "light" : "dark");
    });
    menu->addAction(tr("About MINI max"), this, [this] {
        QMessageBox::about(this, tr("About MINI max"),
                           tr("<b>MINI max</b> %1<br>A Telegram-style messenger.<br>Server: %2:%3")
                               .arg(QApplication::applicationVersion(), m_cfg.host).arg(m_cfg.port));
    });
    menu->addSeparator();
    menu->addAction(tr("Quit"), qApp, &QApplication::quit);

    connect(m_sidebar, &Sidebar::chatActivated, this, &MainWindow::openChat);
    connect(m_chatView, &ChatView::sendRequested, this, [this](int row, const QString &text) {
        m_chats->appendMessage(row, text, true);
        if (row == m_currentRow) m_profile->setChat(m_chats->chat(row));
    });
    connect(m_chatView, &ChatView::infoToggled, this, [this] { m_profile->setVisible(!m_profile->isVisible()); });
    connect(m_profile, &ProfilePanel::closeRequested, m_profile, &QWidget::hide);
    connect(&ThemeManager::instance(), &ThemeManager::changed, this, &MainWindow::applyTheme);

    // Telegram's built-in local chat, always present.
    const int saved = m_chats->addChat(tr("Saved Messages"), true);
    openChat(saved);
    m_sidebar->selectRow(saved);

    m_conn = new Connection(m_cfg, this);
    connect(m_conn, &Connection::stateChanged, this, [this](Connection::State) { updateStatus(); });
    applyTheme();
    updateStatus();
    m_conn->start();
}

void MainWindow::openChat(int row) {
    m_currentRow = row;
    m_chatView->setChat(row);
    m_profile->setChat(m_chats->chat(row));
}

void MainWindow::updateStatus() {
    switch (m_conn->state()) {
    case Connection::State::Ready:
        m_sidebar->setStatus(tr("Connected to %1 %2").arg(m_conn->serverName(), m_conn->serverVersion()), true);
        qInfo().noquote() << "connected to" << m_conn->serverName() << m_conn->serverVersion();
        break;
    case Connection::State::Connecting:
    case Connection::State::Handshaking:
        m_sidebar->setStatus(tr("Connecting to %1:%2…").arg(m_cfg.host).arg(m_cfg.port), false);
        break;
    case Connection::State::Disconnected:
        m_sidebar->setStatus(tr("Offline — reconnecting…"), false);
        break;
    }
}

void MainWindow::applyTheme() {
    qApp->setStyleSheet(ThemeManager::instance().styleSheet());
    m_sidebar->refreshIcons();
    m_chatView->refreshIcons();
    m_profile->refreshIcons();
    update();
}
