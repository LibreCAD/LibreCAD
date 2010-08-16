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

void QG_ArcTangentialOptions::destroy() {
    RS_SETTINGS->beginGroup("/Draw");
    RS_SETTINGS->writeEntry("/ArcTangentialRadius", leRadius->text());    
    RS_SETTINGS->endGroup();
}

void QG_ArcTangentialOptions::setAction(RS_ActionInterface* a, bool update) {
    if (a!=NULL && a->rtti()==RS2::ActionDrawArcTangential) {
        action = (RS_ActionDrawArcTangential*)a;

        QString sr;
        if (update) {
            sr = QString("%1").arg(action->getRadius());
        } else {
            RS_SETTINGS->beginGroup("/Draw");
            sr = RS_SETTINGS->readEntry("/ArcTangentialRadius", "1.0");
            RS_SETTINGS->endGroup();
            action->setRadius(sr.toDouble());
        }
        leRadius->setText(sr);
    } else {
        RS_DEBUG->print(RS_Debug::D_ERROR, 
			"QG_ArcTangentialOptions::setAction: wrong action type");
        action = NULL;
    }

}


/*void QG_ArcTangentialOptions::init() {
    data = NULL;
    RS_SETTINGS->beginGroup("/Draw");
    bool reversed = RS_SETTINGS->readNumEntry("/ArcReversed", 0);
    RS_SETTINGS->endGroup();
    
    rbNeg->setChecked(reversed);
}*/



/*void QG_ArcTangentialOptions::setData(RS_ArcData* d) {
    data = d;
    updateDirection(false);
}*/

void QG_ArcTangentialOptions::updateRadius(const QString& s) {
    if (action!=NULL) {
        action->setRadius(RS_Math::eval(s));
    }
}
