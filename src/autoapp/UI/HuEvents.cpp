/*
*  This file is part of openauto project.
*  (UI-2b: HuEvents bus implementation. See header for the contract.)
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
std::atomic<bool> HuEvents::splashActive_{false};
std::atomic<bool> HuEvents::screenOffActive_{false};

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

void HuEvents::notifyTempExt(int tempC)
{
    emit instance().tempExtChanged(tempC);
}

void HuEvents::notifyIgnition(bool on)
{
    emit instance().ignitionChanged(on);
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

void HuEvents::setSplashActive(bool active)
{
    splashActive_.store(active);
}

bool HuEvents::isSplashActive()
{
    return splashActive_.load();
}

void HuEvents::setScreenOffActive(bool active)
{
    screenOffActive_.store(active);
}

bool HuEvents::isScreenOffActive()
{
    return screenOffActive_.load();
}

}
}
}
}
