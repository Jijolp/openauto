/*
*  This file is part of openauto project.
*  (UI-2b: single-window event bus. Decouples AA backend threads from the
*  UI without touching any protocol/service logic: see 2a header plus
*   - temp_ext: CanBridge stub → StatusBar (placeholder "--°" on PC, real
*     W203 outside temp later without touching the UI).
*   - ignition_off: CanBridge → ScreenOffOverlay (1 line synergy).
*   - splashActive / screenOffActive atomic flags read by InputDevice to
*     shield touches (first tap consumed).
*  Emitting from worker threads is safe: receivers live in GUI thread,
*  Qt::AutoConnection delivers queued across threads.)
*/

#pragma once

#include <atomic>
#include <QObject>
#include <QRect>

class QWidget;

namespace f1x
{
namespace openauto
{
namespace autoapp
{
namespace ui
{

class HuEvents : public QObject
{
    Q_OBJECT
public:
    static HuEvents& instance();

    // --- one-way notifications (emit = fire and forget) ---
    static void notifyNightMode(bool on);
    static void notifyTempExt(int tempC);
    static void notifyIgnition(bool on);
    static void notifyVideoStarted();
    static void notifyVideoStopped();

    // --- video host page (owned by MainWindow, app lifetime) ---
    static void setVideoHost(QWidget* host);
    static QWidget* videoHost();
    // Global geometry of the host widget. GUI thread only (reads QWidget).
    static QRect videoHostGeometry();

    // --- floating status overlay (owned by MainWindow, app lifetime) ---
    // Taps landing on the bar (e.g. the AA logo button) belong to the HU:
    // InputDevice must not forward them to the phone (the bar now floats
    // INSIDE the video host rect since the overlay change). GUI thread only.
    static void setStatusBar(QWidget* bar);
    static bool isStatusBarChild(const QObject* obj);
    // Global geometry of the status bar. GUI thread only (reads QWidget).
    static QRect statusBarGeometry();

    // --- mini-cluster AA bas-droite (même exemption tactile que la barre) ---
    // Enfant de la page AA (horloge + temp + signal + bouton logo).
    // GUI thread only.
    static void setAaOverlay(QWidget* overlay);
    static bool isAaOverlayChild(const QObject* obj);
    static QRect aaOverlayGeometry();

    // --- navigation state (written by MainWindow, read by InputDevice) ---
    static void setAaPageActive(bool active);
    static bool isAaPageActive();

    // --- full-window shields (written by MainWindow, read by InputDevice) ---
    static void setSplashActive(bool active);
    static bool isSplashActive();
    static void setScreenOffActive(bool active);
    static bool isScreenOffActive();

signals:
    void nightModeChanged(bool on);
    void tempExtChanged(int tempC);
    void ignitionChanged(bool on);
    void videoStarted();
    void videoStopped();

private:
    HuEvents();

    static QWidget* videoHost_;
    static QWidget* statusBar_;
    static QWidget* aaOverlay_;
    static std::atomic<bool> aaPageActive_;
    static std::atomic<bool> splashActive_;
    static std::atomic<bool> screenOffActive_;
};

}
}
}
}
