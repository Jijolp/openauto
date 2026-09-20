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

#pragma once

#include <cstdint>
#include <aasdk_proto/ButtonCodeEnum.pb.h>
#include <aasdk_proto/AndroidKeyCodeEnum.pb.h>

namespace f1x
{
namespace openauto
{
namespace autoapp
{
namespace projection
{

// Translation legacy ButtonCode -> Android keycode for the InputSource
// channel (AAP 1.6+). Most legacy values already ARE the Android numbers
// (UP/DOWN/LEFT/RIGHT/ENTER, SEARCH, media keys, rotary); this table makes
// the mapping explicit and documents the single gap (MICROPHONE_2).
// Returns KEYCODE_UNKNOWN (0) when there is no Android equivalent.
inline uint32_t toAndroidKeycode(aasdk::proto::enums::ButtonCode::Enum code)
{
    // Note: protobuf puts enumerators on the message class scope
    // (ButtonCode::HOME), not on the Enum type.
    using ButtonCode = aasdk::proto::enums::ButtonCode;
    using AndroidKeyCode = aasdk::proto::enums::AndroidKeyCode;

    switch(code)
    {
    case ButtonCode::NONE:
        return AndroidKeyCode::KEYCODE_UNKNOWN;
    case ButtonCode::MICROPHONE_2:
        return AndroidKeyCode::KEYCODE_UNKNOWN;
    case ButtonCode::MENU:
        return AndroidKeyCode::KEYCODE_MENU;
    case ButtonCode::HOME:
        return AndroidKeyCode::KEYCODE_HOME;
    case ButtonCode::BACK:
        return AndroidKeyCode::KEYCODE_BACK;
    case ButtonCode::PHONE:
        return AndroidKeyCode::KEYCODE_CALL;
    case ButtonCode::CALL_END:
        return AndroidKeyCode::KEYCODE_ENDCALL;
    case ButtonCode::UP:
        return AndroidKeyCode::KEYCODE_DPAD_UP;
    case ButtonCode::DOWN:
        return AndroidKeyCode::KEYCODE_DPAD_DOWN;
    case ButtonCode::LEFT:
        return AndroidKeyCode::KEYCODE_DPAD_LEFT;
    case ButtonCode::RIGHT:
        return AndroidKeyCode::KEYCODE_DPAD_RIGHT;
    case ButtonCode::ENTER:
        return AndroidKeyCode::KEYCODE_DPAD_CENTER;
    case ButtonCode::VOLUME_UP:
        return AndroidKeyCode::KEYCODE_VOLUME_UP;
    case ButtonCode::VOLUME_DOWN:
        return AndroidKeyCode::KEYCODE_VOLUME_DOWN;
    case ButtonCode::MICROPHONE_1:
        return AndroidKeyCode::KEYCODE_SEARCH;
    case ButtonCode::TOGGLE_PLAY:
        return AndroidKeyCode::KEYCODE_MEDIA_PLAY_PAUSE;
    case ButtonCode::NEXT:
        return AndroidKeyCode::KEYCODE_MEDIA_NEXT;
    case ButtonCode::PREV:
        return AndroidKeyCode::KEYCODE_MEDIA_PREVIOUS;
    case ButtonCode::PLAY:
        return AndroidKeyCode::KEYCODE_MEDIA_PLAY;
    case ButtonCode::PAUSE:
        return AndroidKeyCode::KEYCODE_MEDIA_PAUSE;
    case ButtonCode::SCROLL_WHEEL:
        return AndroidKeyCode::KEYCODE_ROTARY_CONTROLLER;
    default:
        return AndroidKeyCode::KEYCODE_UNKNOWN;
    }
}

}
}
}
}
