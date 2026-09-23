/*
*  This file is part of openauto project.
*  (Race Mode v1: telemetry panels implementation. See header.)
*/

#include <QtMath>
#include <QHBoxLayout>
#include <QPainter>
#include <QVBoxLayout>
#include <f1x/openauto/autoapp/UI/MercedesLogo.hpp>
#include <f1x/openauto/autoapp/UI/RacePanels.hpp>
#include <f1x/openauto/autoapp/UI/UiConstants.hpp>

namespace f1x
{
namespace openauto
{
namespace autoapp
{
namespace ui
{

namespace
{

QLabel* makeCenterLabel(const QString& text, const char* objName)
{
    auto* label = new QLabel(text);
    label->setObjectName(QString::fromLatin1(objName));
    label->setAlignment(Qt::AlignCenter);
    return label;
}

QFont interFont(int pixelSize, QFont::Weight weight)
{
    QFont font(QStringLiteral("Inter"));
    font.setPixelSize(pixelSize);
    font.setWeight(weight);
    return font;
}

}

NavPanel::NavPanel(QWidget* parent)
    : QFrame(parent)
    , videoSlot_(new QWidget(this))
{
    this->setObjectName(QStringLiteral("navPanel"));
    // Future stream host: owns the full nav geometry from day one.
    videoSlot_->setObjectName(QStringLiteral("navVideoSlot"));
    videoSlot_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    auto* slotLayout = new QVBoxLayout(videoSlot_);
    slotLayout->setContentsMargins(8, 8, 8, 8);
    slotLayout->setSpacing(6);
    slotLayout->addStretch(1);
    auto* logoRow = new QHBoxLayout();
    logoRow->addStretch(1);
    auto* logo = new MercedesLogo(videoSlot_, UiConstants::RACE_NAV_LOGO_SIZE);
    logo->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    logo->setColor(QColor(0x6E, 0x6E, 0x6E));
    logoRow->addWidget(logo);
    logoRow->addStretch(1);
    slotLayout->addLayout(logoRow);
    auto* title = makeCenterLabel(QStringLiteral("NAVIGATION"), "navTitle");
    slotLayout->addWidget(title);
    auto* badge = makeCenterLabel(QString::fromUtf8("BIENTÔT"), "soonBadge");
    slotLayout->addWidget(badge);
    slotLayout->addStretch(1);

    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);
    outer->addWidget(videoSlot_, 1);
}

QWidget* NavPanel::videoSlot()
{
    return videoSlot_;
}

GaugePanel::GaugePanel(QWidget* parent)
    : QFrame(parent)
    , speedValue_(makeCenterLabel(QStringLiteral("0"), "raceSpeedValue"))
    , rpmValue_(makeCenterLabel(QStringLiteral("0"), "raceRpmValue"))
{
    this->setObjectName(QStringLiteral("gaugePanel"));
    speedValue_->setFont(interFont(UiConstants::RACE_SPEED_FONT_SIZE, QFont::Bold));
    rpmValue_->setFont(interFont(UiConstants::RACE_RPM_FONT_SIZE, QFont::Bold));

    // Units share the data size (§41) and the row is RIGHT-anchored:
    // the unit never moves, the value grows leftward, glued to it.
    auto* speedUnit = makeCenterLabel(QStringLiteral("km/h"), "raceSpeedUnit");
    speedUnit->setFont(interFont(UiConstants::RACE_SPEED_FONT_SIZE, QFont::DemiBold));
    speedUnit->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    auto* rpmUnit = makeCenterLabel(QStringLiteral("tr/min"), "raceRpmUnit");
    rpmUnit->setFont(interFont(UiConstants::RACE_RPM_FONT_SIZE, QFont::DemiBold));
    rpmUnit->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    auto* speedRow = new QHBoxLayout();
    speedRow->setContentsMargins(0, 0, 0, 0);
    speedRow->setSpacing(8);
    speedRow->addStretch(1);
    speedRow->addWidget(speedValue_, 0, Qt::AlignRight | Qt::AlignVCenter);
    speedRow->addWidget(speedUnit, 0, Qt::AlignLeft | Qt::AlignVCenter);
    auto* rpmRow = new QHBoxLayout();
    rpmRow->setContentsMargins(0, 0, 0, 0);
    rpmRow->setSpacing(8);
    rpmRow->addStretch(1);
    rpmRow->addWidget(rpmValue_, 0, Qt::AlignRight | Qt::AlignVCenter);
    rpmRow->addWidget(rpmUnit, 0, Qt::AlignLeft | Qt::AlignVCenter);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(0);
    layout->addStretch(1);
    layout->addLayout(speedRow);
    layout->addLayout(rpmRow);
    layout->addStretch(1);
}

void GaugePanel::setSpeed(double kmh)
{
    // Neutral 0 state when no CAN (never stale: every frame rewrites).
    speedValue_->setText(QString::number(qMax(0, qRound(kmh))));
}

void GaugePanel::setRpm(double rpm)
{
    rpmValue_->setText(QString::number(qMax(0, qMin(qRound(rpm), UiConstants::RACE_RPM_MAX))));
}

class GForcePanel::Scope : public QWidget
{
public:
    explicit Scope(QWidget* parent = nullptr)
        : QWidget(parent)
        , lat_(0.0)
        , lon_(0.0)
        , night_(false)
    {
        this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    }

