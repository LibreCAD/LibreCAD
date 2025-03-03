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

#include "rs_debug.h"
#include "ui_qg_dimoptions.h"
#include "rs_actiondimension.h"
#include "rs_actiondimlinear.h"
#include "rs_math.h"
#include "lc_linemath.h"
#include "lc_actiondrawdimbaseline.h"
#include "lc_actioncircledimbase.h"

/*
 *  Constructs a QG_DimOptions as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_DimOptions::QG_DimOptions()
    :LC_ActionOptionsWidgetBase(RS2::ActionNone, "Draw", "Dim"),
    ui(std::make_unique<Ui::Ui_DimOptions>()){
    ui->setupUi(this);
    connect(ui->leAngle, &QLineEdit::editingFinished, this, &QG_DimOptions::onAngleEditingFinished);
    connect(ui->bHor, &QToolButton::toggled, this, &QG_DimOptions::onHorClicked);
    connect(ui->bVer, &QToolButton::toggled, this, &QG_DimOptions::onVerClicked);

    connect(ui->leBaselineDistance, &QLineEdit::textChanged, this, &QG_DimOptions::onBaselineDistanceTextChanged);
    connect(ui->cbFreeBaselineDistance, &QCheckBox::toggled, this, &QG_DimOptions::onBaselineDistanceFreeClicked);

    connect(ui->leAngleCircle, &QLineEdit::textChanged, this, &QG_DimOptions::onAngleCircleTextChanged);
    connect(ui->cbAngleCircleFree, &QCheckBox::toggled, this, &QG_DimOptions::onAngleCircleFreeClicked);

    connect(ui->leTol1, &QLineEdit::textChanged, this, &QG_DimOptions::updateLabel);
    connect(ui->leTol2, &QLineEdit::textChanged, this, &QG_DimOptions::updateLabel);
    connect(ui->leLabel, &QLineEdit::textChanged, this, &QG_DimOptions::updateLabel);

    connect(ui->bDiameter, &QCheckBox::toggled, this, &QG_DimOptions::updateLabel);
    connect(ui->cbSymbol, &QComboBox::currentTextChanged, this, &QG_DimOptions::insertSign);
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

void QG_DimOptions::doSaveSettings() {
    save("Label", ui->leLabel->text());
    save("Tol1", ui->leTol1->text());
    save("Tol2", ui->leTol2->text());

    RS2::ActionType rtti = action->rtti();
    if (rtti == RS2::ActionDimRadial) {
        save("Radial", ui->bDiameter->isChecked());
        save("RadialAngle", ui->leAngleCircle->text());
        save("RadialAngleFree", ui->cbAngleCircleFree->isChecked());
    } else if (rtti == RS2::ActionDimDiametric){
        save("Diameter", ui->bDiameter->isChecked());
        save("DiameterAngle", ui->leAngleCircle->text());
        save("DiameterAngleFree", ui->cbAngleCircleFree->isChecked());
    }
    else if (rtti == RS2::ActionDimLinear){
        save("Angle", ui->leAngle->text());
    }
    else if (rtti == RS2::ActionDimBaseline){
        save("BaselineDistanceFree", ui->cbFreeBaselineDistance->isChecked());
        save("BaselineDistance", ui->leBaselineDistance->text());
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
    RS2::ActionType type = action->rtti();
    bool isDimLinear = type == RS2::ActionDimLinear;
    bool baseline = type == RS2::ActionDimBaseline;
    bool circleDim = type == RS2::ActionDimDiametric || type == RS2::ActionDimRadial;
    bool freeBaseLineDistance = false;
    bool circleAngleFree = false;
    QString circleAngle;
    QString baselineDistance;
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
        else if (baseline){
            auto dimBaselineAction = dynamic_cast<LC_ActionDrawDimBaseline *>(action);
            baselineDistance = fromDouble((dimBaselineAction->getBaselineDistance()));
            freeBaseLineDistance = dimBaselineAction->isFreeBaselineDistance();
        }
        else if (circleDim){
            auto dimAction = dynamic_cast<LC_ActionCircleDimBase *>(action);
            circleAngle = fromDouble(RS_Math::rad2deg(dimAction->getAngle()));
            circleAngleFree  = dimAction->isAngleIsFree();
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
        else if (baseline){
            freeBaseLineDistance = loadBool("BaselineDistanceFree", false);
            baselineDistance = load("BaselineDistance", "20");
        }
        else if (type == RS2::ActionDimDiametric){
            circleAngle = load("DiameterAngle", "45");
            circleAngleFree = loadBool("DiameterAngleFree", true);
        }
        else if (type == RS2::ActionDimRadial){
            circleAngle = load("RadialAngle", "45");
            circleAngleFree = loadBool("RadialAngleFree", true);
        }
    }

    if (type == RS2::ActionDimRadial){
            ui->bDiameter->setIcon({});
        ui->bDiameter->setText(tr("R", "Radial dimension prefix"));
        ui->bDiameter->setChecked(radial);
        action->setDiameter(radial);

        ui->cbAngleCircleFree->setChecked(circleAngleFree);
        ui->leAngleCircle->setText(circleAngle);
    } else if (type == RS2::ActionDimDiametric){
        ui->bDiameter->setChecked(diam);
        ui->cbAngleCircleFree->setChecked(circleAngleFree);
        ui->leAngleCircle->setText(circleAngle);
    }
    else if (baseline){
        ui->cbFreeBaselineDistance->setChecked(freeBaseLineDistance);
        ui->leBaselineDistance->setText(baselineDistance);
        ui->leBaselineDistance->setEnabled(!freeBaseLineDistance);
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
    
    ui->lblBaselineDistance->setVisible(baseline);
    ui->leBaselineDistance->setVisible(baseline);
    ui->cbFreeBaselineDistance->setVisible(baseline);

    ui->lblAngleCircle->setVisible(circleDim);
    ui->cbAngleCircleFree->setVisible(circleDim);
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
    double angleDegrees = RS_Math::eval(a);
    dimLinearAction->setAngle(RS_Math::deg2rad(angleDegrees));
    bool checkVert = !LC_LineMath::isMeaningfulAngle(90-angleDegrees);
    ui->bVer->setChecked(checkVert);
    bool checkHor = !LC_LineMath::isMeaningfulAngle(angleDegrees);
    ui->bHor->setChecked(checkHor);
}

void QG_DimOptions::onHorClicked(){
    ui->leAngle->setText("0");
    updateAngle("0");
}

void QG_DimOptions::onVerClicked(){
    ui->leAngle->setText("90");
    updateAngle("90");
}

void QG_DimOptions::onAngleEditingFinished(){
    updateAngle(ui->leAngle->text());
}

void QG_DimOptions::onBaselineDistanceFreeClicked() {
    bool freeDistance = ui->cbFreeBaselineDistance->isChecked();
    ui->leBaselineDistance->setEnabled(!freeDistance);
    auto dimBaselineAction = dynamic_cast<LC_ActionDrawDimBaseline *>(action);
    dimBaselineAction->setFreeBaselineDistance(freeDistance);
}

void QG_DimOptions::onAngleCircleFreeClicked() {
    bool freeAngle = ui->cbAngleCircleFree->isChecked();
    ui->leAngleCircle->setEnabled(!freeAngle);
    auto dimAction = dynamic_cast<LC_ActionCircleDimBase *>(action);
    dimAction->setAngleIsFree(freeAngle);
}

void QG_DimOptions::onBaselineDistanceTextChanged() {
    QString distance = ui->leBaselineDistance->text();
    double len;
    if (toDouble(distance, len, 0.0, true)){
        auto dimBaselineAction = dynamic_cast<LC_ActionDrawDimBaseline *>(action);
        dimBaselineAction->setBaselineDistance(len);
        ui->leBaselineDistance->blockSignals(true);
        ui->leBaselineDistance->setText(fromDouble(len));
        ui->leBaselineDistance->blockSignals(false);
    }
}

void QG_DimOptions::onAngleCircleTextChanged() {
    QString distance = ui->leAngleCircle->text();
    double len;
    if (toDoubleAngle(distance, len, 45, false)){
        auto dimAction = dynamic_cast<LC_ActionCircleDimBase *>(action);
        dimAction->setAngle(RS_Math::deg2rad(len)   );
        ui->leAngleCircle->blockSignals(true);
        ui->leAngleCircle->setText(fromDouble(len));
        ui->leAngleCircle->blockSignals(false);
    }
}

void QG_DimOptions::updateUI(int mode) {
    switch (mode){
        case UI_UPDATE_BASELINE_DISTANCE:{
            auto dimBaselineAction = dynamic_cast<LC_ActionDrawDimBaseline *>(action);
//            ui->leBaselineDistance->blockSignals(true);
            ui->leBaselineDistance->setText(fromDouble(dimBaselineAction->getCurrentBaselineDistance()));
//            ui->leBaselineDistance->blockSignals(false);
            break;
        }
        case UI_UPDATE_CIRCLE_ANGLE:{
            auto dimAction = dynamic_cast<LC_ActionCircleDimBase *>(action);
            ui->leAngleCircle->setText(fromDouble(RS_Math::rad2deg(dimAction->getCurrentAngle())));
            break;
        }
        default:
            break;
    }
}
