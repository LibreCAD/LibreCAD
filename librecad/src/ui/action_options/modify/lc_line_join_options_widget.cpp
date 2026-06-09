/****************************************************************************
**
* Options widget for "LineJoin" action.

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
**********************************************************************/
#include "lc_line_join_options_widget.h"

#include "lc_action_modify_line_join.h"
#include "lc_guarded_signals_blocker.h"
#include "ui_lc_line_join_options_widget.h"

LC_LineJoinOptionsWidget::LC_LineJoinOptionsWidget() :ui(new Ui::LC_LineJoinOptionsWidget) {
    ui->setupUi(this);

    connect(ui->cbLine1EdgeMode, &QComboBox::currentIndexChanged, this, &LC_LineJoinOptionsWidget::onEdgeModelLine1IndexChanged);
    connect(ui->cbLine2EdgeMode, &QComboBox::currentIndexChanged, this, &LC_LineJoinOptionsWidget::onEdgeModelLine2IndexChanged);
    connect(ui->cbAttributesSource, &QComboBox::currentIndexChanged, this, &LC_LineJoinOptionsWidget::onAttributesSourceIndexChanged);

    connect(ui->cbPolyline, &QCheckBox::clicked, this, &LC_LineJoinOptionsWidget::onUsePolylineClicked);
    connect(ui->cbRemoveOriginals, &QCheckBox::clicked, this, &LC_LineJoinOptionsWidget::onRemoveOriginalsClicked);
}

LC_LineJoinOptionsWidget::~LC_LineJoinOptionsWidget() {
    delete ui;
    m_action = nullptr;
}

void LC_LineJoinOptionsWidget::languageChange() {
    ui->retranslateUi(this);
}

void LC_LineJoinOptionsWidget::doUpdateByAction(RS_ActionInterface* a) {
    m_action = static_cast<LC_ActionModifyLineJoin*>(a);

    const int line1EdgeMode = m_action->getLine1EdgeMode();
    const int line2EdgeMode = m_action->getLine2EdgeMode();
    const bool usePolyline = m_action->isCreatePolyline();
    const bool removeOriginals = m_action->isRemoveOriginalLines();
    const int attributesSource = m_action->getAttributesSource();

    LC_GuardedSignalsBlocker({ui->cbPolyline,ui->cbRemoveOriginals,ui->cbAttributesSource,ui->cbLine1EdgeMode, ui->cbLine2EdgeMode});

    ui->cbPolyline->setChecked(usePolyline);
    ui->cbRemoveOriginals->setChecked(removeOriginals);
    ui->cbAttributesSource->setCurrentIndex(attributesSource);
    ui->cbLine1EdgeMode->setCurrentIndex(line1EdgeMode);
    ui->cbLine2EdgeMode->setCurrentIndex(line2EdgeMode);

    const bool allowRemoval = line1EdgeMode == LC_ActionModifyLineJoin::EdgeMode::EDGE_EXTEND_TRIM ||
                line2EdgeMode == LC_ActionModifyLineJoin::EdgeMode::EDGE_EXTEND_TRIM;
    ui->cbRemoveOriginals->setEnabled(allowRemoval);

    const bool dontAllowPolyline = line1EdgeMode == LC_ActionModifyLineJoin::EdgeMode::EDGE_NO_MODIFICATION ||
                line2EdgeMode == LC_ActionModifyLineJoin::EdgeMode::EDGE_NO_MODIFICATION;

    ui->cbPolyline->setDisabled(dontAllowPolyline);
}

void LC_LineJoinOptionsWidget::onUsePolylineClicked(const bool value) const {
    m_action->setCreatePolyline(value);
    m_action->updateOptions();
}

void LC_LineJoinOptionsWidget::onRemoveOriginalsClicked(const bool value) const {
    m_action->setRemoveOriginalLines(value);
    m_action->updateOptions();
}

void LC_LineJoinOptionsWidget::onEdgeModelLine1IndexChanged(const int index) const {
    m_action->setLine1EdgeMode(index);
    m_action->updateOptions();
}

void LC_LineJoinOptionsWidget::onEdgeModelLine2IndexChanged(const int index) const {
    m_action->setLine2EdgeMode(index);
    m_action->updateOptions();
}

void LC_LineJoinOptionsWidget::onAttributesSourceIndexChanged(const int index) const {
    m_action->setAttributesSource(index);
    m_action->updateOptions();
}