    void setPoint(double lat, double lon)
    {
        lat_ = lat;
        lon_ = lon;
        this->update();
    }

    void setNightMode(bool on)
    {
        night_ = on;
        this->update();
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        // Reentrancy guard: prevent recursive paintEvent calls during
        // fade transitions (StackAll + opacity effects on Race page
        // can cause nested paint events on the same widget).
        static thread_local bool painting = false;
        if(painting)
        {
            return;
        }
        painting = true;

        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);
        const qreal size = qMin(this->width(), this->height());
        const QPointF c(this->width() / 2.0, this->height() / 2.0);
        const qreal rOuter = size / 2.0 - 4.0;  // 1.0g ring
        const qreal rInner = rOuter / 2.0;      // 0.5g ring
        const QColor ring = night_ ? QColor(0x3A, 0x3A, 0x40) : QColor(0x2A, 0x2A, 0x30);
        const QColor grad = night_ ? QColor(0x6E, 0x6E, 0x6E) : QColor(0x9E, 0x9E, 0x9E);

        p.setPen(QPen(ring, 1.5));
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(c, rOuter, rOuter);
        p.drawEllipse(c, rInner, rInner);
        // Cross hairs + discreet ticks every 30°.
        p.setPen(QPen(ring, 1.0));
        p.drawLine(QPointF(c.x() - rOuter, c.y()), QPointF(c.x() + rOuter, c.y()));
        p.drawLine(QPointF(c.x(), c.y() - rOuter), QPointF(c.x(), c.y() + rOuter));
        p.setPen(QPen(grad, 1.0));
        for(int deg = 0; deg < 360; deg += 30)
        {
            const qreal rad = qDegreesToRadians(static_cast<qreal>(deg));
            const QPointF o(c.x() + rOuter * qCos(rad), c.y() + rOuter * qSin(rad));
            const QPointF i(c.x() + (rOuter - 5.0) * qCos(rad), c.y() + (rOuter - 5.0) * qSin(rad));
            p.drawLine(i, o);
        }

