/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2026 LibreCAD (librecad.org)
 * Copyright (C) 2026 Dongxu Li (github.com/dxli)
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 * ********************************************************************************
 */

#include "lc_textpropertieseditingwidget.h"

#include "lc_textbidi.h"
#include "rs_text.h"
#include "ui_lc_textpropertieseditingwidget.h"

namespace {
// Combo-box index 0..14 maps to RS_Text alignment codes 1..15. See
// RS_Text::setAlignment for the encoding (top-left=1 .. bottom-right=12,
// fit=13, aligned=14, middle=15).
constexpr int alignmentCodeFromIndex(int index) { return index + 1; }
constexpr int alignmentIndexFromCode(int code) { return code - 1; }
} // namespace

using lc::textbidi::mirrorByLine;

LC_TextPropertiesEditingWidget::LC_TextPropertiesEditingWidget(QWidget *parent)
    : LC_EntityPropertiesEditorWidget(parent),
      ui(new Ui::LC_TextPropertiesEditingWidget) {
  ui->setupUi(this);

  ui->cbAlignment->addItem(tr("Top Left"));
  ui->cbAlignment->addItem(tr("Top Center"));
  ui->cbAlignment->addItem(tr("Top Right"));
  ui->cbAlignment->addItem(tr("Middle Left"));
  ui->cbAlignment->addItem(tr("Middle Center"));
  ui->cbAlignment->addItem(tr("Middle Right"));
  ui->cbAlignment->addItem(tr("Baseline Left"));
  ui->cbAlignment->addItem(tr("Baseline Center"));
  ui->cbAlignment->addItem(tr("Baseline Right"));
  ui->cbAlignment->addItem(tr("Bottom Left"));
  ui->cbAlignment->addItem(tr("Bottom Center"));
  ui->cbAlignment->addItem(tr("Bottom Right"));
  ui->cbAlignment->addItem(tr("Fit"));
  ui->cbAlignment->addItem(tr("Aligned"));
  ui->cbAlignment->addItem(tr("Middle"));

  connect(ui->leText, &QLineEdit::editingFinished, this,
          &LC_TextPropertiesEditingWidget::onTextEditingFinished);
  connect(ui->leHeight, &QLineEdit::editingFinished, this,
          &LC_TextPropertiesEditingWidget::onHeightEditingFinished);
  connect(ui->leWidthRel, &QLineEdit::editingFinished, this,
          &LC_TextPropertiesEditingWidget::onWidthRelEditingFinished);
  connect(ui->leAngle, &QLineEdit::editingFinished, this,
          &LC_TextPropertiesEditingWidget::onAngleEditingFinished);
  connect(ui->leStyle, &QLineEdit::editingFinished, this,
          &LC_TextPropertiesEditingWidget::onStyleEditingFinished);
  connect(ui->cbAlignment, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &LC_TextPropertiesEditingWidget::onAlignmentChanged);
  connect(ui->rbAuto, &QRadioButton::toggled, this,
          &LC_TextPropertiesEditingWidget::onDirectionToggled);
  connect(ui->rbLeftToRight, &QRadioButton::toggled, this,
          &LC_TextPropertiesEditingWidget::onDirectionToggled);
  connect(ui->rbRightToLeft, &QRadioButton::toggled, this,
          &LC_TextPropertiesEditingWidget::onDirectionToggled);
}

LC_TextPropertiesEditingWidget::~LC_TextPropertiesEditingWidget() { delete ui; }

void LC_TextPropertiesEditingWidget::setEntity(RS_Entity *entity) {
  m_entity = static_cast<RS_Text *>(entity);

  const auto direction = m_entity->getDrawingDirection();
  const bool mirror = (direction == RS_TextData::RightToLeft);

  QSignalBlocker textBlocker(ui->leText);
  ui->leText->setText(mirror ? mirrorByLine(m_entity->getText())
                             : m_entity->getText());

  toUIValue(m_entity->getHeight(), ui->leHeight);
  toUIValue(m_entity->getWidthRel(), ui->leWidthRel);
  toUIAngleDeg(m_entity->getAngle(), ui->leAngle);
  ui->leStyle->setText(m_entity->getStyle());

  QSignalBlocker alignBlocker(ui->cbAlignment);
  ui->cbAlignment->setCurrentIndex(
      alignmentIndexFromCode(m_entity->getAlignment()));

  QSignalBlocker bAuto(ui->rbAuto);
  QSignalBlocker bL(ui->rbLeftToRight);
  QSignalBlocker bR(ui->rbRightToLeft);
  switch (direction) {
  case RS_TextData::LeftToRight:
    ui->rbLeftToRight->setChecked(true);
    break;
  case RS_TextData::RightToLeft:
    ui->rbRightToLeft->setChecked(true);
    break;
  case RS_TextData::ByContent:
  default:
    ui->rbAuto->setChecked(true);
    break;
  }
  applyDirectionToEditor();
}

