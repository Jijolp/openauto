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
    // Logo only, no background/border — sized to match text height (~24px)
    // to blend with clock/temp (§42).
    const int logoSize = UiConstants::STATUS_BACK_ICON_SIZE;
    backButton_->setFixedSize(logoSize, logoSize);
    backButton_->setFlat(true);
    backButton_->setFocusPolicy(Qt::NoFocus);
    backButton_->setCursor(Qt::PointingHandCursor);
    backButton_->setStyleSheet(QStringLiteral("background:transparent;border:none;"));
    backLogo_->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    backLogo_->setColor(QColor(0xC8, 0xC8, 0xCC));
    backLogo_->move(0, 0);
    // Touch-first (§40): navigate on PRESS, not clicked.
    // No active state change for status bar logo (unlike central logo):
    // changing active state triggers repaint which conflicts with
    // race page fade animation (StackAll + opacity effect).
    connect(backButton_, &QPushButton::pressed, this, &StatusBar::backClicked);
    backButton_->hide();

    titleLabel_->setObjectName(QStringLiteral("statusTitle"));
    QFont titleFont(QStringLiteral("Inter"));
    titleFont.setPixelSize(UiConstants::STATUS_TITLE_FONT_SIZE);
    titleFont.setWeight(QFont::DemiBold);
    titleFont.setLetterSpacing(QFont::PercentageSpacing, UiConstants::QUADRANT_LABEL_SPACING_PCT);
    titleLabel_->setFont(titleFont);
    titleLabel_->hide();

    // 40px bar, content ~22px clock + 18px temp = ~24px: top margin 6px
    // centers vertically (§42: was hugging top edge).
    layout_->setContentsMargins(UiConstants::STATUS_BAR_MARGIN, 6,
                                UiConstants::STATUS_BAR_MARGIN, 0);
    layout_->setSpacing(8);
    // Home layout (unchanged): clock | temp | ... | NO SIG. Every child
    // explicitly V-centered (§40: default alignment hugged the top edge).
    layout_->addWidget(labelClock_, 0, Qt::AlignVCenter);
    layout_->addWidget(labelTemp_, 0, Qt::AlignVCenter);
    layout_->addStretch();
    layout_->addWidget(labelSignal_, 0, Qt::AlignVCenter);

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
    // Back button ALWAYS visible (logo only, no border/bg) in the status bar,
    // except when AA video is connected (handled by MainWindow calling hide()).
    const bool titled = !title.isEmpty();
    if(titled)
    {
        titleLabel_->setText(title.toUpper());
    }
    // backButton_ is always visible here; MainWindow hides the whole bar for AA video.
    backButton_->setVisible(true);
    titleLabel_->setVisible(titled);
    // Signal only in home layout (empty title).
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
        // Home layout: back logo | clock | temp | ... | NO SIG
        // Add small spacing between back logo and clock to avoid overlap.
        layout_->addWidget(backButton_, 0, Qt::AlignVCenter);
        layout_->addSpacing(10);
        layout_->addWidget(labelClock_, 0, Qt::AlignVCenter);
        layout_->addWidget(labelTemp_, 0, Qt::AlignVCenter);
        layout_->addStretch();
        layout_->addWidget(labelSignal_, 0, Qt::AlignVCenter);
        return;
    }
    layout_->addWidget(backButton_, 0, Qt::AlignVCenter);
    layout_->addWidget(titleLabel_, 0, Qt::AlignVCenter);
    layout_->addStretch();
    layout_->addWidget(labelTemp_, 0, Qt::AlignVCenter);
    layout_->addWidget(labelClock_, 0, Qt::AlignVCenter);
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
