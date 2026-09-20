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

#include <chrono>
#include <aasdk_proto/InputSourceMessages.pb.h>
#include <f1x/openauto/Common/Log.hpp>
#include <f1x/openauto/autoapp/Service/InputSourceService.hpp>
#include <f1x/openauto/autoapp/Projection/InputSourceKeycodes.hpp>

namespace f1x
{
namespace openauto
{
namespace autoapp
{
namespace service
{

InputSourceService::InputSourceService(boost::asio::io_context& ioService, aasdk::messenger::IMessenger::Pointer messenger, projection::IInputDevice::Pointer inputDevice, InputBindingState::Pointer bindingState)
    : strand_(ioService)
    , channel_(std::make_shared<aasdk::channel::inputsource::InputSourceChannel>(strand_, std::move(messenger)))
    , inputDevice_(std::move(inputDevice))
    , bindingState_(std::move(bindingState))
    , suppressed_(false)
{

}

void InputSourceService::start()
{
    boost::asio::dispatch(strand_, [this, self = this->shared_from_this()]() {
        OPENAUTO_LOG(info) << "[InputSourceService] start.";
        channel_->receive(this->shared_from_this());
    });
}

void InputSourceService::stop()
{
    boost::asio::dispatch(strand_, [this, self = this->shared_from_this()]() {
        OPENAUTO_LOG(info) << "[InputSourceService] stop.";
        inputDevice_->stop();
    });
}

void InputSourceService::fillFeatures(aasdk::proto::messages::ServiceDiscoveryResponse& response)
{
    OPENAUTO_LOG(info) << "[InputSourceService] fill features.";

    auto* channelDescriptor = response.add_channels();
    channelDescriptor->set_channel_id(static_cast<uint32_t>(channel_->getId()));

    auto* inputSourceChannel = channelDescriptor->mutable_input_source_channel();

    const auto& supportedButtonCodes = inputDevice_->getSupportedButtonCodes();

    for(const auto& buttonCode : supportedButtonCodes)
    {
        const uint32_t androidCode = projection::toAndroidKeycode(buttonCode);
        if(androidCode != aasdk::proto::enums::AndroidKeyCode::KEYCODE_UNKNOWN)
        {
            inputSourceChannel->add_keycodes_supported(static_cast<int32_t>(androidCode));
        }
    }

    if(inputDevice_->hasTouchscreen())
    {
        // Dynamic stream-video geometry, same units as the sent coordinates.
        const auto& touchscreenSurface = inputDevice_->getTouchscreenGeometry();
        auto touchscreenConfig = inputSourceChannel->add_touchscreen();
        touchscreenConfig->set_width(touchscreenSurface.width());
        touchscreenConfig->set_height(touchscreenSurface.height());
    }
}

void InputSourceService::onChannelOpenRequest(const aasdk::proto::messages::ChannelOpenRequest& request)
{
    // Log the received channel id: plan B if a phone hardcodes 8 instead of
    // our declared 9 (validated design Q2).
    OPENAUTO_LOG(info) << "[InputSourceService] open request, priority: " << request.priority()
                       << ", channel id: " << request.channel_id();
    const aasdk::proto::enums::Status::Enum status = aasdk::proto::enums::Status::OK;
    OPENAUTO_LOG(info) << "[InputSourceService] open status: " << status;

    aasdk::proto::messages::ChannelOpenResponse response;
    response.set_status(status);

    // S3 verified: the phone binds (KeyBindingRequest) ~ms after open, so
    // subscription stays binding-gated like the reference (subscribe-on-open
    // was a diagnostic step, reverted after the fix was proven elsewhere).

    auto promise = aasdk::channel::SendPromise::defer(strand_);
    promise->then([]() {}, std::bind(&InputSourceService::onChannelError, this->shared_from_this(), std::placeholders::_1));
    channel_->sendChannelOpenResponse(response, std::move(promise));

    channel_->receive(this->shared_from_this());
}

void InputSourceService::onKeyBindingRequest(const aasdk::proto::messages::KeyBindingRequest& request)
{
    OPENAUTO_LOG(info) << "[InputSourceService] key binding request, keycodes count: " << request.keycodes_size();

    int32_t status = aasdk::proto::messages::KEY_BINDING_SUCCESS;
    const auto& supportedButtonCodes = inputDevice_->getSupportedButtonCodes();

    for(int i = 0; i < request.keycodes_size(); ++i)
    {
        bool found = false;
        for(const auto& buttonCode : supportedButtonCodes)
        {
            if(static_cast<int32_t>(projection::toAndroidKeycode(buttonCode)) == request.keycodes(i))
            {
                found = true;
                break;
            }
        }

        if(!found)
        {
            OPENAUTO_LOG(error) << "[InputSourceService] key binding request, keycode: " << request.keycodes(i)
                                << " is not supported.";

            status = aasdk::proto::messages::KEY_BINDING_KEYCODE_NOT_BOUND;
            break;
        }
    }

    aasdk::proto::messages::KeyBindingResponse response;
    response.set_status(status);

    if(status == aasdk::proto::messages::KEY_BINDING_SUCCESS)
    {
        bindingState_->inputSourceBound = true;
        if(bindingState_->legacyBound)
        {
            suppressed_ = true;
            OPENAUTO_LOG(info) << "[InputSourceService] suppressed: legacy channel bound first.";
        }
        else
        {
            inputDevice_->start(*this);
        }
    }

    OPENAUTO_LOG(info) << "[InputSourceService] key binding request, status: " << status;

    auto promise = aasdk::channel::SendPromise::defer(strand_);
    promise->then([]() {}, std::bind(&InputSourceService::onChannelError, this->shared_from_this(), std::placeholders::_1));
    channel_->sendKeyBindingResponse(response, std::move(promise));
    channel_->receive(this->shared_from_this());
}

void InputSourceService::onChannelError(const aasdk::error::Error& e)
{
    OPENAUTO_LOG(error) << "[InputSourceService] channel error: " << e.what();
}

void InputSourceService::onButtonEvent(const projection::ButtonEvent& event)
{
    if(suppressed_)
    {
        OPENAUTO_LOG(debug) << "[InputSourceService] button event dropped (legacy channel active).";
        return;
    }

    OPENAUTO_LOG(info) << "[InputSourceService] button event, code: " << static_cast<int>(event.code)
                       << ", type: " << static_cast<int>(event.type);
    auto timestamp = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch());

