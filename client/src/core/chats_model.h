#pragma once
#include <QAbstractListModel>
#include <QDateTime>
#include <QVector>

struct Message {
    qint64 id = 0;
    bool outgoing = false;
    QString text;
    QDateTime time;
};

struct Chat {
    qint64 id = 0;
    QString title;
    QVector<Message> messages;
    int unread = 0;
    bool pinned = false;
    bool muted = false;
};

class ChatsModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum Roles { TitleRole = Qt::UserRole + 1, LastTextRole, TimeRole, UnreadRole, PinnedRole, MutedRole, IdRole };

    explicit ChatsModel(QObject *parent = nullptr) : QAbstractListModel(parent) {}
    int rowCount(const QModelIndex &parent = {}) const override { return parent.isValid() ? 0 : m_chats.size(); }
    QVariant data(const QModelIndex &index, int role) const override;

    int addChat(const QString &title, bool pinned = false);
    void appendMessage(int row, const QString &text, bool outgoing);
    const Chat *chat(int row) const { return row >= 0 && row < m_chats.size() ? &m_chats[row] : nullptr; }
    void markRead(int row);

signals:
    void messageAppended(int row);

private:
    QVector<Chat> m_chats;
    qint64 m_nextChatId = 1;
    qint64 m_nextMsgId = 1;
};
