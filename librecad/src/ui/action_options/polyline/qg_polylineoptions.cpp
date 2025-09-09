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
#include "qg_polylineoptions.h"
#include "rs_actiondrawpolyline.h"
#include "ui_qg_polylineoptions.h"

using wLists = std::initializer_list<QWidget*>;
/*
 *  Constructs a QG_PolylineOptions as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_PolylineOptions::QG_PolylineOptions()
    :LC_ActionOptionsWidgetBase(RS2::ActionDrawPolyline, "Draw", "Polyline"),
    ui(new Ui::Ui_PolylineOptions{}){
    ui->setupUi(this);
    connect(ui->leAngle, &QLineEdit::editingFinished, this, &QG_PolylineOptions::onAngleEditingFinished);
    connect(ui->leRadius, &QLineEdit::editingFinished, this, &QG_PolylineOptions::onRadiusEditingFinished);
    connect(ui->rbNeg, &QRadioButton::toggled, this, &QG_PolylineOptions::onNegToggled);
    connect(ui->rbPos, &QRadioButton::toggled, this, &QG_PolylineOptions::onNegToggled);

    connect(ui->tbLine, &QToolButton::clicked, this, &QG_PolylineOptions::tbLineClicked);
    connect(ui->tbTangental, &QToolButton::clicked, this, &QG_PolylineOptions::tbTangentalClicked);
    connect(ui->tbTanRadius, &QToolButton::clicked, this, &QG_PolylineOptions::tbTanRadiusClicked);
    connect(ui->tbTanAngle, &QToolButton::clicked, this, &QG_PolylineOptions::tbTanAngleClicked);
    connect(ui->tbArcAngle, &QToolButton::clicked, this, &QG_PolylineOptions::tbArcAngleClicked);

    pickAngleSetup("angle", ui->tbPickAngle, ui->leAngle);
    pickDistanceSetup("radius", ui->tbPickRadius, ui->leRadius);

    // hide for now, probably it will be removed later
    ui->cbMode->setVisible(false);
}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_PolylineOptions::~QG_PolylineOptions(){
    destroy();
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_PolylineOptions::languageChange(){
    ui->retranslateUi(this);
}

void QG_PolylineOptions::doSaveSettings(){
    save("Mode", ui->cbMode->currentIndex());
    save("Radius", ui->leRadius->text());
    save("Angle", ui->leAngle->text());
    save("Reversed", ui->rbNeg->isChecked());
}

bool QG_PolylineOptions::checkActionRttiValid(RS2::ActionType actionType) {
    return actionType == RS2::ActionDrawPolyline || actionType == RS2::ActionPolylineAppend;
}

void QG_PolylineOptions::doSetAction(RS_ActionInterface *a, bool update){
    m_action = dynamic_cast<RS_ActionDrawPolyline *>(a);
    QString radius, angle;
    int mode;
    bool reversed;

    if (update){
        radius = fromDouble(m_action->getRadius());
        angle = fromDouble(m_action->getAngle());
        mode = m_action->getMode();
        reversed = m_action->isReversed();
    } else {
        radius = load("Radius", "1.0");
        angle = load("Angle", "180.0");
        mode = loadInt("Mode", 0);
        reversed = loadBool("Reversed", false);

        m_action->setRadius(radius.toDouble());
        m_action->setAngleDegrees(angle.toDouble());
        m_action->setMode((RS_ActionDrawPolyline::SegmentMode) mode);
        m_action->setReversed(reversed);
    }
    ui->leRadius->setText(radius);
    ui->leAngle->setText(angle);

    setAngleToActionAndView(angle);
    setRadiusToActionAndView(radius);
    setReversedToActionAndView(reversed);
    setModeToActionAndView(mode);
}

void QG_PolylineOptions::close(){
    m_action->close();
}

void QG_PolylineOptions::undo(){
    m_action->undo();
}

void QG_PolylineOptions::setReversedToActionAndView(bool reversed){
    ui->rbNeg->setChecked(reversed);
    m_action->setReversed(reversed);
}

void QG_PolylineOptions::setAngleToActionAndView(const QString& strVal){
    double angle;
    if (toDoubleAngleDegrees(strVal, angle, 0.0, true)){
        if (angle > 359.999){
            angle = 359.999;
        }
        m_action->setAngleDegrees(angle);
        ui->leAngle->setText(fromDouble(angle));
    }
}

void QG_PolylineOptions::setRadiusToActionAndView(const QString& strVal){
    double val;
    if (toDouble(strVal, val, 1.0, false)){
        m_action->setRadius(val);
        ui->leRadius->setText(fromDouble(val));
    }
}

void QG_PolylineOptions::setModeToActionAndView(int m){
//    enum Mode {
//        Line,Right-click on the package and choose Mark for Upgrade from the context menu, or press Ctrl + U.
//        Tangential,
//        TanRad,
////	TanAng,
////	TanRadAng,
//        Ang,
////	RadAngEndp,
////	RadAngCenp
//    };

    ui->tbTanRadius->setChecked(false);
    ui->tbTanAngle->setChecked(false);
    ui->tbTangental->setChecked(false);
    ui->tbLine->setChecked(false);
    ui->tbArcAngle->setChecked(false);

    auto segmentMode = (RS_ActionDrawPolyline::SegmentMode) m;

    m_action->setMode(segmentMode);
    ui->cbMode->setCurrentIndex(m);

    switch (segmentMode) {
        case RS_ActionDrawPolyline::Line:{
            ui->tbLine->setChecked(true);
            for (QWidget* p : wLists{
                     ui->leRadius,
                     ui->tbPickRadius,
                     ui->leAngle,
                     ui->tbPickAngle,
                     ui->lRadius,
                     ui->lAngle,
                     ui->buttonGroup1,
                     ui->rbPos,
                     ui->rbNeg
                 }) {
                p->hide();
            }
            break;
        }
        case RS_ActionDrawPolyline::Tangential:{
            ui->tbTangental->setChecked(true);
            for (QWidget* p : wLists{
                     ui->leRadius,
                     ui->tbPickRadius,
                     ui->leAngle,
                     ui->tbPickAngle,
                     ui->lRadius,
                     ui->lAngle,
                     ui->buttonGroup1,
                     ui->rbPos,
                     ui->rbNeg
                 }) {
                p->hide();
            }
            break;
        }
        case RS_ActionDrawPolyline::TanRad: {
            for (QWidget* p : wLists{
                     ui->leAngle,
                     ui->tbPickAngle,
                     ui->lAngle,
                     ui->buttonGroup1,
                     ui->rbPos,
                     ui->rbNeg
                 }) {
                p->hide();
            }
            for (QWidget* p : wLists{
                     ui->leRadius,
                     ui->tbPickRadius,
                     ui->lRadius
                 }) {
                p->show();
            }
            ui->tbTanRadius->setChecked(true);
            break;
        }
        case RS_ActionDrawPolyline::TanAng: {
            for (QWidget* p : wLists{
                     ui->leRadius,
                     ui->tbPickRadius,
                     ui->lRadius,
                     ui->buttonGroup1,
                     ui->rbPos,
                     ui->rbNeg
                 }) {
                p->hide();
            }
            for (QWidget* p : wLists{
                     ui->leAngle,
                     ui->tbPickAngle,
                     ui->lAngle
                 }) {
                p->show();
            }
            ui->tbTanAngle->setChecked(true);
            break;
        }
        case RS_ActionDrawPolyline::Ang: {
            for (QWidget* p : wLists{
                     ui->leRadius,
                     ui->tbPickRadius,
                     ui->lRadius
                 }) {
                p->hide();
            }
            for (QWidget* p : wLists{
                     ui->leAngle,
                     ui->tbPickAngle,
                     ui->lAngle,
                     ui->buttonGroup1,
                     ui->rbPos,
                     ui->rbNeg
                 }) {
                p->show();
            }
            ui->tbArcAngle->setChecked(true);
            break;
        }
            /*        case TanRadAng:
            case RadAngEndp:
            case RadAngCenp:
       ui->leRadius->setDisabled(false);
       ui->leAngle->setDisabled(false);
       ui->lRadius->setDisabled(false);
       ui->lAngle->setDisabled(false);
       ui->buttonGroup1->setDisabled(false);*/
    }
}

