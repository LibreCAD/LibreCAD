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
#include "qg_linerelangleoptions.h"

#include "rs_actiondrawlinerelangle.h"
#include "rs_debug.h"
#include "rs_dimension.h"
#include "rs_math.h"
#include "ui_qg_linerelangleoptions.h"

namespace {
// format a number with specified digits after point
    QString formatNumber(double value, int precision = 8)
    {
        precision = std::max(precision, 0);
        precision = std::min(precision, 16);
        LC_ERR<<"value: "<<value;
        QString text = QString("%1").arg(value, 0, 'f', precision);
        LC_ERR<<"value: "<<text;
        text = RS_Dimension::stripZerosLinear(text, 12);
        LC_ERR<<"value: "<<text;
        return text;
    }
}

/*
 *  Constructs a QG_LineRelAngleOptions as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_LineRelAngleOptions::QG_LineRelAngleOptions()
    :LC_ActionOptionsWidgetBase(RS2::ActionNone, "Draw", "LineRelAngleAngle"),
     ui(std::make_unique<Ui::Ui_LineRelAngleOptions>()){
    ui->setupUi(this);
    connect(ui->leLength, &QLineEdit::editingFinished, this, &QG_LineRelAngleOptions::onLengthEditingFinished);
    connect(ui->leAngle, &QLineEdit::editingFinished, this, &QG_LineRelAngleOptions::onAngleEditingFinished);
}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_LineRelAngleOptions::~QG_LineRelAngleOptions() = default;

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_LineRelAngleOptions::languageChange(){
    ui->retranslateUi(this);
}

void QG_LineRelAngleOptions::doSaveSettings(){
    if (!action->hasFixedAngle()){
        save("Angle", ui->leAngle->text());
    }
    save("Length", ui->leLength->text());
}

bool QG_LineRelAngleOptions::checkActionRttiValid(RS2::ActionType actionType){
    return actionType == RS2::ActionDrawLineRelAngle ||
           actionType == RS2::ActionDrawLineOrthogonal;
}

void QG_LineRelAngleOptions::doSetAction(RS_ActionInterface *a, bool update){
    action = dynamic_cast<RS_ActionDrawLineRelAngle *>(a);
    bool fixedAngle = action->hasFixedAngle();

    ui->lAngle->setVisible(!fixedAngle);
    ui->leAngle->setVisible(!fixedAngle);

    QString angle;
    QString length;

    // settings from action:
    if (update){
        angle = fromDouble(action->getAngle());
        length = fromDouble(action->getLength());
    }
        // settings from config file:
    else {
        if (!action->hasFixedAngle()){
            angle = load("Angle", "30.0");
        }
        length = load("Length", "10.0");
    }

    if (!fixedAngle){
        setAngleToActionAndView(angle);
    }
    setLengthToActionAndView(length);
}

void QG_LineRelAngleOptions::onLengthEditingFinished(){
    setLengthToActionAndView(ui->leLength->text());
}

void QG_LineRelAngleOptions::onAngleEditingFinished(){
    setAngleToActionAndView(ui->leAngle->text());
}

void QG_LineRelAngleOptions::setLengthToActionAndView(QString val){
    double length;
    if (toDouble(val, length, 1.0, false)){
        action->setLength(length);
        ui->leLength->setText(fromDouble(length));
    }
}

void QG_LineRelAngleOptions::setAngleToActionAndView(QString val){
    double angle;
    if (toDoubleAngle(val, angle, 1.0, false)){
        action->setAngle(angle);
        ui->leAngle->setText(fromDouble(angle));
    }
}
