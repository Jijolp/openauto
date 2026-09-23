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
QWidget* HuEvents::statusBar_ = nullptr;
QWidget* HuEvents::aaOverlay_ = nullptr;
std::atomic<bool> HuEvents::phoneConnected_{false};
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

void HuEvents::notifySpeed(double kmh)
{
    emit instance().speedChanged(kmh);
}

void HuEvents::notifyRpm(double rpm)
{
    emit instance().rpmChanged(rpm);
}

void HuEvents::notifyPhoneConnected(bool connected)
{
    phoneConnected_.store(connected);
    emit instance().phoneConnectedChanged(connected);
}

void HuEvents::setPhoneConnected(bool connected)
{
    notifyPhoneConnected(connected);
}

bool HuEvents::isPhoneConnected()
{
    return phoneConnected_.load();
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

void HuEvents::setStatusBar(QWidget* bar)
{
    statusBar_ = bar;
}

bool HuEvents::isStatusBarChild(const QObject* obj)
{
    const QWidget* bar = statusBar_;
    if(bar == nullptr || obj == nullptr)
    {
        return false;
    }
    for(const QObject* o = obj; o != nullptr; o = o->parent())
    {
        if(o == bar)
        {
            return true;
        }
    }
    return false;
}

QRect HuEvents::statusBarGeometry()
{
    const QWidget* bar = statusBar_;
    if(bar == nullptr || !bar->isVisible())
    {
        return QRect();
    }
    return QRect(bar->mapToGlobal(QPoint(0, 0)), bar->size());
}

void HuEvents::setAaOverlay(QWidget* overlay)
{
    aaOverlay_ = overlay;
}

bool HuEvents::isAaOverlayChild(const QObject* obj)
{
    const QWidget* overlay = aaOverlay_;
    if(overlay == nullptr || obj == nullptr)
    {
        return false;
    }
    for(const QObject* o = obj; o != nullptr; o = o->parent())
    {
        if(o == overlay)
        {
            return true;
        }
    }
    return false;
}

QRect HuEvents::aaOverlayGeometry()
{
    const QWidget* overlay = aaOverlay_;
    if(overlay == nullptr || !overlay->isVisible())
    {
        return QRect();
    }
    return QRect(overlay->mapToGlobal(QPoint(0, 0)), overlay->size());
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
