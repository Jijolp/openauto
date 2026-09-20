/*
*  This file is part of openauto project.
*  Copyright (C) 2018 f1x.studio (Michal Szwaj)
*
*  openauto is free software: you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 3 of the License, or
*  (at your option) any later version.

*  openauto is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with openauto. If not, see <http://www.gnu.org/licenses/>.
*/

#include <f1x/openauto/Common/Log.hpp>
#include <f1x/openauto/autoapp/Projection/IInputDeviceEventHandler.hpp>
#include <f1x/openauto/autoapp/Projection/InputDevice.hpp>
#include <f1x/openauto/autoapp/UI/HuEvents.hpp>

namespace f1x
{
namespace openauto
{
namespace autoapp
{
namespace projection
{

InputDevice::InputDevice(QObject& parent, configuration::IConfiguration::Pointer configuration, const QRect& touchscreenGeometry, const QRect& displayGeometry)
    : parent_(parent)
    , configuration_(std::move(configuration))
    , touchscreenGeometry_(touchscreenGeometry)
    , displayGeometry_(displayGeometry)
{
    this->moveToThread(parent.thread());
}

void InputDevice::start(IInputDeviceEventHandler& eventHandler)
{
    std::lock_guard<decltype(mutex_)> lock(mutex_);

    if(std::find(eventHandlers_.begin(), eventHandlers_.end(), &eventHandler) == eventHandlers_.end())
    {
        eventHandlers_.push_back(&eventHandler);
    }

    if(eventHandlers_.size() == 1)
    {
        OPENAUTO_LOG(info) << "[InputDevice] start (first subscriber, filter installed).";
        parent_.installEventFilter(this);
    }
    else
    {
        OPENAUTO_LOG(info) << "[InputDevice] start (filter already installed).";
    }
}

void InputDevice::stop()
{
    std::lock_guard<decltype(mutex_)> lock(mutex_);

    OPENAUTO_LOG(info) << "[InputDevice] stop.";
    parent_.removeEventFilter(this);
    eventHandlers_.clear();
}

bool InputDevice::eventFilter(QObject* obj, QEvent* event)
{
    std::lock_guard<decltype(mutex_)> lock(mutex_);

    if(!eventHandlers_.empty())
    {
        if(event->type() == QEvent::KeyPress || event->type() == QEvent::KeyRelease)
        {
            QKeyEvent* key = static_cast<QKeyEvent*>(event);
            if(!key->isAutoRepeat())
            {
                return this->handleKeyEvent(event, key);
            }
        }
        else if(event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonRelease || event->type() == QEvent::MouseMove)
        {
            return this->handleTouchEvent(event);
        }
    }

    return QObject::eventFilter(obj, event);
}

bool InputDevice::handleKeyEvent(QEvent* event, QKeyEvent* key)
{
    // UI-2a single window: off the AA page every key belongs to the HU
    // (clickable Home/Settings during a session) — never to the phone.
    if(!ui::HuEvents::isAaPageActive())
    {
        return false;
    }

    // UI-2a navigation keys: always reach the MainWindow shortcuts, never
    // the phone (retires the legacy keyboard scroll-wheel on 1/2).
    if(key->key() == Qt::Key_1 || key->key() == Qt::Key_2 || key->key() == Qt::Key_3)
    {
        return false;
    }

    auto eventType = event->type() == QEvent::KeyPress ? ButtonEventType::PRESS : ButtonEventType::RELEASE;
    aasdk::proto::enums::ButtonCode::Enum buttonCode;
    WheelDirection wheelDirection = WheelDirection::NONE;

    switch(key->key())
    {
    case Qt::Key_Return:
    case Qt::Key_Enter:
        buttonCode = aasdk::proto::enums::ButtonCode::ENTER;
        break;

    case Qt::Key_Left:
        buttonCode = aasdk::proto::enums::ButtonCode::LEFT;
        break;

    case Qt::Key_Right:
        buttonCode = aasdk::proto::enums::ButtonCode::RIGHT;
        break;

    case Qt::Key_Up:
        buttonCode = aasdk::proto::enums::ButtonCode::UP;
        break;

    case Qt::Key_Down:
        buttonCode = aasdk::proto::enums::ButtonCode::DOWN;
        break;

    case Qt::Key_Escape:
        buttonCode = aasdk::proto::enums::ButtonCode::BACK;
        break;

    case Qt::Key_H:
        buttonCode = aasdk::proto::enums::ButtonCode::HOME;
        break;

    case Qt::Key_P:
        buttonCode = aasdk::proto::enums::ButtonCode::PHONE;
        break;

    case Qt::Key_O:
        buttonCode = aasdk::proto::enums::ButtonCode::CALL_END;
        break;

    case Qt::Key_X:
        buttonCode = aasdk::proto::enums::ButtonCode::PLAY;
        break;

    case Qt::Key_C:
        buttonCode = aasdk::proto::enums::ButtonCode::PAUSE;
        break;

    case Qt::Key_MediaPrevious:
    case Qt::Key_V:
        buttonCode = aasdk::proto::enums::ButtonCode::PREV;
        break;

    case Qt::Key_MediaPlay:
    case Qt::Key_B:
        buttonCode = aasdk::proto::enums::ButtonCode::TOGGLE_PLAY;
        break;

    case Qt::Key_MediaNext:
    case Qt::Key_N:
        buttonCode = aasdk::proto::enums::ButtonCode::NEXT;
        break;

    case Qt::Key_M:
        buttonCode = aasdk::proto::enums::ButtonCode::MICROPHONE_1;
        break;

    default:
        return true;
    }

    const auto& buttonCodes = this->getSupportedButtonCodes();
    if(std::find(buttonCodes.begin(), buttonCodes.end(), buttonCode) != buttonCodes.end())
    {
        if(buttonCode != aasdk::proto::enums::ButtonCode::SCROLL_WHEEL || event->type() == QEvent::KeyRelease)
        {
            for(auto* handler : eventHandlers_)
            {
                handler->onButtonEvent({eventType, wheelDirection, buttonCode});
            }
        }
    }

    return true;
}

bool InputDevice::handleTouchEvent(QEvent* event)
{
    // UI-2a single window: off the AA page every click belongs to the HU
    // (keeps Home/Settings buttons clickable during a session and avoids
    // injecting garbage touches for taps outside the video).
    if(!ui::HuEvents::isAaPageActive())
    {
        return false;
    }

    if(!configuration_->getTouchscreenEnabled())
    {
        return true;
    }

    aasdk::proto::enums::TouchAction::Enum type;

    switch(event->type())
    {
    case QEvent::MouseButtonPress:
        type = aasdk::proto::enums::TouchAction::PRESS;
        break;
    case QEvent::MouseButtonRelease:
        type = aasdk::proto::enums::TouchAction::RELEASE;
        break;
    case QEvent::MouseMove:
        type = aasdk::proto::enums::TouchAction::DRAG;
        break;
    default:
        return true;
    };

    QMouseEvent* mouse = static_cast<QMouseEvent*>(event);
    if(event->type() == QEvent::MouseButtonRelease || mouse->buttons().testFlag(Qt::LeftButton))
    {
        // UI-2a: the video is an embedded widget now, so widget-local
        // pos() is no longer screen coordinates. Map GLOBAL position
        // into the host page rect, proportionally to the declared stream
        // geometry (exact like the S3 fullscreen mapping). Taps outside
        // the video (e.g. the status band) are left to the UI.
        // No host (legacy separate window): keep the historic formula.
        const QRect hostGeometry = ui::HuEvents::videoHostGeometry();
        uint32_t x = 0, y = 0;
        if(hostGeometry.isValid() && !hostGeometry.isEmpty())
        {
            const QPoint global = mouse->globalPos();
            if(!hostGeometry.contains(global))
            {
                return false;
            }
            x = (static_cast<float>(global.x() - hostGeometry.x()) / hostGeometry.width()) * displayGeometry_.width();
            y = (static_cast<float>(global.y() - hostGeometry.y()) / hostGeometry.height()) * displayGeometry_.height();
        }
        else
        {
            x = (static_cast<float>(mouse->pos().x()) / touchscreenGeometry_.width()) * displayGeometry_.width();
            y = (static_cast<float>(mouse->pos().y()) / touchscreenGeometry_.height()) * displayGeometry_.height();
        }
        for(auto* handler : eventHandlers_)
        {
            handler->onTouchEvent({type, x, y, 0});
        }
    }

    return true;
}

bool InputDevice::hasTouchscreen() const
{
    return configuration_->getTouchscreenEnabled();
}

QRect InputDevice::getTouchscreenGeometry() const
{
    // Declared to the phone as the touch surface: the video stream
    // geometry, matching the coordinates sent in onTouchEvent.
    return displayGeometry_;
}

IInputDevice::ButtonCodes InputDevice::getSupportedButtonCodes() const
{
    return configuration_->getButtonCodes();
}

}
}
}
}
