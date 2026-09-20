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

#include <thread>
#include <f1x/aasdk/USB/AOAPDevice.hpp>
#include <f1x/aasdk/TCP/TCPEndpoint.hpp>
#include <f1x/openauto/autoapp/App.hpp>
#include <f1x/openauto/Common/Log.hpp>

namespace f1x
{
namespace openauto
{
namespace autoapp
{

App::App(boost::asio::io_context& ioService, aasdk::usb::USBWrapper& usbWrapper, aasdk::tcp::ITCPWrapper& tcpWrapper, service::IAndroidAutoEntityFactory& androidAutoEntityFactory,
         aasdk::usb::IUSBHub::Pointer usbHub, aasdk::usb::IConnectedAccessoriesEnumerator::Pointer connectedAccessoriesEnumerator)
    : ioService_(ioService)
    , usbWrapper_(usbWrapper)
    , tcpWrapper_(tcpWrapper)
    , strand_(ioService_)
    , androidAutoEntityFactory_(androidAutoEntityFactory)
    , usbHub_(std::move(usbHub))
     , connectedAccessoriesEnumerator_(std::move(connectedAccessoriesEnumerator))
     , isStopped_(false)
     , hubWaitArmed_(false)
{

}

void App::waitForUSBDevice()
{
    boost::asio::dispatch(strand_, [this, self = this->shared_from_this()]() {
        this->waitForDevice();
        this->enumerateDevices();
    });
    OPENAUTO_LOG(debug) << "[App] waitForUSBDevice requested.";
}

void App::start(aasdk::tcp::ITCPEndpoint::SocketPointer socket)
{
    boost::asio::dispatch(strand_, [this, self = this->shared_from_this(), socket = std::move(socket)]() mutable {
        if(androidAutoEntity_ != nullptr)
        {
            tcpWrapper_.close(*socket);
            OPENAUTO_LOG(warning) << "[App] android auto entity is still running.";
            return;
        }

        try
        {
            usbHub_->cancel();
            connectedAccessoriesEnumerator_->cancel();

            auto tcpEndpoint(std::make_shared<aasdk::tcp::TCPEndpoint>(tcpWrapper_, std::move(socket)));
            androidAutoEntity_ = androidAutoEntityFactory_.create(std::move(tcpEndpoint));
            androidAutoEntity_->start(*this);
        }
        catch(const aasdk::error::Error& error)
        {
            OPENAUTO_LOG(error) << "[App] TCP AndroidAutoEntity create error: " << error.what();

            androidAutoEntity_.reset();
            this->waitForDevice();
        }
    });
}

void App::stop()
{
    boost::asio::dispatch(strand_, [this, self = this->shared_from_this()]() {
        isStopped_ = true;
        connectedAccessoriesEnumerator_->cancel();
        usbHub_->cancel();

        if(androidAutoEntity_ != nullptr)
        {
            androidAutoEntity_->stop();
            androidAutoEntity_.reset();
        }
    });
}

void App::aoapDeviceHandler(aasdk::usb::DeviceHandle deviceHandle)
{
    OPENAUTO_LOG(info) << "[App] Device connected.";
    hubWaitArmed_ = false;

    if(androidAutoEntity_ != nullptr)
    {
        OPENAUTO_LOG(warning) << "[App] android auto entity is still running.";
        return;
    }

    try
    {
        connectedAccessoriesEnumerator_->cancel();

        auto aoapDevice(aasdk::usb::AOAPDevice::create(usbWrapper_, ioService_, deviceHandle));
        androidAutoEntity_ = androidAutoEntityFactory_.create(std::move(aoapDevice));
        androidAutoEntity_->start(*this);
    }
    catch(const aasdk::error::Error& error)
    {
        OPENAUTO_LOG(error) << "[App] USB AndroidAutoEntity create error: " << error.what();

        androidAutoEntity_.reset();
        this->waitForDevice();
    }
}

void App::enumerateDevices()
{
    auto promise = aasdk::usb::IConnectedAccessoriesEnumerator::Promise::defer(strand_);
    promise->then([this, self = this->shared_from_this()](auto result) {
            OPENAUTO_LOG(info) << "[App] Devices enumeration result: " << result;
        },
        [this, self = this->shared_from_this()](auto e) {
            OPENAUTO_LOG(error) << "[App] Devices enumeration failed: " << e.what();
        });

    connectedAccessoriesEnumerator_->enumerate(std::move(promise));
}

void App::waitForDevice()
{
    // P0 (Error 30): never arm two hub waits — USBHub::start() rejects the
    // pending promise with OPERATION_ABORTED when called twice (double quit
    // → double wait). All callers run on the strand (promise handlers are
    // strand-posted, other callers dispatch), so the flag needs no lock.
    if(hubWaitArmed_)
    {
        OPENAUTO_LOG(warning) << "[App] hub wait already armed, skip duplicate.";
        return;
    }
    OPENAUTO_LOG(info) << "[App] Waiting for device...";

    auto promise = aasdk::usb::IUSBHub::Promise::defer(strand_);
    promise->then(std::bind(&App::aoapDeviceHandler, this->shared_from_this(), std::placeholders::_1),
                  std::bind(&App::onUSBHubError, this->shared_from_this(), std::placeholders::_1));
    usbHub_->start(std::move(promise));
    hubWaitArmed_ = true;
}

void App::onAndroidAutoQuit()
{
    boost::asio::dispatch(strand_, [this, self = this->shared_from_this()]() {
        OPENAUTO_LOG(info) << "[App] quit.";

        // A pre-session USB error can trigger quit with no entity (§13).
        if(androidAutoEntity_ != nullptr)
        {
            androidAutoEntity_->stop();
            androidAutoEntity_.reset();
        }

        if(!isStopped_)
        {
            this->waitForDevice();
        }
    });
}

void App::onUSBHubError(const aasdk::error::Error& error)
{
    OPENAUTO_LOG(error) << "[App] usb hub error: " << error.what();
    hubWaitArmed_ = false;

    // P0 (Error 30 = OPERATION_ABORTED): the old code dropped the wait here
    // and the app never re-armed the hub (bricked "Waiting", phone never
    // connects). Re-arm whenever no session owns the hub — the
    // hubWaitArmed_ guard above makes this converge (no start ping-pong):
    // a newer wait would have set the flag and this path would not run.
    if(!isStopped_ && androidAutoEntity_ == nullptr)
    {
        this->waitForDevice();
    }
}

}
}
}
