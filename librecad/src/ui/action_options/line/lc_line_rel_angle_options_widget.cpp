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
#include "lc_line_rel_angle_options_widget.h"

#include "lc_action_draw_line_rel_angle.h"
#include "lc_guarded_signals_blocker.h"
#include "rs_dimension.h"
#include "ui_lc_line_rel_angle_options_widget.h"

namespace {
    // format a number with specified digits after point
    [[maybe_unused]] QString formatNumber(const double value, int precision = 8) {
        precision = std::max(precision, 0);
        precision = std::min(precision, 16);
        //        LC_ERR<<"value: "<<value;
        QString text = QString("%1").arg(value, 0, 'f', precision);
        //        LC_ERR<<"value: "<<text;
        text = RS_Dimension::stripZerosLinear(text, 12);
        //        LC_ERR<<"value: "<<text;
        return text;
    }
}

/*
 *  Constructs a QG_LineRelAngleOptions as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
LC_LineRelAngleOptionsWidget::LC_LineRelAngleOptionsWidget(): ui(std::make_unique<Ui::LC_LineRelAngleOptionsWidget>()) {
    ui->setupUi(this);
    connect(ui->leLength, &QLineEdit::editingFinished, this, &LC_LineRelAngleOptionsWidget::onLengthEditingFinished);
    connect(ui->leAngle, &QLineEdit::editingFinished, this, &LC_LineRelAngleOptionsWidget::onAngleEditingFinished);

    pickAngleSetup("angle", ui->tbPickAngle, ui->leAngle);
    pickDistanceSetup("distance", ui->tbPickLength, ui->leLength);
}

/*
 *  Destroys the object and frees any allocated resources
 */
LC_LineRelAngleOptionsWidget::~LC_LineRelAngleOptionsWidget() = default;

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void LC_LineRelAngleOptionsWidget::languageChange() {
    ui->retranslateUi(this);
}

void LC_LineRelAngleOptionsWidget::doUpdateByAction(RS_ActionInterface* a) {
    m_action = static_cast<LC_ActionDrawLineRelAngle*>(a);
    const bool fixedAngle = m_action->hasFixedAngle();

    ui->lAngle->setVisible(!fixedAngle);
    ui->leAngle->setVisible(!fixedAngle);
    ui->tbPickAngle->setVisible(!fixedAngle && m_interactiveInputControlsVisible);

    const QString angle = fromDouble(m_action->getAngle());
    const QString length = fromDouble(m_action->getLength());

    LC_GuardedSignalsBlocker({ui->leAngle,});

    if (!fixedAngle) {
        ui->leAngle->setText(angle);
    }
    ui->leLength->setText(length);
}

void LC_LineRelAngleOptionsWidget::onLengthEditingFinished() {
    const auto val = ui->leLength->text();
    double length;
    if (toDouble(val, length, 1.0, false)) {
        m_action->setLength(length);
    }
    m_action->updateOptions();
}

void LC_LineRelAngleOptionsWidget::onAngleEditingFinished() {
    const auto val = ui->leAngle->text();
    double angle;
    if (toDoubleAngleDegrees(val, angle, 1.0, false)) {
        m_action->setAngle(angle);
    }
    m_action->updateOptions();
}
