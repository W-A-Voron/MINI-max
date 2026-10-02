#include "ui/theme.h"

#include <QGuiApplication>
#include <QPalette>

ThemeManager::ThemeManager() { setMode("dark"); }

ThemeManager &ThemeManager::instance() {
    static ThemeManager t;
    return t;
}

void ThemeManager::setMode(const QString &mode) {
    if (mode == "system") m_dark = QGuiApplication::palette().color(QPalette::Window).lightness() < 128;
    else m_dark = (mode != "light");
    if (m_dark) {
        m_pal = {"#17212b", "#17212b", "#0e1621", "#17212b", "#202b36", "#2b5278",
                 "#182533", "#2b5278", "#5288c1", "#f5f5f5", "#7f91a4", "#ffffff",
                 "#0e1621", "#242f3d", "#3e546a", "#3e546a"};
    } else {
        m_pal = {"#ffffff", "#ffffff", "#e6ebee", "#ffffff", "#f1f1f1", "#419fd9",
                 "#ffffff", "#effdde", "#40a7e3", "#000000", "#8e979f", "#ffffff",
                 "#e6e6e6", "#f1f3f4", "#40a7e3", "#c6c9cc"};
    }
    emit changed();
}

QString ThemeManager::styleSheet() const {
    const Palette &p = m_pal;
    auto c = [](const QColor &col) { return col.name(); };
    return QString(R"(
QWidget { background: %1; color: %2; }
QMainWindow, QSplitter { background: %1; }
QSplitter::handle { background: %3; }
QListView { border: none; outline: none; background: %4; }
QLineEdit { background: %5; border: none; border-radius: 17px; padding: 7px 14px; selection-background-color: %6; }
QPlainTextEdit { background: %7; border: none; padding: 6px 4px; selection-background-color: %6; }
QToolButton { background: transparent; border: none; border-radius: 18px; padding: 6px; }
QToolButton:hover { background: %8; }
QLabel { background: transparent; }
QMenu { background: %7; border: 1px solid %3; padding: 4px; }
QMenu::item { padding: 7px 22px; border-radius: 4px; }
QMenu::item:selected { background: %8; }
QScrollBar:vertical { background: transparent; width: 8px; margin: 0; }
QScrollBar::handle:vertical { background: %9; border-radius: 4px; min-height: 30px; }
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: transparent; }
QToolTip { background: %7; color: %2; border: 1px solid %3; }
)").arg(c(p.windowBg), c(p.text), c(p.divider), c(p.sidebarBg), c(p.inputBg), c(p.accent), c(p.headerBg), c(p.itemHover), c(p.badgeMuted));
}
