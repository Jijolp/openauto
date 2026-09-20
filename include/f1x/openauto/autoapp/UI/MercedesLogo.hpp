/*
*  This file is part of openauto project.
*  (UI-2b: Mercedes star — geometric, drawn with QPainter (no trademark
*  asset, vector, crisp at any scale). Outer + inner circles, 3 spokes
*  at 120°, thin stroke. Monochrome light grey, tintable red for active
*  states. Also used via assets/mercedes.svg (same geometry, file source).)
*/

#pragma once

#include <QColor>
#include <QWidget>

namespace f1x
{
namespace openauto
{
namespace autoapp
{
namespace ui
{

class MercedesLogo : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(QColor color READ color WRITE setColor)
    Q_PROPERTY(qreal scaleFactor READ scaleFactor WRITE setScaleFactor)
public:
    explicit MercedesLogo(QWidget* parent = nullptr, int baseSize = 96);
    void setColor(const QColor& c);
    QColor color() const;
    void setScaleFactor(qreal s);
    qreal scaleFactor() const;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QColor color_;
    qreal scale_;
    int baseSize_;
};

}
}
}
}
