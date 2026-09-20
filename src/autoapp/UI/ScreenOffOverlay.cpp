/*
*  This file is part of openauto project.
*  (UI-2b: screen-off overlay implementation.)
*/

#include <QPainter>
#include <QMouseEvent>
#include <f1x/openauto/autoapp/UI/MercedesLogo.hpp>
#include <f1x/openauto/autoapp/UI/ScreenOffOverlay.hpp>
#include <f1x/openauto/autoapp/UI/UiConstants.hpp>

namespace f1x
{
namespace openauto
{
namespace autoapp
{
namespace ui
{

ScreenOffOverlay::ScreenOffOverlay(QWidget* parent)
    : QWidget(parent)
    , logo_(new MercedesLogo(this, UiConstants::LOGO_SCREENOFF_SIZE))
{
    this->setAttribute(Qt::WA_StyledBackground, true);
    this->hide();
    logo_->setColor(QColor(0x3A, 0x3A, 0x3E));
    logo_->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    this->centerLogo();
}

void ScreenOffOverlay::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.fillRect(this->rect(), QColor(0, 0, 0));
}

void ScreenOffOverlay::mousePressEvent(QMouseEvent* event)
{
    event->accept();
    emit wakeRequested();
}

void ScreenOffOverlay::resizeEvent(QResizeEvent*)
{
    this->centerLogo();
}

void ScreenOffOverlay::centerLogo()
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
