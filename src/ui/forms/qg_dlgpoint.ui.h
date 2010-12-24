/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
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

void QG_DlgPoint::setPoint(RS_Point& p) {
    point = &p;

    wPen->setPen(point->getPen(false), true, false, "Pen");
    RS_Graphic* graphic = point->getGraphic();
    if (graphic!=NULL) {
        cbLayer->init(*(graphic->getLayerList()), false, false);
    }
    RS_Layer* lay = point->getLayer(false);
    if (lay!=NULL) {
        cbLayer->setLayer(*lay);
    }

    QString s;
    s.setNum(point->getPos().x);
    lePosX->setText(s);
    s.setNum(point->getPos().y);
    lePosY->setText(s);
}

void QG_DlgPoint::updatePoint() {
    point->setPos(RS_Vector(RS_Math::eval(lePosX->text()),
                            RS_Math::eval(lePosY->text())));
    point->setPen(wPen->getPen());
    point->setLayer(cbLayer->currentText());
}

