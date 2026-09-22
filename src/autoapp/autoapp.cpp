/*
*  This file is part of openauto project.
*  Copyright (C) 2018 f1x.studio (Michal Szwaj)
*
*  openauto is free software: you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 3 of the License, or
*  (at your option) any later version.

*  openauto is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with openauto. If not, see <http://www.gnu.org/licenses/>.
*/

#include <thread>
#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QFontDatabase>
#include <f1x/aasdk/USB/USBHub.hpp>
#include <f1x/aasdk/USB/ConnectedAccessoriesEnumerator.hpp>
#include <f1x/aasdk/USB/AccessoryModeQueryChain.hpp>
#include <f1x/aasdk/USB/AccessoryModeQueryChainFactory.hpp>
#include <f1x/aasdk/USB/AccessoryModeQueryFactory.hpp>
#include <f1x/aasdk/TCP/TCPWrapper.hpp>
#include <f1x/openauto/autoapp/App.hpp>
#include <f1x/openauto/autoapp/Configuration/IConfiguration.hpp>
#include <f1x/openauto/autoapp/Configuration/RecentAddressesList.hpp>
#include <f1x/openauto/autoapp/Service/AndroidAutoEntityFactory.hpp>
#include <f1x/openauto/autoapp/Service/ServiceFactory.hpp>
#include <f1x/openauto/autoapp/Projection/CanManager.hpp>
#include <f1x/openauto/autoapp/Configuration/Configuration.hpp>
#include <f1x/openauto/autoapp/UI/MainWindow.hpp>
#include <f1x/openauto/autoapp/UI/UiConstants.hpp>
#include <f1x/openauto/autoapp/UI/SettingsWindow.hpp>
#include <f1x/openauto/autoapp/UI/ConnectDialog.hpp>
#include <f1x/openauto/Common/Log.hpp>

namespace aasdk = f1x::aasdk;
namespace autoapp = f1x::openauto::autoapp;
using ThreadPool = std::vector<std::thread>;

void startUSBWorkers(boost::asio::io_context& ioService, libusb_context* usbContext, ThreadPool& threadPool)
{
    auto usbWorker = [&ioService, usbContext]() {
        timeval libusbEventTimeout{180, 0};

        while(!ioService.stopped())
        {
            libusb_handle_events_timeout_completed(usbContext, &libusbEventTimeout, nullptr);
        }
    };

    threadPool.emplace_back(usbWorker);
    threadPool.emplace_back(usbWorker);
    threadPool.emplace_back(usbWorker);
    threadPool.emplace_back(usbWorker);
}

void startIOServiceWorkers(boost::asio::io_context& ioService, ThreadPool& threadPool)
{
    auto ioServiceWorker = [&ioService]() {
        ioService.run();
    };

    threadPool.emplace_back(ioServiceWorker);
    threadPool.emplace_back(ioServiceWorker);
    threadPool.emplace_back(ioServiceWorker);
    threadPool.emplace_back(ioServiceWorker);
}

