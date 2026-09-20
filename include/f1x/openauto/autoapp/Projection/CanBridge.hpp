/*
*  This file is part of openauto project.
*  Copyright (C) 2018 f1x.studio (Michal Szwaj)
*
*  openauto is free software: you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 3 of the License, or
*  (at your option) any later version.
*
*  You should have received a copy of the GNU General Public License
*  along with openauto. If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <atomic>
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <aasdk_proto/ButtonCodeEnum.pb.h>
#include <f1x/openauto/autoapp/Projection/IInputDeviceEventHandler.hpp>
#include <f1x/openauto/autoapp/Projection/InputEvent.hpp>

namespace f1x
{
namespace openauto
{
namespace autoapp
{
namespace projection
{

// One steering-wheel button: a masked byte of a CAN frame maps to an
// Android Auto keycode. Press/release are edge-detected (no auto-repeat).
struct CanButtonBinding
{
    std::string name;
    uint32_t canId = 0;
    uint8_t byteIndex = 0;
    uint8_t mask = 0xFF;
    uint8_t pressValue = 0;
    uint8_t releaseValue = 0;
    aasdk::proto::enums::ButtonCode::Enum aaButton = aasdk::proto::enums::ButtonCode::NONE;
};

// Event stubs below are log-only: nothing consumes them on PC yet
// (future: SensorService / driving-state / night mode).
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

struct CanMap
{
    std::vector<CanButtonBinding> buttons;
    CanIgnitionConfig ignition;
    CanSpeedConfig speed;
    CanNightConfig nightMode;
    CanTempConfig tempExt;
};

// CanBridge: socketcan reader thread translating CAN frames into actions.
// Buttons reuse the existing injection path (IInputDeviceEventHandler::
// onButtonEvent, i.e. InputService), events are log-only stubs.
// Mapping is 100 % external (car/can_map.json): real W203 IDs are plugged
// in via config, never via code changes.
class CanBridge
{
public:
    typedef std::shared_ptr<CanBridge> Pointer;
    typedef std::vector<aasdk::proto::enums::ButtonCode::Enum> ButtonCodes;

    CanBridge(IInputDeviceEventHandler& eventHandler, std::string interfaceName, std::string mapPath);
    ~CanBridge();

    CanBridge(const CanBridge&) = delete;
    CanBridge& operator=(const CanBridge&) = delete;

    void start();
    void stop();
    bool isRunning() const;

    // Extra sinks (e.g. the InputSourceService alongside the legacy
    // InputService): every button press/release is delivered to all.
    void addEventHandler(IInputDeviceEventHandler& eventHandler);

    static std::string interfaceFromEnv();
    static std::string mapPathFromEnv();
    static CanMap loadMap(const std::string& mapPath);
    // AA keycodes referenced by the map: the factory unions them into the
    // declared discovery keycodes so the phone binds them.
    static ButtonCodes requiredButtonCodes(const std::string& mapPath);

private:
    void run();
    bool openSocket();
    void closeSocket();
    void handleFrame(uint32_t canId, const uint8_t* data, uint8_t dlc);

    IInputDeviceEventHandler& eventHandler_;
    std::vector<IInputDeviceEventHandler*> extraHandlers_;
    std::string interfaceName_;
    std::string mapPath_;
    CanMap map_;

    std::thread thread_;
    mutable std::mutex mutex_;
    std::atomic<bool> running_;
    int socket_;
    std::map<size_t, bool> pressedState_;
    bool ignitionKnown_;
    bool ignitionOn_;
    bool nightKnown_;
    bool nightOn_;
    double lastSpeed_;
    bool tempKnown_;
    int lastTemp_;
};

}
}
}
}
