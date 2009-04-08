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

#ifndef __PictureProperties_h__
#define __PictureProperties_h__

#include "AbstractProperties.h"
class QListWidgetItem;
class PictureContent;
namespace Ui { class PictureProperties; }


class PictureProperties : public AbstractProperties {
    Q_OBJECT
    public:
        PictureProperties(PictureContent * pictureContent, QGraphicsItem * parent = 0);
        ~PictureProperties();

    Q_SIGNALS:
        void applyEffectToAll(int effectClass);

    private:
        Ui::PictureProperties * m_pictureUi;
        PictureContent *        m_pictureContent;

    private Q_SLOTS:
        void slotEffectSelected(QListWidgetItem * item);
};

#endif
