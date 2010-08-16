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

void QG_LinePolygon2Options::destroy() {
    RS_SETTINGS->beginGroup("/Draw");
    RS_SETTINGS->writeEntry("/LinePolygon2Number", sbNumber->text());
    RS_SETTINGS->endGroup();
}

void QG_LinePolygon2Options::setAction(RS_ActionInterface* a, bool update) {
    if (a!=NULL && a->rtti()==RS2::ActionDrawLinePolygon2) {
        action = (RS_ActionDrawLinePolygon2*)a;

        QString sn;
        if (update) {
            sn = QString("%1").arg(action->getNumber());
        } else {
            RS_SETTINGS->beginGroup("/Draw");
            sn = RS_SETTINGS->readEntry("/LinePolygon2Number", "3");
            RS_SETTINGS->endGroup();
        }
        sbNumber->setValue(sn.toInt());
    } else {
        RS_DEBUG->print(RS_Debug::D_ERROR, 
			"QG_LinePolygon2Options::setAction: wrong action type");
        action = NULL;
    }

}

void QG_LinePolygon2Options::updateNumber(int n) {
    if (action!=NULL) {
        action->setNumber(n);
    }
}
