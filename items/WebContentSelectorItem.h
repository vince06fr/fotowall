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

#ifndef __WebContentSelectorItem_h__
#define __WebContentSelectorItem_h__

#include <QGraphicsWidget>
class Frame;
class GSuggestCompletion;
class Ui_WebContentSelectorItem;

class WebContentSelectorItem : public QGraphicsWidget
{
    Q_OBJECT
    public:
        WebContentSelectorItem(QGraphicsItem * parent = 0);
        ~WebContentSelectorItem();

        // ::QGraphicsItem
        void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0);

    private:
        Frame * m_frame;
        GSuggestCompletion * m_completion;
        Ui_WebContentSelectorItem * m_ui;

    private Q_SLOTS:
        void doSearch();
};

#endif
