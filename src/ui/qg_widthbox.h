/****************************************************************************
**
** This file is part of the CADuntu project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (caduntu@rvt.dds.nl)
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

#ifndef QG_WIDTHBOX_H
#define QG_WIDTHBOX_H

#include <qcombobox.h>

#include "rs.h"

/**
 * A combobox for choosing a line width.
 */
class QG_WidthBox: public QComboBox {
    Q_OBJECT

public:
    QG_WidthBox(QWidget* parent=0, const char* name=0);
    QG_WidthBox(bool showByLayer, bool showUnchanged,
                QWidget* parent=0, const char* name=0);
    virtual ~QG_WidthBox();

    RS2::LineWidth getWidth() {
        return currentWidth;
    }
    void setWidth(RS2::LineWidth w);
    void setLayerWidth(RS2::LineWidth w);

    void init(bool showByLayer, bool showUnchanged);

    bool isUnchanged() {
        return unchanged;
    }

private slots:
    void slotWidthChanged(int index);

signals:
    void widthChanged(RS2::LineWidth);

private:
    RS2::LineWidth currentWidth;
    bool showByLayer;
    bool showUnchanged;
    bool unchanged;
};

#endif

