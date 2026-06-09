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
    m_formatter = nullptr;
}

LC_RelZeroCoordinatesWidget::~LC_RelZeroCoordinatesWidget(){
    delete ui;
}

void LC_RelZeroCoordinatesWidget::clearContent() const {
    ui->lCartesianCoordinates->setText("0 , 0");
    ui->lPolarCoordinates->setText("0 < 0");
}

void LC_RelZeroCoordinatesWidget::setGraphicView(RS_GraphicView *gv) {
    if (m_graphicView != nullptr) {
        disconnect(m_graphicView, &RS_GraphicView::relativeZeroChanged, this, &LC_RelZeroCoordinatesWidget::relativeZeroChanged);
    }
    if (m_viewport != nullptr){
        m_viewport->removeViewportListener(this);
    }
    if (gv == nullptr){
        setRelativeZero(RS_Vector(0.0,0.0), true); // in which system? ucs? wcs?
        m_viewport = nullptr;
        m_graphicView  = nullptr;
        m_graphic = nullptr;
        m_formatter = nullptr;
    }
    else {
        m_graphicView = gv;
        m_graphic = m_graphicView->getGraphic();
        m_viewport = gv->getViewPort();
        m_formatter = m_viewport->getFormatter();

        m_viewport->addViewportListener(this);

        connect(m_graphicView, &RS_GraphicView::relativeZeroChanged, this, &LC_RelZeroCoordinatesWidget::relativeZeroChanged);
        setRelativeZero(m_viewport->getRelativeZero(), true);
    }
}

void LC_RelZeroCoordinatesWidget::relativeZeroChanged(const RS_Vector &pos) {
    setRelativeZero(pos, true);
}

void LC_RelZeroCoordinatesWidget::showRelZero(const RS_Vector& rel) const {
    if (m_viewport != nullptr) {
        const RS_Vector ucsRelZero = m_viewport->toUCS(rel);

        double x = ucsRelZero.x;
        double y = ucsRelZero.y;

        const double magnitude = ucsRelZero.magnitude();
        double len = magnitude;
        double angle = ucsRelZero.angle();
        if (LC_LineMath::isNotMeaningful(magnitude)) {
            len = 0;
            angle = 0;
        }

        angle = m_viewport->toBasisUCSAngle(angle);

        if (!LC_GET_ONE_BOOL("Appearance", "UnitlessGrid", true)) {
            x = RS_Units::convert(x);
            y = RS_Units::convert(y);
            len = RS_Units::convert(magnitude);
        }

        // cartesian coordinates
        const QString relX = m_formatter->formatLinear(x);
        const QString relY = m_formatter->formatLinear(y);

        ui->lCartesianCoordinates->setText(relX + " , " + relY);

        // polar coordinates:

        const QString rStr = m_formatter->formatLinear(len);
        const QString aStr = m_formatter->formatRawAngle(angle);
        const QString str = rStr + " < " + aStr;
        ui->lPolarCoordinates->setText(str);
    }
    else {
        ui->lPolarCoordinates->setText("");
        ui->lCartesianCoordinates->setText("");
    }
}

void LC_RelZeroCoordinatesWidget::setRelativeZero(const RS_Vector &rel, const bool updateFormat) {
    if (m_graphic != nullptr) {
        if (updateFormat) {
            m_formatter = m_viewport->getFormatter();  // fixme - fmt - most probably it should be removed
        }
        m_currentRelZero = rel;
        showRelZero(rel);
    }
    else {
        ui->lCartesianCoordinates->setText("");
        ui->lPolarCoordinates->setText("");
    }
}

void LC_RelZeroCoordinatesWidget::updateFormats() {
    setRelativeZero(m_currentRelZero, true);
}

void LC_RelZeroCoordinatesWidget::onUCSChanged([[maybe_unused]]LC_UCS *ucs) {
    setRelativeZero(m_viewport->getRelativeZero(), true);
}
