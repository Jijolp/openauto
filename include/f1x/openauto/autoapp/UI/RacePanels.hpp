/*
*  This file is part of openauto project.
*  (Race Mode v1: telemetry panels + G simulator. Same visual language as
*  the rest — Nothing palette, Inter, UiConstants, night mode. Data is
*  SIMULATED for now (future: I2C accelerometer on the Pi + real CAN).
*  Qt Widgets + QSS + QPainter only, no QML, no OpenGL.)
*/

#pragma once

#include <QFrame>
#include <QLabel>
#include <QObject>
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

class MercedesLogo;

// Future video slot: the "Waze alone" stream needs a second cluster
// display (dedicated protocol work, backlog). When it lands, parent the
// video widget to videoSlot() — it already owns the full nav geometry
// (Expanding, setGeometry-ready, no layout blocking insertion).
class NavPanel : public QFrame
{
    Q_OBJECT
public:
    explicit NavPanel(QWidget* parent = nullptr);
    QWidget* videoSlot();

private:
    QWidget* videoSlot_;
};

// Speed + RPM as raw giant figures (human choice: no bar/arc).
class GaugePanel : public QFrame
{
    Q_OBJECT
public:
    explicit GaugePanel(QWidget* parent = nullptr);

public slots:
    void setSpeed(double kmh);
    void setRpm(double rpm);

private:
    QLabel* speedValue_;
    QLabel* rpmValue_;
};

// Concentric G scope (0.5g / 1.0g rings) + smoothed red dot.
// The ONLY input is setG(lat, lon): GSim below is just a replaceable
// producer (later: an I2C sensor driver calling the same setter).
class GForcePanel : public QFrame
{
    Q_OBJECT
public:
    explicit GForcePanel(QWidget* parent = nullptr);
    void setNightMode(bool on);

public slots:
    void setG(double latG, double lonG);

private slots:
    void onTick();

private:
    class Scope;
    Scope* scope_;
    QLabel* latValue_;
    QLabel* lonValue_;
    QLabel* totalValue_;
    QLabel* peakValue_;
    QTimer* tick_;
    double targetLat_;
    double targetLon_;
    double dispLat_;
    double dispLon_;
    double peak_;
};

// Internal G generator: plausible cornering sinusoids + deterministic
// wobble, |g| < 1.2. Replaceable — the real sensor will feed setG().
class GSim : public QObject
{
    Q_OBJECT
public:
    explicit GSim(GForcePanel* target, QObject* parent = nullptr);
    void start();
    void stop();
    bool isRunning() const;

private slots:
    void onTick();

private:
    GForcePanel* target_;
    QTimer* timer_;
    double phase_;
};

}
}
}
}
