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
#include <QVBoxLayout>
#include <QWidget>
#include <f1x/openauto/autoapp/Projection/QtVideoOutput.hpp>
#include <f1x/openauto/autoapp/UI/HuEvents.hpp>
#include <f1x/openauto/Common/Log.hpp>

namespace f1x
{
namespace openauto
{
namespace autoapp
{
namespace projection
{

QtVideoOutput::QtVideoOutput(configuration::IConfiguration::Pointer configuration)
    : VideoOutput(std::move(configuration))
    , videoWidget_(nullptr)
{
    this->moveToThread(QApplication::instance()->thread());
    connect(this, &QtVideoOutput::startPlayback, this, &QtVideoOutput::onStartPlayback, Qt::QueuedConnection);
    connect(this, &QtVideoOutput::stopPlayback, this, &QtVideoOutput::onStopPlayback, Qt::QueuedConnection);

    QMetaObject::invokeMethod(this, "createVideoOutput", Qt::BlockingQueuedConnection);
}

QtVideoOutput::~QtVideoOutput()
{
    // Embedded widgets are owned by the host page (app lifetime); only
    // the legacy parentless fallback window is owned here.
    if(videoWidget_ != nullptr && videoWidget_->parent() == nullptr)
    {
        delete videoWidget_;
    }
}

void QtVideoOutput::createVideoOutput()
{
    OPENAUTO_LOG(debug) << "[QtVideoOutput] create.";
    videoWidget_ = new QVideoWidget(ui::HuEvents::videoHost());
    mediaPlayer_ = std::make_unique<QMediaPlayer>(nullptr, QMediaPlayer::StreamPlayback);
}


bool QtVideoOutput::open()
{
    return videoBuffer_.open(QIODevice::ReadWrite);
}

bool QtVideoOutput::init()
{
    emit startPlayback();
    return true;
}

void QtVideoOutput::stop()
{
    emit stopPlayback();
}

void QtVideoOutput::write(uint64_t, const aasdk::common::DataConstBuffer& buffer)
{
    videoBuffer_.write(reinterpret_cast<const char*>(buffer.cdata), buffer.size);
}

void QtVideoOutput::onStartPlayback()
{
    QWidget* host = ui::HuEvents::videoHost();
    videoWidget_->setAspectRatioMode(Qt::IgnoreAspectRatio);

    if(host != nullptr && host->layout() != nullptr)
    {
        // UI-2a single window: the video widget lives in the AA stack
        // page. Drop widgets orphaned by previous sessions (their
        // QtVideoOutput released ownership, see destructor), then embed.
        // The GStreamer pipeline below keeps running untouched: hiding
        // the page later only hides presentation, never the session.
        const auto stale = host->findChildren<QVideoWidget*>(QString(), Qt::FindDirectChildrenOnly);
        for(auto* widget : stale)
        {
            if(widget != videoWidget_)
            {
                host->layout()->removeWidget(widget);
                widget->deleteLater();
            }
        }
        if(videoWidget_->parent() != host)
        {
            videoWidget_->setParent(host);
        }
        if(auto* box = qobject_cast<QVBoxLayout*>(host->layout()))
        {
            box->addWidget(videoWidget_);
        }
        else
        {
            videoWidget_->setGeometry(host->rect());
        }
        videoWidget_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        videoWidget_->show();
        videoWidget_->setFocus();
    }
    else
    {
        // Legacy fallback (no host registered, e.g. unit context):
        // separate fullscreen top-level window, as before UI-2a.
        videoWidget_->setFocus();
        videoWidget_->setWindowFlags(Qt::WindowStaysOnTopHint);
        videoWidget_->setFullScreen(true);
        videoWidget_->show();
    }

    mediaPlayer_->setVideoOutput(videoWidget_);
    mediaPlayer_->setMedia(QMediaContent(), &videoBuffer_);
    mediaPlayer_->play();
    ui::HuEvents::notifyVideoStarted();
}

void QtVideoOutput::onStopPlayback()
{
    videoWidget_->hide();
    mediaPlayer_->stop();
    ui::HuEvents::notifyVideoStopped();
}

}
}
}
}
