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

    // Positions this overlay on top of target (same x/y/width, 40 px high).
    // Must be called after target is shown. Never grabs input:
    // WA_TransparentForMouseEvents lets touch events reach AA video below.
    void attachTo(const QWidget* target);

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
