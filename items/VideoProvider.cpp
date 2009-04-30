/***************************************************************************
 *                                                                         *
 *   This file is part of the FotoWall project,                            *
 *       http://code.google.com/p/fotowall                                 *
 *                                                                         *
 *   Copyright (C) 2009 by Enrico Ros <enrico.ros@gmail.com>               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "VideoProvider.h"
#include <QDirIterator>
#include <QTimer>
#if defined(HAS_VIDEOCAPTURE)
#include "3rdparty/videocapture/VideoDevice_linux.h"
#endif

// the global video provider instance
Q_GLOBAL_STATIC(VideoProvider, s_providerInstance)
VideoProvider * VideoProvider::instance()
{
    return s_providerInstance();
}

VideoProvider::VideoProvider()
    : QObject()
    , m_snapTimer(0)
{
    // defer video initialization, to show gui...
    QTimer::singleShot(50, this, SLOT(scanDevices()));
}

VideoProvider::~VideoProvider()
{
    delete m_snapTimer;
    qDeleteAll(m_inputs);
}

int VideoProvider::inputCount() const
{
    return m_inputs.size();
}

void VideoProvider::connectInput(int iIdx, QObject * receiver, const char * method)
{
    // safety check
    if (iIdx < 0 || iIdx >= m_inputs.size() || !receiver || !method) {
        qWarning("VideoProvider::connectInput: fail");
        return;
    }

    // connect input
    VideoInput * input = m_inputs[iIdx];
    if (!connect(input, SIGNAL(newPixmap(QPixmap)), receiver, method)) {
        qWarning("VideoProvider::connectInput: error connecting input %d to %s", iIdx, method);
        return;
    }

    // add it to the receivers list (for ref. counting only)
    input->receivers.append(receiver);

    // start the video if we're the first
    if (!input->active) {
        // start the video
#if defined(HAS_VIDEOCAPTURE)
        input->device->setSize(input->device->maxWidth(), input->device->maxHeight());
        input->device->startCapturing();
#else
        // TODO
#endif

        // mark as active
        input->active = true;
    }

    // start the capture timer if needed
    if (!m_snapTimer) {
        m_snapTimer = new QTimer(this);
        connect(m_snapTimer, SIGNAL(timeout()), this, SLOT(slotCaptureVideoFrames()));
        m_snapTimer->start(50);
    }
}

void VideoProvider::disconnectReceiver(QObject * receiver)
{
    if (!receiver)
        return;

    // remove the receiver from each input
    int activeInputs = 0;
    foreach (VideoInput * input, m_inputs) {

        // perform the disconnection
        disconnect(input, 0, receiver, 0);

        // remove the receiver, if any
        input->receivers.removeAll(receiver);

        // stop video if cleared all receivers
        if (input->receivers.isEmpty() && input->active) {

            // stop capturing
#if defined(HAS_VIDEOCAPTURE)
            input->device->stopCapturing();
#else
            // TODO
#endif

            // mark as inactive
            input->active = false;
        }

        // if active, increment active input count
        if (input->active)
            activeInputs++;
    }

    // stop the timer too if not needed
    if (!activeInputs && m_snapTimer) {
        delete m_snapTimer;
        m_snapTimer = 0;
    }
}

void VideoProvider::setSwapped(int iIdx, bool swapped)
{
    if (iIdx >= 0 && iIdx < m_inputs.size())
        m_inputs[iIdx]->swapped = swapped;
}

bool VideoProvider::swapped(int iIdx) const
{
    if (iIdx >= 0 && iIdx < m_inputs.size())
        return m_inputs[iIdx]->swapped;
    return false;
}

void VideoProvider::scanDevices()
{
#if defined(Q_OS_LINUX)
    QDirIterator dirIt("/dev", QStringList() << "video*", QDir::Files | QDir::System);
    while (dirIt.hasNext())
        slotInitVideo(dirIt.next());
#else
    qWarning("VideoProvider::scanDevices: not implemented for your system");
#endif
}

void VideoProvider::slotInitVideo(const QString & device)
{
#if defined(HAS_VIDEOCAPTURE)
    // create a new capture device and initialize it
    VideoCapture::VideoDevice * capture = new VideoCapture::VideoDevice();
    capture->setFileName(device);
    capture->open();
    if (!capture->isOpen() || capture->inputs() < 1) {
        delete capture;
        return;
    }

    // setup input
    //capture->close();         // leave capture open...
    //capture->selectInput(0);  // this is not useful
    capture->setInputParameters();

    // if everything's ok setup the device and add it to the internal queue
    VideoInput * input = new VideoInput();
    input->device = capture;
    m_inputs.append(input);

    // update status
    emit inputCountChanged(m_inputs.size());
#else
    // TODO
    Q_UNUSED(device);
#endif
}

void VideoProvider::slotCaptureVideoFrames()
{
    // capture from every input to every receiver
    foreach (VideoInput * input, m_inputs) {

        // skip inactive inputs
        if (!input->active)
            continue;

#if defined(HAS_VIDEOCAPTURE)
        // get frame (and check correctness)
        if (input->device->getFrame() != EXIT_SUCCESS)
            continue;

        // get the qimage from the frame
        QImage frameImage;
        input->device->getImage(&frameImage);

        // apply mirror, if requested
        if (input->swapped)
            frameImage = frameImage.mirrored(true, false);

        // set the pixmap
        emit input->newPixmap(QPixmap::fromImage(frameImage));
#else
        //TODO
#endif
    }
}

VideoInput::VideoInput()
    : active(false)
    , swapped(false)
{
}

VideoInput::~VideoInput()
{
#if defined(HAS_VIDEOCAPTURE)
    if (active)
        device->stopCapturing();
    device->close();
    delete device;
#endif
}
