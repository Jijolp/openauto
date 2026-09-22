/*
*  This file is part of openauto project.
*  (MISSION 1: CanBridge lifecycle manager implementation.)
*/

#include <algorithm>
#include <f1x/openauto/autoapp/Projection/CanManager.hpp>
#include <f1x/openauto/autoapp/UI/HuEvents.hpp>
#include <f1x/openauto/Common/Log.hpp>

#ifdef USE_CAN
#include <f1x/openauto/autoapp/Projection/CanBridge.hpp>
#endif

namespace f1x
{
namespace openauto
{
namespace autoapp
{
namespace projection
{

CanManager::CanManager(boost::asio::io_context& ioService)
    : ioService_(ioService)
{
#ifdef USE_CAN
    // Primary sink is *this (session-gated buttons); UI stubs (speed, rpm,
    // night, temp, ignition) flow via HuEvents straight from the bridge.
    // The bridge is a member: the reference stays valid for its lifetime.
    canBridge_ = std::make_shared<CanBridge>(*this,
                                             CanBridge::interfaceFromEnv(),
                                             CanBridge::mapPathFromEnv());
    OPENAUTO_LOG(info) << "[CanManager] created, CanBridge ready to start.";
#else
    OPENAUTO_LOG(info) << "[CanManager] created (USE_CAN=OFF, no-op).";
#endif
}

CanManager::~CanManager()
{
    stop();
}

void CanManager::start()
{
#ifdef USE_CAN
    // The bridge was built with *this as sink in the ctor: just start it.
    // (No recreation here — the old code bound a reference to a LOCAL
    // handler, dangling as soon as start() returned.)
    if(canBridge_ && !canBridge_->isRunning())
    {
        canBridge_->start();
        OPENAUTO_LOG(info) << "[CanManager] started CanBridge.";
    }
#else
    OPENAUTO_LOG(info) << "[CanManager] start (USE_CAN=OFF, no-op).";
#endif
}

void CanManager::onButtonEvent(const ButtonEvent& event)
{
#ifdef USE_CAN
    this->onCanButtonEvent(event);
#else
    (void)event;
#endif
}

void CanManager::onTouchEvent(const TouchEvent& event)
{
    (void)event;
}

void CanManager::stop()
{
#ifdef USE_CAN
    if (canBridge_ && canBridge_->isRunning())
    {
        canBridge_->stop();
        OPENAUTO_LOG(info) << "[CanManager] stopped CanBridge.";
    }
#else
    OPENAUTO_LOG(info) << "[CanManager] stop (USE_CAN=OFF, no-op).";
#endif
}

CanManager::RegistrationToken CanManager::registerButtonHandler(IInputDeviceEventHandler& handler)
{
#ifdef USE_CAN
    std::lock_guard<std::mutex> lock(handlersMutex_);
    RegistrationToken token{nextHandlerId_++};
    buttonHandlers_.emplace_back(token.id, &handler);

    // Also add to CanBridge's extra handlers so it receives events
    if (canBridge_)
    {
        canBridge_->addEventHandler(handler);
    }

    sessionActive_.store(true);
    OPENAUTO_LOG(info) << "[CanManager] registered button handler, token=" << token.id
                       << ", total=" << buttonHandlers_.size();
    return token;
#else
    OPENAUTO_LOG(debug) << "[CanManager] registerButtonHandler (USE_CAN=OFF, no-op).";
    return RegistrationToken{0};
#endif
}

void CanManager::unregisterButtonHandler(const RegistrationToken& token)
{
#ifdef USE_CAN
    std::lock_guard<std::mutex> lock(handlersMutex_);
    auto it = std::find_if(buttonHandlers_.begin(), buttonHandlers_.end(),
                           [&token](const auto& p) { return p.first == token.id; });
    if (it != buttonHandlers_.end())
    {
        buttonHandlers_.erase(it);
        OPENAUTO_LOG(info) << "[CanManager] unregistered button handler, token=" << token.id
                           << ", remaining=" << buttonHandlers_.size();
    }

    if (buttonHandlers_.empty())
    {
        sessionActive_.store(false);
    }
#else
    OPENAUTO_LOG(debug) << "[CanManager] unregisterButtonHandler (USE_CAN=OFF, no-op).";
#endif
}

bool CanManager::isRunning() const
{
#ifdef USE_CAN
    return canBridge_ && canBridge_->isRunning();
#else
    return false;
#endif
}

void CanManager::onCanButtonEvent(const ButtonEvent& event)
{
    // Forward to registered handlers ONLY when session is active
    if (sessionActive_.load())
    {
        std::lock_guard<std::mutex> lock(handlersMutex_);
        for (auto& p : buttonHandlers_)
        {
            p.second->onButtonEvent(event);
        }
    }
    else
    {
        // Debug log when buttons received but no session
        OPENAUTO_LOG(debug) << "[CanManager] button event dropped (no active session): "
                            << static_cast<int>(event.code)
                            << " " << (event.type == ButtonEventType::PRESS ? "PRESS" : "RELEASE");
    }
}

void CanManager::onCanIgnitionEvent(bool on)
{
    // ALWAYS forward to UI (night mode, screen off)
    ui::HuEvents::notifyIgnition(on);
}

void CanManager::onCanNightEvent(bool on)
{
    // ALWAYS forward to UI (theme switch)
    ui::HuEvents::notifyNightMode(on);
}

void CanManager::onCanTempEvent(int tempC)
{
    // ALWAYS forward to UI (status bar temp)
    ui::HuEvents::notifyTempExt(tempC);
}

// Static utility implementations (always available, delegate to CanBridge when USE_CAN)
std::string CanManager::interfaceFromEnv()
{
#ifdef USE_CAN
    return CanBridge::interfaceFromEnv();
#else
    return "vcan0";
#endif
}

std::string CanManager::mapPathFromEnv()
{
#ifdef USE_CAN
    return CanBridge::mapPathFromEnv();
#else
    return "car/can_map.json";
#endif
}

CanMap CanManager::loadMap(const std::string& mapPath)
{
#ifdef USE_CAN
    return CanBridge::loadMap(mapPath);
#else
    return CanMap{};
#endif
}

CanManagerButtonCodes CanManager::requiredButtonCodes(const std::string& mapPath)
{
#ifdef USE_CAN
    return CanBridge::requiredButtonCodes(mapPath);
#else
    return CanManagerButtonCodes{};
#endif
}

}
}
}
}
