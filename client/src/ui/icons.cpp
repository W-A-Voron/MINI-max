#include "ui/icons.h"

#include <QFile>
#include <QGuiApplication>
#include <QPainter>
#include <QSvgRenderer>

namespace Icons {

QPixmap pixmap(const QString &name, const QColor &color, int size) {
    QFile f(":/icons/" + name + ".svg");
    if (!f.open(QIODevice::ReadOnly)) return {};
    QByteArray svg = f.readAll();
    svg.replace("currentColor", color.name().toUtf8());
    const qreal dpr = qApp->devicePixelRatio();
    QPixmap pm(QSize(size, size) * dpr);
    pm.setDevicePixelRatio(dpr);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    QSvgRenderer(svg).render(&p, QRectF(0, 0, size, size));
    return pm;
}

QIcon icon(const QString &name, const QColor &color, int size) { return QIcon(pixmap(name, color, size)); }

}  // namespace Icons
