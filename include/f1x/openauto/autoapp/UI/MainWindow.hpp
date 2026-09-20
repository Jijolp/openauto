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
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

    void showHomePage();
    void showAAPage();
    void showSettingsPage();

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
        SETTINGS_PAGE = 2
    };

    void applyTheme() const;
    QWidget* buildHomePage();
    QWidget* buildAAPage();
    QWidget* buildSettingsPage();
    void animateQuadrantsIn();
    void positionCenterLogo();

    StatusBar* statusBar_;
    QStackedWidget* stack_;
    QWidget* homePage_;
    QWidget* aaPage_;
    QWidget* settingsPage_;
    QLabel* aaPlaceholder_;
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

    // Misc
    QPushButton* nightButton_;
    bool night_;
    bool splashActive_;
};

}
}
}
}
