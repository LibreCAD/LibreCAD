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

#include "lc_polygon_options_widget.h"

#include "lc_action_draw_polygon_base.h"
#include "lc_action_draw_polygon_side_side.h"
#include "lc_guarded_signals_blocker.h"
#include "ui_lc_polygon_options_widget.h"

LC_PolygonOptionsWidget::LC_PolygonOptionsWidget(): ui(new Ui::LC_PolygonOptionsWidget{}) {
    ui->setupUi(this);
    connect(ui->sbNumber, &QSpinBox::valueChanged, this, &LC_PolygonOptionsWidget::onNumberValueChanged);
    connect(ui->cbPolyline, &QCheckBox::toggled, this, &LC_PolygonOptionsWidget::onPolylineToggled);
    connect(ui->cbRadius, &QCheckBox::toggled, this, &LC_PolygonOptionsWidget::onRadiusToggled);
    connect(ui->cbVertexToVertex, &QCheckBox::toggled, this, &LC_PolygonOptionsWidget::onVertexToggled);
    connect(ui->leRadius, &QLineEdit::editingFinished, this, &LC_PolygonOptionsWidget::onRadiusEditingFinished);
    pickDistanceSetup("radius", ui->tbPickRadius, ui->leRadius);
}

/*
 *  Destroys the object and frees any allocated resources
 */
LC_PolygonOptionsWidget::~LC_PolygonOptionsWidget() = default;

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void LC_PolygonOptionsWidget::languageChange() {
    ui->retranslateUi(this);
}

void LC_PolygonOptionsWidget::doUpdateByAction(RS_ActionInterface* a) {
    m_action = static_cast<LC_ActionDrawPolygonBase*>(a);
    bool vertextVertex = false;

    const int number = m_action->getNumber();
    const bool polyline = m_action->isPolyline();
    const bool rounded = m_action->isCornersRounded();
    const QString radius = fromDouble(m_action->getRoundingRadius());
    if (m_sideSideAction) {
        const auto* specificAction = static_cast<LC_ActionDrawLinePolygonSideSide*>(a);
        vertextVertex = specificAction->isVertexVertexMode();
    }

    ui->cbVertexToVertex->setVisible(m_sideSideAction);

    LC_GuardedSignalsBlocker({ui->sbNumber, ui->cbPolyline, ui->cbRadius, ui->leRadius, ui->cbVertexToVertex});
    ui->sbNumber->setValue(number);
    ui->cbPolyline->setChecked(polyline);
    ui->cbRadius->setChecked(rounded);

    ui->leRadius->setEnabled(rounded);
    ui->tbPickRadius->setEnabled(rounded);
    ui->leRadius->setText(radius);

    if (m_sideSideAction) {
        ui->cbVertexToVertex->setChecked(vertextVertex);
    }
}

void LC_PolygonOptionsWidget::onNumberValueChanged([[maybe_unused]] int number) {
    m_action->setNumber(ui->sbNumber->value());
    m_action->updateOptions();
}

void LC_PolygonOptionsWidget::onPolylineToggled([[maybe_unused]] bool value) {
    m_action->setPolyline(ui->cbPolyline->isChecked());
    m_action->updateOptions();
}

void LC_PolygonOptionsWidget::onRadiusToggled([[maybe_unused]] bool val) {
    m_action->setCornersRounded(ui->cbRadius->isChecked());
    m_action->updateOptions();
}

void LC_PolygonOptionsWidget::onVertexToggled([[maybe_unused]] bool val) {
    const bool value = ui->cbVertexToVertex->isChecked();
    if (m_sideSideAction) {
        auto* specificAction = static_cast<LC_ActionDrawLinePolygonSideSide*>(m_action);
        Q_ASSERT(specificAction != nullptr);
        specificAction->setVertexVertexMode(value);
    }
    m_action->updateOptions();
}

void LC_PolygonOptionsWidget::onRadiusEditingFinished() {
    const auto val = ui->leRadius->text();
    double value = 0.;
    if (toDouble(val, value, 0.0, true)) {
        m_action->setRoundingRadius(value);
    }
    m_action->updateOptions();
}
