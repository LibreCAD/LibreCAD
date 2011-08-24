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

#ifndef QG_SCROLLBAR_H
#define QG_SCROLLBAR_H

#include <qscrollbar.h>
#include <QWheelEvent>

/**
 * A small wrapper for the Qt scrollbar. This class offers a slot
 * for scroll events.
 */
class QG_ScrollBar: public QScrollBar {
    Q_OBJECT

public:
    QG_ScrollBar(QWidget* parent=0)
            : QScrollBar(parent) {}
    QG_ScrollBar(Qt::Orientation orientation,
                 QWidget* parent=0)
            : QScrollBar(orientation, parent) {}

public slots:
    void slotWheelEvent(QWheelEvent* e) {
        wheelEvent(e);
    }

};

#endif

