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
#include "lc_arc_tangential_options_widget.h"

#include "lc_action_draw_arc_tangential.h"
#include "lc_guarded_signals_blocker.h"
#include "ui_lc_arc_tangential_options_widget.h"

#ifdef EMU_C99
#include "emu_c99.h"
#endif

/*
 *  Constructs a QG_ArcTangentialOptions as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
LC_ArcTangentialOptionsWidget::LC_ArcTangentialOptionsWidget(): ui(new Ui::LC_ArcTangentialOptionsWidget{}) {
    ui->setupUi(this);
    connect(ui->rbRadius, &QRadioButton::clicked, this, &LC_ArcTangentialOptionsWidget::onRadiusClicked);
    connect(ui->rbAngle, &QRadioButton::clicked, this, &LC_ArcTangentialOptionsWidget::onAngleClicked);
    connect(ui->leRadius, &QLineEdit::editingFinished, this, &LC_ArcTangentialOptionsWidget::onRadiusEditingFinished);
    connect(ui->leAngle, &QLineEdit::editingFinished, this, &LC_ArcTangentialOptionsWidget::onAngleEditingFinished);

    pickDistanceSetup("radius", ui->tbPickRadius, ui->leRadius);
    pickAngleSetup("angle", ui->tbPickAngle, ui->leAngle);
}

LC_ArcTangentialOptionsWidget::~LC_ArcTangentialOptionsWidget() = default;

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void LC_ArcTangentialOptionsWidget::languageChange() {
    ui->retranslateUi(this);
}

void LC_ArcTangentialOptionsWidget::doUpdateByAction(RS_ActionInterface* a) {
    m_action = static_cast<LC_ActionDrawArcTangential*>(a);
    const QString radius = fromDouble(m_action->getRadius());
    const QString angleDegrees = fromDouble(RS_Math::rad2deg(m_action->getAngle()));
    const bool byRadius = m_action->isByRadius();

    LC_GuardedSignalsBlocker({ui->leAngle, ui->leRadius, ui->rbAngle, ui->rbRadius});

    ui->rbRadius->setChecked(byRadius);
    ui->rbAngle->setChecked(!byRadius);
    ui->leRadius->setEnabled(byRadius);
    ui->tbPickRadius->setEnabled(byRadius);
    ui->leAngle->setEnabled(!byRadius);
    ui->tbPickAngle->setEnabled(!byRadius);

    ui->leRadius->setText(radius);
    ui->leAngle->setText(angleDegrees);
}

void LC_ArcTangentialOptionsWidget::onRadiusEditingFinished() {
    const QString& val = ui->leRadius->text();
    double radius;
    if (toDouble(val, radius, 1.0, true)) {
        m_action->setRadius(radius);
    }
    m_action->updateOptions();
}

void LC_ArcTangentialOptionsWidget::onAngleEditingFinished() {
    const auto val = ui->leAngle->text();
    double angleDegree;
    if (toDoubleAngleDegrees(val, angleDegree, 1.0, true)) {
        double angleRad = RS_Math::correctAngle(RS_Math::deg2rad(angleDegree));
        if (angleRad < RS_TOLERANCE_ANGLE || angleRad + RS_TOLERANCE_ANGLE > 2. * M_PI) {
            angleRad = M_PI; // can not do full circle
        }
        m_action->setAngle(angleRad);
    }
    m_action->updateOptions();
}

void LC_ArcTangentialOptionsWidget::onRadiusClicked(bool /*checked*/) {
    m_action->setByRadius(true);
    m_action->updateOptions();
}

void LC_ArcTangentialOptionsWidget::onAngleClicked(bool /*checked*/) {
    m_action->setByRadius(false);
    m_action->updateOptions();
}
