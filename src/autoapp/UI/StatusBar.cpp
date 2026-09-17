/*
*  This file is part of openauto project.
*  (UI-1 head-unit overlay: clock + placeholders, click-transparent so the
*  AA touch path underneath keeps working.)
*/

#include <QHBoxLayout>
#include <QEvent>
#include <QTime>
#include <f1x/openauto/autoapp/UI/StatusBar.hpp>

namespace f1x
{
namespace openauto
{
namespace autoapp
{
namespace ui
{

StatusBar::StatusBar(QWidget* parent)
    // Qt::Tool: no taskbar entry. X11BypassWindowManagerHint: stay above
    // the fullscreen AA video window (a plain StayOnTop is stacked below
    // fullscreen by the WM). Input stays untouched via
    // WA_TransparentForMouseEvents (see below).
    : QWidget(parent, Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool | Qt::X11BypassWindowManagerHint)
    , labelClock_(new QLabel(this))
    , labelTemp_(new QLabel(QStringLiteral("--°C"), this))
    , labelSignal_(new QLabel(QStringLiteral("NO SIG"), this))
    , timer_(new QTimer(this))
{
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setFocusPolicy(Qt::NoFocus);

    // Fallback background until the external theme (assets/theme.qss) is
    // loaded; the QSS background rule overrides this palette cleanly.
    QPalette palette;
    palette.setColor(QPalette::Window, QColor(13, 13, 15, 180));
    setPalette(palette);
    setAutoFillBackground(true);

    labelClock_->setObjectName(QStringLiteral("labelClock"));
    labelTemp_->setObjectName(QStringLiteral("labelTemp"));
    labelSignal_->setObjectName(QStringLiteral("labelSignal"));

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(12, 0, 12, 0);
    layout->addWidget(labelClock_);
    layout->addStretch();
    layout->addWidget(labelTemp_);
    layout->addWidget(labelSignal_);

    connect(timer_, &QTimer::timeout, this, &StatusBar::updateClock);
    timer_->start(1000);
    this->updateClock();
}

void StatusBar::attachTo(QWidget* target)
{
    target_ = target;
    target->installEventFilter(this);
    this->syncGeometry();
    this->show();
}

bool StatusBar::eventFilter(QObject* obj, QEvent* event)
{
    if(obj == target_ && (event->type() == QEvent::Move || event->type() == QEvent::Resize))
    {
        this->syncGeometry();
    }
    return QWidget::eventFilter(obj, event);
}

void StatusBar::syncGeometry()
{
    const QPoint topLeft = target_->mapToGlobal(QPoint(0, 0));
    this->setGeometry(topLeft.x(), topLeft.y(), target_->width(), height_);
}

void StatusBar::updateClock()
{
    labelClock_->setText(QTime::currentTime().toString(QStringLiteral("HH:mm")));
}

}
}
}
}
