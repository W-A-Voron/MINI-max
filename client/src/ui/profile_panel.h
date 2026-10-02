#pragma once
#include <QWidget>

#include "core/chats_model.h"

class QLabel;
class QToolButton;

class ProfilePanel : public QWidget {
    Q_OBJECT
public:
    explicit ProfilePanel(QWidget *parent = nullptr);
    void setChat(const Chat *chat);
    void refreshIcons();

signals:
    void closeRequested();

protected:
    void paintEvent(QPaintEvent *) override;

private:
    QLabel *m_title, *m_info;
    QToolButton *m_close;
    qint64 m_id = 0;
    QString m_name;
};
