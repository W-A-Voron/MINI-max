#include "core/chats_model.h"

#include <QLocale>

QVariant ChatsModel::data(const QModelIndex &index, int role) const {
    const Chat *c = chat(index.row());
    if (!c) return {};
    const Message *last = c->messages.isEmpty() ? nullptr : &c->messages.last();
    switch (role) {
    case Qt::DisplayRole:
    case TitleRole: return c->title;
    case LastTextRole: return last ? last->text.simplified() : QString();
    case TimeRole: {
        if (!last) return QString();
        const QDate today = QDate::currentDate();
        const QDate d = last->time.date();
        if (d == today) return QLocale().toString(last->time.time(), QLocale::ShortFormat);
        if (d.daysTo(today) < 7) return QLocale().dayName(d.dayOfWeek(), QLocale::ShortFormat);
        return QLocale().toString(d, QLocale::ShortFormat);
    }
    case UnreadRole: return c->unread;
    case PinnedRole: return c->pinned;
    case MutedRole: return c->muted;
    case IdRole: return c->id;
    }
    return {};
}

int ChatsModel::addChat(const QString &title, bool pinned) {
    const int row = m_chats.size();
    beginInsertRows({}, row, row);
    Chat c;
    c.id = m_nextChatId++;
    c.title = title;
    c.pinned = pinned;
    m_chats.append(c);
    endInsertRows();
    return row;
}

void ChatsModel::appendMessage(int row, const QString &text, bool outgoing) {
    if (row < 0 || row >= m_chats.size()) return;
    Message m;
    m.id = m_nextMsgId++;
    m.outgoing = outgoing;
    m.text = text;
    m.time = QDateTime::currentDateTime();
    Chat &c = m_chats[row];
    c.messages.append(m);
    if (!outgoing) c.unread++;
    const QModelIndex ix = index(row);
    emit dataChanged(ix, ix);
    emit messageAppended(row);
}

void ChatsModel::markRead(int row) {
    if (row < 0 || row >= m_chats.size() || m_chats[row].unread == 0) return;
    m_chats[row].unread = 0;
    const QModelIndex ix = index(row);
    emit dataChanged(ix, ix, {UnreadRole});
}
