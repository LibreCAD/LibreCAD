/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software 
** Foundation and appearing in the file gpl-2.0.txt included in the
** packaging of this file.
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

void QG_DimOptions::destroy() {
    RS_SETTINGS->beginGroup("/Draw");
    RS_SETTINGS->writeEntry("/DimLabel", leLabel->text());
    RS_SETTINGS->writeEntry("/DimTol1", leTol1->text());
    RS_SETTINGS->writeEntry("/DimTol2", leTol2->text());
    RS_SETTINGS->endGroup();
}

void QG_DimOptions::setAction(RS_ActionInterface* a, bool update) {
    if (a!=NULL && RS_ActionDimension::isDimensionAction(a->rtti())) {
        action = (RS_ActionDimension*)a;
        
        QString st;
        QString stol1;
        QString stol2;
        bool diam;
        if (update) {
            st = action->getLabel();
            stol1 = action->getTol1();
            stol2 = action->getTol2();
            diam = action->getDiameter();
        } else {
            //st = "";
            RS_SETTINGS->beginGroup("/Draw");
            st = RS_SETTINGS->readEntry("/DimLabel", "");
            stol1 = RS_SETTINGS->readEntry("/DimTol1", "");
            stol2 = RS_SETTINGS->readEntry("/DimTol2", "");
            diam = (bool)RS_SETTINGS->readNumEntry("/DimDiameter", 0);
            RS_SETTINGS->endGroup();
        }
        leLabel->setText(st);
        leTol1->setText(stol1);
        leTol2->setText(stol2);
        bDiameter->setOn(diam);
    } else {
        RS_DEBUG->print(RS_Debug::D_ERROR, 
			"QG_DimensionOptions::setAction: wrong action type");
        action = NULL;
    }
}



void QG_DimOptions::updateLabel() {
    if (action!=NULL) {
        action->setText("");
        action->setLabel(leLabel->text());
        action->setDiameter(bDiameter->isOn());
        action->setTol1(leTol1->text());
        action->setTol2(leTol2->text());
        
        action->setText(action->getText());
  }
}

void QG_DimOptions::insertSign(const QString& c) {
    leLabel->insert(c);
}


