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

#include "lc_spline_options_widget.h"

#include "lc_action_draw_spline.h"
#include "lc_guarded_signals_blocker.h"
#include "ui_lc_spline_options_widget.h"

/*
 *  Constructs a QG_SplineOptions as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
LC_SplineOptionsWidget::LC_SplineOptionsWidget(): ui(new Ui::LC_SplineOptionsWidget{}) {
    ui->setupUi(this);

    connect(ui->cbDegree, &QComboBox::currentIndexChanged, this, &LC_SplineOptionsWidget::onDegreeIndexChanged);
    connect(ui->cbClosed, &QCheckBox::clicked, this, &LC_SplineOptionsWidget::onClosedClicked);
    connect(ui->bUndo, &QToolButton::clicked, this, &LC_SplineOptionsWidget::undo);
}

/*
 *  Destroys the object and frees any allocated resources
 */
LC_SplineOptionsWidget::~LC_SplineOptionsWidget() = default;

void LC_SplineOptionsWidget::doUpdateByAction(RS_ActionInterface* a) {
    m_action = static_cast<LC_ActionDrawSpline*>(a);

    const bool drawSplineAction = a->rtti() == RS2::ActionDrawSpline;

    const int degree = m_action->getDegree();
    const bool closed = m_action->isClosed();

    LC_GuardedSignalsBlocker({ui->lDegree, ui->cbClosed});

    ui->lDegree->setVisible(drawSplineAction);
    if (drawSplineAction) {
        ui->cbDegree->setCurrentIndex(degree - 1);
    }
    ui->cbDegree->setVisible(drawSplineAction);
    ui->cbClosed->setChecked(closed);
}

void LC_SplineOptionsWidget::onClosedClicked(const bool value) {
    m_action->setClosed(value);
    m_action->updateOptions();
}

void LC_SplineOptionsWidget::undo() const {
    m_action->undo();
    m_action->updateOptions();
}

void LC_SplineOptionsWidget::onDegreeIndexChanged(const int index) {
    m_action->setDegree(index + 1);
    m_action->updateOptions();
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void LC_SplineOptionsWidget::languageChange() {
    ui->retranslateUi(this);
}
