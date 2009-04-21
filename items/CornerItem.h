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

#ifndef __CornerItem_h__
#define __CornerItem_h__

#include <QGraphicsItem>
#include <QPointF>
class AbstractContent;

class CornerItem : public QGraphicsItem
{
    public:
        CornerItem(Qt::Corner corner, AbstractContent * parent);

        void relayout(const QRect & rect);

        // ::QGraphicsItem
        QRectF boundingRect() const;
        void mouseDoubleClickEvent(QGraphicsSceneMouseEvent * event);
        void mousePressEvent(QGraphicsSceneMouseEvent * event);
        void mouseMoveEvent(QGraphicsSceneMouseEvent * event);
        void mouseReleaseEvent(QGraphicsSceneMouseEvent * event);
        void paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0);

    private:
        enum Operation {
            Off         = 0x00,
            Rotate      = 0x01,
            FixRotate   = 0x02,
            Scale       = 0x10,
            FixScale    = 0x20,
        };
        AbstractContent * m_content;
        Qt::Corner m_corner;
        int m_opMask;
        int m_side;
        int m_operation;
        double m_startRatio;
};

#endif
