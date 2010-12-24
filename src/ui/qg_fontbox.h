/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by 
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
**
** This copyright notice MUST APPEAR in all copies of the script!  
**
**********************************************************************/

#ifndef QG_FONTBOX_H
#define QG_FONTBOX_H

#include <qcombobox.h>

#include "rs.h"
#include "rs_font.h"
#include "rs_fontlist.h"

/**
 * A combobox for choosing a font name.
 */
class QG_FontBox: public QComboBox {
    Q_OBJECT

public:
    QG_FontBox(QWidget* parent=0, const char* name=0);
    virtual ~QG_FontBox();

    RS_Font* getFont() {
        return currentFont;
    }
    void setFont(const RS_String& fName);

    void init();

private slots:
    void slotFontChanged(int index);

signals:
    void fontChanged(RS_Font* font);

private:
    RS_Font* currentFont;
};

#endif

