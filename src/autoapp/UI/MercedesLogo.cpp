/*
*  This file is part of openauto project.
*  (UI-2b fix: human SVG via QSvgRenderer, QPainter fallback.)
*/

#include <QtMath>
#include <QCoreApplication>
#include <QFile>
#include <QPainter>
#include <QPainterPath>
#include <QSvgRenderer>
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
    , active_(false)
{
    this->setFixedSize(baseSize_, baseSize_);
    this->setAttribute(Qt::WA_TransparentForMouseEvents, false);
    this->reloadSvg();
}

// Defined here (not inline): QSvgRenderer is complete only in this TU,
// std::unique_ptr needs it at destruction.
MercedesLogo::~MercedesLogo() = default;

void MercedesLogo::setColor(const QColor& c)
{
    if(color_ != c)
    {
        color_ = c;
        // A clearly red tint selects the red SVG variant (keeps the
        // existing callers working unchanged); otherwise grey.
        const bool red = (c.red() >= 0xC0 && c.red() > c.green() + 0x40 && c.red() > c.blue() + 0x40);
        if(red != active_)
        {
            active_ = red;
            this->reloadSvg();
        }
        this->update();
    }
}

QColor MercedesLogo::color() const
{
    return color_;
}

void MercedesLogo::setActive(bool active)
{
    if(active_ != active)
    {
        active_ = active;
        this->reloadSvg();
        this->update();
    }
}

bool MercedesLogo::isActive() const
{
    return active_;
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

void MercedesLogo::reloadSvg()
{
    const QString fileName = active_ ? QStringLiteral("/../assets/mercedes-red.svg")
                                     : QStringLiteral("/../assets/mercedes.svg");
    const QString path = QCoreApplication::applicationDirPath() + fileName;
    if(QFile::exists(path))
    {
        renderer_ = std::make_unique<QSvgRenderer>(path, this);
        if(!renderer_->isValid())
        {
            renderer_.reset();
        }
    }
    else
    {
        renderer_.reset();
    }
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

    if(renderer_ != nullptr)
    {
        // Vector re-render at widget size — crisp at 26/120/180 alike,
        // never a scaled bitmap.
        renderer_->render(&p, QRectF(0.0, 0.0, w, h));
        return;
    }

    // Fallback (no asset files): geometric star, same spirit.
    const qreal size = qMin(w, h);
    this->paintFallback(p, cx, cy, size);
}

void MercedesLogo::paintFallback(QPainter& p, qreal cx, qreal cy, qreal size)
{
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
