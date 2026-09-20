/*
*  This file is part of openauto project.
*  Copyright (C) 2018 f1x.studio (Michal Szwaj)
*
*  openauto is free software: you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 3 of the License, or
*  (at your option) any later version.
*
*  You should have received a copy of the GNU General Public License
*  along with openauto. If not, see <http://www.gnu.org/licenses/>.
*/

#include <QApplication>
#include <QCoreApplication>
#include <QFile>
#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QShortcut>
#include <QStackedLayout>
#include <QStackedWidget>
#include <QTimer>
#include <QVBoxLayout>
#include <QGridLayout>
#include <f1x/openauto/autoapp/UI/HuEvents.hpp>
#include <f1x/openauto/autoapp/UI/MainWindow.hpp>
#include <f1x/openauto/autoapp/UI/MercedesLogo.hpp>
#include <f1x/openauto/autoapp/UI/ScreenOffOverlay.hpp>
#include <f1x/openauto/autoapp/UI/SplashOverlay.hpp>
#include <f1x/openauto/autoapp/UI/StatusBar.hpp>
#include <f1x/openauto/autoapp/UI/UiConstants.hpp>
#include <f1x/openauto/Common/Log.hpp>

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

QLabel* makeTitle(const QString& text)
{
    auto* label = new QLabel(text);
    label->setObjectName(QStringLiteral("pageTitle"));
    label->setAlignment(Qt::AlignCenter);
    return label;
}

QLabel* makeSubtitle(const QString& text)
{
    auto* label = new QLabel(text);
    label->setObjectName(QStringLiteral("pageSubtitle"));
    label->setAlignment(Qt::AlignCenter);
    label->setWordWrap(true);
    return label;
}

