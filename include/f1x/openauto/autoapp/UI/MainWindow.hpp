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

#include <QMainWindow>

#include <vector>
#include <QMap>
#include <QPoint>
#include <QPropertyAnimation>

class QLabel;
class QPushButton;
class QStackedWidget;
class QGraphicsOpacityEffect;

namespace f1x
{
namespace openauto
{
namespace autoapp
{
namespace ui
{

class StatusBar;
class SplashOverlay;
class ScreenOffOverlay;
class MercedesLogo;
class NavPanel;
class GaugePanel;
class GForcePanel;
class GSim;

// UI-2b: Nothing/Mercedes design — splash + home quadrants around
// central logo + AA bandeau button + screen-off + auto-switch.
class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    // embeddedSettings: the existing OpenAuto SettingsWindow, reparented
    // into the settings stack page (P2 — real config, never a dead-end).
    // Null keeps the legacy placeholder (unit contexts).
    explicit MainWindow(QWidget* embeddedSettings = nullptr, QWidget *parent = nullptr);
    ~MainWindow() override;

    void showHomePage();
    void showAAPage();
    void showSettingsPage();
    void showRacePage();
    void showCarPage();

signals:
    void exit();
    void openSettings();
    void toggleCursor();
    void openConnectDialog();
    void stopAndroidAuto();

public slots:
    void setNightMode(bool on);

private slots:
    void onVideoStarted();
    void onVideoStopped();
    void onTempExt(int tempC);
    void onIgnition(bool on);
    void onSplashFinished();
    void onSplashShrinkStarted();
    void onScreenOffWake();
    void showScreenOff();
    void hideScreenOff();
    void returnHomeFromRace();
    void onStatusBack();

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    enum Page : int
    {
        HOME_PAGE = 0,
        AA_PAGE = 1,
        SETTINGS_PAGE = 2,
        RACE_PAGE = 3,
        CAR_PAGE = 4
    };

    void applyTheme() const;
    QWidget* buildHomePage();
    QWidget* buildAAPage();
    QWidget* buildSettingsPage();
    QWidget* buildRacePage();
    QWidget* buildCarPage();
    void animateQuadrantsIn();
    void positionCenterLogo();
    void layoutStatusOverlay();
    void layoutAaCluster();
    // Race Mode v1 transition (signature entry via MODE RACE quadrant).
    // OPENAUTO_NO_ANIM=1 skips straight to the page (dev).
    void startRaceTransition();
    void finishRaceEntry(int gen);
    void animateRacePanelsIn(int gen);
    void cancelRaceTransition();
    void directShowRace();
    void trackRaceAnim(QPropertyAnimation* anim);
    // Snap the central logo button back to its 180px medallion state
    // (geometry, fixed sizes, QSS style, opacity, logo color/rotation).
    void restoreCenterButton();

    StatusBar* statusBar_;
    QStackedWidget* stack_;
    QWidget* homePage_;
    QWidget* gridContainer_;
    QWidget* aaPage_;
    QWidget* settingsPage_;
    QWidget* racePage_;
    QWidget* carPage_;
    QLabel* aaPlaceholder_;
    // Mini-cluster AA flottant bas-droite (horloge + temp + signal +
    // bouton logo → accueil), enfant de la page AA, intégré à la barre AA.
    QWidget* aaCluster_;
    QPushButton* aaHomeButton_;
    MercedesLogo* aaHomeLogo_;
    SplashOverlay* splash_;
    ScreenOffOverlay* screenOff_;

    // Home quadrants
    QPushButton* quadrantAA_;
    QPushButton* quadrantRace_;
    QPushButton* quadrantCar_;
    QPushButton* quadrantParams_;
    QWidget* centerHit_;
    MercedesLogo* centerLogo_;
    std::vector<QGraphicsOpacityEffect*> quadrantEffects_;

    // Race Mode v1 page (nav 58% | gauge/G 42%) + G simulator.
    NavPanel* navPanel_;
    GaugePanel* gaugePanel_;
    GForcePanel* gforcePanel_;
    GSim* gsim_;
    QGraphicsOpacityEffect* navEffect_;
    QGraphicsOpacityEffect* gaugeEffect_;
    QGraphicsOpacityEffect* gforceEffect_;
    // Signature entry transition state. Bumped on every cancel/finish so
    // stale singleShots no-op; AA auto-switch always wins (cancel first).
    int raceTransitionGen_;
    bool raceTransitionActive_;
    bool aaSessionActive_;
    QMap<QWidget*, QPoint> raceQuadOrigPos_;
    // Central logo button geometry before the grow (fixed 180px released
    // for the animation, restored after — see startRaceTransition).
    QRect raceLogoOrigGeom_;
    std::vector<QPropertyAnimation*> raceAnims_;

    // P2: existing OpenAuto config embedded in the settings page.
    QWidget* embeddedSettings_;

    // Misc
    QPushButton* nightButton_;
    bool night_;
    bool splashActive_;
};

}
}
}
}
