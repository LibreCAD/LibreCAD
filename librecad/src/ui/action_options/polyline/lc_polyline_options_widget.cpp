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
#include "lc_polyline_options_widget.h"

#include "lc_action_draw_polyline.h"
#include "lc_guarded_signals_blocker.h"
#include "ui_lc_polyline_options_widget.h"

using wLists = std::initializer_list<QWidget*>;
/*
 *  Constructs a QG_PolylineOptions as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
LC_PolylineOptionsWidget::LC_PolylineOptionsWidget(): ui(new Ui::LC_PolylineOptionsWidget{}) {
    ui->setupUi(this);
    connect(ui->leAngle, &QLineEdit::editingFinished, this, &LC_PolylineOptionsWidget::onAngleEditingFinished);
    connect(ui->leRadius, &QLineEdit::editingFinished, this, &LC_PolylineOptionsWidget::onRadiusEditingFinished);
    connect(ui->rbNeg, &QRadioButton::toggled, this, &LC_PolylineOptionsWidget::onNegToggled);
    connect(ui->rbPos, &QRadioButton::toggled, this, &LC_PolylineOptionsWidget::onNegToggled);

    connect(ui->tbLine, &QToolButton::clicked, this, &LC_PolylineOptionsWidget::tbLineClicked);
    connect(ui->tbTangental, &QToolButton::clicked, this, &LC_PolylineOptionsWidget::tbTangentalClicked);
    connect(ui->tbTanRadius, &QToolButton::clicked, this, &LC_PolylineOptionsWidget::tbTanRadiusClicked);
    connect(ui->tbTanAngle, &QToolButton::clicked, this, &LC_PolylineOptionsWidget::tbTanAngleClicked);
    connect(ui->tbArcAngle, &QToolButton::clicked, this, &LC_PolylineOptionsWidget::tbArcAngleClicked);

    connect(ui->bClose, &QToolButton::clicked, this, &LC_PolylineOptionsWidget::close);
    connect(ui->bUndo, &QToolButton::clicked, this, &LC_PolylineOptionsWidget::undo);

    pickAngleSetup("angle", ui->tbPickAngle, ui->leAngle);
    pickDistanceSetup("radius", ui->tbPickRadius, ui->leRadius);
}

/*
 *  Destroys the object and frees any allocated resources
 */
LC_PolylineOptionsWidget::~LC_PolylineOptionsWidget() {
    destroy();
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void LC_PolylineOptionsWidget::languageChange() {
    ui->retranslateUi(this);
}


void LC_PolylineOptionsWidget::doUpdateByAction(RS_ActionInterface* a) {
    m_action = static_cast<LC_ActionDrawPolyline*>(a);

    const QString radius = fromDouble(m_action->getRadius());
    const QString angle = fromDouble(m_action->getAngleDegrees());
    const int mode = m_action->getMode();
    const bool reversed = m_action->isReversed();

    LC_GuardedSignalsBlocker({
        ui->leRadius,
        ui->leAngle,
        ui->rbNeg,
        ui->rbPos,
        ui->tbLine,
        ui->tbArcAngle,
        ui->tbTanAngle,
        ui->tbTangental,
        ui->tbTanRadius
    });
    ui->leRadius->setText(radius);
    ui->leAngle->setText(angle);
    ui->rbNeg->setChecked(reversed);

    updateUIBySegmentMode(mode);
}

void LC_PolylineOptionsWidget::close() const {
    m_action->close();
}

void LC_PolylineOptionsWidget::undo() const {
    m_action->undo();
}

void LC_PolylineOptionsWidget::updateUIBySegmentMode(int m) const {
    ui->tbTanRadius->setChecked(false);
    ui->tbTanAngle->setChecked(false);
    ui->tbTangental->setChecked(false);
    ui->tbLine->setChecked(false);
    ui->tbArcAngle->setChecked(false);
    const auto segmentMode = static_cast<LC_ActionDrawPolyline::SegmentMode>(m);

    switch (segmentMode) {
        case LC_ActionDrawPolyline::Line: {
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
        case LC_ActionDrawPolyline::Tangential: {
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
        case LC_ActionDrawPolyline::TangentalArcFixedRadius: {
            for (QWidget* p : wLists{ui->leAngle, ui->tbPickAngle, ui->lAngle, ui->buttonGroup1, ui->rbPos, ui->rbNeg}) {
                p->hide();
            }
            for (QWidget* p : wLists{ui->leRadius, ui->tbPickRadius, ui->lRadius}) {
                p->show();
            }
            ui->tbTanRadius->setChecked(true);
            break;
        }
        case LC_ActionDrawPolyline::TangentalArcFixedAngle: {
            for (QWidget* p : wLists{ui->leRadius, ui->tbPickRadius, ui->lRadius, ui->buttonGroup1, ui->rbPos, ui->rbNeg}) {
                p->hide();
            }
            for (QWidget* p : wLists{ui->leAngle, ui->tbPickAngle, ui->lAngle}) {
                p->show();
            }
            ui->tbTanAngle->setChecked(true);
            break;
        }
        case LC_ActionDrawPolyline::ArcFixedAngle: {
            for (QWidget* p : wLists{ui->leRadius, ui->tbPickRadius, ui->lRadius}) {
                p->hide();
            }
            for (QWidget* p : wLists{ui->leAngle, ui->tbPickAngle, ui->lAngle, ui->buttonGroup1, ui->rbPos, ui->rbNeg}) {
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

void LC_PolylineOptionsWidget::onAngleEditingFinished() {
    const auto strVal = ui->leAngle->text();
    double angle;
    if (toDoubleAngleDegrees(strVal, angle, 0.0, true)) {
        if (angle > 359.999) {
            angle = 359.999;
        }
        m_action->setAngleDegrees(angle);
    }
    m_action->updateOptions();
}

void LC_PolylineOptionsWidget::onRadiusEditingFinished() {
    const auto strVal = ui->leRadius->text();
    double val;
    if (toDouble(strVal, val, 1.0, false)) {
        m_action->setRadius(val);
    }
    m_action->updateOptions();
}

void LC_PolylineOptionsWidget::onNegToggled([[maybe_unused]] bool checked) {
    const bool enable = ui->rbNeg->isChecked();
    m_action->setReversed(enable);
    m_action->updateOptions();
}

void LC_PolylineOptionsWidget::tbLineClicked() {
    if (ui->tbLine->isChecked()) {
        m_action->setMode(LC_ActionDrawPolyline::Line);
        m_action->updateOptions();
    }
}

void LC_PolylineOptionsWidget::tbTangentalClicked() {
    if (ui->tbTangental->isChecked()) {
        m_action->setMode(LC_ActionDrawPolyline::Tangential);
        m_action->updateOptions();
    }
}

void LC_PolylineOptionsWidget::tbTanRadiusClicked() {
    if (ui->tbTanRadius->isChecked()) {
        m_action->setMode(LC_ActionDrawPolyline::TangentalArcFixedRadius);
        m_action->updateOptions();
    }
}

void LC_PolylineOptionsWidget::tbTanAngleClicked() {
    if (ui->tbTanAngle->isChecked()) {
        m_action->setMode(LC_ActionDrawPolyline::TangentalArcFixedAngle);
        m_action->updateOptions();
    }
}

void LC_PolylineOptionsWidget::tbArcAngleClicked() {
    if (ui->tbArcAngle->isChecked()) {
        m_action->setMode(LC_ActionDrawPolyline::ArcFixedAngle);
        m_action->updateOptions();
    }
}
