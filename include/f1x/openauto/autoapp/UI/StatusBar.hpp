/*
*  This file is part of openauto project.
*  (UI-1 head-unit overlay: clock + placeholders, click-transparent so the
*  AA touch path underneath keeps working.)
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
    // Width is set by attachTo() from the main window geometry.
    explicit StatusBar(QWidget* parent = nullptr);

    // Positions this overlay on top of the screen (same x/y/width as the
    // primary screen, 40 px high) so it stays above the fullscreen AA
    // video window, which is a separate top-level window. Never grabs
    // input: WA_TransparentForMouseEvents lets touch events reach the
    // video below.
    void attachTo();

private slots:
    void updateClock();

private:
    static constexpr int height_ = 40;
    QLabel* labelClock_;
    QLabel* labelTemp_;
    QLabel* labelSignal_;
    QTimer* timer_;
};

}
}
}
}