int main(int argc, char* argv[])
{
    libusb_context* usbContext;
    if(libusb_init(&usbContext) != 0)
    {
        OPENAUTO_LOG(error) << "[OpenAuto] libusb init failed.";
        return 1;
    }

    boost::asio::io_context ioService;
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work(ioService.get_executor());
    std::vector<std::thread> threadPool;
    startUSBWorkers(ioService, usbContext, threadPool);
    startIOServiceWorkers(ioService, threadPool);

    QApplication qApplication(argc, argv);

    // UI-2b: bundle Inter (assets/fonts/*.ttf) — fallback sans-serif if missing.
    // No sudo install required; fonts travel with the binary.
    {
        const QString fontDir = QCoreApplication::applicationDirPath() + QStringLiteral("/../assets/fonts");
        const QStringList fonts = {QStringLiteral("Inter-Regular.ttf"), QStringLiteral("Inter-Medium.ttf"),
                                   QStringLiteral("Inter-SemiBold.ttf"), QStringLiteral("Inter-Bold.ttf")};
        for(const auto& f : fonts)
        {
            const QString path = fontDir + QStringLiteral("/") + f;
            if(QFile::exists(path))
            {
                const int id = QFontDatabase::addApplicationFont(path);
                if(id < 0)
                {
                    OPENAUTO_LOG(warning) << "[OpenAuto] failed to load font " << path.toStdString();
                }
            }
        }
        QFont appFont(QStringLiteral("Inter"));
        QFontDatabase db;
        if(db.families().contains(QStringLiteral("Inter")))
        {
            appFont.setStyleHint(QFont::SansSerif);
            qApplication.setFont(appFont);
        }
    }

    auto configuration = std::make_shared<autoapp::configuration::Configuration>();
    autoapp::ui::SettingsWindow settingsWindow(configuration);

    // UI-2a single window: status overlay + [Home|AA|Settings|Race|Car]
    // stack inside MainWindow (theme loaded there, incl. night variant).
    // No separate top-level windows anymore (video embeds into the AA page).
    // Item3: the real SettingsWindow is embedded in the settings stack page
    // (reparented by MainWindow), so the page shows the actual config.
    autoapp::ui::MainWindow mainWindow(&settingsWindow);
    // Head-unit target: frameless fullscreen. OPENAUTO_WINDOWED=1 keeps a
    // fixed window for development on PC (see UiConstants).
    const bool windowed = qEnvironmentVariableIsSet("OPENAUTO_WINDOWED");
    if(windowed)
    {
        mainWindow.resize(autoapp::ui::UiConstants::WINDOWED_WIDTH,
                          autoapp::ui::UiConstants::WINDOWED_HEIGHT);
    }
    else
    {
        mainWindow.setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    }

    autoapp::configuration::RecentAddressesList recentAddressesList(7);
    recentAddressesList.read();

    aasdk::tcp::TCPWrapper tcpWrapper;
    autoapp::ui::ConnectDialog connectDialog(ioService, tcpWrapper, recentAddressesList);
    connectDialog.setWindowFlags(Qt::WindowStaysOnTopHint);

    QObject::connect(&mainWindow, &autoapp::ui::MainWindow::exit, []() { std::exit(0); });
    QObject::connect(&mainWindow, &autoapp::ui::MainWindow::openSettings, &mainWindow, &autoapp::ui::MainWindow::showSettingsPage);
    QObject::connect(&mainWindow, &autoapp::ui::MainWindow::openConnectDialog, &connectDialog, &autoapp::ui::ConnectDialog::exec);

    // Blank cursor is a head-unit behavior; keep the cursor visible in
    // windowed dev mode so the mouse can be used (and aimed) on PC.
    if(!windowed)
    {
        qApplication.setOverrideCursor(Qt::BlankCursor);
    }
    QObject::connect(&mainWindow, &autoapp::ui::MainWindow::toggleCursor, [&qApplication]() {
        // FIX UI-2b cursor: overrideCursor() is null when no override was
        // ever set (windowed dev) — dereferencing it segfaulted the toggle.
        const QCursor* current = qApplication.overrideCursor();
        const auto cursor = (current != nullptr && current->shape() == Qt::BlankCursor) ? Qt::ArrowCursor : Qt::BlankCursor;
        qApplication.setOverrideCursor(cursor);
    });

    if(windowed)
    {
        mainWindow.show();
    }
    else
    {
        mainWindow.showFullScreen();
    }

    // MISSION 1: Create CanManager at app level (shared with App and ServiceFactory)
    auto canManager = std::make_shared<autoapp::projection::CanManager>(ioService);
    // Race Mode v1 fix: the bridge thread must actually RUN — start it at
    // launch (UI stubs flow always, buttons stay session-gated inside).
    // Stopped at shutdown by App::stop(). Without this, no CAN frame
    // (cansend / sim.py) was ever read.
    canManager->start();

    aasdk::usb::USBWrapper usbWrapper(usbContext);
    aasdk::usb::AccessoryModeQueryFactory queryFactory(usbWrapper, ioService);
    aasdk::usb::AccessoryModeQueryChainFactory queryChainFactory(usbWrapper, ioService, queryFactory);
    autoapp::service::ServiceFactory serviceFactory(ioService, configuration, canManager);
    autoapp::service::AndroidAutoEntityFactory androidAutoEntityFactory(ioService, configuration, serviceFactory);

    auto usbHub(std::make_shared<aasdk::usb::USBHub>(usbWrapper, ioService, queryChainFactory));
    auto connectedAccessoriesEnumerator(std::make_shared<aasdk::usb::ConnectedAccessoriesEnumerator>(usbWrapper, ioService, queryChainFactory));
    auto app = std::make_shared<autoapp::App>(ioService, usbWrapper, tcpWrapper, androidAutoEntityFactory, std::move(usbHub), std::move(connectedAccessoriesEnumerator), canManager);

    QObject::connect(&connectDialog, &autoapp::ui::ConnectDialog::connectionSucceed, [&app](auto socket) {
        app->start(std::move(socket));
    });

    app->waitForUSBDevice();

    auto result = qApplication.exec();
    std::for_each(threadPool.begin(), threadPool.end(), std::bind(&std::thread::join, std::placeholders::_1));

    libusb_exit(usbContext);
    return result;
}
