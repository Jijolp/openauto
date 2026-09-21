/*
*  This file is part of openauto project.
*  (MISSION 1: CanBridge lifecycle manager — owns CanBridge at app level,
*  starts at app launch, emits UI events always, forwards buttons only when
*  session is active. Decouples CAN from session lifecycle.)
*/

#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <vector>

#include <aasdk_proto/ButtonCodeEnum.pb.h>
#include <f1x/openauto/autoapp/Projection/IInputDeviceEventHandler.hpp>
#include <f1x/openauto/autoapp/UI/HuEvents.hpp>

#ifdef USE_CAN
#include <f1x/openauto/autoapp/Projection/CanBridge.hpp>
#else
// Mirror the types from CanBridge for when USE_CAN is OFF
struct CanIgnitionConfig
{
    bool present = false;
    uint32_t canId = 0;
    uint8_t byteIndex = 0;
    uint8_t mask = 0xFF;
    uint8_t onValue = 0;
    uint8_t offValue = 0;
};

struct CanSpeedConfig
{
    bool present = false;
    uint32_t canId = 0;
    uint8_t byteIndex = 0;
    double factor = 1.0;
};

struct CanNightConfig
{
    bool present = false;
    uint32_t canId = 0;
    uint8_t byteIndex = 0;
    uint8_t mask = 0xFF;
    uint8_t onValue = 0;
};

struct CanTempConfig
{
    bool present = false;
    uint32_t canId = 0;
    uint8_t byteIndex = 0;
    double factor = 1.0;
    double offset = 0.0;
};

struct CanButtonBinding
{
    std::string name;
    uint32_t canId = 0;
    uint8_t byteIndex = 0;
    uint8_t mask = 0xFF;
    uint8_t pressValue = 0;
    uint8_t releaseValue = 0;
    f1x::aasdk::proto::enums::ButtonCode_Enum aaButton = f1x::aasdk::proto::enums::ButtonCode_Enum::NONE;
};

struct CanMap
{
    std::vector<CanButtonBinding> buttons;
    CanIgnitionConfig ignition;
    CanSpeedConfig speed;
    CanNightConfig nightMode;
    CanTempConfig tempExt;
};

typedef std::vector<f1x::aasdk::proto::enums::ButtonCode_Enum> ButtonCodes;
#endif

namespace f1x
{
namespace openauto
{
namespace autoapp
{
namespace projection
{

// Type aliases for CanManager interface
#ifdef USE_CAN
using CanManagerButtonCodes = CanBridge::ButtonCodes;
#else
using CanManagerButtonCodes = std::vector<f1x::aasdk::proto::enums::ButtonCode_Enum>;
#endif

class CanManager
{
public:
    typedef std::shared_ptr<CanManager> Pointer;

    // Creates and starts CanBridge immediately (at app launch).
    // Requires HuEvents for UI notifications (night/temp/ignition).
    explicit CanManager(boost::asio::io_context& ioService);

    ~CanManager();

    // Start/stop CAN reading (called at app launch/exit).
    void start();
    void stop();

    // Register an input handler for button events (InputService/InputSourceService).
    // Called when AA session starts. Returns a token to unregister.
    struct RegistrationToken
    {
        size_t id;
    };
    RegistrationToken registerButtonHandler(IInputDeviceEventHandler& handler);

    // Unregister a button handler (called when AA session ends).
    void unregisterButtonHandler(const RegistrationToken& token);

    // Check if CAN is running.
    bool isRunning() const;

    // Static utilities for CAN configuration (always available, even without USE_CAN).
    static std::string interfaceFromEnv();
    static std::string mapPathFromEnv();
    static CanMap loadMap(const std::string& mapPath);
    static CanManagerButtonCodes requiredButtonCodes(const std::string& mapPath);

private:
    void onCanButtonEvent(const ButtonEvent& event);
    void onCanIgnitionEvent(bool on);
    void onCanNightEvent(bool on);
    void onCanTempEvent(int tempC);

    boost::asio::io_context& ioService_;
#ifdef USE_CAN
    CanBridge::Pointer canBridge_;
#else
    std::shared_ptr<void> canBridge_; // opaque pointer when USE_CAN is OFF
#endif
    std::atomic<bool> sessionActive_{false};
    std::mutex handlersMutex_;
    std::vector<std::pair<size_t, IInputDeviceEventHandler*>> buttonHandlers_;
    size_t nextHandlerId_{1};
};

}
}
}
}
