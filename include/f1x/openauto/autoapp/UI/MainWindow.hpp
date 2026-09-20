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

namespace f1x
{
namespace openauto
{
namespace autoapp
{
namespace ui
{

class StatusBar;

// UI-2a single window: one frameless fullscreen window holding a fixed
// status band above a QStackedWidget [Home | AA | Settings]. Navigation
// is show/hide only — the AA session NEVER stops (the GStreamer pipeline
// keeps decoding while its page is hidden; the video resumes on return).
// Home/Settings are themed placeholders; the real design lands in UI-2b.
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

    StatusBar* statusBar_;
    QStackedWidget* stack_;
    QWidget* aaPage_;
    QLabel* aaPlaceholder_;
    QPushButton* nightButton_;
    bool night_;
};

}
}
}
}
