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
#include "qg_circleoptions.h"

#include "rs_settings.h"
#include "rs_math.h"
#include "rs_debug.h"
#include "rs_actiondrawcirclecr.h"
#include "ui_qg_circleoptions.h"


/*
 *  Constructs a QG_CircleOptions as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_CircleOptions::QG_CircleOptions()
    : LC_ActionOptionsWidgetBase(RS2::ActionNone, "Draw", "Circle")
    , ui(std::make_unique<Ui::Ui_CircleOptions>())
{
	ui->setupUi(this);
    connect(ui->leRadius, &QLineEdit::editingFinished, this, &QG_CircleOptions::onRadiusEditingFinished);
}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_CircleOptions::~QG_CircleOptions() = default;

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_CircleOptions::languageChange(){
	ui->retranslateUi(this);
}

bool QG_CircleOptions::checkActionRttiValid(RS2::ActionType actionType){
    return  actionType ==RS2::ActionDrawCircleCR ||  actionType ==RS2::ActionDrawCircle2PR;
}

void QG_CircleOptions::doSaveSettings(){
	  save("Radius", ui->leRadius->text());
}

void QG_CircleOptions::doSetAction(RS_ActionInterface *a, bool update){

    action = dynamic_cast<RS_ActionDrawCircleCR *>(a);

    QString radius;
    if (update){
        radius = fromDouble(action->getRadius());
    } else {
        radius = load("Radius", "1.0");
    }
    
    setRadiusToActionAndVIew(radius);
}

void QG_CircleOptions::setRadiusToActionAndVIew(QString val){
    double radius;
    if (toDouble(val, radius, 1.0, true)){
        action->setRadius(radius);
        ui->leRadius->setText(fromDouble(radius));
    }
}

void QG_CircleOptions::onRadiusEditingFinished(){
    setRadiusToActionAndVIew(ui->leRadius->text());
}
