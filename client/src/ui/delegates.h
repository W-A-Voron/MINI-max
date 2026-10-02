#pragma once
#include <QStyledItemDelegate>

QColor avatarColor(qint64 id);
void paintAvatar(QPainter *p, const QRect &r, const QString &title, qint64 id);

class ChatListDelegate : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;
    void paint(QPainter *p, const QStyleOptionViewItem &opt, const QModelIndex &idx) const override;
    QSize sizeHint(const QStyleOptionViewItem &, const QModelIndex &) const override { return {320, 64}; }
};

class MessageDelegate : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;
    void setViewportWidth(int w) { m_width = w; }
    void paint(QPainter *p, const QStyleOptionViewItem &opt, const QModelIndex &idx) const override;
    QSize sizeHint(const QStyleOptionViewItem &opt, const QModelIndex &idx) const override;

private:
    struct Layout {
        QRect bubble;  // relative to item rect origin (0,0)
        QRect text;
        int height;
    };
    Layout layout(const QFont &font, const QString &text, const QString &time, bool outgoing) const;
    int m_width = 600;
};
