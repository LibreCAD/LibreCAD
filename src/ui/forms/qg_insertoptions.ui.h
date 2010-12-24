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

void QG_InsertOptions::destroy() {
    RS_SETTINGS->beginGroup("/Insert");
    RS_SETTINGS->writeEntry("/InsertAngle", leAngle->text());
    RS_SETTINGS->writeEntry("/InsertFactor", leFactor->text());
    RS_SETTINGS->writeEntry("/InsertColumns", sbColumns->text());
    RS_SETTINGS->writeEntry("/InsertRows", sbRows->text());
    RS_SETTINGS->writeEntry("/InsertColumnSpacing", leColumnSpacing->text());
    RS_SETTINGS->writeEntry("/InsertRowSpacing", leRowSpacing->text());
    RS_SETTINGS->endGroup();
}

void QG_InsertOptions::setAction(RS_ActionInterface* a, bool update) {
    if (a!=NULL && a->rtti()==RS2::ActionBlocksInsert) {
        action = (RS_ActionBlocksInsert*)a;

        QString sAngle;
        QString sFactor;
	QString sColumns;
    	QString sRows;
        QString sColumnSpacing;
        QString sRowSpacing;
        if (update) {
            sAngle = QString("%1").arg(RS_Math::rad2deg(action->getAngle()));
            sFactor = QString("%1").arg(action->getFactor());
            sColumns = QString("%1").arg(action->getColumns());
            sRows = QString("%1").arg(action->getRows());
            sColumnSpacing = QString("%1").arg(action->getColumnSpacing());
            sRowSpacing = QString("%1").arg(action->getRowSpacing());
        } else {
            RS_SETTINGS->beginGroup("/Insert");
            sAngle = RS_SETTINGS->readEntry("/InsertAngle", "0.0");
            sFactor = RS_SETTINGS->readEntry("/InsertFactor", "1.0");
            sColumns = RS_SETTINGS->readEntry("/InsertColumns", "1");
            sRows = RS_SETTINGS->readEntry("/InsertRows", "1");
            sColumnSpacing = RS_SETTINGS->readEntry("/InsertColumnSpacing", "1.0");
            sRowSpacing = RS_SETTINGS->readEntry("/InsertRowSpacing", "1.0");
            RS_SETTINGS->endGroup();
        }
	leAngle->setText(sAngle);
	leFactor->setText(sFactor);
    	sbColumns->setValue(sColumns.toInt());
        sbRows->setValue(sRows.toInt());
        leColumnSpacing->setText(sColumnSpacing);
        leRowSpacing->setText(sRowSpacing);
    } else {
        RS_DEBUG->print(RS_Debug::D_ERROR, 
			"QG_InsertOptions::setAction: wrong action type");
        action = NULL;
    }
}

void QG_InsertOptions::updateData() {
    if (action!=NULL) {
        action->setAngle(RS_Math::deg2rad(RS_Math::eval(leAngle->text())));
        action->setFactor(RS_Math::eval(leFactor->text()));
        action->setColumns(sbColumns->value());
        action->setRows(sbRows->value());
        action->setColumnSpacing(RS_Math::eval(leColumnSpacing->text()));
        action->setRowSpacing(RS_Math::eval(leRowSpacing->text()));
    }
}
