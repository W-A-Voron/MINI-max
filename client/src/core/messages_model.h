#pragma once
#include <QAbstractListModel>

class ChatsModel;

// Messages of the currently opened chat.
class MessagesModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum Roles { TextRole = Qt::UserRole + 1, OutgoingRole, TimeRole };

    explicit MessagesModel(ChatsModel *chats, QObject *parent = nullptr);
    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    void setChatRow(int row);
    int chatRow() const { return m_row; }

private:
    ChatsModel *m_chats;
    int m_row = -1;
    int m_count = 0;
};
