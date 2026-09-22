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
    // Displayed in Paramètres → "Version logicielle". Bump per release so
    // a human can check the running binary is the latest build.
    static constexpr const char* APP_VERSION = "0.9.0-ui2b";

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
    // Visible gap between the quadrant inner edges and the logo medallion.
    // The inner cut radius is derived: LOGO_HOME_SIZE/2 + HOME_LOGO_GAP
    // (180/2+20 = 110), set per-button in code so QSS never hardcodes it.
    static constexpr int HOME_LOGO_GAP = 20;
    static constexpr int QUADRANT_OUTER_RADIUS = 6;
    // P3: quadrant labels sized for the surface (~32px @1024p, SemiBold),
    // SOON badge small and discreet. QFont pixel sizes set in code.
    static constexpr int QUADRANT_LABEL_FONT_SIZE = 32;
    static constexpr int QUADRANT_LABEL_SPACING_PCT = 112;
    static constexpr int QUADRANT_BADGE_FONT_SIZE = 11;
    static constexpr int QUADRANT_BADGE_SPACING_PCT = 160;
    static constexpr int QUADRANT_BADGE_MARGIN = 14;

    // Settings page local bandeau (same back-logo pattern as the AA page).
    static constexpr int SETTINGS_BACK_BUTTON_SIZE = 48;
    static constexpr int SETTINGS_BACK_ICON_SIZE = 26;

    // Central Mercedes logo (human SVG via QSvgRenderer, QPainter fallback).
    static constexpr int LOGO_HOME_SIZE = 180;
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
    // Mini-cluster AA flottant bas-droite (marge au bord, colle à la barre AA).
    static constexpr int AA_OVERLAY_MARGIN = 8;
    // Décalage du bouton logo vers la gauche pour ne pas chevaucher la
    // zone infos AA (5G/horloge en bas à droite). À ajuster visuellement.
    static constexpr int AA_FLOAT_SHIFT_LEFT = 140;
    // Floating AA home button (bottom-right over the video, blended with
    // AA's own bottom bar). No status bandeau on the AA page anymore.
    // Same size as the AA center buttons (~64px), icon scaled to match.
    static constexpr int AA_FLOAT_BUTTON_SIZE = 64;
    static constexpr int AA_FLOAT_ICON_SIZE = 34;
};

}
}
}
}
