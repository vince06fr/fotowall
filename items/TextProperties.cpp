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

#include "TextProperties.h"
#include "3rdparty/richtextedit/richtexteditor_p.h"
#include "TextContent.h"
#include <QSettings>

TextProperties::TextProperties(TextContent * textContent, QGraphicsItem * parent)
    : AbstractProperties(textContent, parent)
    , m_textContent(textContent)
{
    // Inject Editor setup
    m_editor = new RichTextEditorDialog();
    m_editor->setMinimumSize(430, 200);
    m_editor->setText(m_textContent->toHtml());
    m_editor->adjustSize();
    addTab(m_editor, tr("Text"), false, true);
    m_editor->setFocus();

    // callback for closure
    connect(this, SIGNAL(closing()), this, SLOT(slotClosing()));
}

TextProperties::~TextProperties()
{
}

void TextProperties::slotClosing()
{
    m_textContent->setHtml(m_editor->text(Qt::RichText));
}