void LC_TextPropertiesEditingWidget::applyDirectionToEditor() {
  // For "Auto" we leave the line edit's direction at LayoutDirectionAuto so
  // Qt does first-strong-character detection on the input live — matching
  // RS_Text's resolveTextBaseDirection for ByContent.
  Qt::LayoutDirection direction = Qt::LayoutDirectionAuto;
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
  if (m_entity == nullptr)
    return;
  const QString widgetText = ui->leText->text();
  const bool mirror = ui->rbRightToLeft->isChecked();
  m_entity->setText(mirror ? mirrorByLine(widgetText) : widgetText);
}

void LC_TextPropertiesEditingWidget::onHeightEditingFinished() {
  if (m_entity == nullptr)
    return;
  m_entity->setHeight(toWCSValue(ui->leHeight, m_entity->getHeight()));
}

void LC_TextPropertiesEditingWidget::onWidthRelEditingFinished() {
  if (m_entity == nullptr)
    return;
  m_entity->setWidthRel(toWCSValue(ui->leWidthRel, m_entity->getWidthRel()));
}

void LC_TextPropertiesEditingWidget::onAngleEditingFinished() {
  if (m_entity == nullptr)
    return;
  m_entity->setAngle(toWCSAngle(ui->leAngle, m_entity->getAngle()));
}

void LC_TextPropertiesEditingWidget::onStyleEditingFinished() {
  if (m_entity == nullptr)
    return;
  m_entity->setStyle(ui->leStyle->text());
}

void LC_TextPropertiesEditingWidget::onAlignmentChanged(int index) {
  if (m_entity == nullptr)
    return;
  m_entity->setAlignment(alignmentCodeFromIndex(index));
}

void LC_TextPropertiesEditingWidget::onDirectionToggled(bool checked) {
  // Each user click fires twice across the radio group (one off, one on).
  // Skip the off-edge so we mirror the buffer at most once per actual flip.
  if (!checked)
    return;

  // The line edit's current layoutDirection reflects the previously-applied
  // direction (set by applyDirectionToEditor on the last toggle). If the
  // mirror requirement changed (RTL ↔ non-RTL), flip the visible buffer so
  // the same logical string reads in the new direction.
  const bool mirrorPrev = ui->leText->layoutDirection() == Qt::RightToLeft;
  const bool mirrorNow = ui->rbRightToLeft->isChecked();
  if (mirrorPrev != mirrorNow) {
    QSignalBlocker textBlocker(ui->leText);
    const int len = ui->leText->text().size();
    const int oldStart = ui->leText->selectionStart();
    const int oldLen = ui->leText->selectedText().size();
    const int oldCursor = ui->leText->cursorPosition();
    ui->leText->setText(mirrorByLine(ui->leText->text()));
    // Flip column for cursor and (if any) selection across the buffer.
    if (oldStart >= 0) {
      const int newStart = len - (oldStart + oldLen);
      ui->leText->setSelection(newStart, oldLen);
    } else {
      ui->leText->setCursorPosition(len - oldCursor);
    }
  }

  if (m_entity != nullptr) {
    if (ui->rbLeftToRight->isChecked()) {
      m_entity->setDrawingDirection(RS_TextData::LeftToRight);
    } else if (ui->rbRightToLeft->isChecked()) {
      m_entity->setDrawingDirection(RS_TextData::RightToLeft);
    } else {
      m_entity->setDrawingDirection(RS_TextData::ByContent);
    }
  }
  applyDirectionToEditor();
}
