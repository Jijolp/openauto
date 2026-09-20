/*
*  This file is part of openauto project.
*  (UI-2b head-unit status band.)
*/

#include <QHBoxLayout>
#include <QPainter>
#include <QPushButton>
#include <QTime>
#include <f1x/openauto/autoapp/UI/MercedesLogo.hpp>
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
    : QWidget(parent)
    , labelClock_(new QLabel(this))
    , labelTemp_(new QLabel(QStringLiteral("--°"), this))
    , labelSignal_(new QLabel(QStringLiteral("NO SIG"), this))
    , aaButton_(new QPushButton(this))
    , aaLogo_(new MercedesLogo(aaButton_, UiConstants::LOGO_AA_ICON_SIZE))
    , timer_(new QTimer(this))
    , night_(false)
    , aaMode_(false)
{
    setFocusPolicy(Qt::NoFocus);
    setFixedHeight(UiConstants::STATUS_BAR_HEIGHT);

    labelClock_->setObjectName(QStringLiteral("labelClock"));
    labelTemp_->setObjectName(QStringLiteral("labelTemp"));
    labelSignal_->setObjectName(QStringLiteral("labelSignal"));

    aaButton_->setObjectName(QStringLiteral("aaHomeButton"));
    aaButton_->setFixedSize(UiConstants::LOGO_AA_BUTTON_SIZE, UiConstants::STATUS_BAR_HEIGHT);
    aaButton_->setFocusPolicy(Qt::NoFocus);
    aaButton_->setCursor(Qt::PointingHandCursor);
    aaLogo_->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    aaLogo_->setColor(QColor(0xC8, 0xC8, 0xCC));
    aaLogo_->move((aaButton_->width() - aaLogo_->width()) / 2,
                  (aaButton_->height() - aaLogo_->height()) / 2);
    connect(aaButton_, &QPushButton::clicked, this, &StatusBar::aaHomeClicked);
    aaButton_->setVisible(false);

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(UiConstants::STATUS_BAR_MARGIN, 0,
                               UiConstants::STATUS_BAR_MARGIN, 0);
    layout->setSpacing(8);
    layout->addWidget(labelClock_);
    layout->addWidget(labelTemp_);
    layout->addStretch();
    layout->addWidget(labelSignal_);
    layout->addWidget(aaButton_);

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

void StatusBar::setTemp(int tempC)
{
    labelTemp_->setText(QString::number(tempC) + QStringLiteral("°"));
}

void StatusBar::setTempPlaceholder()
{
    labelTemp_->setText(QStringLiteral("--°"));
}

void StatusBar::setAaMode(bool on)
{
    if(aaMode_ != on)
    {
        aaMode_ = on;
        aaButton_->setVisible(on);
    }
}

void StatusBar::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.fillRect(rect(), night_ ? QColor(0, 0, 0)
                                    : QColor(13, 13, 15));
}

void StatusBar::updateClock()
{
    labelClock_->setText(QTime::currentTime().toString(QStringLiteral("HH:mm")));
}

}
}
}
}
