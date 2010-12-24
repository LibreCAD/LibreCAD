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

#ifndef QG_LISTVIEWITEM_H
#define QG_LISTVIEWITEM_H

#include <q3listview.h>


/**
 * An item in a hierarchical list view with a nice folder icon.
 */
class QG_ListViewItem : public Q3ListViewItem {
public:
    QG_ListViewItem(Q3ListView *par, const QString& label,
                    bool open=false, int id=-1);
    QG_ListViewItem(QG_ListViewItem *par, const QString& label,
                    bool open=false, int id=-1);

    QString getFullPath();
    QString text(int column) const;

    QString getLabel() const {
        return label;
    }

    void setOpen(bool o);
    void setup();

    void setId(int id) {
        this->id = id;
    }
    int getId() {
        return id;
    }

private:
    QG_ListViewItem* par;
    QString label;
    QString object;
    int id;
};

#endif

