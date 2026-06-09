/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2025 LibreCAD.org
 * Copyright (C) 2025 sand1024
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

#include "lc_property_editor_utils.h"

#include <QCompleter>
#include <QKeyEvent>
#include <QWidget>

#include "lc_property_view_editable.h"

bool LC_PropertyEditorUtils::isAcceptableKeyEventForLineEdit(const QKeyEvent* keyEvent) {
    if (keyEvent->type() != QEvent::KeyPress) {
        return false;
    }
    // any printable key press is acceptable
    QString text = keyEvent->text();
    return text.size() == 1 && text[0].isPrint();
}

void LC_PropertyEditorUtils::initializeLineEditor(QLineEdit* lineEdit, const LC_PropertyViewEditable::EditActivationContext* ctx) {
    if (lineEdit == nullptr || ctx == nullptr) {
        return;
    }

    if (!lineEdit->isReadOnly() && (ctx->isActivatedByKeyPress())) {
        const auto* keyEvent = static_cast<QKeyEvent*>(ctx->activationEvent);
        if (isAcceptableKeyEventForLineEdit(keyEvent)) {
            lineEdit->setText(keyEvent->text());
            const auto completer = lineEdit->completer();
            if (completer != nullptr) {
                completer->setCompletionPrefix(lineEdit->text());
            }
        }
    }
    else {
        lineEdit->selectAll();
    }
}

void LC_PropertyEditorUtils::initializeNumberEditor(QWidget* numEdit, const LC_PropertyViewEditable::EditActivationContext* ctx,
                                                    const NumberType type) {
    if (nullptr != ctx && ctx->isActivatedByKeyPress()) {
        auto keyEvent = static_cast<QKeyEvent*>(ctx->activationEvent);

        if (isAcceptableKeyEvenForNumberEdit(keyEvent, type)) {
            keyEvent = new QKeyEvent(keyEvent->type(), keyEvent->key(), keyEvent->modifiers(), keyEvent->text());
            QCoreApplication::postEvent(numEdit, keyEvent);
        }
    }
}

bool LC_PropertyEditorUtils::isAcceptableKeyEvenForNumberEdit(const QKeyEvent* keyEvent, const NumberType type) {
    if (keyEvent->type() != QEvent::KeyPress) {
        return false;
    }
    // any numeric key press is acceptable
    const QString text = keyEvent->text();

    if (text.size() == 1) {
        const QChar c = text.at(0);
        const QLocale locale;

        switch (type) {
            case NUM_TYPE_FLOAT: {
                if (c == QLatin1Char('.') || c == locale.decimalPoint()) {
                    return true;
                }

                // fall through
                [[fallthrough]];
            }
            case NUM_TYPE_SIGNED_INT: {
                if (c == QLatin1Char('-') || c == QLatin1Char('+') || c == locale.negativeSign() || c == locale.positiveSign()) {
                    return true;
                }
                // fall through
                [[fallthrough]];
            }
            case NUM_TYPE_UNSIGNED_INT: {
                if (c.isDigit()) {
                    return true;
                }
                break;
            }
        }
    }
    return false;
}
