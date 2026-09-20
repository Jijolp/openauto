/*
*  This file is part of openauto project.
*  (UI-2b: centralized UI dimensions. No raw px outside these constants
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

    // --- UI-2b: home quadrants around central logo ---
    static constexpr int HOME_OUTER_MARGIN = 16;
    static constexpr int HOME_GRID_SPACING = 16;
    static constexpr int QUADRANT_INNER_RADIUS = 70;
    static constexpr int QUADRANT_OUTER_RADIUS = 6;
    static constexpr int QUADRANT_LABEL_FONT_SIZE = 13;
    static constexpr int QUADRANT_BADGE_FONT_SIZE = 10;

    // Central Mercedes logo (geometry hand-drawn, tinted at runtime).
    static constexpr int LOGO_HOME_SIZE = 96;
    static constexpr int LOGO_SPLASH_SIZE = 180;
    static constexpr int LOGO_AA_BUTTON_SIZE = 48;
    static constexpr int LOGO_AA_ICON_SIZE = 26;
    static constexpr int LOGO_SCREENOFF_SIZE = 120;

    // Splash timings (all in ms, sequential).
    static constexpr int SPLASH_FADE_IN_MS = 600;
    static constexpr int SPLASH_HOLD_MS = 800;
    static constexpr int SPLASH_SHRINK_MS = 500;
    static constexpr int SPLASH_STAGGER_MS = 80;
    static constexpr int SCREENOFF_FADE_MS = 250;

    // AA bandeau inner (logo button hit area, present inside StatusBar).
    static constexpr int AA_BANDEAU_HEIGHT = 40;
};

}
}
}
}