QPushButton* makeQuadrant(const QString& text, const QString& objName, bool enabled)
{
    auto* btn = new QPushButton(text);
    btn->setObjectName(objName);
    btn->setEnabled(enabled);
    // FIX UI-2b layout: quadrants must fill their grid cells (were thin
    // bars — default policies never took the stretched space).
    btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    // Uppercase + letter-spacing handled in QSS via font + letter-spacing.
    // Keep text uppercase in code.
    return btn;
}

}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , statusBar_(new StatusBar(this))
    , stack_(new QStackedWidget(this))
    , homePage_(nullptr)
    , gridContainer_(nullptr)
    , aaPage_(nullptr)
    , settingsPage_(nullptr)
    , aaPlaceholder_(nullptr)
    , splash_(new SplashOverlay(this))
    , screenOff_(new ScreenOffOverlay(this))
    , quadrantAA_(nullptr)
    , quadrantRace_(nullptr)
    , quadrantCar_(nullptr)
    , quadrantParams_(nullptr)
    , centerHit_(nullptr)
    , centerLogo_(nullptr)
    , nightButton_(nullptr)
    , night_(false)
    , splashActive_(false)
{
    auto* central = new QWidget(this);
    auto* layout = new QVBoxLayout(central);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(statusBar_);
    layout->addWidget(stack_, 1);
    this->setCentralWidget(central);

    // FIX UI-2b video: keep every page rendered at all times, stacked —
    // the GStreamer sink behind QVideoWidget loses its overlay window on
    // unmap (hide) and never re-acquires it on remap (show) → black or
    // deformed frame on return. StackAll keeps the video window mapped
    // under the opaque pages (home/settings cover it visually, capture
    // all input as the top widgets). See BUILD_NOTES §24.
    // (StackingMode lives on QStackedLayout; QStackedWidget's internal
    // layout is one — documented relationship.)
    if(auto* stackedLayout = qobject_cast<QStackedLayout*>(stack_->layout()))
    {
        stackedLayout->setStackingMode(QStackedLayout::StackAll);
    }

    homePage_ = this->buildHomePage();
    aaPage_ = this->buildAAPage();
    settingsPage_ = this->buildSettingsPage();
    stack_->addWidget(homePage_);      // HOME_PAGE = 0
    stack_->addWidget(aaPage_);        // AA_PAGE = 1
    stack_->addWidget(settingsPage_);  // SETTINGS_PAGE = 2
    stack_->setCurrentIndex(HOME_PAGE);
    HuEvents::setAaPageActive(false);
    statusBar_->setAaMode(false);

    HuEvents::setVideoHost(aaPage_);

    // Overlays cover the whole MainWindow (including status bar).
    splash_->hide();
    screenOff_->hide();

    connect(&HuEvents::instance(), &HuEvents::videoStarted, this, &MainWindow::onVideoStarted);
    connect(&HuEvents::instance(), &HuEvents::videoStopped, this, &MainWindow::onVideoStopped);
    connect(&HuEvents::instance(), &HuEvents::nightModeChanged, this, &MainWindow::setNightMode);
    connect(&HuEvents::instance(), &HuEvents::tempExtChanged, this, &MainWindow::onTempExt);
    connect(&HuEvents::instance(), &HuEvents::ignitionChanged, this, &MainWindow::onIgnition);
    connect(statusBar_, &StatusBar::aaHomeClicked, this, &MainWindow::showHomePage);
    connect(splash_, &SplashOverlay::shrinkStarted, this, &MainWindow::onSplashShrinkStarted);
    connect(splash_, &SplashOverlay::finished, this, &MainWindow::onSplashFinished);
    connect(screenOff_, &ScreenOffOverlay::wakeRequested, this, &MainWindow::onScreenOffWake);

    connect(new QShortcut(QKeySequence(Qt::Key_S), this), &QShortcut::activated, this, &MainWindow::openSettings);
    connect(new QShortcut(QKeySequence(Qt::Key_E), this), &QShortcut::activated, this, &MainWindow::exit);
    connect(new QShortcut(QKeySequence(Qt::Key_C), this), &QShortcut::activated, this, &MainWindow::toggleCursor);
    connect(new QShortcut(QKeySequence(Qt::Key_W), this), &QShortcut::activated, this, &MainWindow::openConnectDialog);
    connect(new QShortcut(QKeySequence(Qt::Key_1), this), &QShortcut::activated, this, &MainWindow::showHomePage);
    connect(new QShortcut(QKeySequence(Qt::Key_2), this), &QShortcut::activated, this, &MainWindow::showAAPage);
    connect(new QShortcut(QKeySequence(Qt::Key_3), this), &QShortcut::activated, this, &MainWindow::showSettingsPage);

    this->applyTheme();

    // Splash: holds ~800ms of real init (config/CanBridge). Interruptible
    // by AA-connect at any moment → jump to final then auto-switch.
    if(!qEnvironmentVariableIsSet("OPENAUTO_NO_SPLASH"))
    {
        splashActive_ = true;
        HuEvents::setSplashActive(true);
        // Quadrants start invisible, will fade in staggered during shrink.
        for(auto* eff : quadrantEffects_)
        {
            eff->setOpacity(0.0);
        }
        if(centerLogo_ != nullptr)
        {
            auto* eff = new QGraphicsOpacityEffect(centerLogo_);
            eff->setOpacity(0.0);
            centerLogo_->setGraphicsEffect(eff);
            // Will be faded with first quadrant.
        }
        splash_->setGeometry(this->rect());
        splash_->show();
        splash_->raise();
        QTimer::singleShot(80, splash_, [this]() { splash_->start(); });
    }
    else
    {
        // No splash dev mode: quadrants already visible.
        for(auto* eff : quadrantEffects_)
        {
            eff->setOpacity(1.0);
        }
    }
}

MainWindow::~MainWindow() = default;

void MainWindow::showHomePage()
{
    stack_->setCurrentIndex(HOME_PAGE);
    HuEvents::setAaPageActive(false);
    statusBar_->setAaMode(false);
}

void MainWindow::showAAPage()
{
    stack_->setCurrentIndex(AA_PAGE);
    HuEvents::setAaPageActive(true);
    statusBar_->setAaMode(true);
}

void MainWindow::showSettingsPage()
{
    stack_->setCurrentIndex(SETTINGS_PAGE);
    HuEvents::setAaPageActive(false);
    statusBar_->setAaMode(false);
}

void MainWindow::setNightMode(bool on)
{
    if(night_ == on)
    {
        return;
    }
    night_ = on;
    OPENAUTO_LOG(info) << "[MainWindow] night mode " << (on ? "ON" : "OFF") << ".";
    statusBar_->setNightMode(on);
    if(nightButton_ != nullptr)
    {
        const bool blocked = nightButton_->blockSignals(true);
        nightButton_->setChecked(on);
        nightButton_->blockSignals(blocked);
    }
    this->applyTheme();
}

