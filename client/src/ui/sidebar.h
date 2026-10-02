#pragma once
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QMenu>
#include <QSortFilterProxyModel>
#include <QToolButton>
#include <QWidget>

class ChatsModel;

class Sidebar : public QWidget {
    Q_OBJECT
public:
    explicit Sidebar(ChatsModel *model, QWidget *parent = nullptr);
    QMenu *menu() { return m_menu; }
    void setStatus(const QString &text, bool ok);
    void selectRow(int sourceRow);
    void refreshIcons();

signals:
    void chatActivated(int sourceRow);

private:
    ChatsModel *m_model;
    QSortFilterProxyModel *m_proxy;
    QListView *m_list;
    QLineEdit *m_search;
    QToolButton *m_menuBtn;
    QMenu *m_menu;
    QLabel *m_status;
};
