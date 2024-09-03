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
#include "qg_trimamountoptions.h"

#include "rs_actionmodifytrimamount.h"
#include "rs_settings.h"
#include "rs_math.h"
#include "ui_qg_trimamountoptions.h"
#include "rs_debug.h"

/*
 *  Constructs a QG_TrimAmountOptions as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_TrimAmountOptions::QG_TrimAmountOptions()
    :LC_ActionOptionsWidgetBase(RS2::ActionModifyTrimAmount,"Modify","Trim"), ui(new Ui::Ui_TrimAmountOptions{}){
    ui->setupUi(this);
    connect(ui->leDist, &QLineEdit::editingFinished, this, &QG_TrimAmountOptions::onDistEditingFinished);
    connect(ui->cbSymmetric, &QCheckBox::toggled, this, &QG_TrimAmountOptions::onSymmetricToggled);
    connect(ui->cbTotalLength, &QCheckBox::toggled, this, &QG_TrimAmountOptions::onTotalLengthToggled);
 }

/*
 *  Destroys the object and frees any allocated resources
 */
QG_TrimAmountOptions::~QG_TrimAmountOptions(){
    action = nullptr;
};

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_TrimAmountOptions::languageChange(){
    ui->retranslateUi(this);
}

void QG_TrimAmountOptions::doSaveSettings(){
    save("Amount", ui->leDist->text());
    save("AmountTotal", ui->cbTotalLength->isChecked());
    save("AmountSymmetric", ui->cbSymmetric->isChecked());
}

void QG_TrimAmountOptions::doSetAction(RS_ActionInterface *a, bool update){
        action = dynamic_cast<RS_ActionModifyTrimAmount *>(a);

        QString distance;
        bool byTotal;
        bool symmetric;
        // settings from action:
        if (update){
            distance = QString("%1").arg(action->getDistance());
            byTotal = action->isDistanceTotalLength();
            symmetric = action->isSymmetricDistance();
        }
            // settings from config file:
        else {
            distance = load("Amount", "1.0");
            byTotal = loadBool("AmountTotal", false);
            symmetric = loadBool("AmountSymmetric", false);
        }
        setDistanceToActionAndView(distance);
        setByTotalToActionAndView(byTotal);
        setDistanceSymmetricToActionAndView(symmetric);
}

void QG_TrimAmountOptions::onDistEditingFinished(){
    setDistanceToActionAndView(ui->leDist->text());
}

void QG_TrimAmountOptions::onTotalLengthToggled(bool checked){
    setByTotalToActionAndView(checked);
}

void QG_TrimAmountOptions::onSymmetricToggled(bool checked){
    setDistanceSymmetricToActionAndView(checked);
}

void QG_TrimAmountOptions::setDistanceToActionAndView(const QString &strValue){
    double val;
    if (toDouble(strValue, val, 1.0, false)){
        action->setDistance(val);
        ui->leDist->setText(strValue);
    }
}

void QG_TrimAmountOptions::setByTotalToActionAndView(bool val){
    action->setDistanceIsTotalLength(val);
    ui->cbTotalLength->setChecked(val);
    ui->cbSymmetric->setEnabled(!val);
}

void QG_TrimAmountOptions::setDistanceSymmetricToActionAndView(bool val){
    action->setSymmetricDistance(val);
    ui->cbSymmetric->setChecked(val);
}
