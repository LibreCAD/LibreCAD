/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2026 LibreCAD.org
 * Copyright (C) 2026 Dongxu Li github.com/dxli
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 * ********************************************************************************
 */

#include "lc_textpropertieseditingwidget.h"

#include "rs_text.h"
#include "ui_lc_textpropertieseditingwidget.h"

LC_TextPropertiesEditingWidget::LC_TextPropertiesEditingWidget(QWidget *parent)
    : LC_EntityPropertiesEditorWidget(parent)
    , ui(new Ui::LC_TextPropertiesEditingWidget) {
    ui->setupUi(this);

    connect(ui->leText, &QLineEdit::editingFinished, this,
            &LC_TextPropertiesEditingWidget::onTextEditingFinished);
    connect(ui->leHeight, &QLineEdit::editingFinished, this,
            &LC_TextPropertiesEditingWidget::onHeightEditingFinished);
    connect(ui->leAngle, &QLineEdit::editingFinished, this,
            &LC_TextPropertiesEditingWidget::onAngleEditingFinished);
    connect(ui->leStyle, &QLineEdit::editingFinished, this,
            &LC_TextPropertiesEditingWidget::onStyleEditingFinished);
    connect(ui->rbAuto, &QRadioButton::toggled, this,
            &LC_TextPropertiesEditingWidget::onDirectionToggled);
    connect(ui->rbLeftToRight, &QRadioButton::toggled, this,
            &LC_TextPropertiesEditingWidget::onDirectionToggled);
    connect(ui->rbRightToLeft, &QRadioButton::toggled, this,
            &LC_TextPropertiesEditingWidget::onDirectionToggled);
}

LC_TextPropertiesEditingWidget::~LC_TextPropertiesEditingWidget() {
    delete ui;
}

void LC_TextPropertiesEditingWidget::setEntity(RS_Entity *entity) {
    m_entity = static_cast<RS_Text *>(entity);

    QSignalBlocker textBlocker(ui->leText);
    ui->leText->setText(m_entity->getText());

    toUIValue(m_entity->getHeight(), ui->leHeight);
    toUIAngleDeg(m_entity->getAngle(), ui->leAngle);
    ui->leStyle->setText(m_entity->getStyle());

    QSignalBlocker bAuto(ui->rbAuto);
    QSignalBlocker bL(ui->rbLeftToRight);
    QSignalBlocker bR(ui->rbRightToLeft);
    switch (m_entity->getDrawingDirection()) {
    case RS_TextData::LeftToRight: ui->rbLeftToRight->setChecked(true); break;
    case RS_TextData::RightToLeft: ui->rbRightToLeft->setChecked(true); break;
    case RS_TextData::ByContent:
    default:                       ui->rbAuto->setChecked(true);        break;
    }
    applyDirectionToEditor();
}

void LC_TextPropertiesEditingWidget::applyDirectionToEditor() {
    // Mirror the QG_DlgMText pattern so the line edit's bidi matches the
    // rendered output. For "Auto" we leave the line edit's direction at the
    // application default so Qt does first-strong-character detection on the
    // input live.
    Qt::LayoutDirection direction;
    if (ui->rbLeftToRight->isChecked()) {
        direction = Qt::LeftToRight;
    } else if (ui->rbRightToLeft->isChecked()) {
        direction = Qt::RightToLeft;
    } else {
        direction = Qt::LayoutDirectionAuto;
    }
    ui->leText->setLayoutDirection(direction);
    ui->leText->update();
}

void LC_TextPropertiesEditingWidget::onTextEditingFinished() {
    if (m_entity == nullptr) return;
    m_entity->setText(ui->leText->text());
}

void LC_TextPropertiesEditingWidget::onHeightEditingFinished() {
    m_entity->setHeight(toWCSValue(ui->leHeight, m_entity->getHeight()));
}

void LC_TextPropertiesEditingWidget::onAngleEditingFinished() {
    m_entity->setAngle(toWCSAngle(ui->leAngle, m_entity->getAngle()));
}

void LC_TextPropertiesEditingWidget::onStyleEditingFinished() {
    m_entity->setStyle(ui->leStyle->text());
}

void LC_TextPropertiesEditingWidget::onDirectionToggled(
    [[maybe_unused]] bool checked) {
    if (m_entity == nullptr) return;
    if (ui->rbLeftToRight->isChecked()) {
        m_entity->setDrawingDirection(RS_TextData::LeftToRight);
    } else if (ui->rbRightToLeft->isChecked()) {
        m_entity->setDrawingDirection(RS_TextData::RightToLeft);
    } else {
        m_entity->setDrawingDirection(RS_TextData::ByContent);
    }
    applyDirectionToEditor();
}
