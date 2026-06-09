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
#include "lc_arc_center_point_param_options_widget.h"

#include "lc_action_draw_arc_center_point_param.h"
#include "lc_guarded_signals_blocker.h"
#include "ui_lc_arc_center_point_param_options_widget.h"

/*
 *  Constructs a QG_ArcOptions as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
LC_ArcCenterPointParamOptionsWidget::LC_ArcCenterPointParamOptionsWidget()
    : ui(std::make_unique<Ui::LC_ArcCenterPointParamOptionsWidget>()) {
    ui->setupUi(this);
    connect(ui->rbPos, &QRadioButton::toggled, this, &LC_ArcCenterPointParamOptionsWidget::onDirectionChanged);
    connect(ui->rbNeg, &QRadioButton::toggled, this, &LC_ArcCenterPointParamOptionsWidget::onDirectionChanged);
}

/*
 *  Destroys the object and frees any allocated resources
 */
LC_ArcCenterPointParamOptionsWidget::~LC_ArcCenterPointParamOptionsWidget() = default;

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void LC_ArcCenterPointParamOptionsWidget::languageChange() {
    ui->retranslateUi(this);
}

void LC_ArcCenterPointParamOptionsWidget::doUpdateByAction(RS_ActionInterface* a) {
    m_action = static_cast<LC_ActionDrawArcCenterPointParam*>(a);
    const bool reversed = m_action->isReversed();

    LC_GuardedSignalsBlocker({ui->rbNeg});
    ui->rbNeg->setChecked(reversed);
}

void LC_ArcCenterPointParamOptionsWidget::onDirectionChanged(bool /*pos*/) {
     m_action->setReversed(ui->rbNeg->isChecked());
}
