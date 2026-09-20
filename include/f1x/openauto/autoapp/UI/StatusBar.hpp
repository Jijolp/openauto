/*
*  This file is part of openauto project.
*  (UI-2b head-unit status band: clock + placeholders. Lives INSIDE the
*  single MainWindow, fixed above the page stack on every page.
*  UI-2b: shows real temp_ext when available, AA home button (48px) on
*  the right that returns to home without forwarding to the video.)
*/

#pragma once

#include <QLabel>
#include <QTimer>
#include <QWidget>

class QPushButton;

namespace f1x
{
namespace openauto
{
namespace autoapp
{
namespace ui
{

class MercedesLogo;

class StatusBar : public QWidget
{
    Q_OBJECT
public:
    explicit StatusBar(QWidget* parent = nullptr);

    void setNightMode(bool on);
    void setTemp(int tempC);
    void setTempPlaceholder();
    void setAaMode(bool on);

signals:
    void aaHomeClicked();

protected:
    void paintEvent(QPaintEvent* event) override;

private slots:
    void updateClock();

private:
    QLabel* labelClock_;
    QLabel* labelTemp_;
    QLabel* labelSignal_;
    QPushButton* aaButton_;
    MercedesLogo* aaLogo_;
    QTimer* timer_;
    bool night_;
    bool aaMode_;
};

}
}
}
}
