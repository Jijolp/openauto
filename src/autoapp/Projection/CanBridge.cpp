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

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
#include <linux/can.h>
#include <linux/can/raw.h>

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <f1x/openauto/Common/Log.hpp>
#include <f1x/openauto/autoapp/Projection/CanBridge.hpp>
// UI-2a: one-way night_mode notification (CanBridge emits, UI consumes;
// no reverse coupling). This TU only builds with USE_CAN; HuEvents
// itself is CAN-agnostic so OFF builds stay green.
#include <f1x/openauto/autoapp/UI/HuEvents.hpp>

namespace f1x
{
namespace openauto
{
namespace autoapp
{
namespace projection
{

namespace
{

const char* cCanInterfaceEnvVar = "OPENAUTO_CAN_IF";
const char* cCanMapEnvVar = "OPENAUTO_CAN_MAP";
const char* cDefaultInterface = "vcan0";
const char* cDefaultMapPath = "car/can_map.json";

bool parseHex(const QJsonValue& value, uint32_t& out)
{
    bool ok = false;
    if(value.isString())
    {
        out = value.toString().toUInt(&ok, 0);
    }
    else if(value.isDouble())
    {
        out = static_cast<uint32_t>(value.toInt());
        ok = true;
    }
    return ok;
}

uint32_t parseHexOr(const QJsonObject& obj, const char* key, uint32_t fallback)
{
    uint32_t out = fallback;
    if(obj.contains(key))
    {
        parseHex(obj.value(key), out);
    }
    return out;
}

bool parseButtonCode(const QString& name, aasdk::proto::enums::ButtonCode::Enum& out)
{
    // Exhaustive for this aasdk proto snapshot (ButtonCodeEnum.proto).
    // NOTE: there are NO volume codes in this snapshot (no VOLUME_UP /
    // VOLUME_DOWN / PLAY_PAUSE): use TOGGLE_PLAY + NEXT / PREV instead.
    static const std::map<QString, aasdk::proto::enums::ButtonCode::Enum> table = {
        {"NONE", aasdk::proto::enums::ButtonCode::NONE},
        {"MICROPHONE_2", aasdk::proto::enums::ButtonCode::MICROPHONE_2},
        {"MENU", aasdk::proto::enums::ButtonCode::MENU},
        {"HOME", aasdk::proto::enums::ButtonCode::HOME},
        {"BACK", aasdk::proto::enums::ButtonCode::BACK},
        {"PHONE", aasdk::proto::enums::ButtonCode::PHONE},
        {"CALL_END", aasdk::proto::enums::ButtonCode::CALL_END},
        {"UP", aasdk::proto::enums::ButtonCode::UP},
        {"DOWN", aasdk::proto::enums::ButtonCode::DOWN},
        {"LEFT", aasdk::proto::enums::ButtonCode::LEFT},
        {"RIGHT", aasdk::proto::enums::ButtonCode::RIGHT},
        {"ENTER", aasdk::proto::enums::ButtonCode::ENTER},
        {"VOLUME_UP", aasdk::proto::enums::ButtonCode::VOLUME_UP},
        {"VOLUME_DOWN", aasdk::proto::enums::ButtonCode::VOLUME_DOWN},
        {"MICROPHONE_1", aasdk::proto::enums::ButtonCode::MICROPHONE_1},
        {"TOGGLE_PLAY", aasdk::proto::enums::ButtonCode::TOGGLE_PLAY},
        {"NEXT", aasdk::proto::enums::ButtonCode::NEXT},
        {"PREV", aasdk::proto::enums::ButtonCode::PREV},
        {"PLAY", aasdk::proto::enums::ButtonCode::PLAY},
        {"PAUSE", aasdk::proto::enums::ButtonCode::PAUSE},
        {"SCROLL_WHEEL", aasdk::proto::enums::ButtonCode::SCROLL_WHEEL},
    };

    const auto it = table.find(name.toUpper());
    if(it == table.end())
    {
        return false;
    }
    out = it->second;
    return true;
}

}

CanBridge::CanBridge(IInputDeviceEventHandler& eventHandler, std::string interfaceName, std::string mapPath)
    : eventHandler_(eventHandler)
    , interfaceName_(std::move(interfaceName))
    , mapPath_(std::move(mapPath))
    , map_(loadMap(mapPath_))
    , running_(false)
    , socket_(-1)
    , ignitionKnown_(false)
    , ignitionOn_(false)
    , nightKnown_(false)
    , nightOn_(false)
    , lastSpeed_(-1.0)
    , tempKnown_(false)
    , lastTemp_(0)
{
    OPENAUTO_LOG(info) << "[CanBridge] interface: " << interfaceName_
                       << ", map: " << mapPath_
                       << ", buttons: " << map_.buttons.size()
                       << ", ignition: " << (map_.ignition.present ? "yes" : "no")
                       << ", speed: " << (map_.speed.present ? "yes" : "no")
                       << ", night: " << (map_.nightMode.present ? "yes" : "no")
                       << ", temp: " << (map_.tempExt.present ? "yes" : "no");
}

CanBridge::~CanBridge()
{
    this->stop();
}

void CanBridge::start()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if(running_)
    {
        return;
    }
    running_ = true;
    thread_ = std::thread(&CanBridge::run, this);
    OPENAUTO_LOG(info) << "[CanBridge] started.";
}

void CanBridge::stop()
{
    const bool wasRunning = running_.exchange(false);
    this->closeSocket();
    std::lock_guard<std::mutex> lock(mutex_);
    if(thread_.joinable())
    {
        thread_.join();
    }
    if(wasRunning)
    {
        OPENAUTO_LOG(info) << "[CanBridge] stopped.";
    }
}

bool CanBridge::isRunning() const
{
    return running_;
}

void CanBridge::addEventHandler(IInputDeviceEventHandler& eventHandler)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if(std::find(extraHandlers_.begin(), extraHandlers_.end(), &eventHandler) == extraHandlers_.end())
    {
        extraHandlers_.push_back(&eventHandler);
    }
}

std::string CanBridge::interfaceFromEnv()
{
    if(const char* value = std::getenv(cCanInterfaceEnvVar))
    {
        if(value[0] != '\0')
        {
            return value;
        }
    }
    return cDefaultInterface;
}

std::string CanBridge::mapPathFromEnv()
{
    if(const char* value = std::getenv(cCanMapEnvVar))
    {
        if(value[0] != '\0')
        {
            return value;
        }
    }
    return cDefaultMapPath;
}

CanBridge::ButtonCodes CanBridge::requiredButtonCodes(const std::string& mapPath)
{
    ButtonCodes codes;
    const CanMap map = loadMap(mapPath);
    for(const auto& button : map.buttons)
    {
        if(std::find(codes.begin(), codes.end(), button.aaButton) == codes.end())
        {
            codes.push_back(button.aaButton);
        }
    }
    return codes;
}

CanMap CanBridge::loadMap(const std::string& mapPath)
{
    CanMap map;

    QFile file(QString::fromStdString(mapPath));
    if(!file.open(QIODevice::ReadOnly))
    {
        OPENAUTO_LOG(warning) << "[CanBridge] cannot open map file: " << mapPath
                              << " (buttons/events disabled).";
        return map;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    if(parseError.error != QJsonParseError::NoError || !doc.isObject())
    {
        OPENAUTO_LOG(warning) << "[CanBridge] malformed map file: " << mapPath
                              << ": " << parseError.errorString().toStdString()
                              << " (buttons/events disabled).";
        return map;
    }

    const QJsonObject root = doc.object();
    const QJsonArray buttons = root.value("buttons").toArray();
    for(const auto& entry : buttons)
    {
        const QJsonObject obj = entry.toObject();
        uint32_t canId = 0;
        if(!parseHex(obj.value("can_id"), canId))
        {
            OPENAUTO_LOG(warning) << "[CanBridge] button entry without valid can_id skipped.";
            continue;
        }

        aasdk::proto::enums::ButtonCode::Enum code = aasdk::proto::enums::ButtonCode::NONE;
        const QString codeName = obj.value("aa_button").toString();
        if(!parseButtonCode(codeName, code) || code == aasdk::proto::enums::ButtonCode::NONE)
        {
            OPENAUTO_LOG(warning) << "[CanBridge] button entry with unknown aa_button \""
                                  << codeName.toStdString() << "\" skipped.";
            continue;
        }

        CanButtonBinding binding;
        binding.name = obj.value("name").toString("unnamed").toStdString();
        binding.canId = canId;
        binding.byteIndex = static_cast<uint8_t>(parseHexOr(obj, "byte", 0));
        binding.mask = static_cast<uint8_t>(parseHexOr(obj, "mask", 0xFF));
        binding.pressValue = static_cast<uint8_t>(parseHexOr(obj, "press", 0));
        binding.releaseValue = static_cast<uint8_t>(parseHexOr(obj, "release", 0));
        binding.aaButton = code;
        map.buttons.push_back(std::move(binding));
    }

    if(root.contains("ignition") && root.value("ignition").isObject())
    {
        const QJsonObject obj = root.value("ignition").toObject();
        uint32_t canId = 0;
        if(parseHex(obj.value("can_id"), canId))
        {
            map.ignition.present = true;
            map.ignition.canId = canId;
            map.ignition.byteIndex = static_cast<uint8_t>(parseHexOr(obj, "byte", 0));
            map.ignition.mask = static_cast<uint8_t>(parseHexOr(obj, "mask", 0xFF));
            map.ignition.onValue = static_cast<uint8_t>(parseHexOr(obj, "on", 0));
            map.ignition.offValue = static_cast<uint8_t>(parseHexOr(obj, "off", 0));
        }
    }

    if(root.contains("speed") && root.value("speed").isObject())
    {
        const QJsonObject obj = root.value("speed").toObject();
        uint32_t canId = 0;
        if(parseHex(obj.value("can_id"), canId))
        {
            map.speed.present = true;
            map.speed.canId = canId;
            map.speed.byteIndex = static_cast<uint8_t>(parseHexOr(obj, "byte", 0));
            map.speed.factor = obj.value("factor").toDouble(1.0);
        }
    }

    if(root.contains("night_mode") && root.value("night_mode").isObject())
    {
        const QJsonObject obj = root.value("night_mode").toObject();
        uint32_t canId = 0;
        if(parseHex(obj.value("can_id"), canId))
        {
            map.nightMode.present = true;
            map.nightMode.canId = canId;
            map.nightMode.byteIndex = static_cast<uint8_t>(parseHexOr(obj, "byte", 0));
            map.nightMode.mask = static_cast<uint8_t>(parseHexOr(obj, "mask", 0xFF));
            map.nightMode.onValue = static_cast<uint8_t>(parseHexOr(obj, "on", 0));
        }
    }

    if(root.contains("temp_ext") && root.value("temp_ext").isObject())
    {
        const QJsonObject obj = root.value("temp_ext").toObject();
        uint32_t canId = 0;
        if(parseHex(obj.value("can_id"), canId))
        {
            map.tempExt.present = true;
            map.tempExt.canId = canId;
            map.tempExt.byteIndex = static_cast<uint8_t>(parseHexOr(obj, "byte", 0));
            map.tempExt.factor = obj.value("factor").toDouble(1.0);
            map.tempExt.offset = obj.value("offset").toDouble(0.0);
        }
    }

    return map;
}

void CanBridge::run()
{
    bool openFailedOnce = false;
    while(running_)
    {
        if(socket_ < 0 && !this->openSocket())
        {
            // Interface missing (e.g. vcan0 not created yet): retry quietly.
            if(!openFailedOnce)
            {
                OPENAUTO_LOG(warning) << "[CanBridge] cannot open " << interfaceName_
                                      << ", retrying until it appears.";
                openFailedOnce = true;
            }
            std::this_thread::sleep_for(std::chrono::seconds(2));
            continue;
        }
        openFailedOnce = false;

        struct can_frame frame;
        const ssize_t received = ::recv(socket_, &frame, sizeof(frame), 0);
        if(!running_)
        {
            break;
        }
        if(received < 0)
        {
            if(errno == EAGAIN || errno == EWOULDBLOCK)
            {
                continue;
            }
            OPENAUTO_LOG(warning) << "[CanBridge] recv error, reopening socket.";
            this->closeSocket();
            continue;
        }
        if(static_cast<size_t>(received) < sizeof(frame))
        {
            continue;
        }
        this->handleFrame(frame.can_id & CAN_SFF_MASK, frame.data, frame.can_dlc);
    }
    this->closeSocket();
}

bool CanBridge::openSocket()
{
    const int fd = ::socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if(fd < 0)
    {
        return false;
    }

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 500 * 1000;
    ::setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    struct ifreq ifr;
    std::memset(&ifr, 0, sizeof(ifr));
    std::strncpy(ifr.ifr_name, interfaceName_.c_str(), IFNAMSIZ - 1);
    if(::ioctl(fd, SIOCGIFINDEX, &ifr) < 0)
    {
        ::close(fd);
        return false;
    }

    struct sockaddr_can addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;
    if(::bind(fd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0)
    {
        ::close(fd);
        return false;
    }

    socket_ = fd;
    OPENAUTO_LOG(info) << "[CanBridge] listening on " << interfaceName_ << ".";
    return true;
}

void CanBridge::closeSocket()
{
    if(socket_ >= 0)
    {
        ::shutdown(socket_, SHUT_RDWR);
        ::close(socket_);
        socket_ = -1;
    }
}

void CanBridge::handleFrame(uint32_t canId, const uint8_t* data, uint8_t dlc)
{
    for(size_t i = 0; i < map_.buttons.size(); ++i)
    {
        const auto& binding = map_.buttons[i];
        if(binding.canId != canId || binding.byteIndex >= dlc)
        {
            continue;
        }

        const uint8_t value = data[binding.byteIndex] & binding.mask;
        const bool pressed = (value == (binding.pressValue & binding.mask));
        const bool released = (value == (binding.releaseValue & binding.mask));
        if(!pressed && !released)
        {
            continue;
        }

        const bool wasPressed = pressedState_[i];
        if(pressed == wasPressed)
        {
            continue;
        }
        pressedState_[i] = pressed;

        OPENAUTO_LOG(info) << "[CanBridge] button " << binding.name
                           << " (" << static_cast<int>(binding.aaButton) << ") "
                           << (pressed ? "PRESS" : "RELEASE");

        ButtonEvent event;
        event.type = pressed ? ButtonEventType::PRESS : ButtonEventType::RELEASE;
        event.wheelDirection = WheelDirection::NONE;
        event.code = binding.aaButton;
        eventHandler_.onButtonEvent(event);
        for(auto* handler : extraHandlers_)
        {
            handler->onButtonEvent(event);
        }
    }

    if(map_.ignition.present && map_.ignition.canId == canId && map_.ignition.byteIndex < dlc)
    {
        const uint8_t value = data[map_.ignition.byteIndex] & map_.ignition.mask;
        if(value == (map_.ignition.onValue & map_.ignition.mask) ||
           value == (map_.ignition.offValue & map_.ignition.mask))
        {
            const bool on = (value == (map_.ignition.onValue & map_.ignition.mask));
            if(!ignitionKnown_ || on != ignitionOn_)
            {
                ignitionKnown_ = true;
                ignitionOn_ = on;
                OPENAUTO_LOG(info) << "[CanBridge] ignition " << (on ? "ON" : "OFF");
                ui::HuEvents::notifyIgnition(on);
            }
        }
    }

    if(map_.speed.present && map_.speed.canId == canId && map_.speed.byteIndex < dlc)
    {
        const double speed = data[map_.speed.byteIndex] * map_.speed.factor;
        if(lastSpeed_ < 0.0 || std::fabs(speed - lastSpeed_) >= 1.0)
        {
            lastSpeed_ = speed;
            OPENAUTO_LOG(info) << "[CanBridge] speed " << speed << " km/h (GALA stub, not forwarded).";
        }
    }

    if(map_.nightMode.present && map_.nightMode.canId == canId && map_.nightMode.byteIndex < dlc)
    {
        const bool on = ((data[map_.nightMode.byteIndex] & map_.nightMode.mask) ==
                         (map_.nightMode.onValue & map_.nightMode.mask));
        if(!nightKnown_ || on != nightOn_)
        {
            nightKnown_ = true;
            nightOn_ = on;
            OPENAUTO_LOG(info) << "[CanBridge] night mode " << (on ? "ON" : "OFF");
            // UI-2a: first real CAN consumer — the single window re-themes
            // (read-only signal, no coupling back into this bridge).
            ui::HuEvents::notifyNightMode(on);
        }
    }

    if(map_.tempExt.present && map_.tempExt.canId == canId && map_.tempExt.byteIndex < dlc)
    {
        const int temp = static_cast<int>(std::lround(data[map_.tempExt.byteIndex] * map_.tempExt.factor + map_.tempExt.offset));
        if(!tempKnown_ || temp != lastTemp_)
        {
            tempKnown_ = true;
            lastTemp_ = temp;
            OPENAUTO_LOG(info) << "[CanBridge] temp_ext " << temp << "°C";
            ui::HuEvents::notifyTempExt(temp);
        }
    }
}

}
}
}
}
