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
#include "qg_dimoptions.h"

#include "rs_settings.h"
#include "rs_debug.h"
#include "ui_qg_dimoptions.h"
#include "rs_actiondimension.h"
#include "rs_actiondimlinear.h"
#include "rs_math.h"

/*
 *  Constructs a QG_DimOptions as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_DimOptions::QG_DimOptions()
    :LC_ActionOptionsWidgetBase(RS2::ActionNone, "/Draw", "/Dim"),
    ui(std::make_unique<Ui::Ui_DimOptions>()){
    ui->setupUi(this);
}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_DimOptions::~QG_DimOptions() = default;

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_DimOptions::languageChange(){
    ui->retranslateUi(this);
}

bool QG_DimOptions::checkActionRttiValid(RS2::ActionType actionType){
    return RS_ActionDimension::isDimensionAction(actionType);
}

void QG_DimOptions::doSaveSettings(){
    save("Label", ui->leLabel->text());
    save("Tol1", ui->leTol1->text());
    save("Tol2", ui->leTol2->text());

    RS2::ActionType rtti = action->rtti();
    if (rtti == RS2::ActionDimRadial)
        save("Radial", ui->bDiameter->isChecked());
    else if (rtti == RS2::ActionDimDiametric)
        save("Diameter", ui->bDiameter->isChecked());
    else if (rtti == RS2::ActionDimLinear){
        save("Angle", ui->leAngle->text());
    }
}

void QG_DimOptions::doSetAction(RS_ActionInterface *a, bool update){
    action = dynamic_cast<RS_ActionDimension *>(a);
    QString st;
    QString stol1;
    QString stol2;
    QString sa;
    bool diam = false;
    bool radial = false;
    bool isDimLinear = action->rtti() == RS2::ActionDimLinear;
    if (update){
        st = action->getLabel();
        stol1 = action->getTol1();
        stol2 = action->getTol2();
        diam = action->getDiameter();
        ui->bDiameter->setChecked(action->getDiameter());
        if (isDimLinear){
            auto dimLinearAction = dynamic_cast<RS_ActionDimLinear *>(action); 
            sa = fromDouble(RS_Math::rad2deg(dimLinearAction->getAngle()));
        }
    } else {
        //st = "";
        st = load("Label", "");
        stol1 = load("Tol1", "");
        stol2 = load("Tol2", "");
        diam = loadBool("Diameter", false);
        radial = loadBool("Radial", false);
        if (isDimLinear){
            sa = load("Angle", "0.0");
        }
    }

    RS2::ActionType type = action->rtti();
    if (type == RS2::ActionDimRadial){
        ui->bDiameter->setIcon({});
        ui->bDiameter->setText(tr("R", "Radial dimension prefix"));
        ui->bDiameter->setChecked(radial);
        action->setDiameter(radial);
    } else {
        ui->bDiameter->setChecked(diam);
    }
    ui->leLabel->setText(st);
    ui->leTol1->setText(stol1);
    ui->leTol2->setText(stol2);

    ui->lAngle->setVisible(isDimLinear);
    ui->leAngle->setVisible(isDimLinear);
    ui->bHor->setVisible(isDimLinear);
    ui->bVer->setVisible(isDimLinear);

    if (isDimLinear){
        ui->leAngle->setText(sa);
    }
}

void QG_DimOptions::updateLabel(){
    action->setText("");
    action->setLabel(ui->leLabel->text());
    action->setDiameter(ui->bDiameter->isChecked());
    action->setTol1(ui->leTol1->text());
    action->setTol2(ui->leTol2->text());
    action->setText(action->getText());
}

void QG_DimOptions::insertSign(const QString &c){
    ui->leLabel->insert(c);
}

void QG_DimOptions::updateAngle(const QString & a) {
    auto dimLinearAction = dynamic_cast<RS_ActionDimLinear *>(action);
    dimLinearAction->setAngle(RS_Math::deg2rad(RS_Math::eval(a)));
}

void QG_DimOptions::on_bHor_clicked(){
    ui->leAngle->setText("0");
    updateAngle("0");
}

void QG_DimOptions::on_bVer_clicked(){
    ui->leAngle->setText("90");
    updateAngle("90");
}

void QG_DimOptions::on_leAngle_editingFinished(){
    updateAngle(ui->leAngle->text());
}
