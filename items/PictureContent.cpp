/***************************************************************************
 *                                                                         *
 *   This file is part of the FotoWall project,                            *
 *       http://code.google.com/p/fotowall                                 *
 *                                                                         *
 *   Copyright (C) 2007-2009 by Enrico Ros <enrico.ros@gmail.com>          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "PictureContent.h"
#include "ButtonItem.h"
#include "CPixmap.h"
#include "RenderOpts.h"
#include "frames/Frame.h"
#include <QFileInfo>
#include <QGraphicsScene>
#include <QGraphicsSceneDragDropEvent>
#include <QMimeData>
#include <QPainter>
#include <QUrl>

PictureContent::PictureContent(QGraphicsScene * scene, QGraphicsItem * parent)
    : AbstractContent(scene, parent, false)
    , m_photo(0)
    , m_opaquePhoto(false)
{
    // enable frame text
    setFrameTextEnabled(true);
    setFrameText(tr("..."));

    // add flipping buttons
    ButtonItem * bFlipH = new ButtonItem(ButtonItem::FlipH, Qt::blue, QIcon(":/data/action-flip-horizontal.png"), this);
    bFlipH->setToolTip(tr("Flip horizontally"));
    bFlipH->setFlag(QGraphicsItem::ItemIgnoresTransformations, false);
    connect(bFlipH, SIGNAL(clicked()), this, SIGNAL(flipHorizontally()));
    addButtonItem(bFlipH);

    ButtonItem * bFlipV = new ButtonItem(ButtonItem::FlipV, Qt::blue, QIcon(":/data/action-flip-vertical.png"), this);
    bFlipV->setToolTip(tr("Flip vertically"));
    bFlipV->setFlag(QGraphicsItem::ItemIgnoresTransformations, false);
    addButtonItem(bFlipV);
    connect(bFlipV, SIGNAL(clicked()), this, SIGNAL(flipVertically()));
}

PictureContent::~PictureContent()
{
    delete m_photo;
}

bool PictureContent::loadPhoto(const QString & fileName, bool keepRatio, bool setName)
{
    delete m_photo;
    m_cachedPhoto = QPixmap();
    m_opaquePhoto = false;
    m_photo = new CPixmap(fileName);
    if (m_photo->isNull()) {
        delete m_photo;
        m_photo = 0;
        m_filePath = QString();
        return false;
    }
    m_opaquePhoto = !m_photo->hasAlpha();
    m_filePath = fileName;
    if (keepRatio)
        adjustSize();
    if (setName) {
        QString string = QFileInfo(fileName).fileName().section('.', 0, 0);
        string = string.mid(0, 10);
        setFrameText(string + tr("..."));
    }
    update();
    GFX_CHANGED();
    return true;
}

void PictureContent::addEffect(const CEffect & effect)
{
    m_photo->addEffect(effect);
    m_cachedPhoto = QPixmap();
    update();
    GFX_CHANGED();
}

QString PictureContent::getFilePath() const
{
    return m_filePath;
}

QPixmap PictureContent::renderAsBackground(const QSize & size) const
{
    if (m_photo)
        return m_photo->scaled(size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    return AbstractContent::renderAsBackground(size);
}

int PictureContent::contentHeightForWidth(int width) const
{
    if (!m_photo || m_photo->width() < 1)
        return -1;
    return (m_photo->height() * width) / m_photo->width();
}

bool PictureContent::contentOpaque() const
{
    return m_opaquePhoto;
}

void PictureContent::dropEvent(QGraphicsSceneDragDropEvent * event)
{
    // load the first valid picture
    foreach (QUrl url, event->mimeData()->urls()) {
        if (loadPhoto(url.toLocalFile(), true, false)) {
            event->accept();
            return;
        }
    }
}

void PictureContent::mouseDoubleClickEvent(QGraphicsSceneMouseEvent * event)
{
    emit backgroundMe();
    QGraphicsItem::mouseDoubleClickEvent(event);
}

void PictureContent::paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget)
{
    // paint parent
    AbstractContent::paint(painter, option, widget);

    // skip if no photo
    if (!m_photo)
        return;

    // blit if opaque picture
#if QT_VERSION >= 0x040500
    //disabled for 4.5 too, since it relies on raster.
    //if (m_opaquePhoto)
    //    painter->setCompositionMode(QPainter::CompositionMode_Source);
#endif

    // draw high-resolution photo when exporting png
    QRect targetRect = contentsRect();
    if (RenderOpts::HQRendering) {
        painter->setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
        painter->drawPixmap(targetRect, *m_photo);
        return;
    }

    // draw photo using caching and deferred rescales
    if (beingTransformed()) {
        if (!m_cachedPhoto.isNull())
            painter->drawPixmap(targetRect, m_cachedPhoto);
    } else {
        if (m_cachedPhoto.isNull() || m_cachedPhoto.size() != targetRect.size())
            m_cachedPhoto = m_photo->scaled(targetRect.size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        painter->setRenderHints(QPainter::SmoothPixmapTransform);
        painter->drawPixmap(targetRect.topLeft(), m_cachedPhoto);
    }

#if QT_VERSION >= 0x040500
//    if (m_opaquePhoto)
//        painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
#endif
}

CPixmap * PictureContent::getCPixmap() const
{
    return m_photo;
}

void PictureContent::flipH()
{
    m_photo->toHFlip();
    m_cachedPhoto = QPixmap();
    update();
    GFX_CHANGED();
}

void PictureContent::flipV()
{
    m_photo->toVFlip();
    m_cachedPhoto = QPixmap();
    update();
    GFX_CHANGED();
}
