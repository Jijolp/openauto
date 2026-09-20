/*
*  This file is part of openauto project.
*  (UI-2b: splash — black fullscreen, logo fade in + scale, hold, shrink
*  while quadrants fade in. Interruptible: AA connect jumps to final.
*  QPropertyAnimation + QGraphicsOpacityEffect, OPENAUTO_NO_SPLASH=1 skips.)
*/

#pragma once

#include <QWidget>

class QGraphicsOpacityEffect;
class QPropertyAnimation;
class QTimer;

namespace f1x
{
namespace openauto
{
namespace autoapp
{
namespace ui
{

class MercedesLogo;

class SplashOverlay : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(qreal logoScale READ logoScale WRITE setLogoScale)
public:
    explicit SplashOverlay(QWidget* parent = nullptr);
    void start();
    void interrupt();
    bool isActive() const;
    qreal logoScale() const;
    void setLogoScale(qreal s);

signals:
    void finished();
    void shrinkStarted();

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private slots:
    void onFadeInFinished();
    void onHoldTimeout();
    void onShrinkFinished();

private:
    void finishAndHide();
    void centerLogo();

    MercedesLogo* logo_;
    QGraphicsOpacityEffect* opacity_;
    QPropertyAnimation* fadeIn_;
    QTimer* holdTimer_;
    QPropertyAnimation* shrink_;
    bool active_;
    bool interrupted_;
};

}
}
}
}
