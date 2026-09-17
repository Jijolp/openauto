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
    // Follows target moves/resizes (needed in windowed dev mode where the
    // WM places the window after show()). Never grabs input:
    // WA_TransparentForMouseEvents lets touch events reach AA video below.
    void attachTo(QWidget* target);

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private slots:
    void updateClock();

private:
    void syncGeometry();

    static constexpr int height_ = 40;

    QWidget* target_ = nullptr;
    QLabel* labelClock_;
    QLabel* labelTemp_;
    QLabel* labelSignal_;
    QTimer* timer_;
};

}
}
}
}
