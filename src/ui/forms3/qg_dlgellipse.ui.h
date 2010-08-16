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

void QG_DlgEllipse::setEllipse(RS_Ellipse& e) {
    ellipse = &e;
    //pen = ellipse->getPen();
    wPen->setPen(ellipse->getPen(false), true, false, "Pen");
    RS_Graphic* graphic = ellipse->getGraphic();
    if (graphic!=NULL) {
        cbLayer->init(*(graphic->getLayerList()), false, false);
    }
    RS_Layer* lay = ellipse->getLayer(false);
    if (lay!=NULL) {
        cbLayer->setLayer(*lay);
    }
    QString s;
    s.setNum(ellipse->getCenter().x);
    leCenterX->setText(s);
    s.setNum(ellipse->getCenter().y);
    leCenterY->setText(s);
    s.setNum(ellipse->getMajorP().magnitude());
    leMajor->setText(s);
    s.setNum(ellipse->getMajorP().magnitude()*ellipse->getRatio());
    leMinor->setText(s);
    s.setNum(RS_Math::rad2deg(ellipse->getMajorP().angle()));
    leRotation->setText(s);
    s.setNum(RS_Math::rad2deg(ellipse->getAngle1()));
    leAngle1->setText(s);
    s.setNum(RS_Math::rad2deg(ellipse->getAngle2()));
    leAngle2->setText(s);
    cbReversed->setChecked(ellipse->isReversed());
}

void QG_DlgEllipse::updateEllipse() {
    ellipse->setCenter(RS_Vector(RS_Math::eval(leCenterX->text()),
                                  RS_Math::eval(leCenterY->text())));
    RS_Vector v;
    v.setPolar(RS_Math::eval(leMajor->text()), 
               RS_Math::deg2rad(RS_Math::eval(leRotation->text())));
    ellipse->setMajorP(v);
    if (RS_Math::eval(leMajor->text())>1.0e-6) {
    	ellipse->setRatio(RS_Math::eval(leMinor->text())/RS_Math::eval(leMajor->text()));
    }
    else {
        ellipse->setRatio(1.0);
    }
    ellipse->setAngle1(RS_Math::deg2rad(RS_Math::eval(leAngle1->text())));
    ellipse->setAngle2(RS_Math::deg2rad(RS_Math::eval(leAngle2->text())));
    ellipse->setReversed(cbReversed->isChecked());
    ellipse->setPen(wPen->getPen());
    ellipse->setLayer(cbLayer->currentText());
}

