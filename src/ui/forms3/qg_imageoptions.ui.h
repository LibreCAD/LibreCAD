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

void QG_ImageOptions::destroy() {
    RS_SETTINGS->beginGroup("/Image");
    RS_SETTINGS->writeEntry("/ImageAngle", leAngle->text());
    RS_SETTINGS->writeEntry("/ImageFactor", leFactor->text());
    RS_SETTINGS->endGroup();
}

void QG_ImageOptions::setAction(RS_ActionInterface* a, bool update) {
    if (a!=NULL && a->rtti()==RS2::ActionDrawImage) {
        action = (RS_ActionDrawImage*)a;

        QString sAngle;
        QString sFactor;
        if (update) {
            sAngle = QString("%1").arg(RS_Math::rad2deg(action->getAngle()));
            sFactor = QString("%1").arg(action->getFactor());
        } else {
            RS_SETTINGS->beginGroup("/Image");
            sAngle = RS_SETTINGS->readEntry("/ImageAngle", "0.0");
            sFactor = RS_SETTINGS->readEntry("/ImageFactor", "1.0");
            RS_SETTINGS->endGroup();
        }
	leAngle->setText(sAngle);
	leFactor->setText(sFactor);
        updateData();
    } else {
        RS_DEBUG->print(RS_Debug::D_ERROR, 
			"QG_ImageOptions::setAction: wrong action type");
        action = NULL;
    }
}

void QG_ImageOptions::updateData() {
    if (action!=NULL) {
        action->setAngle(RS_Math::deg2rad(RS_Math::eval(leAngle->text())));
        action->setFactor(RS_Math::eval(leFactor->text()));
    }
}
