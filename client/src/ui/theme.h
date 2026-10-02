#pragma once
#include <QColor>
#include <QObject>

// Colours follow Telegram Desktop's default "Night" and "Day" themes.
struct Palette {
    QColor windowBg, sidebarBg, chatBg, headerBg, itemHover, itemActive;
    QColor bubbleIn, bubbleOut, accent, text, textSecondary, textOnActive;
    QColor divider, inputBg, badge, badgeMuted;
};

class ThemeManager : public QObject {
    Q_OBJECT
public:
    static ThemeManager &instance();
    // mode: "dark" | "light" | "system"
    void setMode(const QString &mode);
    bool isDark() const { return m_dark; }
    const Palette &palette() const { return m_pal; }
    QString styleSheet() const;

signals:
    void changed();

private:
    ThemeManager();
    bool m_dark = true;
    Palette m_pal;
};

inline const Palette &pal() { return ThemeManager::instance().palette(); }
