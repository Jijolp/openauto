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
#include <QScrollArea>
#include <QFrame>
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
    // P3: label type sized for the surface in code (UiConstants) — QSS
    // keeps colors/borders/padding only. Uppercase text in code.
    QFont labelFont(QStringLiteral("Inter"));
    labelFont.setPixelSize(UiConstants::QUADRANT_LABEL_FONT_SIZE);
    labelFont.setWeight(QFont::DemiBold);
    labelFont.setLetterSpacing(QFont::PercentageSpacing, UiConstants::QUADRANT_LABEL_SPACING_PCT);
    btn->setFont(labelFont);
    return btn;
}

}

MainWindow::MainWindow(QWidget* embeddedSettings, QWidget *parent)
    : QMainWindow(parent)
    , statusBar_(new StatusBar(this))
    , stack_(new QStackedWidget(this))
    , homePage_(nullptr)
    , gridContainer_(nullptr)
    , aaPage_(nullptr)
    , settingsPage_(nullptr)
    , racePage_(nullptr)
    , carPage_(nullptr)
    , aaPlaceholder_(nullptr)
    , aaCluster_(nullptr)
    , aaHomeButton_(nullptr)
    , aaHomeLogo_(nullptr)
    , splash_(new SplashOverlay(this))
    , screenOff_(new ScreenOffOverlay(this))
    , quadrantAA_(nullptr)
    , quadrantRace_(nullptr)
    , quadrantCar_(nullptr)
    , quadrantParams_(nullptr)
    , centerHit_(nullptr)
    , centerLogo_(nullptr)
    , quadrantEffects_()
    , embeddedSettings_(embeddedSettings)
    , nightButton_(nullptr)
    , night_(false)
    , splashActive_(false)
{
    auto* central = new QWidget(this);
    auto* layout = new QVBoxLayout(central);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    // Item4: the stack takes the WHOLE window — the status bar floats
    // above it as a click-transparent overlay (video is full-height,
    // no longer squashed by a 40px layout row).
    layout->addWidget(stack_, 1);
    this->setCentralWidget(central);
    statusBar_->setAttribute(Qt::WA_TransparentForMouseEvents, true);

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
    racePage_ = this->buildRacePage();
    carPage_ = this->buildCarPage();
    stack_->addWidget(homePage_);      // HOME_PAGE = 0
    stack_->addWidget(aaPage_);        // AA_PAGE = 1
    stack_->addWidget(settingsPage_);  // SETTINGS_PAGE = 2
    stack_->addWidget(racePage_);      // RACE_PAGE = 3
    stack_->addWidget(carPage_);       // CAR_PAGE = 4
    stack_->setCurrentIndex(HOME_PAGE);
    this->layoutStatusOverlay();
    HuEvents::setAaPageActive(false);
    statusBar_->setAaMode(false);

    HuEvents::setVideoHost(aaPage_);
    HuEvents::setStatusBar(statusBar_);

    // Bouton logo AA seul : flottant au-dessus de la vidéo, décalé à
    // gauche de la zone infos AA (5G/horloge en bas à droite). Enfant de
    // la page AA : visible seulement quand AA est au-dessus (StackAll).
    // Seul le bouton prend les clics (le conteneur est transparent).
    aaCluster_ = new QWidget(aaPage_);
    aaCluster_->setObjectName(QStringLiteral("aaCluster"));
    aaCluster_->setAttribute(Qt::WA_StyledBackground, true);
    aaCluster_->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    aaCluster_->setFocusPolicy(Qt::NoFocus);
    auto* clusterLayout = new QHBoxLayout(aaCluster_);
    clusterLayout->setContentsMargins(0, 0, 0, 0);
    clusterLayout->setSpacing(0);
    aaHomeButton_ = new QPushButton(aaCluster_);
    aaHomeButton_->setObjectName(QStringLiteral("aaFloatingButton"));
    aaHomeButton_->setFixedSize(UiConstants::AA_FLOAT_BUTTON_SIZE, UiConstants::AA_FLOAT_BUTTON_SIZE);
    aaHomeButton_->setFlat(true);
    aaHomeButton_->setFocusPolicy(Qt::NoFocus);
    aaHomeButton_->setCursor(Qt::PointingHandCursor);
    aaHomeLogo_ = new MercedesLogo(aaHomeButton_, UiConstants::AA_FLOAT_ICON_SIZE);
    aaHomeLogo_->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    aaHomeLogo_->setColor(QColor(0xC8, 0xC8, 0xCC));
    aaHomeLogo_->move((UiConstants::AA_FLOAT_BUTTON_SIZE - UiConstants::AA_FLOAT_ICON_SIZE) / 2,
                      (UiConstants::AA_FLOAT_BUTTON_SIZE - UiConstants::AA_FLOAT_ICON_SIZE) / 2);
    clusterLayout->addWidget(aaHomeButton_);
    HuEvents::setAaOverlay(aaCluster_);
    connect(aaHomeButton_, &QPushButton::clicked, this, &MainWindow::showHomePage);
    connect(aaHomeButton_, &QPushButton::pressed, this, [this]() {
        if(aaHomeLogo_ != nullptr) { aaHomeLogo_->setActive(true); }
    });
    connect(aaHomeButton_, &QPushButton::released, this, [this]() {
        if(aaHomeLogo_ != nullptr) { aaHomeLogo_->setActive(false); }
    });
    aaCluster_->hide();

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
    statusBar_->show();
    if(aaCluster_ != nullptr)
    {
        aaCluster_->hide();
    }
}