        // Red dot: +lat = right, +lon (accel) = up. Clamped to the 1g ring.
        const qreal gx = qBound(-1.2, lat_, 1.2) * rOuter;
        const qreal gy = qBound(-1.2, lon_, 1.2) * rOuter;
        QPointF dot(c.x() + gx, c.y() - gy);
        const qreal dist = qSqrt(gx * gx + gy * gy);
        if(dist > rOuter)
        {
            dot = c + (dot - c) * (rOuter / dist);
        }
        p.setPen(Qt::NoPen);
        p.setBrush(night_ ? QColor(0x96, 0x13, 0x1B) : QColor(0xD7, 0x19, 0x20));
        p.drawEllipse(dot, 7.0, 7.0);
        p.setBrush(QColor(0xFF, 0xFF, 0xFF));
        p.drawEllipse(dot, 2.5, 2.5);
        painting = false;
    }

private:
    double lat_;
    double lon_;
    bool night_;
};

GForcePanel::GForcePanel(QWidget* parent)
    : QFrame(parent)
    , scope_(new Scope(this))
    , latValue_(makeCenterLabel(QStringLiteral("+0.00 g"), "raceGValue"))
    , lonValue_(makeCenterLabel(QStringLiteral("+0.00 g"), "raceGValue"))
    , totalValue_(makeCenterLabel(QStringLiteral("0.00 g"), "raceGValue"))
    , peakValue_(makeCenterLabel(QStringLiteral("max 0.00 g"), "raceGLabel"))
    , tick_(new QTimer(this))
    , targetLat_(0.0)
    , targetLon_(0.0)
    , dispLat_(0.0)
    , dispLon_(0.0)
    , peak_(0.0)
{
    this->setObjectName(QStringLiteral("gforcePanel"));

    auto* titleLat = makeCenterLabel(QStringLiteral("LAT"), "raceGLabel");
    auto* titleLon = makeCenterLabel(QStringLiteral("LON"), "raceGLabel");
    auto* titleTot = makeCenterLabel(QStringLiteral("TOTAL"), "raceGLabel");
    latValue_->setFont(interFont(UiConstants::RACE_G_FONT_SIZE, QFont::DemiBold));
    lonValue_->setFont(interFont(UiConstants::RACE_G_FONT_SIZE, QFont::DemiBold));
    totalValue_->setFont(interFont(UiConstants::RACE_G_FONT_SIZE, QFont::Bold));

    auto* col = new QVBoxLayout();
    col->setContentsMargins(0, 0, 0, 0);
    col->setSpacing(2);
    col->addStretch(1);
    col->addWidget(titleLat);
    col->addWidget(latValue_);
    col->addSpacing(6);
    col->addWidget(titleLon);
    col->addWidget(lonValue_);
    col->addSpacing(6);
    col->addWidget(titleTot);
    col->addWidget(totalValue_);
    col->addSpacing(6);
    col->addWidget(peakValue_);
    col->addStretch(1);

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(8);
    layout->addWidget(scope_, 1);
    layout->addLayout(col);

    // 60 fps smoothing: exponential approach, never shaky.
    tick_->setInterval(16);
    connect(tick_, &QTimer::timeout, this, &GForcePanel::onTick);
    tick_->start();
}

void GForcePanel::setNightMode(bool on)
{
    scope_->setNightMode(on);
}

void GForcePanel::setG(double latG, double lonG)
{
    targetLat_ = latG;
    targetLon_ = lonG;
    const double total = qSqrt(latG * latG + lonG * lonG);
    if(total > peak_)
    {
        peak_ = total;
        peakValue_->setText(QStringLiteral("max %1 g").arg(peak_, 0, 'f', 2));
    }
}

void GForcePanel::onTick()
{
    // Lerp toward target (~180 ms settle). Cheap, stable at 60 fps.
    dispLat_ += (targetLat_ - dispLat_) * 0.18;
    dispLon_ += (targetLon_ - dispLon_) * 0.18;
    if(qAbs(targetLat_ - dispLat_) < 0.0005) { dispLat_ = targetLat_; }
    if(qAbs(targetLon_ - dispLon_) < 0.0005) { dispLon_ = targetLon_; }
    scope_->setPoint(dispLat_, dispLon_);
    const auto fmtG = [](double v) {
        return QStringLiteral("%1%2 g")
            .arg(v < 0 ? QStringLiteral("-") : QStringLiteral("+"))
            .arg(qAbs(v), 0, 'f', 2);
    };
    latValue_->setText(fmtG(dispLat_));
    lonValue_->setText(fmtG(dispLon_));
    totalValue_->setText(QStringLiteral("%1 g")
        .arg(qSqrt(dispLat_ * dispLat_ + dispLon_ * dispLon_), 0, 'f', 2));
}

GSim::GSim(GForcePanel* target, QObject* parent)
    : QObject(parent)
    , target_(target)
    , timer_(new QTimer(this))
    , phase_(0.0)
{
    timer_->setInterval(100);
    connect(timer_, &QTimer::timeout, this, &GSim::onTick);
}

void GSim::start()
{
    phase_ = 0.0;
    timer_->start();
}

void GSim::stop()
{
    timer_->stop();
    if(target_ != nullptr)
    {
        target_->setG(0.0, 0.0);
    }
}

bool GSim::isRunning() const
{
    return timer_->isActive();
}

void GSim::onTick()
{
    if(target_ == nullptr)
    {
        return;
    }
    phase_ += 0.1;
    // Plausible cornering: slow sweeps + deterministic wobble (no rand:
    // reproducible, and the panel smoothing kills any residual jitter).
    double lat = 0.85 * qSin(phase_ * 0.55) + 0.08 * qSin(phase_ * 1.7);
    double lon = 0.55 * qSin(phase_ * 0.37 + 1.0) + 0.06 * qSin(phase_ * 2.3);
    const double total = qSqrt(lat * lat + lon * lon);
    if(total > 1.2)
    {
        lat *= 1.2 / total;
        lon *= 1.2 / total;
    }
    target_->setG(lat, lon);
}

}
}
}
}
