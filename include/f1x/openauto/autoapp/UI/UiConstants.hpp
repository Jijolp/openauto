/*
*  This file is part of openauto project.
*  (UI-2a: centralized UI dimensions. No raw px outside these constants
*  in C++ UI code — prepares the 1080p port. QSS theme files keep their
*  own sizes: they ARE the centralized place for styling.)
*/

#pragma once

namespace f1x
{
namespace openauto
{
namespace autoapp
{
namespace ui
{

struct UiConstants
{
    // Dev windowed geometry (OPENAUTO_WINDOWED=1), target head-unit 1024x600.
    static constexpr int WINDOWED_WIDTH = 1024;
    static constexpr int WINDOWED_HEIGHT = 600;

    // Status band fixed on top of the single window, above the stack,
    // present on every page (never an overlay on the video only).
    static constexpr int STATUS_BAR_HEIGHT = 40;
    static constexpr int STATUS_BAR_MARGIN = 12;

    // Clock / placeholders font sizes (points are scaled by Qt, px in QSS).
    static constexpr int CLOCK_FONT_SIZE = 22;
    static constexpr int META_FONT_SIZE = 16;
    static constexpr int PAGE_TITLE_FONT_SIZE = 28;
};

}
}
}
}
