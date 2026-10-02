#include "ui/chat_view.h"

#include <QAbstractTextDocumentLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QPainter>
#include <QScrollBar>
#include <QVBoxLayout>

#include "core/chats_model.h"
#include "core/messages_model.h"
#include "ui/delegates.h"
#include "ui/icons.h"
#include "ui/theme.h"

InputEdit::InputEdit(QWidget *parent) : QPlainTextEdit(parent) {
    setPlaceholderText(tr("Write a message..."));
    setFrameShape(QFrame::NoFrame);
    setTabChangesFocus(true);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    connect(document()->documentLayout(), &QAbstractTextDocumentLayout::documentSizeChanged, this, [this] { adjustHeight(); });
    adjustHeight();
}

void InputEdit::adjustHeight() {
    const int line = fontMetrics().lineSpacing();
    const int lines = qBound(1, int(document()->size().height()), 8);
    setFixedHeight(lines * line + 28);
}

void InputEdit::keyPressEvent(QKeyEvent *e) {
    if ((e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter) && !(e->modifiers() & Qt::ShiftModifier)) {
        emit submit();
        return;
    }
    QPlainTextEdit::keyPressEvent(e);
}

static QToolButton *iconButton(const QString &tip) {
    auto *b = new QToolButton;
    b->setToolTip(tip);
    b->setFixedSize(40, 40);
    b->setIconSize({24, 24});
    b->setCursor(Qt::PointingHandCursor);
    return b;
}

ChatView::ChatView(ChatsModel *chats, QWidget *parent) : QWidget(parent), m_chats(chats) {
    m_messages = new MessagesModel(chats, this);

    m_title = new QLabel;
    QFont tf = font();
    tf.setBold(true);
    tf.setPointSizeF(tf.pointSizeF() + 1);
    m_title->setFont(tf);
    m_subtitle = new QLabel;
    m_searchBtn = iconButton(tr("Search"));
    m_infoBtn = iconButton(tr("Chat info"));
    connect(m_infoBtn, &QToolButton::clicked, this, &ChatView::infoToggled);

    auto *titles = new QVBoxLayout;
    titles->setSpacing(0);
    titles->addWidget(m_title);
    titles->addWidget(m_subtitle);
    auto *header = new QWidget;
    header->setFixedHeight(56);
    auto *hl = new QHBoxLayout(header);
    hl->setContentsMargins(18, 0, 10, 0);
    hl->addLayout(titles, 1);
    hl->addWidget(m_searchBtn);
    hl->addWidget(m_infoBtn);

    m_list = new QListView;
    m_delegate = new MessageDelegate(m_list);
    m_list->setModel(m_messages);
    m_list->setItemDelegate(m_delegate);
    m_list->setSelectionMode(QAbstractItemView::NoSelection);
    m_list->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_list->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_list->setFocusPolicy(Qt::NoFocus);
    m_list->viewport()->installEventFilter(this);
    connect(m_messages, &QAbstractItemModel::rowsInserted, m_list, [this] { m_list->scrollToBottom(); });
    connect(m_messages, &QAbstractItemModel::modelReset, m_list, [this] { m_list->scrollToBottom(); });

    auto *empty = new QLabel(tr("Select a chat to start messaging"));
    empty->setAlignment(Qt::AlignCenter);
    m_stack = new QStackedWidget;
    m_stack->addWidget(empty);
    m_stack->addWidget(m_list);

    m_attachBtn = iconButton(tr("Attach"));
    m_emojiBtn = iconButton(tr("Emoji"));
    m_sendBtn = iconButton(tr("Send"));
    m_input = new InputEdit;
    connect(m_input, &InputEdit::submit, this, &ChatView::submit);
    connect(m_sendBtn, &QToolButton::clicked, this, &ChatView::submit);
    auto *bar = new QWidget;
    auto *bl = new QHBoxLayout(bar);
    bl->setContentsMargins(10, 6, 10, 6);
    bl->setAlignment(Qt::AlignBottom);
    bl->addWidget(m_attachBtn, 0, Qt::AlignBottom);
    bl->addWidget(m_input, 1);
    bl->addWidget(m_emojiBtn, 0, Qt::AlignBottom);
    bl->addWidget(m_sendBtn, 0, Qt::AlignBottom);
    bar->setObjectName("inputBar");

    auto *lay = new QVBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(0);
    lay->addWidget(header);
    lay->addWidget(m_stack, 1);
    lay->addWidget(bar);
    header->setObjectName("chatHeader");
    refreshIcons();
    setChat(-1);
}

void ChatView::refreshIcons() {
    const QColor s = pal().textSecondary;
    m_searchBtn->setIcon(Icons::icon("search", s));
    m_infoBtn->setIcon(Icons::icon("info", s));
    m_attachBtn->setIcon(Icons::icon("attach", s));
    m_emojiBtn->setIcon(Icons::icon("smile", s));
    m_sendBtn->setIcon(Icons::icon("send", pal().accent));
    m_subtitle->setStyleSheet(QString("color:%1").arg(s.name()));
    m_list->setStyleSheet(QString("QListView { background: %1; }").arg(pal().chatBg.name()));
    m_stack->setStyleSheet(QString("QStackedWidget, QStackedWidget > QLabel { background: %1; color: %2; }")
                               .arg(pal().chatBg.name(), s.name()));
    setStyleSheet(QString("#chatHeader, #inputBar, #chatHeader QLabel, #inputBar QWidget { background: %1; }").arg(pal().headerBg.name()));
    m_list->doItemsLayout();
    m_list->viewport()->update();
}

void ChatView::setChat(int row) {
    const Chat *c = m_chats->chat(row);
    m_messages->setChatRow(row);
    m_title->setText(c ? c->title : QString());
    m_subtitle->setText(c ? tr("%n message(s)", "", c->messages.size()) : QString());
    m_stack->setCurrentIndex(c ? 1 : 0);
    m_input->setEnabled(c != nullptr);
    m_sendBtn->setEnabled(c != nullptr);
    if (c) {
        m_chats->markRead(row);
        m_input->setFocus();
    }
}

void ChatView::submit() {
    const QString text = m_input->toPlainText().trimmed();
    const int row = m_messages->chatRow();
    if (text.isEmpty() || row < 0) return;
    m_input->clear();
    emit sendRequested(row, text);
    m_subtitle->setText(tr("%n message(s)", "", m_chats->chat(row)->messages.size()));
}

bool ChatView::eventFilter(QObject *o, QEvent *e) {
    if (o == m_list->viewport() && e->type() == QEvent::Resize) {
        m_delegate->setViewportWidth(m_list->viewport()->width());
        m_list->doItemsLayout();
    }
    return QWidget::eventFilter(o, e);
}
