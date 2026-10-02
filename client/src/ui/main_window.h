#pragma once
#include <QMainWindow>

#include "config.h"

class ChatsModel;
class ChatView;
class Connection;
class ProfilePanel;
class Sidebar;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(const ClientConfig &cfg, QWidget *parent = nullptr);

private:
    void applyTheme();
    void openChat(int row);
    void updateStatus();

    ClientConfig m_cfg;
    ChatsModel *m_chats;
    Connection *m_conn;
    Sidebar *m_sidebar;
    ChatView *m_chatView;
    ProfilePanel *m_profile;
    int m_currentRow = -1;
};
