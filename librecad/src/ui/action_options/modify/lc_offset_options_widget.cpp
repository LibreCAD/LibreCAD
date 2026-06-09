/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2011 Dongxu Li ( dongxuli2011@gmail.com )
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**

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

** This copyright notice MUST APPEAR in all copies of the script!
**
**********************************************************************/

#include "lc_offset_options_widget.h"

#include "lc_action_modify_offset.h"
#include "lc_guarded_signals_blocker.h"
#include "ui_lc_offset_options_widget.h"

/*
 *  Constructs a QG_ModifyOffsetOptions
 */
LC_OffsetOptionsWidget::LC_OffsetOptionsWidget(): ui(new Ui::LC_OffsetOptionsWidget{}) {
    ui->setupUi(this);
    connect(ui->cbKeepOriginals, &QCheckBox::clicked, this, &LC_OffsetOptionsWidget::cbKeepOriginalsClicked);
    connect(ui->cbMultipleCopies, &QCheckBox::clicked, this, &LC_OffsetOptionsWidget::cbMultipleCopiesClicked);
    connect(ui->cbCurrentAttr, &QCheckBox::clicked, this, &LC_OffsetOptionsWidget::cbUseCurrentAttributesClicked);
    connect(ui->cbCurrentLayer, &QCheckBox::clicked, this, &LC_OffsetOptionsWidget::cbUseCurrentLayerClicked);
    connect(ui->leDist, &QLineEdit::editingFinished, this, &LC_OffsetOptionsWidget::onDistEditingFinished);
    connect(ui->sbNumberOfCopies, &QSpinBox::valueChanged, this, &LC_OffsetOptionsWidget::onNumberOfCopiesValueChanged);
    connect(ui->cbFixedDistance, &QCheckBox::clicked, this, &LC_OffsetOptionsWidget::onFixedDistanceClicked);
    pickDistanceSetup("distance", ui->tbPickDistance, ui->leDist);
}

/*
 *  Destroys the object and frees any allocated resources
 */
LC_OffsetOptionsWidget::~LC_OffsetOptionsWidget() = default;

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void LC_OffsetOptionsWidget::languageChange() {
    ui->retranslateUi(this);
}

void LC_OffsetOptionsWidget::doUpdateByAction(RS_ActionInterface* a) {
    m_action = static_cast<LC_ActionModifyOffset*>(a);

    const QString dist = fromDouble(m_action->getDistance());
    const bool distanceFixed = m_action->isFixedDistance();
    const bool useCurrentLayer = m_action->isUseCurrentLayer();
    const bool useCurrentAttributes = m_action->isUseCurrentAttributes();
    const int copiesNumber = m_action->getCopiesNumber();
    const bool keepOriginals = m_action->isKeepOriginals();
    const bool useMultipleCopies = m_action->isUseMultipleCopies();

    LC_GuardedSignalsBlocker({
        ui->leDist,
        ui->cbFixedDistance,
        ui->cbMultipleCopies,
        ui->sbNumberOfCopies,
        ui->cbCurrentLayer,
        ui->cbCurrentAttr,
        ui->cbKeepOriginals
    });
    ui->leDist->setText(dist);
    ui->leDist->setEnabled(distanceFixed);
    ui->tbPickDistance->setEnabled(distanceFixed);
    ui->cbFixedDistance->setChecked(distanceFixed);
    ui->cbMultipleCopies->setChecked(useMultipleCopies);
    ui->sbNumberOfCopies->setEnabled(useMultipleCopies);

    ui->sbNumberOfCopies->setValue(copiesNumber);
    ui->cbCurrentLayer->setChecked(useCurrentLayer);
    ui->cbCurrentAttr->setChecked(useCurrentAttributes);
    ui->cbKeepOriginals->setChecked(keepOriginals);
}

void LC_OffsetOptionsWidget::onDistEditingFinished() {
    const auto val = ui->leDist->text();
    double distance;
    if (toDouble(val, distance, 1.0, false)) {
        m_action->setDistance(distance);
    }
}

void LC_OffsetOptionsWidget::onFixedDistanceClicked(const bool val) {
    m_action->setDistanceFixed(val);
    m_action->updateOptions();
}

void LC_OffsetOptionsWidget::cbKeepOriginalsClicked(const bool val) {
    m_action->setKeepOriginals(val);
    m_action->updateOptions();
}

void LC_OffsetOptionsWidget::cbMultipleCopiesClicked(const bool val) {
    m_action->setUseMultipleCopies(val);
    m_action->updateOptions();
}

void LC_OffsetOptionsWidget::cbUseCurrentAttributesClicked(const bool val) {
    m_action->setUseCurrentAttributes(val);
    m_action->updateOptions();
}

void LC_OffsetOptionsWidget::cbUseCurrentLayerClicked(const bool val) {
    m_action->setUseCurrentLayer(val);
    m_action->updateOptions();
}

void LC_OffsetOptionsWidget::onNumberOfCopiesValueChanged(int number) {
    if (number < 1) {
        number = 1;
    }
    m_action->setCopiesNumber(number);
    m_action->updateOptions();
}
