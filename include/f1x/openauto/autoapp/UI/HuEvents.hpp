/*
*  This file is part of openauto project.
*  (UI-2a: single-window event bus. Decouples the AA backend threads from
*  the UI without touching any protocol/service logic:
*   - night_mode: CanBridge worker thread notifies, MainWindow consumes
*     (one-way, read-only for the services).
*   - videoStarted/videoStopped: QtVideoOutput notifies, MainWindow
*     switches stack pages (navigation = show/hide, session untouched).
*   - video host registry: the AA stack page where QtVideoOutput embeds
*     its QVideoWidget (replaces the former separate fullscreen window).
*   - aaPageActive: set by MainWindow on navigation; lets InputDevice
*     forward touch/keys to the phone ONLY while the AA page is shown
*     (other pages stay fully clickable during a session).
*  Emitting from worker threads is safe: receivers live in the GUI
*  thread, Qt::AutoConnection delivers queued across threads.)
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
    static void notifyVideoStarted();
    static void notifyVideoStopped();

    // --- video host page (owned by MainWindow, app lifetime) ---
    static void setVideoHost(QWidget* host);
    static QWidget* videoHost();
    // Global geometry of the host widget. GUI thread only (reads QWidget).
    static QRect videoHostGeometry();

    // --- navigation state (written by MainWindow, read by InputDevice) ---
    static void setAaPageActive(bool active);
    static bool isAaPageActive();

signals:
    void nightModeChanged(bool on);
    void videoStarted();
    void videoStopped();

private:
    HuEvents();

    static QWidget* videoHost_;
    static std::atomic<bool> aaPageActive_;
};

}
}
}
}
