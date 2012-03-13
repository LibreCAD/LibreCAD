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
#include "qg_dlgoptionsvariables.h"

/*#include <qvariant.h>
#include <qmessagebox.h>
#include "rs_units.h"*/
#include "rs_filterdxf.h"

/*
 *  Constructs a QG_DlgOptionsVariables as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */
QG_DlgOptionsVariables::QG_DlgOptionsVariables(QWidget* parent, bool modal, Qt::WindowFlags fl)
    : QDialog(parent, fl)
{
    setModal(modal);
    setupUi(this);

    init();
}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_DlgOptionsVariables::~QG_DlgOptionsVariables()
{
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_DlgOptionsVariables::languageChange()
{
    retranslateUi(this);
}

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
    QHash<QString, RS_Variable>vars = graphic->getVariableDict();
    QHash<QString, RS_Variable>::iterator it = vars.begin();
    while (it != vars.end()) {
        tabVariables->insertRows(tabVariables->numRows(), 1);
        
        tabVariables->setText(tabVariables->numRows()-1, 0, it.key());
        tabVariables->setText(tabVariables->numRows()-1, 1, QString("%1").arg(it.value().getCode()));
        QString str = "";
        switch (it.value().getType()) {
            case RS2::VariableVoid:
                break;
            case RS2::VariableInt:
                str = QString("%1").arg(it.value().getInt());
                break;
            case RS2::VariableDouble:
                str = QString("%1").arg(it.value().getDouble());
                break;
            case RS2::VariableString:
                str = QString("%1").arg(it.value().getString());
                break;
            case RS2::VariableVector:
                str = QString("%1/%2")
                      .arg(it.value().getVector().x)
                      .arg(it.value().getVector().y);
                if (RS_FilterDXF::isVariableTwoDimensional(it.key())==false) {
                    str+= QString("/%1").arg(it.value().getVector().z);
                }
                break;
        }
        tabVariables->setText(tabVariables->numRows()-1, 2, str);
        ++it;
    }
}
