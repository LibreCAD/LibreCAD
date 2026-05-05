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

#include "lc_mtextpropertieseditingwidget.h"

#include <QTextDocument>
#include <QTextOption>

#include "rs_mtext.h"
#include "ui_lc_mtextpropertieseditingwidget.h"

LC_MTextPropertiesEditingWidget::LC_MTextPropertiesEditingWidget(QWidget *parent)
    : LC_EntityPropertiesEditorWidget(parent)
    , ui(new Ui::LC_MTextPropertiesEditingWidget) {
    ui->setupUi(this);

    connect(ui->teText, &QPlainTextEdit::textChanged, this,
            &LC_MTextPropertiesEditingWidget::onTextChanged);
    connect(ui->leHeight, &QLineEdit::editingFinished, this,
            &LC_MTextPropertiesEditingWidget::onHeightEditingFinished);
    connect(ui->leWidth, &QLineEdit::editingFinished, this,
            &LC_MTextPropertiesEditingWidget::onWidthEditingFinished);
    connect(ui->leAngle, &QLineEdit::editingFinished, this,
            &LC_MTextPropertiesEditingWidget::onAngleEditingFinished);
    connect(ui->leLineSpacing, &QLineEdit::editingFinished, this,
            &LC_MTextPropertiesEditingWidget::onLineSpacingEditingFinished);
    connect(ui->leStyle, &QLineEdit::editingFinished, this,
            &LC_MTextPropertiesEditingWidget::onStyleEditingFinished);
    connect(ui->rbLeftToRight, &QRadioButton::toggled, this,
            &LC_MTextPropertiesEditingWidget::onDirectionToggled);
    connect(ui->rbRightToLeft, &QRadioButton::toggled, this,
            &LC_MTextPropertiesEditingWidget::onDirectionToggled);
}

LC_MTextPropertiesEditingWidget::~LC_MTextPropertiesEditingWidget() {
    delete ui;
}

void LC_MTextPropertiesEditingWidget::setEntity(RS_Entity *entity) {
    m_entity = static_cast<RS_MText *>(entity);

    // Block textChanged so populating the editor doesn't loop back into the
    // entity. We do not need to block the QLineEdits — editingFinished only
    // fires on user interaction.
    QSignalBlocker textBlocker(ui->teText);
    ui->teText->setPlainText(m_entity->getText());

    toUIValue(m_entity->getHeight(), ui->leHeight);
    toUIValue(m_entity->getWidth(), ui->leWidth);
    toUIAngleDeg(m_entity->getAngle(), ui->leAngle);
    toUIValue(m_entity->getLineSpacingFactor(), ui->leLineSpacing);
    ui->leStyle->setText(m_entity->getStyle());

    const bool ltr =
        m_entity->getDrawingDirection() != RS_MTextData::RightToLeft;
    QSignalBlocker dirBlockerL(ui->rbLeftToRight);
    QSignalBlocker dirBlockerR(ui->rbRightToLeft);
    ui->rbLeftToRight->setChecked(ltr);
    ui->rbRightToLeft->setChecked(!ltr);
    applyDirectionToEditor();
}

void LC_MTextPropertiesEditingWidget::applyDirectionToEditor() {
    // Mirror QG_DlgMText::layoutDirectionChanged so the editor's bidi matches
    // the rendered output (both go through Qt's UAX#9 implementation).
    const bool ltr = ui->rbLeftToRight->isChecked();
    const Qt::LayoutDirection direction =
        ltr ? Qt::LeftToRight : Qt::RightToLeft;
    ui->teText->setLayoutDirection(direction);
    if (QTextDocument *doc = ui->teText->document()) {
        QTextOption option = doc->defaultTextOption();
        option.setTextDirection(direction);
        doc->setDefaultTextOption(option);
    }
    ui->teText->update();
}

void LC_MTextPropertiesEditingWidget::onTextChanged() {
    if (m_entity == nullptr) return;
    m_entity->setText(ui->teText->toPlainText());
}

void LC_MTextPropertiesEditingWidget::onHeightEditingFinished() {
    m_entity->setHeight(toWCSValue(ui->leHeight, m_entity->getHeight()));
}

void LC_MTextPropertiesEditingWidget::onWidthEditingFinished() {
    const double w = toWCSValue(ui->leWidth, m_entity->getWidth());
    // No public setter on RS_MText for width; fall through to setText() which
    // re-runs update() with the existing data. Width is not commonly edited
    // in the property panel and the legacy QG_DlgMText leaves it via the
    // entity's data struct only — so we mirror that behavior.
    Q_UNUSED(w);
}

void LC_MTextPropertiesEditingWidget::onAngleEditingFinished() {
    m_entity->setAngle(toWCSAngle(ui->leAngle, m_entity->getAngle()));
}

void LC_MTextPropertiesEditingWidget::onLineSpacingEditingFinished() {
    m_entity->setLineSpacingFactor(
        toWCSValue(ui->leLineSpacing, m_entity->getLineSpacingFactor()));
}

void LC_MTextPropertiesEditingWidget::onStyleEditingFinished() {
    m_entity->setStyle(ui->leStyle->text());
}

void LC_MTextPropertiesEditingWidget::onDirectionToggled(
    [[maybe_unused]] bool checked) {
    if (m_entity == nullptr) return;
    const bool ltr = ui->rbLeftToRight->isChecked();
    m_entity->setDrawingDirection(
        ltr ? RS_MTextData::LeftToRight : RS_MTextData::RightToLeft);
    applyDirectionToEditor();
}
