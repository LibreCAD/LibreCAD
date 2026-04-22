/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2024 LibreCAD.org
 Copyright (C) 2024 sand1024

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 ******************************************************************************/

#include "lc_ellipse_arc_options_widget.h"

#include "lc_action_draw_ellipse_axis.h"
#include "lc_guarded_signals_blocker.h"
#include "ui_lc_ellipse_arc_options_widget.h"

/*
 *  Constructs a QG_ArcOptions as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
LC_EllipseArcOptionsWidget::LC_EllipseArcOptionsWidget()
    : ui(std::make_unique<Ui::LC_EllipseArcOptionsWidget>()){
    ui->setupUi(this);
    connect(ui->rbPos, &QRadioButton::toggled, this, &LC_EllipseArcOptionsWidget::onDirectionChanged);
    connect(ui->rbNeg,  &QRadioButton::toggled, this, &LC_EllipseArcOptionsWidget::onDirectionChanged);
}

/*
 *  Destroys the object and frees any allocated resources
 */
LC_EllipseArcOptionsWidget::~LC_EllipseArcOptionsWidget() = default;

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void LC_EllipseArcOptionsWidget::languageChange(){
    ui->retranslateUi(this);
}

void LC_EllipseArcOptionsWidget::doUpdateByAction(RS_ActionInterface *a){
    m_action = static_cast<RS_ActionDrawEllipseAxis *>(a);
    const bool reversed = m_action->isReversed();
    LC_GuardedSignalsBlocker({ui->rbNeg, ui->rbPos});
    ui->rbNeg->setChecked(reversed);
}

/*void QG_ArcOptions::setData(RS_ArcData* d) {
    data = d;
    updateDirection(false);
}*/
void LC_EllipseArcOptionsWidget::onDirectionChanged(bool /*pos*/) const {
    m_action->setReversed(ui->rbNeg->isChecked());
    m_action->updateOptions();
}
