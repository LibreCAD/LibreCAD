/*
**********************************************************************************
**
** This file is part of the LibreCAD project (librecad.org), a 2D CAD program.
**
** Copyright (C) 2024 LibreCAD.org
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**
**********************************************************************************
*/

#ifndef LG_DIMZEROSBOX_H
#define LG_DIMZEROSBOX_H

#include <QComboBox>
#include <QStandardItemModel>
#include <QListView>

/*
 * DimZin value is mixed integer and bit flag value
 * inches and feet are integer values, removal of left and right zeros are flags
 * 0: removes 0' & 0"
 * 1: draw 0' & 0"
 * 2: removes 0"
 * 4: removes 0'
 * bit 3 set (4) remove 0 to the left
 * bit 4 set (8) removes 0's to the right
 *
 * DimAZin value is integer or bit flag value
 * 0: draw all
 * 1: remove 0 to the left
 * 2: removes 0's to the right
 * 3: removes all zeros
*/

class LG_DimzerosBox : public QComboBox {
    Q_OBJECT

public:
    explicit LG_DimzerosBox(QWidget *parent = 0);
    ~LG_DimzerosBox();
    void setLinear();
    void setData(int i);
    int getData();
private:
    QStandardItemModel *model = nullptr;
    QListView *view = nullptr;
    bool dimLine =false;
    int convertDimZin(int v, bool toIdx);
};

#endif // LG_DIMZEROSBOX_H
