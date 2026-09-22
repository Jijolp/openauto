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
    // Shared page bandeau: left title + right logo back button → home.
    // (Item4: pages start below the floating status overlay.)
    QWidget* buildPageBandeau(const QString& title, const char* backObjName);
    void animateQuadrantsIn();
    void positionCenterLogo();
    void layoutStatusOverlay();
    void layoutAaCluster();
    static QPushButton* makeLogoBackButton(QWidget* parent, int size, int iconSize, const char* objName);

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
    QLabel* aaClock_;
    QLabel* aaTemp_;
    QLabel* aaSignal_;
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
