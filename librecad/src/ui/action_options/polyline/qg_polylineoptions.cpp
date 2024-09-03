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
#include "rs_settings.h"
#include "rs_math.h"
#include "ui_qg_polylineoptions.h"
#include "rs_debug.h"

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
    action = dynamic_cast<RS_ActionDrawPolyline *>(a);
    QString radius, angle;
    int mode;
    bool reversed;

    if (update){
        radius = fromDouble(action->getRadius());
        angle = fromDouble(action->getAngle());
        mode = action->getMode();
        reversed = action->isReversed();
    } else {
        radius = load("Radius", "1.0");
        angle = load("Angle", "180.0");
        mode = loadInt("Mode", 0);
        reversed = loadBool("Reversed", false);

        action->setRadius(radius.toDouble());
        action->setAngle(angle.toDouble());
        action->setMode((RS_ActionDrawPolyline::SegmentMode) mode);
        action->setReversed(reversed);
    }
    ui->leRadius->setText(radius);
    ui->leAngle->setText(angle);

    setAngleToActionAndView(angle);
    setRadiusToActionAndView(radius);
    setReversedToActionAndView(reversed);
    setModeToActionAndView(mode);
}

void QG_PolylineOptions::close(){
    action->close();
}

void QG_PolylineOptions::undo(){
    action->undo();
}

void QG_PolylineOptions::setReversedToActionAndView(bool reversed){
    ui->rbNeg->setChecked(reversed);
    action->setReversed(reversed);
}

void QG_PolylineOptions::setAngleToActionAndView(const QString& strVal){
    double angle;
    if (toDouble(strVal, angle, 0.0, false)){
        if (angle > 359.999){
            angle = 359.999;
        }
        else if (angle < 0.0){
            angle = 0.0;
        }
        action->setAngle(angle);
        ui->leAngle->setText(fromDouble(angle));
    }
}

void QG_PolylineOptions::setRadiusToActionAndView(const QString& strVal){
    double val;
    if (toDouble(strVal, val, 1.0, false)){
        action->setRadius(val);
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

    auto segmentMode = (RS_ActionDrawPolyline::SegmentMode) m;

    action->setMode(segmentMode);
    ui->cbMode->setCurrentIndex(m);

    switch (segmentMode) {
        case RS_ActionDrawPolyline::Line:
        case RS_ActionDrawPolyline::Tangential:
        default: {
            for (QWidget *p: wLists{ui->leRadius, ui->leAngle, ui->lRadius, ui->lAngle, ui->buttonGroup1, ui->rbPos, ui->rbNeg})
                p->hide();
            break;
        }
        case RS_ActionDrawPolyline::TanRad: {
            for (QWidget *p: wLists{ui->leAngle, ui->lAngle, ui->buttonGroup1, ui->rbPos, ui->rbNeg})
                p->hide();
            for (QWidget *p: wLists{ui->leRadius, ui->lRadius})
                p->show();
            break;
        }
            //        case TanAng:
        case RS_ActionDrawPolyline::Ang: {
            for (QWidget *p: wLists{ui->leRadius, ui->lRadius})
                p->hide();
            for (QWidget *p: wLists{ui->leAngle, ui->lAngle, ui->buttonGroup1, ui->rbPos, ui->rbNeg})
                p->show();
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

void QG_PolylineOptions::onNegToggled(bool checked){
    setReversedToActionAndView(!checked);
}
