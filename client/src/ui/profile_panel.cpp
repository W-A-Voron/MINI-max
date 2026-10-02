#include "ui/profile_panel.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QToolButton>
#include <QVBoxLayout>

#include "ui/delegates.h"
#include "ui/icons.h"
#include "ui/theme.h"

ProfilePanel::ProfilePanel(QWidget *parent) : QWidget(parent) {
    setMinimumWidth(260);
    auto *heading = new QLabel(tr("Info"));
    QFont f = font();
    f.setBold(true);
    heading->setFont(f);
    m_close = new QToolButton;
    m_close->setFixedSize(36, 36);
    m_close->setIconSize({20, 20});
    connect(m_close, &QToolButton::clicked, this, &ProfilePanel::closeRequested);
    auto *top = new QHBoxLayout;
    top->addWidget(heading, 1);
    top->addWidget(m_close);

    m_title = new QLabel;
    m_title->setAlignment(Qt::AlignCenter);
    QFont tf = font();
    tf.setBold(true);
    tf.setPointSizeF(tf.pointSizeF() + 3);
    m_title->setFont(tf);
    m_info = new QLabel;
    m_info->setAlignment(Qt::AlignCenter);

    auto *lay = new QVBoxLayout(this);
    lay->setContentsMargins(14, 10, 14, 14);
    lay->addLayout(top);
    lay->addSpacing(120);  // room for the avatar painted in paintEvent
    lay->addWidget(m_title);
    lay->addWidget(m_info);
    lay->addStretch(1);
    refreshIcons();
}

void ProfilePanel::refreshIcons() {
    m_close->setIcon(Icons::icon("close", pal().textSecondary, 20));
    m_info->setStyleSheet(QString("color:%1").arg(pal().textSecondary.name()));
}

void ProfilePanel::setChat(const Chat *c) {
    m_id = c ? c->id : 0;
    m_name = c ? c->title : QString();
    m_title->setText(m_name);
    m_info->setText(c ? tr("%n message(s)", "", c->messages.size()) : QString());
    update();
}

void ProfilePanel::paintEvent(QPaintEvent *) {
    QPainter p(this);
    if (m_id) paintAvatar(&p, QRect((width() - 96) / 2, 56, 96, 96), m_name, m_id);
}
