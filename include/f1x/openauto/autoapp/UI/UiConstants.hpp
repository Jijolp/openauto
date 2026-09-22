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
    static constexpr const char* APP_VERSION = "0.9.0-race2";

    // Dev windowed geometry (OPENAUTO_WINDOWED=1), target head-unit 1024x600.
    static constexpr int WINDOWED_WIDTH = 1024;
    static constexpr int WINDOWED_HEIGHT = 600;

    // Status band fixed on top of the single window, above the stack,
    // present on every page (never an overlay on the video only).
    static constexpr int STATUS_BAR_HEIGHT = 40;
    static constexpr int STATUS_BAR_MARGIN = 12;
    // Unified bar (§39): titled pages show [back logo] TITLE left,
    // temp + clock right. Home layout unchanged.
    static constexpr int STATUS_TITLE_FONT_SIZE = 20;
    // Back hit area: full bar height, wide (touch-first: fat fingers +
    // release-slip must not miss it — see §40).
    static constexpr int STATUS_BACK_BUTTON_WIDTH = 48;
    static constexpr int STATUS_BACK_ICON_SIZE = 24;

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

    // --- Race Mode v1: layout (1024x600 dev, 1080p-ready via constantes) ---
    // Colonne nav ~58% largeur, colonne droite 42% (gauge haut / G bas).
    static constexpr int RACE_OUTER_MARGIN = 16;
    static constexpr int RACE_GRID_SPACING = 16;
    static constexpr int RACE_NAV_WIDTH_PCT = 58;
    static constexpr int RACE_PANEL_RADIUS = 6;
    // Vitesse + RPM : lignes ancrées à DROITE (l'unité ne bouge jamais,
    // la valeur grandit vers la gauche), unités collées aux valeurs et
    // de MÊME TAILLE (§41). Tailles calculées pour tenir sur une ligne
    // à 1024 (speed "130 km/h" ~4.3em, rpm "2500 tr/min" ~5.3em).
    static constexpr int RACE_SPEED_FONT_SIZE = 80;
    static constexpr int RACE_RPM_FONT_SIZE = 66;
    // Compte-tours : fond d'échelle pour le clamp d'affichage.
    static constexpr int RACE_RPM_MAX = 8000;
    // G-mètre : cercles 0.5g / 1.0g, point rouge, colonne chiffres.
    static constexpr int RACE_G_FONT_SIZE = 18;
    static constexpr int RACE_G_LABEL_FONT_SIZE = 12;
    static constexpr int RACE_NAV_LOGO_SIZE = 64;
    // --- Race Mode v1: timings transition (ms) ---
    // Logo central : 720° + gris→rouge + GROSSIT énormément (OutCubic),
    // PUIS fade out qui libère l'interface (panneaux entrent derrière).
    static constexpr int RACE_LOGO_SPIN_MS = 700;
    static constexpr double RACE_LOGO_GROW = 2.6;
    static constexpr int RACE_LOGO_FADE_MS = 300;
    // Quadrants : sortie + fade out, ease-in.
    static constexpr int RACE_QUAD_OUT_MS = 450;
    // Panneaux : entrée + fade in, stagger 80ms (pattern splash), ease-out.
    static constexpr int RACE_PANEL_IN_MS = 350;
    static constexpr int RACE_PANEL_STAGGER_MS = 80;
    // Retour menu : fade court (pas d'inverse complet — choix documenté).
    static constexpr int RACE_BACK_FADE_MS = 250;

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
    // Bouton logo seul : remonté pour être centré verticalement dans la
    // barre AA du bas, et collé à droite contre la zone infos (5G/horloge)
    // avec un petit espace. À ajuster visuellement si besoin.
    static constexpr int AA_FLOAT_BOTTOM_MARGIN = 14;
    static constexpr int AA_FLOAT_RIGHT_GAP = 100;
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
