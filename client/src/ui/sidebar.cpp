#include "ui/sidebar.h"

#include <QHBoxLayout>
#include <QVBoxLayout>

#include "core/chats_model.h"
#include "ui/delegates.h"
#include "ui/icons.h"
#include "ui/theme.h"

Sidebar::Sidebar(ChatsModel *model, QWidget *parent) : QWidget(parent), m_model(model) {
    m_proxy = new QSortFilterProxyModel(this);
    m_proxy->setSourceModel(model);
    m_proxy->setFilterRole(ChatsModel::TitleRole);
    m_proxy->setFilterCaseSensitivity(Qt::CaseInsensitive);

    m_menuBtn = new QToolButton;
    m_menuBtn->setPopupMode(QToolButton::InstantPopup);
    m_menuBtn->setFixedSize(40, 40);
    m_menu = new QMenu(this);
    m_menuBtn->setMenu(m_menu);
    m_menuBtn->setStyleSheet("QToolButton::menu-indicator { image: none; }");

    m_search = new QLineEdit;
    m_search->setPlaceholderText(tr("Search"));
    m_search->setClearButtonEnabled(true);
    m_search->setFixedHeight(36);
    connect(m_search, &QLineEdit::textChanged, m_proxy, &QSortFilterProxyModel::setFilterFixedString);

    auto *head = new QHBoxLayout;
    head->setContentsMargins(10, 8, 10, 8);
    head->setSpacing(8);
    head->addWidget(m_menuBtn);
    head->addWidget(m_search, 1);

    m_list = new QListView;
    m_list->setModel(m_proxy);
    m_list->setItemDelegate(new ChatListDelegate(m_list));
    m_list->setMouseTracking(true);
    m_list->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_list->setSelectionMode(QAbstractItemView::SingleSelection);
    m_list->setEditTriggers(QAbstractItemView::NoEditTriggers);
    connect(m_list, &QListView::clicked, this, [this](const QModelIndex &ix) {
        emit chatActivated(m_proxy->mapToSource(ix).row());
    });

    m_status = new QLabel;
    m_status->setContentsMargins(14, 6, 14, 6);

    auto *lay = new QVBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(0);
    lay->addLayout(head);
    lay->addWidget(m_list, 1);
    lay->addWidget(m_status);
    setMinimumWidth(260);
    refreshIcons();
}

void Sidebar::refreshIcons() {
    m_menuBtn->setIcon(Icons::icon("menu", pal().textSecondary, 22));
    m_menuBtn->setIconSize({22, 22});
}

void Sidebar::setStatus(const QString &text, bool ok) {
    m_status->setText(QString("<span style='color:%1'>●</span>&nbsp; %2").arg(ok ? "#4fae4e" : "#e0a030", text.toHtmlEscaped()));
}

void Sidebar::selectRow(int sourceRow) {
    const QModelIndex ix = m_proxy->mapFromSource(m_model->index(sourceRow));
    m_list->setCurrentIndex(ix);
}