    boost::asio::dispatch(strand_, [this, self = this->shared_from_this(), event = std::move(event), timestamp = std::move(timestamp)]() {
        aasdk::proto::inputsource::InputReport inputReport;
        inputReport.set_timestamp(timestamp.count());

        const uint32_t androidCode = projection::toAndroidKeycode(event.code);

        if(event.code == aasdk::proto::enums::ButtonCode::SCROLL_WHEEL)
        {
            auto relativeEvent = inputReport.mutable_relative_event()->add_data();
            relativeEvent->set_delta(event.wheelDirection == projection::WheelDirection::LEFT ? -1 : 1);
            relativeEvent->set_keycode(androidCode);
        }
        else
        {
            auto keyEvent = inputReport.mutable_key_event()->add_keys();
            keyEvent->set_metastate(0);
            keyEvent->set_down(event.type == projection::ButtonEventType::PRESS);
            keyEvent->set_longpress(false);
            keyEvent->set_keycode(androidCode);
        }

        auto promise = aasdk::channel::SendPromise::defer(strand_);
        promise->then([]() {}, std::bind(&InputSourceService::onChannelError, this->shared_from_this(), std::placeholders::_1));
        channel_->sendInputReport(inputReport, std::move(promise));
    });
}

void InputSourceService::onTouchEvent(const projection::TouchEvent& event)
{
    if(suppressed_)
    {
        OPENAUTO_LOG(debug) << "[InputSourceService] touch event dropped (legacy channel active).";
        return;
    }

    OPENAUTO_LOG(info) << "[InputSourceService] touch event, action: " << event.type
                       << ", x: " << event.x << ", y: " << event.y;
    auto timestamp = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch());

    boost::asio::dispatch(strand_, [this, self = this->shared_from_this(), event = std::move(event), timestamp = std::move(timestamp)]() {
        aasdk::proto::inputsource::InputReport inputReport;
        inputReport.set_timestamp(timestamp.count());

        auto touchEvent = inputReport.mutable_touch_event();
        switch(event.type)
        {
        case aasdk::proto::enums::TouchAction::PRESS:
            touchEvent->set_action(aasdk::proto::inputsource::ACTION_DOWN);
            break;
        case aasdk::proto::enums::TouchAction::RELEASE:
            touchEvent->set_action(aasdk::proto::inputsource::ACTION_UP);
            break;
        default:
            touchEvent->set_action(aasdk::proto::inputsource::ACTION_MOVED);
            break;
        }

        // Phase 2a: single-touch only.
        touchEvent->set_action_index(0);
        auto touchLocation = touchEvent->add_pointer_data();
        touchLocation->set_x(event.x);
        touchLocation->set_y(event.y);
        touchLocation->set_pointer_id(0);

        auto promise = aasdk::channel::SendPromise::defer(strand_);
        promise->then([]() {}, std::bind(&InputSourceService::onChannelError, this->shared_from_this(), std::placeholders::_1));
        channel_->sendInputReport(inputReport, std::move(promise));
    });
}

}
}
}
}
