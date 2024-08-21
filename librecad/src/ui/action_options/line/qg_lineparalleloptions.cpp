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
#include "qg_lineparalleloptions.h"

#include "rs_actiondrawlineparallel.h"
#include "rs_settings.h"
#include "rs_math.h"
#include "ui_qg_lineparalleloptions.h"


/*
 *  Constructs a QG_LineParallelOptions as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_LineParallelOptions::QG_LineParallelOptions(RS2::ActionType actionType)
    :LC_ActionOptionsWidgetBase(actionType, "Draw", "LineParallel"),
     ui(new Ui::Ui_LineParallelOptions{}) {
    ui->setupUi(this);
    connect(ui->leDist, &QLineEdit::editingFinished, this, &QG_LineParallelOptions::onDistEditingFinished);
    connect(ui->sbNumber, &QSpinBox::valueChanged, this, &QG_LineParallelOptions::onNumberValueChanged);
}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_LineParallelOptions::~QG_LineParallelOptions() = default;

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_LineParallelOptions::languageChange() {
    ui->retranslateUi(this);
}

bool QG_LineParallelOptions::checkActionRttiValid(RS2::ActionType actionType) {
    return actionType == RS2::ActionDrawLineParallel ||
           actionType == RS2::ActionDrawCircleParallel ||
           actionType == RS2::ActionDrawArcParallel;
}

void QG_LineParallelOptions::doSaveSettings() {
    save("Distance", ui->leDist->text());
    save("Number", ui->sbNumber->text());
}

void QG_LineParallelOptions::doSetAction(RS_ActionInterface *a, bool update) {
    action = dynamic_cast<RS_ActionDrawLineParallel *>(a);
    QString distance;
    int copiesNumber;
    if (update) {
        distance = fromDouble(action->getDistance());
        copiesNumber = action->getNumber();
    } else {
        distance = load("Distance", "1.0");
        copiesNumber = loadInt("Number", 1);
    }

    setDistanceToActionAndView(distance);
    setNumberToActionAndView(copiesNumber);
}

void QG_LineParallelOptions::onNumberValueChanged(int number) {
    setNumberToActionAndView(number);
}

void QG_LineParallelOptions::onDistEditingFinished() {
    setDistanceToActionAndView(ui->leDist->text());
}

void QG_LineParallelOptions::setDistanceToActionAndView(QString val) {
    double distance;
    if (toDouble(val, distance, 1.0, false)) {
        action->setDistance(distance);
        ui->leDist->setText(fromDouble(distance));
    }
}

void QG_LineParallelOptions::setNumberToActionAndView(int number) {
    action->setNumber(number);
    ui->sbNumber->setValue(number);
}
