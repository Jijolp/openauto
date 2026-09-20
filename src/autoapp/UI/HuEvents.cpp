/*
*  This file is part of openauto project.
*  (UI-2a: HuEvents bus implementation. See header for the contract.)
*/

#include <QWidget>
#include <f1x/openauto/autoapp/UI/HuEvents.hpp>

namespace f1x
{
namespace openauto
{
namespace autoapp
{
namespace ui
{

QWidget* HuEvents::videoHost_ = nullptr;
std::atomic<bool> HuEvents::aaPageActive_{false};

HuEvents::HuEvents()
    : QObject(nullptr)
{
}

HuEvents& HuEvents::instance()
{
    static HuEvents bus;
    return bus;
}

void HuEvents::notifyNightMode(bool on)
{
    emit instance().nightModeChanged(on);
}

void HuEvents::notifyVideoStarted()
{
    emit instance().videoStarted();
}

void HuEvents::notifyVideoStopped()
{
    emit instance().videoStopped();
}

void HuEvents::setVideoHost(QWidget* host)
{
    videoHost_ = host;
}

QWidget* HuEvents::videoHost()
{
    return videoHost_;
}

QRect HuEvents::videoHostGeometry()
{
    QWidget* host = videoHost_;
    if(host == nullptr)
    {
        return QRect();
    }
    return QRect(host->mapToGlobal(QPoint(0, 0)), host->size());
}

void HuEvents::setAaPageActive(bool active)
{
    aaPageActive_.store(active);
}

bool HuEvents::isAaPageActive()
{
    return aaPageActive_.load();
}

}
}
}
}
