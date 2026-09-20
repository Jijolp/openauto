/*
*  This file is part of openauto project.
*  (UI-2b: splash implementation.)
*/

#include <QCoreApplication>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QTimer>
#include <QEasingCurve>
#include <QPainter>
#include <f1x/openauto/autoapp/UI/MercedesLogo.hpp>
#include <f1x/openauto/autoapp/UI/SplashOverlay.hpp>
#include <f1x/openauto/autoapp/UI/UiConstants.hpp>

namespace f1x
{
namespace openauto
{
namespace autoapp
{
namespace ui
{

SplashOverlay::SplashOverlay(QWidget* parent)
    : QWidget(parent)
    , logo_(new MercedesLogo(this, UiConstants::LOGO_SPLASH_SIZE))
    , opacity_(new QGraphicsOpacityEffect(this))
    , fadeIn_(new QPropertyAnimation(opacity_, "opacity", this))
    , holdTimer_(new QTimer(this))
    , shrink_(new QPropertyAnimation(this, "logoScale", this))
    , active_(false)
    , interrupted_(false)
{
    this->setAttribute(Qt::WA_StyledBackground, true);
    this->hide();
    logo_->setColor(QColor(0xE0, 0xE0, 0xE0));
    logo_->setScaleFactor(0.85);
    logo_->setGraphicsEffect(opacity_);
    opacity_->setOpacity(0.0);

    fadeIn_->setDuration(UiConstants::SPLASH_FADE_IN_MS);
    fadeIn_->setStartValue(0.0);
    fadeIn_->setEndValue(1.0);
    fadeIn_->setEasingCurve(QEasingCurve::OutCubic);
    connect(fadeIn_, &QPropertyAnimation::finished, this, &SplashOverlay::onFadeInFinished);

    holdTimer_->setSingleShot(true);
    connect(holdTimer_, &QTimer::timeout, this, &SplashOverlay::onHoldTimeout);

    shrink_->setDuration(UiConstants::SPLASH_SHRINK_MS);
    shrink_->setStartValue(1.0);
    shrink_->setEndValue(0.48);
    shrink_->setEasingCurve(QEasingCurve::InOutCubic);
    connect(shrink_, &QPropertyAnimation::finished, this, &SplashOverlay::onShrinkFinished);
}

void SplashOverlay::start()
{
    if(qEnvironmentVariableIsSet("OPENAUTO_NO_SPLASH"))
    {
        this->finishAndHide();
        return;
    }
    active_ = true;
    interrupted_ = false;
    this->show();
    this->raise();
    this->centerLogo();
    opacity_->setOpacity(0.0);
    logo_->setScaleFactor(0.85);
    // Animate both opacity and scale together: opacity via fadeIn_, scale via property on logo
    // We drive scale manually via a second animation? Simple: set scale to 1.0 with same duration/easing.
    // Use a temporary animation for logo scale 0.85→1.0 (same as fade).
    auto* scaleIn = new QPropertyAnimation(logo_, "scaleFactor", this);
    scaleIn->setDuration(UiConstants::SPLASH_FADE_IN_MS);
    scaleIn->setStartValue(0.85);
    scaleIn->setEndValue(1.0);
    scaleIn->setEasingCurve(QEasingCurve::OutCubic);
    connect(scaleIn, &QPropertyAnimation::finished, scaleIn, &QObject::deleteLater);
    scaleIn->start(QAbstractAnimation::DeleteWhenStopped);
    fadeIn_->start();
}

void SplashOverlay::interrupt()
{
    if(!active_)
    {
        return;
    }
    interrupted_ = true;
    fadeIn_->stop();
    holdTimer_->stop();
    shrink_->stop();
    // Jump to final visuals (quadrants already handled by MainWindow via shrinkStarted)
    this->finishAndHide();
}

bool SplashOverlay::isActive() const
{
    return active_;
}

qreal SplashOverlay::logoScale() const
{
    return logo_->scaleFactor();
}

void SplashOverlay::setLogoScale(qreal s)
{
    logo_->setScaleFactor(s);
}

void SplashOverlay::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.fillRect(this->rect(), QColor(0x0D, 0x0D, 0x0F));
}

void SplashOverlay::resizeEvent(QResizeEvent*)
{
    this->centerLogo();
}

void SplashOverlay::onFadeInFinished()
{
    if(interrupted_ || !active_)
    {
        return;
    }
    holdTimer_->start(UiConstants::SPLASH_HOLD_MS);
}

void SplashOverlay::onHoldTimeout()
{
    if(interrupted_ || !active_)
    {
        return;
    }
    emit shrinkStarted();
    shrink_->setStartValue(logo_->scaleFactor());
    shrink_->setEndValue(0.48);
    shrink_->start();
}

void SplashOverlay::onShrinkFinished()
{
    if(!active_)
    {
        return;
    }
    this->finishAndHide();
}

void SplashOverlay::finishAndHide()
{
    active_ = false;
    this->hide();
    emit finished();
}

void SplashOverlay::centerLogo()
{
    if(logo_ == nullptr)
    {
        return;
    }
    const int x = (this->width() - logo_->width()) / 2;
    const int y = (this->height() - logo_->height()) / 2;
    logo_->move(x, y);
}

}
}
}
}
