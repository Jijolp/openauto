/*
*  This file is part of openauto project.
*  (UI head-unit status band: unified bar (§39). Home layout unchanged
*  (clock | temp | ... | NO SIG); titled pages (Race/Voiture/Paramètres)
*  show [back logo] title on the left and temp + clock on the right, no
*  signal. AA page keeps only its floating logo button (bar hidden).
*  Local per-page bandeaux were removed to free content space.)
*/

#pragma once

#include <QLabel>
#include <QTimer>
#include <QWidget>

class QHBoxLayout;
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
    // Empty title = home layout; non-empty = titled page layout.
    void setTitle(const QString& title);

signals:
    void backClicked();

protected:
    void paintEvent(QPaintEvent* event) override;

private slots:
    void updateClock();

private:
    void relayout(bool titled);

    QHBoxLayout* layout_;
    QLabel* labelClock_;
    QLabel* labelTemp_;
    QLabel* labelSignal_;
    QPushButton* backButton_;
    MercedesLogo* backLogo_;
    QLabel* titleLabel_;
    QTimer* timer_;
    bool night_;
};

}
}
}
}
