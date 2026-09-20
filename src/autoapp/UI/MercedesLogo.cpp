/*
*  This file is part of openauto project.
*  (UI-2b: Mercedes star painter.)
*/

#include <QtMath>
#include <QPainter>
#include <QPainterPath>
#include <f1x/openauto/autoapp/UI/MercedesLogo.hpp>

namespace f1x
{
namespace openauto
{
namespace autoapp
{
namespace ui
{

MercedesLogo::MercedesLogo(QWidget* parent, int baseSize)
    : QWidget(parent)
    , color_(QColor(0xC8, 0xC8, 0xCC))
    , scale_(1.0)
    , baseSize_(baseSize)
{
    this->setFixedSize(baseSize_, baseSize_);
    this->setAttribute(Qt::WA_TransparentForMouseEvents, false);
}

void MercedesLogo::setColor(const QColor& c)
{
    if(color_ != c)
    {
        color_ = c;
        this->update();
    }
}

QColor MercedesLogo::color() const
{
    return color_;
}

void MercedesLogo::setScaleFactor(qreal s)
{
    if(!qFuzzyCompare(scale_, s))
    {
        scale_ = s;
        this->update();
    }
}

qreal MercedesLogo::scaleFactor() const
{
    return scale_;
}

void MercedesLogo::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    // Center, optionally scaled (splash animation). Scale around center.
    const qreal w = this->width();
    const qreal h = this->height();
    const qreal cx = w / 2.0;
    const qreal cy = h / 2.0;
    p.translate(cx, cy);
    p.scale(scale_, scale_);
    p.translate(-cx, -cy);

    // Geometry in 100×100 viewBox logic scaled to widget size.
    const qreal size = qMin(w, h);
    const qreal strokeOuter = qMax(1.2, size * 0.016);
    const qreal strokeInner = qMax(1.0, size * 0.011);
    const qreal rOuter = size * 0.46;
    const qreal rInner = size * 0.38;

    p.setPen(QPen(color_, strokeOuter, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.setBrush(Qt::NoBrush);
    p.drawEllipse(QPointF(cx, cy), rOuter, rOuter);

    p.setPen(QPen(color_, strokeInner));
    p.drawEllipse(QPointF(cx, cy), rInner, rInner);

    // 3 spokes at 120°: up (-90°) and ±120° from it (30°, 150°).
    p.setPen(QPen(color_, strokeOuter, Qt::SolidLine, Qt::RoundCap));
    const qreal rEnd = rOuter;
    const struct { qreal deg; } spokes[3] = { {-90}, {30}, {150} };
    for(const auto& s : spokes)
    {
        const qreal rad = qDegreesToRadians(s.deg);
        const qreal x = cx + rEnd * qCos(rad);
        const qreal y = cy + rEnd * qSin(rad);
        p.drawLine(QPointF(cx, cy), QPointF(x, y));
    }
}

}
}
}
}
