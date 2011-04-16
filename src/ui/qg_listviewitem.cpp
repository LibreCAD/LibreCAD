/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010-2011 R. van Twisk (librecad@rvt.dds.nl)
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

#include "qg_listviewitem.h"

#include <qpixmap.h>

/**
 * Constructor for list view items with a folder icon.
 */
QG_ListViewItem::QG_ListViewItem(QG_ListViewItem* par,
                                 const QString& label,
                                 bool open,
                                 int id)
        : Q3ListViewItem(par) {
    this->par = par;
    this->label = label;
    this->id = id;

    setPixmap(0, QPixmap((open ? QPixmap(":/uit/folderopen.png") : QPixmap(":/uit/folderclosed.png"))));
    setOpen(open);
}



/**
 * Constructor for root items.
 */
QG_ListViewItem::QG_ListViewItem(Q3ListView * par,
                                 const QString& label,
                                 bool open,
                                 int id)
        : Q3ListViewItem(par) {
		
    par = 0;
    this->label = label;
    this->id = id;

    setPixmap(0, QPixmap((open ? QPixmap(":/uit/folderopen.png") : QPixmap(":/uit/folderclosed.png"))));

    setOpen(open);
}



/**
 * Opens or closes the item.
 */
void QG_ListViewItem::setOpen(bool open) {
    if (open==true) {
        setPixmap(0, QPixmap(QPixmap(":/uit/folderopen.png")));
	}
    else {
        setPixmap(0, QPixmap(QPixmap(":/uit/folderclosed.png")));
	}

    Q3ListViewItem::setOpen(open);
}



/**
 * Called in the beginning.
 */
void QG_ListViewItem::setup() {
    Q3ListViewItem::setup();
}



/**
 * Returns the "path" of this item (like: "Project/Page1/Paragraph1/").
 */
QString QG_ListViewItem::getFullPath() {
    QString s;
    if (par!=NULL) {
        s = par->getFullPath();
        s.append(text(0));
        s.append("/");
    } else {
        s = text(0);
        s.append("/");
    }
    return s;
}


/**
 * Returns the text of the given column of this item.
 */
QString QG_ListViewItem::text(int column) const {
    if (column==0) {
        return label;
	}
    else {
        return "Column1";
	}
}

