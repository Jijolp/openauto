/*
*  This file is part of openauto project.
*  (UI-2b: screen-off — opaque black overlay above everything (including
*  the status band). OLED = pixels really off. Any tap wakes, first tap
*  consumed (not forwarded to the UI underneath). Also triggered by
*  ignition_off from CAN. Pi provision will also cut the backlight — out
*  of scope here.)
*/

#pragma once

#include <QWidget>

namespace f1x
{
namespace openauto
{
namespace autoapp
{
namespace ui
{

class MercedesLogo;

class ScreenOffOverlay : public QWidget
{
    Q_OBJECT
public:
    explicit ScreenOffOverlay(QWidget* parent = nullptr);

signals:
    void wakeRequested();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    void centerLogo();
    MercedesLogo* logo_;
};

}
}
}
}
