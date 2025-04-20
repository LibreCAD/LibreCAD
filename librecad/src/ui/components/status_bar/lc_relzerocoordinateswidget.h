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
#include "lc_graphicviewaware.h"
#include "lc_graphicviewportlistener.h"
#include "rs.h"

class LC_GraphicViewport;
class RS_Graphic;

namespace Ui {
    class LC_RelZeroCoordinatesWidget;
}

class LC_RelZeroCoordinatesWidget : public QWidget, public LC_GraphicViewAware, public  LC_GraphicViewPortListener{
    Q_OBJECT
public:
    explicit LC_RelZeroCoordinatesWidget(QWidget *parent = 0, const char *name = 0);
    ~LC_RelZeroCoordinatesWidget();

    void clearContent();
    void setGraphicView( RS_GraphicView * graphic ) override;
    void setRelativeZero( const RS_Vector & rel, bool updateFormat );
public slots:
    void relativeZeroChanged(const RS_Vector &pos);
protected:
    RS_Graphic* m_graphic = nullptr;
    RS_GraphicView* m_graphicView = nullptr;
    LC_GraphicViewport* m_viewport = nullptr;
    int m_linearPrecision = 0;
    RS2::LinearFormat m_linearFormat = RS2::Decimal;
    int m_anglePrecision = 0;
    RS2::AngleFormat m_angleFormat = RS2::DegreesDecimal;
private:
    void onUCSChanged(LC_UCS *ucs) override;
    Ui::LC_RelZeroCoordinatesWidget *ui;
};

#endif // LC_RELZEROCOORDINATESWIDGET_H
