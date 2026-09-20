/*
*  This file is part of openauto project.
*  (UI-2b fix: human-supplied assets/mercedes.svg rendered via
*  QSvgRenderer — re-rendered at every size (splash 180, home 180,
*  bandeau 26), never bitmap-scaled. Red active variant via
*  assets/mercedes-red.svg. QPainter geometric star kept as fallback
*  when the files are missing.)
*/

#pragma once

#include <memory>
#include <QColor>
#include <QWidget>

class QSvgRenderer;

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
    ~MercedesLogo() override;
    void setColor(const QColor& c);
    QColor color() const;
    // Red active variant (assets/mercedes-red.svg); grey otherwise.
    void setActive(bool active);
    bool isActive() const;
    void setScaleFactor(qreal s);
    qreal scaleFactor() const;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    void reloadSvg();
    void paintFallback(QPainter& p, qreal cx, qreal cy, qreal size);

    QColor color_;
    qreal scale_;
    int baseSize_;
    bool active_;
    std::unique_ptr<QSvgRenderer> renderer_;
};

}
}
}
}