void QG_PolylineOptions::onAngleEditingFinished(){
    setAngleToActionAndView(ui->leAngle->text());
}

void QG_PolylineOptions::onRadiusEditingFinished(){
    setRadiusToActionAndView(ui->leRadius->text());
}

void QG_PolylineOptions::onNegToggled([[maybe_unused]]bool checked){
    bool enable = ui->rbNeg->isChecked();
    setReversedToActionAndView(enable);
}

void QG_PolylineOptions::tbLineClicked() {
    if (ui->tbLine->isChecked()){
        setModeToActionAndView(RS_ActionDrawPolyline::Line);
    }
}

void QG_PolylineOptions::tbTangentalClicked() {
    if (ui->tbTangental->isChecked()){
        setModeToActionAndView(RS_ActionDrawPolyline::Tangential);
    }
}

void QG_PolylineOptions::tbTanRadiusClicked() {
    if (ui->tbTanRadius->isChecked()) {
        setModeToActionAndView(RS_ActionDrawPolyline::TanRad);
    }
}
void QG_PolylineOptions::tbTanAngleClicked() {
    if (ui->tbTanAngle->isChecked()) {
        setModeToActionAndView(RS_ActionDrawPolyline::TanAng);
    }
}

void QG_PolylineOptions::tbArcAngleClicked() {
    if (ui->tbArcAngle->isChecked()){
        setModeToActionAndView(RS_ActionDrawPolyline::Ang);
    }
}