void MainWindow::onVideoStarted()
{
    // Interruptible splash: AA connect at any instant → jump to final.
    if(splashActive_ && splash_ != nullptr && splash_->isActive())
    {
        splash_->interrupt();
        splashActive_ = false;
        HuEvents::setSplashActive(false);
        // Ensure quadrants become visible immediately.
        for(auto* eff : quadrantEffects_)
        {
            eff->setOpacity(1.0);
        }
        if(centerLogo_ != nullptr && centerLogo_->graphicsEffect() != nullptr)
        {
            static_cast<QGraphicsOpacityEffect*>(centerLogo_->graphicsEffect())->setOpacity(1.0);
        }
    }
    // Wake from screen-off (any state) and show AA.
    if(HuEvents::isScreenOffActive())
    {
        this->hideScreenOff();
    }
    if(aaPlaceholder_ != nullptr)
    {
        aaPlaceholder_->hide();
    }
    this->showAAPage();
}

void MainWindow::onVideoStopped()
{
    if(HuEvents::isScreenOffActive())
    {
        this->hideScreenOff();
    }
    if(aaPlaceholder_ != nullptr)
    {
        aaPlaceholder_->show();
    }
    this->showHomePage();
}

void MainWindow::onTempExt(int tempC)
{
    statusBar_->setTemp(tempC);
}

void MainWindow::onIgnition(bool on)
{
    if(!on)
    {
        this->showScreenOff();
    }
    else
    {
        if(HuEvents::isScreenOffActive())
        {
            this->hideScreenOff();
        }
    }
}

void MainWindow::onSplashFinished()
{
    splashActive_ = false;
    HuEvents::setSplashActive(false);
    for(auto* eff : quadrantEffects_)
    {
        eff->setOpacity(1.0);
    }
    if(centerLogo_ != nullptr && centerLogo_->graphicsEffect() != nullptr)
    {
        static_cast<QGraphicsOpacityEffect*>(centerLogo_->graphicsEffect())->setOpacity(1.0);
    }
}

void MainWindow::onSplashShrinkStarted()
{
    this->animateQuadrantsIn();
}

void MainWindow::onScreenOffWake()
{
    this->hideScreenOff();
}

void MainWindow::showScreenOff()
{
    if(screenOff_ == nullptr || screenOff_->isVisible())
    {
        return;
    }
    OPENAUTO_LOG(info) << "[MainWindow] screen off.";
    screenOff_->setGeometry(this->rect());
    screenOff_->show();
    screenOff_->raise();
    HuEvents::setScreenOffActive(true);
}

void MainWindow::hideScreenOff()
{
    if(screenOff_ == nullptr || !screenOff_->isVisible())
    {
        HuEvents::setScreenOffActive(false);
        return;
    }
    OPENAUTO_LOG(info) << "[MainWindow] screen on.";
    screenOff_->hide();
    HuEvents::setScreenOffActive(false);
}

void MainWindow::resizeEvent(QResizeEvent* event)
{
    QMainWindow::resizeEvent(event);
    if(splash_ != nullptr && splash_->isVisible())
    {
        splash_->setGeometry(this->rect());
    }
    if(screenOff_ != nullptr && screenOff_->isVisible())
    {
        screenOff_->setGeometry(this->rect());
    }
    this->positionCenterLogo();
}

void MainWindow::applyTheme() const
{
    const QString fileName = night_ ? QStringLiteral("/../assets/theme-night.qss")
                                    : QStringLiteral("/../assets/theme.qss");
    QFile themeFile(QCoreApplication::applicationDirPath() + fileName);
    if(themeFile.open(QFile::ReadOnly))
    {
        qApp->setStyleSheet(QString::fromUtf8(themeFile.readAll()));
    }
}

