#include "ui/delegates.h"

#include <QFontMetrics>
#include <QPainter>
#include <QPainterPath>

#include "core/chats_model.h"
#include "core/messages_model.h"
#include "ui/theme.h"

QColor avatarColor(qint64 id) {
    static const char *cols[] = {"#e17076", "#faa774", "#a695e7", "#7bc862", "#6ec9cb", "#65aadd", "#ee7aae"};
    return QColor(cols[qAbs(id) % 7]);
}

void paintAvatar(QPainter *p, const QRect &r, const QString &title, qint64 id) {
    p->save();
    p->setRenderHint(QPainter::Antialiasing);
    p->setPen(Qt::NoPen);
    p->setBrush(avatarColor(id));
    p->drawEllipse(r);
    QString initials;
    for (const QString &w : title.split(' ', Qt::SkipEmptyParts)) {
        initials += w.at(0).toUpper();
        if (initials.size() == 2) break;
    }
    QFont f = p->font();
    f.setBold(true);
    f.setPixelSize(r.height() * 38 / 100);
    p->setFont(f);
    p->setPen(Qt::white);
    p->drawText(r, Qt::AlignCenter, initials);
    p->restore();
}

void ChatListDelegate::paint(QPainter *p, const QStyleOptionViewItem &opt, const QModelIndex &idx) const {
    const Palette &c = pal();
    const bool sel = opt.state & QStyle::State_Selected;
    const bool hover = opt.state & QStyle::State_MouseOver;
    const QRect r = opt.rect;
    p->save();
    p->setRenderHint(QPainter::Antialiasing);
    p->fillRect(r, sel ? c.itemActive : hover ? c.itemHover : c.sidebarBg);

    const QString title = idx.data(ChatsModel::TitleRole).toString();
    const QRect av(r.left() + 10, r.top() + (r.height() - 48) / 2, 48, 48);
    paintAvatar(p, av, title, idx.data(ChatsModel::IdRole).toLongLong());

    const int left = av.right() + 12, right = r.right() - 12;
    const QColor sec = sel ? c.textOnActive : c.textSecondary;
    const QColor main = sel ? c.textOnActive : c.text;

    QFont bold = opt.font;
    bold.setBold(true);
    QFont small = opt.font;
    small.setPointSizeF(opt.font.pointSizeF() - 1.5);

    const QString time = idx.data(ChatsModel::TimeRole).toString();
    p->setFont(small);
    const int tw = QFontMetrics(small).horizontalAdvance(time);
    const QRect row1(left, r.top() + 10, right - left, 22);
    p->setPen(sec);
    p->drawText(row1, Qt::AlignRight | Qt::AlignVCenter, time);
    p->setFont(bold);
    p->setPen(main);
    p->drawText(row1.adjusted(0, 0, -tw - 8, 0), Qt::AlignLeft | Qt::AlignVCenter,
                QFontMetrics(bold).elidedText(title, Qt::ElideRight, row1.width() - tw - 8));

    const int unread = idx.data(ChatsModel::UnreadRole).toInt();
    int badgeW = 0;
    if (unread > 0) {
        const QString s = QString::number(unread);
        badgeW = qMax(22, QFontMetrics(small).horizontalAdvance(s) + 14);
        const QRect b(right - badgeW, r.top() + 34, badgeW, 22);
        p->setPen(Qt::NoPen);
        p->setBrush(sel ? c.textOnActive : (idx.data(ChatsModel::MutedRole).toBool() ? c.badgeMuted : c.badge));
        p->drawRoundedRect(b, 11, 11);
        p->setFont(small);
        p->setPen(sel ? c.itemActive : Qt::white);
        p->drawText(b, Qt::AlignCenter, s);
        badgeW += 8;
    }
    const QRect row2(left, r.top() + 34, right - left - badgeW, 22);
    p->setFont(opt.font);
    p->setPen(sec);
    p->drawText(row2, Qt::AlignLeft | Qt::AlignVCenter,
                QFontMetrics(opt.font).elidedText(idx.data(ChatsModel::LastTextRole).toString(), Qt::ElideRight, row2.width()));
    p->restore();
}

static constexpr int kSide = 14, kPad = 10, kGap = 6, kRadius = 12;
static constexpr int kTextFlags = Qt::TextWordWrap | Qt::TextWrapAnywhere | Qt::AlignLeft | Qt::AlignTop;

MessageDelegate::Layout MessageDelegate::layout(const QFont &font, const QString &text, const QString &time, bool outgoing) const {
    QFont tf = font;
    tf.setPointSizeF(font.pointSizeF() - 2);
    const int maxBubble = qMax(140, qMin(480, m_width - 2 * kSide - 40));
    const QRect tr = QFontMetrics(font).boundingRect(QRect(0, 0, maxBubble - 2 * kPad, 100000), kTextFlags, text);
    const QFontMetrics tfm(tf);
    const int timeW = tfm.horizontalAdvance(time), timeH = tfm.height();
    const int w = qMax(tr.width(), timeW) + 2 * kPad;
    const int h = kPad + tr.height() + timeH + kPad / 2;
    Layout l;
    const int x = outgoing ? m_width - kSide - w : kSide;
    l.bubble = QRect(x, kGap / 2, w, h);
    l.text = QRect(x + kPad, kGap / 2 + kPad, w - 2 * kPad, tr.height());
    l.height = h + kGap;
    return l;
}

QSize MessageDelegate::sizeHint(const QStyleOptionViewItem &opt, const QModelIndex &idx) const {
    const Layout l = layout(opt.font, idx.data(MessagesModel::TextRole).toString(),
                            idx.data(MessagesModel::TimeRole).toString(), idx.data(MessagesModel::OutgoingRole).toBool());
    return {m_width, l.height};
}

void MessageDelegate::paint(QPainter *p, const QStyleOptionViewItem &opt, const QModelIndex &idx) const {
    const Palette &c = pal();
    const bool out = idx.data(MessagesModel::OutgoingRole).toBool();
    const QString text = idx.data(MessagesModel::TextRole).toString();
    const QString time = idx.data(MessagesModel::TimeRole).toString();
    const Layout l = layout(opt.font, text, time, out);
    p->save();
    p->setRenderHint(QPainter::Antialiasing);
    p->translate(opt.rect.topLeft());
    p->setPen(Qt::NoPen);
    p->setBrush(out ? c.bubbleOut : c.bubbleIn);
    p->drawRoundedRect(l.bubble, kRadius, kRadius);
    p->setFont(opt.font);
    p->setPen(c.text);
    p->drawText(l.text, kTextFlags, text);
    QFont tf = opt.font;
    tf.setPointSizeF(opt.font.pointSizeF() - 2);
    p->setFont(tf);
    p->setPen(out ? (ThemeManager::instance().isDark() ? QColor("#7da8d3") : QColor("#5da658")) : c.textSecondary);
    p->drawText(l.bubble.adjusted(0, 0, -kPad, -kPad / 2 + 1), Qt::AlignRight | Qt::AlignBottom, time);
    p->restore();
}
