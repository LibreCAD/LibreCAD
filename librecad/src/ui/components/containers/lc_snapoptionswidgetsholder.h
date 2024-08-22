/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2024 LibreCAD.org
 Copyright (C) 2024 sand1024

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 ******************************************************************************/

#ifndef LC_SNAPOPTIONSWIDGETSHOLDER_H
#define LC_SNAPOPTIONSWIDGETSHOLDER_H

#include <QWidget>

namespace Ui {
    class LC_SnapOptionsWidgetsHolder;
}

class LC_SnapOptionsWidgetsHolder : public QWidget{
Q_OBJECT

public:
    explicit LC_SnapOptionsWidgetsHolder(QWidget *parent = nullptr);
    ~LC_SnapOptionsWidgetsHolder();
    void showSnapMiddleOptions(int* middlePoints, bool on);
    void showSnapDistOptions(double* dist, bool on);
    void hideSnapOptions();
    void setLocatedOnLeft(bool value){widgetOnLeftWithinContainer = value;};
    void updateBy(LC_SnapOptionsWidgetsHolder *pHolder);
public slots:
    void languageChange();
private:
    bool widgetOnLeftWithinContainer = true;
    Ui::LC_SnapOptionsWidgetsHolder *ui;
    void hideSeparator();
    void showSeparator();

    void updateParent() const;
};

#endif // LC_SNAPOPTIONSWIDGETSHOLDER_H