QWidget* MainWindow::buildHomePage()
{
    auto* page = new QWidget(this);
    page->setObjectName(QStringLiteral("homePage"));

    auto* outer = new QVBoxLayout(page);
    outer->setContentsMargins(UiConstants::HOME_OUTER_MARGIN, UiConstants::HOME_OUTER_MARGIN,
                              UiConstants::HOME_OUTER_MARGIN, 8);
    outer->setSpacing(8);

    auto* gridContainer = new QWidget(page);
    gridContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    gridContainer_ = gridContainer;
    auto* grid = new QGridLayout(gridContainer);
    grid->setContentsMargins(0, 0, 0, 0);
    grid->setSpacing(UiConstants::HOME_GRID_SPACING);

    quadrantAA_ = makeQuadrant(QStringLiteral("ANDROID AUTO"), QStringLiteral("quadrantAA"), true);
    quadrantRace_ = makeQuadrant(QStringLiteral("MODE RACE\nSOON"), QStringLiteral("quadrantRace"), false);
    quadrantCar_ = makeQuadrant(QStringLiteral("VOITURE\nSOON"), QStringLiteral("quadrantCar"), false);
    quadrantParams_ = makeQuadrant(QStringLiteral("PARAMÈTRES"), QStringLiteral("quadrantParams"), true);

    // Inner corner hugging the central logo (70px) — see theme.qss per-id radii.
    quadrantAA_->setProperty("innerCorner", QStringLiteral("bottomRight"));
    quadrantRace_->setProperty("innerCorner", QStringLiteral("bottomLeft"));
    quadrantCar_->setProperty("innerCorner", QStringLiteral("topRight"));
    quadrantParams_->setProperty("innerCorner", QStringLiteral("topLeft"));

    connect(quadrantAA_, &QPushButton::clicked, this, &MainWindow::showAAPage);
    connect(quadrantParams_, &QPushButton::clicked, this, &MainWindow::showSettingsPage);

    grid->addWidget(quadrantAA_, 0, 0);
    grid->addWidget(quadrantRace_, 0, 1);
    grid->addWidget(quadrantCar_, 1, 0);
    grid->addWidget(quadrantParams_, 1, 1);
    grid->setRowStretch(0, 1);
    grid->setRowStretch(1, 1);
    grid->setColumnStretch(0, 1);
    grid->setColumnStretch(1, 1);

    // Fade effects for stagger during splash shrink.
    for(auto* btn : {quadrantAA_, quadrantRace_, quadrantCar_, quadrantParams_})
    {
        auto* eff = new QGraphicsOpacityEffect(btn);
        eff->setOpacity(1.0);
        btn->setGraphicsEffect(eff);
        quadrantEffects_.push_back(eff);
    }

    outer->addWidget(gridContainer, 1);

    // Center Mercedes logo — tappable → screen off (above everything).
    centerHit_ = new QWidget(page);
    centerHit_->setFixedSize(UiConstants::LOGO_HOME_SIZE, UiConstants::LOGO_HOME_SIZE);
    centerHit_->setCursor(Qt::PointingHandCursor);
    centerHit_->setObjectName(QStringLiteral("centerHit"));
    centerLogo_ = new MercedesLogo(centerHit_, UiConstants::LOGO_HOME_SIZE);
    centerLogo_->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    centerLogo_->move(0, 0);
    centerHit_->installEventFilter(this);
    // Capture click via eventFilter below, or use mousePress on centerHit.
    // Simpler: connect via lambda on mouse press using event filter override?
    // Install a direct handler: centerHit catches mousePress.
    centerHit_->setAttribute(Qt::WA_StyledBackground, false);

    // Use a helper to forward mouse press to showScreenOff.
    // Since we can't easily subclass in this factory, install a timer-based
    // event filter via parent: MainWindow::eventFilter will handle it.
    // For now, make centerHit a QPushButton style clickable:
    // Recreate as button for simplicity.
    // Replace centerHit with a flat QPushButton that holds the logo.
    // (Delete the widget container and use button.)
    // --- recreate ---
    delete centerHit_;
    centerHit_ = nullptr;
    auto* centerBtn = new QPushButton(page);
    centerBtn->setObjectName(QStringLiteral("centerLogoButton"));
    centerBtn->setFixedSize(UiConstants::LOGO_HOME_SIZE, UiConstants::LOGO_HOME_SIZE);
    centerBtn->setFlat(true);
    centerBtn->setCursor(Qt::PointingHandCursor);
    centerBtn->setStyleSheet(QStringLiteral("QPushButton#centerLogoButton { background: transparent; border: none; }"));
    centerLogo_ = new MercedesLogo(centerBtn, UiConstants::LOGO_HOME_SIZE);
    centerLogo_->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    centerLogo_->move(0, 0);
    centerHit_ = centerBtn;
    connect(centerBtn, &QPushButton::clicked, this, &MainWindow::showScreenOff);
    // Red SVG variant while pressed (active state).
    connect(centerBtn, &QPushButton::pressed, this, [this]() {
        if(centerLogo_ != nullptr) { centerLogo_->setActive(true); }
    });
    connect(centerBtn, &QPushButton::released, this, [this]() {
        if(centerLogo_ != nullptr) { centerLogo_->setActive(false); }
    });

    // Dev strip (windowed hint + night toggle for quick test).
    auto* devRow = new QWidget(page);
    auto* devLayout = new QHBoxLayout(devRow);
    devLayout->setContentsMargins(0, 0, 0, 0);
    devLayout->setSpacing(8);
    auto* hint = makeSubtitle(QStringLiteral("1=Accueil  2=AA  3=Paramètres  ·  tap logo → veille"));
    hint->setObjectName(QStringLiteral("devHint"));
    nightButton_ = new QPushButton(QStringLiteral("Night"), devRow);
    nightButton_->setObjectName(QStringLiteral("nightToggle"));
    nightButton_->setCheckable(true);
    nightButton_->setChecked(night_);
    nightButton_->setFixedHeight(28);
    connect(nightButton_, &QPushButton::toggled, this, &MainWindow::setNightMode);
    devLayout->addWidget(hint, 1);
    devLayout->addWidget(nightButton_);
    outer->addWidget(devRow);

    // Position the center logo over the grid intersection (absolute).
    // Keep it as child of page (not in layout) so it floats above quadrants.
    centerHit_->setParent(page);
    centerHit_->raise();
    QTimer::singleShot(0, this, [this]() { this->positionCenterLogo(); });

    return page;
}

