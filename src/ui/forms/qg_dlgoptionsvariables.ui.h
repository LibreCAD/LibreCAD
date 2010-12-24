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

void QG_DlgOptionsVariables::init() {
    graphic = NULL;

    // variables
    tabVariables->verticalHeader()->hide();
    tabVariables->verticalHeader()->setFixedWidth(0);
    tabVariables->setColumnReadOnly(0, true);
    tabVariables->setColumnReadOnly(1, true);
    tabVariables->setColumnReadOnly(2, true);
}


/**
 * Sets the graphic and updates the GUI to match the drawing.
 */
void QG_DlgOptionsVariables::setGraphic(RS_Graphic* g) {
    graphic = g;
    updateVariables();
}


/**
 * Updates the Variables tab from the graphic values.
 */
void QG_DlgOptionsVariables::updateVariables() {
    if (graphic==NULL) {
        return;
    }
    
    QVector<int> r(tabVariables->numRows());
    for (int i=0; i<tabVariables->numRows(); ++i) {
        r[i] = i;
    }
    tabVariables->removeRows(r);
    RS_DictIterator<RS_Variable> it(graphic->getVariableDict());
    for (; it.current(); ++it) {
        tabVariables->insertRows(tabVariables->numRows(), 1);
        
        tabVariables->setText(tabVariables->numRows()-1, 0, it.currentKey());
        tabVariables->setText(tabVariables->numRows()-1, 1, QString("%1").arg(it.current()->getCode()));
        QString str = "";
        switch (it.current()->getType()) {
            case RS2::VariableVoid:
                break;
            case RS2::VariableInt:
                str = QString("%1").arg(it.current()->getInt());
                break;
            case RS2::VariableDouble:
                str = QString("%1").arg(it.current()->getDouble());
                break;
            case RS2::VariableString:
                str = QString("%1").arg(it.current()->getString());
                break;
            case RS2::VariableVector:
                str = QString("%1/%2")
                      .arg(it.current()->getVector().x)
                      .arg(it.current()->getVector().y);
                if (RS_FilterDXF::isVariableTwoDimensional(it.currentKey())==false) {
                    str+= QString("/%1").arg(it.current()->getVector().z);
                }
                break;
        }
        tabVariables->setText(tabVariables->numRows()-1, 2, str);
    }
}