void MainWindow::showAAPage()
{
    stack_->setCurrentIndex(AA_PAGE);
    HuEvents::setAaPageActive(true);
    // No bandeau on the AA page: full-height video, only the floating
    // logo button (bottom-right, blended with AA's own bottom bar).
    statusBar_->hide();
    this->layoutAaCluster();
    if(aaCluster_ != nullptr)
    {
        aaCluster_->show();
        aaCluster_->raise();
    }
}

void MainWindow::showSettingsPage()
{
    // P2: the embedded config hides itself on Save/Cancel (close()) —
    // re-show it on every visit so the page is never an empty hole.
    if(embeddedSettings_ != nullptr && !embeddedSettings_->isVisible())
    {
        embeddedSettings_->show();
    }
    stack_->setCurrentIndex(SETTINGS_PAGE);
    HuEvents::setAaPageActive(false);
    statusBar_->setAaMode(false);
    statusBar_->show();
    if(aaCluster_ != nullptr)
    {
        aaCluster_->hide();
    }
}

void MainWindow::showRacePage()
{
    stack_->setCurrentIndex(RACE_PAGE);
    HuEvents::setAaPageActive(false);
    statusBar_->setAaMode(false);
    statusBar_->show();
    if(aaCluster_ != nullptr)
    {
        aaCluster_->hide();
    }
}

void MainWindow::showCarPage()
{
    stack_->setCurrentIndex(CAR_PAGE);
    HuEvents::setAaPageActive(false);
    statusBar_->setAaMode(false);
    statusBar_->show();
    if(aaCluster_ != nullptr)
    {
        aaCluster_->hide();
    }
}

void MainWindow::layoutStatusOverlay()
{
    // Item4: status bar floats over the full-window stack. Clicks pass
    // through except on child widgets (the AA logo button stays clickable).
    if(statusBar_ == nullptr || this->centralWidget() == nullptr)
    {
        return;
    }
    const int w = this->centralWidget()->width();
    statusBar_->setGeometry(0, 0, w, UiConstants::STATUS_BAR_HEIGHT);
    statusBar_->raise();
}

