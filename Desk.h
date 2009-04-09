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

#ifndef __Desk_h__
#define __Desk_h__

#include <QGraphicsScene>
#include <QDataStream>
#include <QPainter>
#include <QPixmap>
#include <QRect>
class ColorPickerItem;
class HelpItem;
class HighlightItem;
class AbstractContent;
class AbstractProperties;
class PictureContent;
class TextContent;

class Desk : public QGraphicsScene
{
    Q_OBJECT
    public:
        Desk(QObject * parent = 0);
        ~Desk();

        // add content
        void addPictures(const QStringList & fileNames);
        void addText();

        // resize the scene to 0,0,size
        void resize(const QSize & size);

        // set the title text
        QString titleText() const;
        void setTitleText(const QString & text);

        // save, restore, load, help
        void showHelp();
        void save(QDataStream & data) const;
        void restore(QDataStream & data);

    protected:
        void dragEnterEvent( QGraphicsSceneDragDropEvent * event );
        void dragMoveEvent( QGraphicsSceneDragDropEvent * event );
        void dropEvent( QGraphicsSceneDragDropEvent * event );
        void mouseDoubleClickEvent( QGraphicsSceneMouseEvent * event );
        void contextMenuEvent( QGraphicsSceneContextMenuEvent * event );
        void drawBackground( QPainter * painter, const QRectF & rect );
        void drawForeground( QPainter * painter, const QRectF & rect );

    private:
        PictureContent * createPicture(const QPoint & pos);
        TextContent * createText(const QPoint & pos);
        QList<AbstractContent *> m_content;
        QList<AbstractProperties *> m_properties;
        QList<HighlightItem *> m_highlightItems;
        HelpItem * m_helpItem;
        AbstractContent * m_backContent;
        ColorPickerItem * m_titleColorPicker;
        ColorPickerItem * m_foreColorPicker;
        ColorPickerItem * m_grad1ColorPicker;
        ColorPickerItem * m_grad2ColorPicker;
        bool m_topBarEnabled;
        bool m_bottomBarEnabled;
        QSize m_size;
        QRectF m_rect;
        QString m_titleText;
        QPixmap m_backCache;

    private Q_SLOTS:
        void slotConfigureContent(const QPoint & scenePoint);
        void slotBackgroundContent();
        void slotStackContent(int);
        void slotDeleteContent();
        void slotDeleteProperties();
        void slotApplyLooks(quint32 frameClass, bool mirrored);
        void slotApplyEffects(int effectClass);

        void slotSetTopBarEnabled(bool enabled);
        void slotSetBottomBarEnabled(bool enabled);
        void slotSetTitle();
        void slotClearTitle();
        void slotTitleColorChanged();
        void slotForeColorChanged();
        void slotGradColorChanged();

        void slotCloseHelp();
};

#endif
