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

#include "lc_relzerocoordinateswidget.h"
#include "ui_lc_relzerocoordinateswidget.h"
#include "rs_settings.h"
#include "rs_units.h"
#include "rs_graphicview.h"

LC_RelZeroCoordinatesWidget::LC_RelZeroCoordinatesWidget(QWidget *parent, const char* name)
    : QWidget(parent)
    , ui(new Ui::LC_RelZeroCoordinatesWidget){
    ui->setupUi(this);
    setObjectName(name);

    ui->lCartesianCoordinates->setText("");
    ui->lCartesianCoordinates->setText("");

    graphic = nullptr;
    prec = 4;
    format = RS2::Decimal;
    aprec = 2;
    aformat = RS2::DegreesDecimal;
}

LC_RelZeroCoordinatesWidget::~LC_RelZeroCoordinatesWidget(){
    delete ui;
}

void LC_RelZeroCoordinatesWidget::clearContent() {
    ui->lCartesianCoordinates->setText("0 , 0");
    ui->lPolarCoordinates->setText("0 < 0");
}

void LC_RelZeroCoordinatesWidget::setGraphicView(RS_GraphicView *gv) {
    if (gv == nullptr){
        if (graphicView != nullptr){
            disconnect(graphicView, &RS_GraphicView::relative_zero_changed, this,&LC_RelZeroCoordinatesWidget::relativeZeroChanged);
        }
        setRelativeZero(RS_Vector(0.0,0.0), true);
    }
    else{
        graphicView = gv;
        graphic = graphicView->getGraphic();
        connect(graphicView, &RS_GraphicView::relative_zero_changed, this,&LC_RelZeroCoordinatesWidget::relativeZeroChanged);
        setRelativeZero(graphicView->getRelativeZero(), true);
    }
}

void LC_RelZeroCoordinatesWidget::relativeZeroChanged(const RS_Vector &pos) {
    setRelativeZero(pos, true);
}

void LC_RelZeroCoordinatesWidget::setRelativeZero(const RS_Vector &rel, bool updateFormat) {

    if (graphic) {
        if (updateFormat) {
            format = graphic->getLinearFormat();
            prec = graphic->getLinearPrecision();
            aformat = graphic->getAngleFormat();
            aprec = graphic->getAnglePrecision();
        }

        double x = rel.x;
        double y = rel.y;

        if (!LC_GET_ONE_BOOL("Appearance", "UnitlessGrid", true)){
            x  = RS_Units::convert(x);
            y  = RS_Units::convert(y);
        }

        // cartesian coordinates

        RS2::Unit unit = graphic->getUnit();
        QString relX = RS_Units::formatLinear(x,unit,format, prec);
        QString relY = RS_Units::formatLinear(y,unit,format, prec);

        ui->lCartesianCoordinates->setText(relX + " , " + relY);

        // polar coordinates:
        RS_Vector v;
        v = RS_Vector(x, y);
        QString str;
        QString rStr = RS_Units::formatLinear(v.magnitude(),unit,format, prec);
        QString aStr = RS_Units::formatAngle(v.angle(),aformat, aprec);

        str = rStr + " < " + aStr;
        ui->lPolarCoordinates->setText(str);
    }
}
