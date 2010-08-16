/****************************************************************************
**
** This file is part of the CADuntu project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (caduntu@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software 
** Foundation and appearing in the file gpl-2.0.txt included in the
** packaging of this file.
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

