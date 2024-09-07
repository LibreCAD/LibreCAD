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
#include "qg_lineangleoptions.h"

#include "rs_actiondrawlineangle.h"
#include "rs_actioninterface.h"
#include "rs_debug.h"
#include "rs_math.h"
#include "ui_qg_lineangleoptions.h"


/*
 *  Constructs a QG_LineAngleOptions as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_LineAngleOptions::QG_LineAngleOptions()
    : LC_ActionOptionsWidgetBase( RS2::ActionNone, "Draw", "LineAngle")
    , ui(std::make_unique<Ui::Ui_LineAngleOptions>()){
    ui->setupUi(this);

    connect(ui->leAngle, &QLineEdit::editingFinished, this, &QG_LineAngleOptions::onAngleEditingFinished);
    connect(ui->leLength, &QLineEdit::editingFinished, this, &QG_LineAngleOptions::onLengthEditingFinished);
    connect(ui->cbSnapPoint, &QComboBox::currentIndexChanged, this, &QG_LineAngleOptions::onSnapPointCurrentIndexChanged);
}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_LineAngleOptions::~QG_LineAngleOptions() = default;

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_LineAngleOptions::languageChange(){
    ui->retranslateUi(this);
}

bool QG_LineAngleOptions::checkActionRttiValid(RS2::ActionType actionType){
    return actionType == RS2::ActionDrawLineAngle ||
           actionType == RS2::ActionDrawLineHorizontal ||
           actionType == RS2::ActionDrawLineVertical;
}

void QG_LineAngleOptions::doSetAction(RS_ActionInterface *a, bool update){
        action = dynamic_cast<RS_ActionDrawLineAngle*>(a);
        bool angleIsFixed = action->hasFixedAngle();

        QString angle;
        QString length;
        int snapPoint;

        // settings from action:
        if (update) {
            if (!angleIsFixed){
                angle = fromDouble(action->getAngle());
            }
            length = fromDouble(action->getLength());
            snapPoint = action->getSnapPoint();
        } else {
            if (!angleIsFixed){
                angle = load("Angle", "30.0");
            }
            length = load("Length", "10.0");
            snapPoint = loadInt("SnapPoint", 0);
        }

        ui->leAngle->setVisible(!angleIsFixed);
        ui->lAngle->setVisible(!angleIsFixed);
    
        if (!angleIsFixed){
            setAngleToActionAndView(angle);
        }
        setSnapPointToActionAndView(snapPoint);
        setLengthToActionAndVeiw(length);

        ui->leAngle->setText(angle);
        ui->leLength->setText(length);
}

void QG_LineAngleOptions::doSaveSettings() {
    if (!action->hasFixedAngle()){
        save("Angle", ui->leAngle->text());
    }

    save("Length", ui->leLength->text());
    save("SnapPoint", ui->cbSnapPoint->currentIndex());
}

void QG_LineAngleOptions::onSnapPointCurrentIndexChanged(int number){
    setSnapPointToActionAndView(number);
}

void QG_LineAngleOptions::onLengthEditingFinished(){
    setLengthToActionAndVeiw(ui->leLength->text());
}
void QG_LineAngleOptions::onAngleEditingFinished(){
    setAngleToActionAndView(ui->leAngle->text());
}

void QG_LineAngleOptions::setAngleToActionAndView(QString val){
    double angle = 0.;
    if (toDoubleAngle(val, angle, 1.0, false)){
        action->setAngle(angle);
        ui->leAngle->setText(fromDouble(angle));
    }
}

void QG_LineAngleOptions::setSnapPointToActionAndView(int val){
    action->setSnapPoint(val);
    ui->cbSnapPoint->setCurrentIndex(val);
}

void QG_LineAngleOptions::setLengthToActionAndVeiw(QString val){
    double len = 0.;
    if (toDouble(val, len, 1.0, false)){
        action->setLength(len);
        ui->leLength->setText(fromDouble(len));
    }
}
