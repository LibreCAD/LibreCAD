/****************************************************************************
**
** This file is part of the CADuntu project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (caduntu@rvt.dds.nl)
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

void QG_DlgArc::setArc(RS_Arc& a) {
    arc = &a;
    //pen = arc->getPen();
    wPen->setPen(arc->getPen(false), true, false, "Pen");
    RS_Graphic* graphic = arc->getGraphic();
    if (graphic!=NULL) {
        cbLayer->init(*(graphic->getLayerList()), false, false);
    }
    RS_Layer* lay = arc->getLayer(false);
    if (lay!=NULL) {
        cbLayer->setLayer(*lay);
    }
    QString s;
    s.setNum(arc->getCenter().x);
    leCenterX->setText(s);
    s.setNum(arc->getCenter().y);
    leCenterY->setText(s);
    s.setNum(arc->getRadius());
    leRadius->setText(s);
    s.setNum(RS_Math::rad2deg(arc->getAngle1()));
    leAngle1->setText(s);
    s.setNum(RS_Math::rad2deg(arc->getAngle2()));
    leAngle2->setText(s);
    cbReversed->setChecked(arc->isReversed());
}

void QG_DlgArc::updateArc() {
    arc->setCenter(RS_Vector(RS_Math::eval(leCenterX->text()),
                                  RS_Math::eval(leCenterY->text())));
    arc->setRadius(RS_Math::eval(leRadius->text()));
    arc->setAngle1(RS_Math::deg2rad(RS_Math::eval(leAngle1->text())));
    arc->setAngle2(RS_Math::deg2rad(RS_Math::eval(leAngle2->text())));
    arc->setReversed(cbReversed->isChecked());
    arc->setPen(wPen->getPen());
    arc->setLayer(cbLayer->currentText());
    arc->calculateEndpoints();
    arc->calculateBorders();
}

