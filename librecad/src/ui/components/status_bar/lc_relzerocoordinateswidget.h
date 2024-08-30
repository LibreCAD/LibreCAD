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

#ifndef LC_RELZEROCOORDINATESWIDGET_H
#define LC_RELZEROCOORDINATESWIDGET_H

#include <QWidget>
#include "rs_graphic.h"

namespace Ui {
    class LC_RelZeroCoordinatesWidget;
}

class LC_RelZeroCoordinatesWidget : public QWidget{
    Q_OBJECT

public:
    explicit LC_RelZeroCoordinatesWidget(QWidget *parent = 0, const char *name = 0);
    ~LC_RelZeroCoordinatesWidget();

    void clearContent();
    virtual void setGraphicView( RS_GraphicView * graphic );
    virtual void setRelativeZero( const RS_Vector & rel, bool updateFormat );
public slots:
    void relativeZeroChanged(const RS_Vector &pos);
protected:
    RS_Graphic* graphic = nullptr;
    RS_GraphicView* graphicView = nullptr;
    int prec = 0;
    RS2::LinearFormat format = RS2::Decimal;
    int aprec = 0;
    RS2::AngleFormat aformat = RS2::DegreesDecimal;
private:
    Ui::LC_RelZeroCoordinatesWidget *ui;
};

#endif // LC_RELZEROCOORDINATESWIDGET_H
