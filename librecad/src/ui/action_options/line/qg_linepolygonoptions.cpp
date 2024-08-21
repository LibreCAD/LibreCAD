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
#include "qg_linepolygonoptions.h"

#include "ui_qg_linepolygonoptions.h"
#include "rs_debug.h"

/*
 *  Constructs a QG_LinePolygonOptions as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_LinePolygonOptions::QG_LinePolygonOptions()
    : LC_ActionOptionsWidgetBase(RS2::ActionNone, "Draw", "LinePolygon")
	    , ui(new Ui::Ui_LinePolygonOptions{}){
	ui->setupUi(this);
}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_LinePolygonOptions::~QG_LinePolygonOptions() = default;

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_LinePolygonOptions::languageChange(){
	ui->retranslateUi(this);
}

bool QG_LinePolygonOptions::checkActionRttiValid(RS2::ActionType actionType){
    return actionType == RS2::ActionDrawLinePolygonCenCor ||
           actionType == RS2::ActionDrawLinePolygonCenTan ||
           actionType == RS2::ActionDrawLinePolygonCorCor;
}

void QG_LinePolygonOptions::doSaveSettings(){
	save("Number", ui->sbNumber->text());
}

QString QG_LinePolygonOptions::getSettingsOptionNamePrefix(){
    switch (action->rtti()){
        case RS2::ActionDrawLinePolygonCenCor:
            return "/LinePolygon";
        case RS2::ActionDrawLinePolygonCenTan:
            return "/LinePolygon3";            
        case RS2::ActionDrawLinePolygonCorCor:
            return "/LinePolygon2";
        default:
            return "/LinePolygon";
    }
}

void QG_LinePolygonOptions::doSetAction(RS_ActionInterface *a, bool update){
    action = dynamic_cast<LC_ActionDrawLinePolygonBase*>(a);

    int number;
    if (update){
        number = action->getNumber();
    } else {
        number = loadInt("Number", 3);
    }
    setNumberToActionAndView(number);
}

void QG_LinePolygonOptions::setNumberToActionAndView(int number){
    action->setNumber(number);
    ui->sbNumber->setValue(number);
}

void QG_LinePolygonOptions::on_sbNumber_valueChanged( [[maybe_unused]]int number){
    setNumberToActionAndView(ui->sbNumber->value());
}
