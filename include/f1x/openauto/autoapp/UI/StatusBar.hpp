/*
*  This file is part of openauto project.
*  (UI-2a head-unit status band: clock + placeholders. Lives INSIDE the
*  single MainWindow, fixed above the page stack on every page — no
*  longer a top-level overlay (the overlay was invisible under the
*  fullscreen video on Hyprland, and the video is now embedded anyway).
*  Still click-transparent so taps fall through to the page below.)
*/

#pragma once

#include <QLabel>
#include <QTimer>
#include <QWidget>

namespace f1x
{
namespace openauto
{
namespace autoapp
{
namespace ui
{

class StatusBar : public QWidget
{
    Q_OBJECT
public:
    // Embedded mode (parent != nullptr): plain child widget sized by the
    // MainWindow layout. The legacy top-level overlay flags are only
    // kept for the parentless fallback (unused by the app).
    explicit StatusBar(QWidget* parent = nullptr);

    void setNightMode(bool on);

protected:
    // Manual background paint (same as UI-1: translucent look also works
    // embedded; children labels stay styled by theme.qss).
    void paintEvent(QPaintEvent* event) override;

private slots:
    void updateClock();

private:
    QLabel* labelClock_;
    QLabel* labelTemp_;
    QLabel* labelSignal_;
    QTimer* timer_;
    bool night_;
};

}
}
}
}