void MainWindow::layoutAaCluster()
{
    // Bouton logo seul, décalé à gauche de la zone infos AA (5G/horloge
    // en bas à droite). Recomputed on every resize (1080p-ready).
    if(aaCluster_ == nullptr || aaPage_ == nullptr)
    {
        return;
    }
    aaCluster_->adjustSize();
    const int m = UiConstants::AA_OVERLAY_MARGIN;
    aaCluster_->move(aaPage_->width() - aaCluster_->width() - m - UiConstants::AA_FLOAT_SHIFT_LEFT,
                     aaPage_->height() - aaCluster_->height() - m);
    if(aaCluster_->isVisible())
    {
        aaCluster_->raise();
    }
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
    this->layoutStatusOverlay();
    this->layoutAaCluster();
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
    // P1: pages must take all viewport space (StackAll keeps siblings live).
    page->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    auto* outer = new QVBoxLayout(page);
    // Item4: top margin clears the floating status overlay.
    outer->setContentsMargins(UiConstants::HOME_OUTER_MARGIN,
                              UiConstants::STATUS_BAR_HEIGHT + UiConstants::HOME_OUTER_MARGIN,
                              UiConstants::HOME_OUTER_MARGIN, UiConstants::HOME_OUTER_MARGIN);
    outer->setSpacing(8);

    auto* gridContainer = new QWidget(page);
    gridContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    gridContainer_ = gridContainer;
    auto* grid = new QGridLayout(gridContainer);
    grid->setContentsMargins(0, 0, 0, 0);
    grid->setSpacing(UiConstants::HOME_GRID_SPACING);

    quadrantAA_ = makeQuadrant(QStringLiteral("ANDROID AUTO"), QStringLiteral("quadrantAA"), true);
    quadrantRace_ = makeQuadrant(QStringLiteral("MODE RACE"), QStringLiteral("quadrantRace"), true);
    quadrantCar_ = makeQuadrant(QStringLiteral("VOITURE"), QStringLiteral("quadrantCar"), true);
    quadrantParams_ = makeQuadrant(QStringLiteral("PARAMÈTRES"), QStringLiteral("quadrantParams"), true);

    // Item1: inner cut radius clears the logo medallion plus a visible gap
    // (LOGO_HOME_SIZE/2 + HOME_LOGO_GAP). Set inline per button so the value
    // lives in UiConstants, not hardcoded in QSS (QSS keeps the 6px outers
    // and all colors; inline wins only for the inner corner).
    const int innerRadius = UiConstants::LOGO_HOME_SIZE / 2 + UiConstants::HOME_LOGO_GAP;
    const struct { QPushButton* btn; const char* corner; } innerCorners[] = {
        { quadrantAA_, "bottom-right" },
        { quadrantRace_, "bottom-left" },
        { quadrantCar_, "top-right" },
        { quadrantParams_, "top-left" },
    };
    for(const auto& ic : innerCorners)
    {
        ic.btn->setStyleSheet(QStringLiteral("QPushButton#%1 { border-%2-radius: %3px; }")
            .arg(ic.btn->objectName(), QString::fromLatin1(ic.corner), QString::number(innerRadius)));
    }

    connect(quadrantAA_, &QPushButton::clicked, this, &MainWindow::showAAPage);
    connect(quadrantRace_, &QPushButton::clicked, this, &MainWindow::showRacePage);
    connect(quadrantCar_, &QPushButton::clicked, this, &MainWindow::showCarPage);
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
    // P1bis: no inline transparent background — QSS #centerLogoButton paints
    // the medallion fully opaque (nothing must leak the compositor behind).
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
    aaPage_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
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

QWidget* MainWindow::buildPageBandeau(const QString& title, const char* backObjName)
{
    auto* bandeau = new QWidget();
    bandeau->setObjectName(QStringLiteral("pageBandeau"));
    auto* bandLayout = new QHBoxLayout(bandeau);
    bandLayout->setContentsMargins(0, 0, 0, 0);
    bandLayout->setSpacing(12);
    auto* titleLabel = makeTitle(title);
    titleLabel->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    bandLayout->addWidget(titleLabel, 1);
    auto* backBtn = makeLogoBackButton(bandeau, UiConstants::SETTINGS_BACK_BUTTON_SIZE,
                                       UiConstants::SETTINGS_BACK_ICON_SIZE, backObjName);
    connect(backBtn, &QPushButton::clicked, this, &MainWindow::showHomePage);
    bandLayout->addWidget(backBtn);
    return bandeau;
}

QWidget* MainWindow::buildSettingsPage()
{
    settingsPage_ = new QWidget(this);
    settingsPage_->setObjectName(QStringLiteral("settingsPage"));
    settingsPage_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    auto* layout = new QVBoxLayout(settingsPage_);
    // Item4: top margin clears the floating status overlay.
    layout->setContentsMargins(UiConstants::HOME_OUTER_MARGIN,
                               UiConstants::STATUS_BAR_HEIGHT + UiConstants::HOME_OUTER_MARGIN,
                               UiConstants::HOME_OUTER_MARGIN, UiConstants::HOME_OUTER_MARGIN);
    layout->setSpacing(8);

    layout->addWidget(this->buildPageBandeau(QStringLiteral("PARAMÈTRES"), "settingsBackButton"));

    // Item3: software version, so a human can check the running binary
    // is the latest build (UiConstants::APP_VERSION + compile date).
    auto* version = makeSubtitle(QStringLiteral("Version logicielle : v%1 (build %2)")
        .arg(QString::fromLatin1(UiConstants::APP_VERSION),
             QString::fromLatin1(__DATE__)));
    version->setObjectName(QStringLiteral("versionLabel"));
    layout->addWidget(version);

    if(embeddedSettings_ != nullptr)
    {
        // P2: reveal the EXISTING OpenAuto config (tabs general/video/audio/
        // input + Save/Cancel) inside a scroll area — no logic duplicated.
        // Save/Cancel call close() which only hides the widget; navigating
        // back here re-shows it (see showSettingsPage), so no dead-end.
        embeddedSettings_->setParent(settingsPage_);
        embeddedSettings_->setWindowFlags(Qt::Widget);
        auto* scroll = new QScrollArea(settingsPage_);
        scroll->setObjectName(QStringLiteral("settingsScroll"));
        scroll->setWidgetResizable(true);
        scroll->setFrameShape(QFrame::NoFrame);
        scroll->setWidget(embeddedSettings_);
        layout->addWidget(scroll, 1);
    }
    else
    {
        // Fallback (no embedded config): minimal content + S shortcut.
        layout->addWidget(makeSubtitle(QStringLiteral("Réglages avancés (S) — configuration OpenAuto d'origine.")));
        auto* advancedButton = new QPushButton(QStringLiteral("Réglages avancés… (S)"), settingsPage_);
        connect(advancedButton, &QPushButton::clicked, this, &MainWindow::openSettings);
        auto* row = new QHBoxLayout();
        row->addStretch();
        row->addWidget(advancedButton);
        row->addStretch();
        layout->addLayout(row);
        layout->addStretch();
    }
    return settingsPage_;
}

QWidget* MainWindow::buildRacePage()
{
    racePage_ = new QWidget(this);
    racePage_->setObjectName(QStringLiteral("racePage"));
    racePage_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    auto* layout = new QVBoxLayout(racePage_);
    layout->setContentsMargins(UiConstants::HOME_OUTER_MARGIN,
                               UiConstants::STATUS_BAR_HEIGHT + UiConstants::HOME_OUTER_MARGIN,
                               UiConstants::HOME_OUTER_MARGIN, UiConstants::HOME_OUTER_MARGIN);
    layout->setSpacing(8);
    layout->addWidget(this->buildPageBandeau(QStringLiteral("MODE RACE"), "raceBackButton"));
    layout->addStretch();
    layout->addWidget(makeSubtitle(QStringLiteral("EN CONSTRUCTION — télémétrie à venir.")));
    layout->addStretch();
    return racePage_;
}

QWidget* MainWindow::buildCarPage()
{
    carPage_ = new QWidget(this);
    carPage_->setObjectName(QStringLiteral("carPage"));
    carPage_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    auto* layout = new QVBoxLayout(carPage_);
    layout->setContentsMargins(UiConstants::HOME_OUTER_MARGIN,
                               UiConstants::STATUS_BAR_HEIGHT + UiConstants::HOME_OUTER_MARGIN,
                               UiConstants::HOME_OUTER_MARGIN, UiConstants::HOME_OUTER_MARGIN);
    layout->setSpacing(8);
    layout->addWidget(this->buildPageBandeau(QStringLiteral("VOITURE"), "carBackButton"));
    layout->addStretch();
    layout->addWidget(makeSubtitle(QStringLiteral("EN CONSTRUCTION — réglages véhicule à venir.")));
    layout->addStretch();
    return carPage_;
}

QPushButton* MainWindow::makeLogoBackButton(QWidget* parent, int size, int iconSize, const char* objName)
{
    auto* btn = new QPushButton(parent);
    btn->setObjectName(QString::fromLatin1(objName));
    btn->setFixedSize(size, size);
    btn->setFlat(true);
    btn->setFocusPolicy(Qt::NoFocus);
    btn->setCursor(Qt::PointingHandCursor);
    auto* logo = new MercedesLogo(btn, iconSize);
    logo->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    logo->move((size - iconSize) / 2, (size - iconSize) / 2);
    return btn;
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
