#pragma once
#include <QLabel>
#include <QListView>
#include <QPlainTextEdit>
#include <QStackedWidget>
#include <QToolButton>
#include <QWidget>

class ChatsModel;
class MessagesModel;
class MessageDelegate;

// Multi-line input: Enter sends, Shift+Enter inserts a new line.
class InputEdit : public QPlainTextEdit {
    Q_OBJECT
public:
    explicit InputEdit(QWidget *parent = nullptr);
signals:
    void submit();

protected:
    void keyPressEvent(QKeyEvent *e) override;
private:
    void adjustHeight();
};

class ChatView : public QWidget {
    Q_OBJECT
public:
    ChatView(ChatsModel *chats, QWidget *parent = nullptr);
    void setChat(int row);
    void refreshIcons();

signals:
    void sendRequested(int chatRow, const QString &text);
    void infoToggled();

protected:
    bool eventFilter(QObject *o, QEvent *e) override;

private:
    void submit();
    ChatsModel *m_chats;
    MessagesModel *m_messages;
    MessageDelegate *m_delegate;
    QStackedWidget *m_stack;
    QListView *m_list;
    QLabel *m_title, *m_subtitle;
    InputEdit *m_input;
    QToolButton *m_searchBtn, *m_infoBtn, *m_attachBtn, *m_emojiBtn, *m_sendBtn;
};
