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
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QShortcut>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <f1x/openauto/autoapp/UI/HuEvents.hpp>
#include <f1x/openauto/autoapp/UI/MainWindow.hpp>
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
    label->setAlignment(Qt::AlignCenter);
    label->setWordWrap(true);
    return label;
}

}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , statusBar_(new StatusBar(this))
    , stack_(new QStackedWidget(this))
    , aaPage_(nullptr)
    , aaPlaceholder_(nullptr)
    , nightButton_(nullptr)
    , night_(false)
{
    auto* central = new QWidget(this);
    auto* layout = new QVBoxLayout(central);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(statusBar_);
    layout->addWidget(stack_, 1);
    this->setCentralWidget(central);

    stack_->addWidget(this->buildHomePage());      // HOME_PAGE = 0
    stack_->addWidget(this->buildAAPage());        // AA_PAGE = 1
    stack_->addWidget(this->buildSettingsPage());  // SETTINGS_PAGE = 2
    stack_->setCurrentIndex(HOME_PAGE);
    HuEvents::setAaPageActive(false);

    // The AA stack page hosts the QtVideoOutput widget (registered here,
    // embedded at session start — replaces the separate video window).
    HuEvents::setVideoHost(aaPage_);

    // Backend events (one-way): video switches pages, CAN night mode
    // re-themes. Direct connections: all emitted from the GUI thread
    // except nightModeChanged (CanBridge thread → auto-queued by Qt).
    connect(&HuEvents::instance(), &HuEvents::videoStarted, this, &MainWindow::onVideoStarted);
    connect(&HuEvents::instance(), &HuEvents::videoStopped, this, &MainWindow::onVideoStopped);
    connect(&HuEvents::instance(), &HuEvents::nightModeChanged, this, &MainWindow::setNightMode);

    // Dev shortcuts, no menubar on the head unit. S/E/C/W are the UI-1
    // actions; 1/2/3 navigate the stack. Note: while the AA page is
    // shown during a session, InputDevice eats every key except 1/2/3
    // (forwarded to the phone), so only navigation works there — same
    // as UI-1 where all shortcuts were dead under the video.
    connect(new QShortcut(QKeySequence(Qt::Key_S), this), &QShortcut::activated, this, &MainWindow::openSettings);
    connect(new QShortcut(QKeySequence(Qt::Key_E), this), &QShortcut::activated, this, &MainWindow::exit);
    connect(new QShortcut(QKeySequence(Qt::Key_C), this), &QShortcut::activated, this, &MainWindow::toggleCursor);
    connect(new QShortcut(QKeySequence(Qt::Key_W), this), &QShortcut::activated, this, &MainWindow::openConnectDialog);
    connect(new QShortcut(QKeySequence(Qt::Key_1), this), &QShortcut::activated, this, &MainWindow::showHomePage);
    connect(new QShortcut(QKeySequence(Qt::Key_2), this), &QShortcut::activated, this, &MainWindow::showAAPage);
    connect(new QShortcut(QKeySequence(Qt::Key_3), this), &QShortcut::activated, this, &MainWindow::showSettingsPage);

    this->applyTheme();
}

MainWindow::~MainWindow() = default;

void MainWindow::showHomePage()
{
    stack_->setCurrentIndex(HOME_PAGE);
    HuEvents::setAaPageActive(false);
}

void MainWindow::showAAPage()
{
    stack_->setCurrentIndex(AA_PAGE);
    HuEvents::setAaPageActive(true);
}

void MainWindow::showSettingsPage()
{
    stack_->setCurrentIndex(SETTINGS_PAGE);
    HuEvents::setAaPageActive(false);
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
    if(aaPlaceholder_ != nullptr)
    {
        aaPlaceholder_->hide();
    }
    this->showAAPage();
}

void MainWindow::onVideoStopped()
{
    if(aaPlaceholder_ != nullptr)
    {
        aaPlaceholder_->show();
    }
    this->showHomePage();
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
    auto* layout = new QVBoxLayout(page);
    layout->addStretch();
    layout->addWidget(makeTitle(QStringLiteral("ACCUEIL")));
    layout->addWidget(makeSubtitle(QStringLiteral("En attente de l'appareil… (squelette UI-2a)")));
    layout->addStretch();

    // Dev actions (ex boutons de l'écran d'attente UI-1, inchangés).
    auto* buttons = new QWidget(page);
    auto* row = new QHBoxLayout(buttons);
    row->setContentsMargins(UiConstants::STATUS_BAR_MARGIN, 0,
                            UiConstants::STATUS_BAR_MARGIN, 0);
    auto* settingsButton = new QPushButton(QStringLiteral("Settings (S)"), buttons);
    auto* wirelessButton = new QPushButton(QStringLiteral("Wireless (W)"), buttons);
    auto* cursorButton = new QPushButton(QStringLiteral("Cursor (C)"), buttons);
    auto* exitButton = new QPushButton(QStringLiteral("Exit (E)"), buttons);
    nightButton_ = new QPushButton(QStringLiteral("Night mode"), buttons);
    nightButton_->setCheckable(true);
    nightButton_->setChecked(night_);
    connect(settingsButton, &QPushButton::clicked, this, &MainWindow::openSettings);
    connect(wirelessButton, &QPushButton::clicked, this, &MainWindow::openConnectDialog);
    connect(cursorButton, &QPushButton::clicked, this, &MainWindow::toggleCursor);
    connect(exitButton, &QPushButton::clicked, this, &MainWindow::exit);
    connect(nightButton_, &QPushButton::toggled, this, &MainWindow::setNightMode);
    row->addStretch();
    row->addWidget(settingsButton);
    row->addWidget(wirelessButton);
    row->addWidget(cursorButton);
    row->addWidget(nightButton_);
    row->addWidget(exitButton);
    row->addStretch();
    layout->addWidget(buttons);

    auto* hint = makeSubtitle(QStringLiteral("1=Accueil  2=AA  3=Paramètres"));
    layout->addWidget(hint);
    layout->addStretch();
    return page;
}

QWidget* MainWindow::buildAAPage()
{
    aaPage_ = new QWidget(this);
    auto* layout = new QVBoxLayout(aaPage_);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    // Squelette visible avant la première projection (caché dès que la
    // vidéo s'incruste, réaffiché à la fin de session).
    aaPlaceholder_ = makeSubtitle(QStringLiteral("Projection AA\nEn attente de vidéo…"));
    aaPlaceholder_->setObjectName(QStringLiteral("pageTitle"));
    layout->addStretch();
    layout->addWidget(aaPlaceholder_);
    layout->addStretch();
    return aaPage_;
}

QWidget* MainWindow::buildSettingsPage()
{
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);
    layout->addStretch();
    layout->addWidget(makeTitle(QStringLiteral("PARAMÈTRES")));
    layout->addWidget(makeSubtitle(QStringLiteral("Squelette UI-2a — le design vient en 2b.")));
    layout->addStretch();

    auto* advancedButton = new QPushButton(QStringLiteral("Réglages avancés… (S)"), page);
    connect(advancedButton, &QPushButton::clicked, this, &MainWindow::openSettings);
    auto* row = new QHBoxLayout();
    row->addStretch();
    row->addWidget(advancedButton);
    row->addStretch();
    layout->addLayout(row);
    layout->addStretch();
    return page;
}

}
}
}
}
