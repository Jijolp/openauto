/*
*  This file is part of openauto project.
*  (UI head-unit status band: unified bar implementation. See header.)
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
    , layout_(new QHBoxLayout(this))
    , labelClock_(new QLabel(this))
    , labelTemp_(new QLabel(QStringLiteral("--°"), this))
    , labelSignal_(new QLabel(QStringLiteral("NO SIG"), this))
    , backButton_(new QPushButton(this))
    , backLogo_(new MercedesLogo(backButton_, UiConstants::STATUS_BACK_ICON_SIZE))
    , titleLabel_(new QLabel(this))
    , timer_(new QTimer(this))
    , night_(false)
{
    setFocusPolicy(Qt::NoFocus);
    setFixedHeight(UiConstants::STATUS_BAR_HEIGHT);

    labelClock_->setObjectName(QStringLiteral("labelClock"));
    labelTemp_->setObjectName(QStringLiteral("labelTemp"));
    labelSignal_->setObjectName(QStringLiteral("labelSignal"));

    backButton_->setObjectName(QStringLiteral("statusBackButton"));
    backButton_->setFixedSize(UiConstants::STATUS_BACK_BUTTON_SIZE, UiConstants::STATUS_BACK_BUTTON_SIZE);
    backButton_->setFlat(true);
    backButton_->setFocusPolicy(Qt::NoFocus);
    backButton_->setCursor(Qt::PointingHandCursor);
    backLogo_->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    backLogo_->setColor(QColor(0xC8, 0xC8, 0xCC));
    backLogo_->move((backButton_->width() - backLogo_->width()) / 2,
                    (backButton_->height() - backLogo_->height()) / 2);
    connect(backButton_, &QPushButton::clicked, this, &StatusBar::backClicked);
    connect(backButton_, &QPushButton::pressed, this, [this]() { backLogo_->setActive(true); });
    connect(backButton_, &QPushButton::released, this, [this]() { backLogo_->setActive(false); });
    backButton_->hide();

    titleLabel_->setObjectName(QStringLiteral("statusTitle"));
    QFont titleFont(QStringLiteral("Inter"));
    titleFont.setPixelSize(UiConstants::STATUS_TITLE_FONT_SIZE);
    titleFont.setWeight(QFont::DemiBold);
    titleFont.setLetterSpacing(QFont::PercentageSpacing, UiConstants::QUADRANT_LABEL_SPACING_PCT);
    titleLabel_->setFont(titleFont);
    titleLabel_->hide();

    layout_->setContentsMargins(UiConstants::STATUS_BAR_MARGIN, 0,
                                UiConstants::STATUS_BAR_MARGIN, 0);
    layout_->setSpacing(8);
    // Home layout (unchanged): clock | temp | ... | NO SIG.
    layout_->addWidget(labelClock_);
    layout_->addWidget(labelTemp_);
    layout_->addStretch();
    layout_->addWidget(labelSignal_);

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

void StatusBar::setTitle(const QString& title)
{
    const bool titled = !title.isEmpty();
    if(titled)
    {
        titleLabel_->setText(title.toUpper());
    }
    backButton_->setVisible(titled);
    titleLabel_->setVisible(titled);
    labelSignal_->setVisible(!titled);
    this->relayout(titled);
}

void StatusBar::relayout(bool titled)
{
    // Reorder the same widgets (takeAt releases, never deletes widgets;
    // spacers are deleted). Home order is byte-identical to the ctor.
    QLayoutItem* item = nullptr;
    while((item = layout_->takeAt(0)) != nullptr)
    {
        if(item->widget() == nullptr)
        {
            delete item;
        }
    }
    if(!titled)
    {
        layout_->addWidget(labelClock_);
        layout_->addWidget(labelTemp_);
        layout_->addStretch();
        layout_->addWidget(labelSignal_);
        return;
    }
    layout_->addWidget(backButton_);
    layout_->addWidget(titleLabel_);
    layout_->addStretch();
    layout_->addWidget(labelTemp_);
    layout_->addWidget(labelClock_);
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