QWidget* MainWindow::buildAAPage()
{
    aaPage_ = new QWidget(this);
    aaPage_->setObjectName(QStringLiteral("aaPage"));
    auto* layout = new QVBoxLayout(aaPage_);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    aaPlaceholder_ = makeSubtitle(QStringLiteral("Projection AA\nEn attente de vidéo…"));
    aaPlaceholder_->setObjectName(QStringLiteral("pageTitle"));
    layout->addStretch();
    layout->addWidget(aaPlaceholder_);
    layout->addStretch();
    return aaPage_;
}

QWidget* MainWindow::buildSettingsPage()
{
    settingsPage_ = new QWidget(this);
    settingsPage_->setObjectName(QStringLiteral("settingsPage"));
    auto* layout = new QVBoxLayout(settingsPage_);
    layout->addStretch();
    layout->addWidget(makeTitle(QStringLiteral("PARAMÈTRES")));
    layout->addWidget(makeSubtitle(QStringLiteral("Squelette UI-2b — fonds Nothing/Mercedes, quadrants en place.")));
    layout->addStretch();

    auto* advancedButton = new QPushButton(QStringLiteral("Réglages avancés… (S)"), settingsPage_);
    connect(advancedButton, &QPushButton::clicked, this, &MainWindow::openSettings);
    auto* row = new QHBoxLayout();
    row->addStretch();
    row->addWidget(advancedButton);
    row->addStretch();
    layout->addLayout(row);
    layout->addStretch();
    auto* hint = makeSubtitle(QStringLiteral("Tap logo central de l'accueil → écran éteint"));
    layout->addWidget(hint);
    layout->addStretch();
    return settingsPage_;
}

void MainWindow::animateQuadrantsIn()
{
    const int stagger = UiConstants::SPLASH_STAGGER_MS;
    int idx = 0;
    for(auto* eff : quadrantEffects_)
    {
        auto* anim = new QPropertyAnimation(eff, "opacity", this);
        anim->setDuration(320);
        anim->setStartValue(0.0);
        anim->setEndValue(1.0);
        anim->setEasingCurve(QEasingCurve::OutCubic);
        QTimer::singleShot(idx * stagger, anim, [anim]() { anim->start(QAbstractAnimation::DeleteWhenStopped); });
        ++idx;
    }
    if(centerLogo_ != nullptr && centerLogo_->graphicsEffect() != nullptr)
    {
        auto* eff = static_cast<QGraphicsOpacityEffect*>(centerLogo_->graphicsEffect());
        auto* anim = new QPropertyAnimation(eff, "opacity", this);
        anim->setDuration(320);
        anim->setStartValue(0.0);
        anim->setEndValue(1.0);
        anim->setEasingCurve(QEasingCurve::OutCubic);
        QTimer::singleShot(0, anim, [anim]() { anim->start(QAbstractAnimation::DeleteWhenStopped); });
    }
}

void MainWindow::positionCenterLogo()
{
    if(homePage_ == nullptr || centerHit_ == nullptr || gridContainer_ == nullptr)
    {
        return;
    }
    // FIX UI-2b layout: center on the GRID intersection (gridContainer is
    // a child of the page, so its geometry is already in page coords),
    // not on the page — the dev strip below would offset it otherwise.
    // Robust to resize (e.g. 1920x1080): recomputed on every resizeEvent.
    const QPoint gridCenter = gridContainer_->geometry().center();
    const int x = gridCenter.x() - centerHit_->width() / 2;
    const int y = gridCenter.y() - centerHit_->height() / 2;
    centerHit_->move(x, y);
    centerHit_->raise();
}

}
}
}
}
