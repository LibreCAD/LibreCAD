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

#include <QTextBlock>
#include <QTextBlockFormat>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextOption>

#include "lc_textbidi.h"
#include "rs_mtext.h"
#include "ui_lc_mtextpropertieseditingwidget.h"

using lc::textbidi::mirrorByLine;

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

    const bool ltr =
        m_entity->getDrawingDirection() != RS_MTextData::RightToLeft;

    // Block textChanged so populating the editor doesn't loop back into the
    // entity. We do not need to block the QLineEdits — editingFinished only
    // fires on user interaction.
    QSignalBlocker textBlocker(ui->teText);
    ui->teText->setPlainText(ltr ? m_entity->getText()
                                 : mirrorByLine(m_entity->getText()));

    toUIValue(m_entity->getHeight(), ui->leHeight);
    toUIValue(m_entity->getWidth(), ui->leWidth);
    toUIAngleDeg(m_entity->getAngle(), ui->leAngle);
    toUIValue(m_entity->getLineSpacingFactor(), ui->leLineSpacing);
    ui->leStyle->setText(m_entity->getStyle());

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

    QTextDocument *doc = ui->teText->document();
    if (doc == nullptr) return;

    QTextOption option = doc->defaultTextOption();
    option.setTextDirection(direction);
    doc->setDefaultTextOption(option);

    // setDefaultTextOption only governs future relayout, so existing blocks
    // keep their old direction until something else triggers them. Stamp the
    // direction onto every block format so already-typed text flips now.
    QSignalBlocker textBlocker(ui->teText);
    QTextCursor cursor(doc);
    cursor.beginEditBlock();
    cursor.movePosition(QTextCursor::Start);
    do {
        QTextBlockFormat fmt = cursor.blockFormat();
        fmt.setLayoutDirection(direction);
        cursor.setBlockFormat(fmt);
    } while (cursor.movePosition(QTextCursor::NextBlock));
    cursor.endEditBlock();

    ui->teText->update();
}

void LC_MTextPropertiesEditingWidget::onTextChanged() {
    if (m_entity == nullptr) return;
    const QString widgetText = ui->teText->toPlainText();
    const bool ltr = ui->rbLeftToRight->isChecked();
    m_entity->setText(ltr ? widgetText : mirrorByLine(widgetText));
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

void LC_MTextPropertiesEditingWidget::onDirectionToggled(bool checked) {
    // Each user click fires twice (one button goes off, the other comes on).
    // Skip the off-edge so we mirror the buffer exactly once per actual flip.
    if (!checked) return;

    const bool ltr = ui->rbLeftToRight->isChecked();

    // Capture cursor (anchor + position) so we can flip the column index
    // within its block after the mirror. Block structure is preserved by
    // per-line mirroring; only the column flips.
    auto capture = [](const QTextDocument *doc, int pos) {
        const QTextBlock block = doc->findBlock(pos);
        const int blockNum = block.blockNumber();
        int blockLen = block.length();
        if (block.next().isValid()) --blockLen;
        return std::tuple<int, int, int>{blockNum, pos - block.position(),
                                          blockLen};
    };
    const QTextCursor oldCursor = ui->teText->textCursor();
    const auto anchorInfo = capture(ui->teText->document(), oldCursor.anchor());
    const auto posInfo = capture(ui->teText->document(), oldCursor.position());

    // Flip the displayed buffer so the same logical text now reads in the
    // newly selected direction. The entity's text is unchanged — only the
    // widget's visual layout flips.
    {
        QSignalBlocker textBlocker(ui->teText);
        const QString current = ui->teText->toPlainText();
        ui->teText->setPlainText(mirrorByLine(current));
    }

    auto restore = [](const QTextDocument *doc,
                      const std::tuple<int, int, int> &info) -> int {
        const auto [blockNum, col, blockLen] = info;
        const QTextBlock block = doc->findBlockByNumber(blockNum);
        if (!block.isValid()) return 0;
        return block.position() + (blockLen - col);
    };
    {
        QTextCursor c = ui->teText->textCursor();
        c.setPosition(restore(ui->teText->document(), anchorInfo));
        c.setPosition(restore(ui->teText->document(), posInfo),
                      QTextCursor::KeepAnchor);
        ui->teText->setTextCursor(c);
    }

    if (m_entity != nullptr) {
        m_entity->setDrawingDirection(
            ltr ? RS_MTextData::LeftToRight : RS_MTextData::RightToLeft);
    }
    applyDirectionToEditor();
}
