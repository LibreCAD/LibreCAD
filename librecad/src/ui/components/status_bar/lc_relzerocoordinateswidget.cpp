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

#include "lc_graphicviewport.h"
#include "lc_linemath.h"
#include "rs_graphic.h"
#include "rs_graphicview.h"
#include "rs_settings.h"
#include "rs_units.h"
#include "ui_lc_relzerocoordinateswidget.h"


LC_RelZeroCoordinatesWidget::LC_RelZeroCoordinatesWidget(QWidget *parent, const char* name)
    : QWidget(parent)
    , ui(new Ui::LC_RelZeroCoordinatesWidget){
    ui->setupUi(this);
    setObjectName(name);

    ui->lCartesianCoordinates->setText("");
    ui->lCartesianCoordinates->setText("");

    m_graphic = nullptr;
    m_linearPrecision = 4;
    m_linearFormat = RS2::Decimal;
    m_anglePrecision = 2;
    m_angleFormat = RS2::DegreesDecimal;
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
        if (m_graphicView != nullptr){
            disconnect(m_graphicView, &RS_GraphicView::relativeZeroChanged, this, &LC_RelZeroCoordinatesWidget::relativeZeroChanged);
            if (m_viewport != nullptr){
                m_viewport->removeViewportListener(this);
            }
        }
        setRelativeZero(RS_Vector(0.0,0.0), true); // in which system? ucs? wcs?
        m_viewport = nullptr;
        m_graphicView  = nullptr;
        m_graphic = nullptr;
    }
    else {
        if (m_graphicView != nullptr) {
            disconnect(m_graphicView, &RS_GraphicView::relativeZeroChanged, this, &LC_RelZeroCoordinatesWidget::relativeZeroChanged);
        }
        if (m_viewport != nullptr){
            m_viewport->removeViewportListener(this);
        }
        m_graphicView = gv;
        m_graphic = m_graphicView->getGraphic();
        m_viewport = gv->getViewPort();

        m_viewport->addViewportListener(this);

        connect(m_graphicView, &RS_GraphicView::relativeZeroChanged, this, &LC_RelZeroCoordinatesWidget::relativeZeroChanged);
        setRelativeZero(m_viewport->getRelativeZero(), true);
    }
}

void LC_RelZeroCoordinatesWidget::relativeZeroChanged(const RS_Vector &pos) {
    setRelativeZero(pos, true);
}

void LC_RelZeroCoordinatesWidget::setRelativeZero(const RS_Vector &rel, bool updateFormat) {

    if (m_graphic) {
        if (updateFormat) {
            m_linearFormat = m_graphic->getLinearFormat();
            m_linearPrecision = m_graphic->getLinearPrecision();
            m_angleFormat = m_graphic->getAngleFormat();
            m_anglePrecision = m_graphic->getAnglePrecision();
        }

        RS_Vector ucsRelZero = m_viewport->toUCS(rel);

        double x = ucsRelZero.x;
        double y = ucsRelZero.y;

        double magnitude = ucsRelZero.magnitude();
        double len = magnitude;
        double angle = ucsRelZero.angle();
        if (LC_LineMath::isNotMeaningful(magnitude)){
            len = 0;
            angle = 0;
        }

        if (m_viewport != nullptr){
            angle = m_viewport->toBasisUCSAngle(angle);
        }

        if (!LC_GET_ONE_BOOL("Appearance", "UnitlessGrid", true)){
            x  = RS_Units::convert(x);
            y  = RS_Units::convert(y);
            len = RS_Units::convert(magnitude);
        }

        // cartesian coordinates

        RS2::Unit unit = m_graphic->getUnit();
        QString relX = RS_Units::formatLinear(x,unit,m_linearFormat, m_linearPrecision);
        QString relY = RS_Units::formatLinear(y,unit,m_linearFormat, m_linearPrecision);

        ui->lCartesianCoordinates->setText(relX + " , " + relY);

        // polar coordinates:
        QString str;

        QString rStr = RS_Units::formatLinear(len, unit, m_linearFormat, m_linearPrecision);
        QString aStr = RS_Units::formatAngle(angle, m_angleFormat, m_anglePrecision);
        str = rStr + " < " + aStr;
        ui->lPolarCoordinates->setText(str);
    }
}

void LC_RelZeroCoordinatesWidget::onUCSChanged([[maybe_unused]]LC_UCS *ucs) {
    setRelativeZero(m_viewport->getRelativeZero(), true);
}
