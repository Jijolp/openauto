/*
*  This file is part of openauto project.
*  (UI-2a head-unit status band: clock + placeholders, click-transparent.)
*/

#include <QHBoxLayout>
#include <QPainter>
#include <QTime>
#include <f1x/openauto/autoapp/UI/StatusBar.hpp>
#include <f1x/openauto/autoapp/UI/UiConstants.hpp>

namespace f1x
{
namespace openauto
{
namespace autoapp
{
namespace ui
{

StatusBar::StatusBar(QWidget* parent)
    : QWidget(parent, parent != nullptr ? Qt::Widget
                                        : Qt::WindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool | Qt::X11BypassWindowManagerHint))
    , labelClock_(new QLabel(this))
    , labelTemp_(new QLabel(QStringLiteral("--°C"), this))
    , labelSignal_(new QLabel(QStringLiteral("NO SIG"), this))
    , timer_(new QTimer(this))
    , night_(false)
{
    // Never grabs input: taps fall through to the page below (the AA
    // touch path when the video page is shown).
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setFocusPolicy(Qt::NoFocus);
    setFixedHeight(UiConstants::STATUS_BAR_HEIGHT);

    labelClock_->setObjectName(QStringLiteral("labelClock"));
    labelTemp_->setObjectName(QStringLiteral("labelTemp"));
    labelSignal_->setObjectName(QStringLiteral("labelSignal"));

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(UiConstants::STATUS_BAR_MARGIN, 0,
                               UiConstants::STATUS_BAR_MARGIN, 0);
    layout->addWidget(labelClock_);
    layout->addStretch();
    layout->addWidget(labelTemp_);
    layout->addWidget(labelSignal_);

    connect(timer_, &QTimer::timeout, this, &StatusBar::updateClock);
    timer_->start(1000);
    this->updateClock();
}

void StatusBar::setNightMode(bool on)
{
    if(night_ != on)
    {
        night_ = on;
        this->update();
    }
}

void StatusBar::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.fillRect(rect(), night_ ? QColor(0, 0, 0, 220)
                                    : QColor(13, 13, 15, 180));
}

void StatusBar::updateClock()
{
    labelClock_->setText(QTime::currentTime().toString(QStringLiteral("HH:mm")));
}

}
}
}
}
