#include "core/messages_model.h"

#include <QLocale>

#include "core/chats_model.h"

MessagesModel::MessagesModel(ChatsModel *chats, QObject *parent) : QAbstractListModel(parent), m_chats(chats) {
    connect(chats, &ChatsModel::messageAppended, this, [this](int row) {
        if (row != m_row) return;
        const int n = m_chats->chat(row)->messages.size();
        beginInsertRows({}, m_count, n - 1);
        m_count = n;
        endInsertRows();
    });
}

int MessagesModel::rowCount(const QModelIndex &parent) const { return parent.isValid() ? 0 : m_count; }

QVariant MessagesModel::data(const QModelIndex &index, int role) const {
    const Chat *c = m_chats->chat(m_row);
    if (!c || index.row() < 0 || index.row() >= c->messages.size()) return {};
    const Message &m = c->messages[index.row()];
    switch (role) {
    case Qt::DisplayRole:
    case TextRole: return m.text;
    case OutgoingRole: return m.outgoing;
    case TimeRole: return QLocale().toString(m.time.time(), QLocale::ShortFormat);
    }
    return {};
}

void MessagesModel::setChatRow(int row) {
    beginResetModel();
    m_row = row;
    const Chat *c = m_chats->chat(row);
    m_count = c ? c->messages.size() : 0;
    endResetModel();
}
