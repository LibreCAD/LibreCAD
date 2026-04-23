/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2026 LibreCAD.org
 * Copyright (C) 2026 sand1024
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
#include "lc_property_qstring_lineedit_view.h"

#include <QToolButton>

#include "lc_multilinetexteditdialog.h"
#include "lc_property_editor_button_handler.h"
#include "lc_property_editor_utils.h"
#include "lc_property_lineedit_with_button.h"
#include "lc_property_view_utils.h"

namespace Ui {
    class LC_MultilineTextEditDialog;
}

namespace{
    QString toSingleLine(const QString& str) {
        const int n = str.indexOf('\n');
        const int r = str.indexOf('\r');
        const int len = n < 0 ? r : (r < 0 ? n : qMin(n, r));
        return QString(str.data(), len);
    }
}

const QByteArray LC_PropertyQStringLineEditView::VIEW_NAME = LC_PropertyViewUtils::getViewNameLineEdit();
const QByteArray LC_PropertyQStringLineEditView::ATTR_MULTILINE_EDIT = QByteArrayLiteral("multilineEdit");
const QByteArray LC_PropertyQStringLineEditView::ATTR_MAX_LENGTH = QByteArrayLiteral("maxLength");
const QByteArray LC_PropertyQStringLineEditView::ATTR_PLACEHOLDER = QByteArrayLiteral("placeholder");

class LC_PropertyQStringMultilineEditBttnHandler : public LC_PropertyEditorButtonHandler<LC_PropertyQString, LC_PropertyLineEditWithButton> {
public:
    LC_PropertyQStringMultilineEditBttnHandler(LC_PropertyViewEditable* view, LC_PropertyLineEditWithButton& editor,
                                               const QString& placeholder)
        : LC_PropertyEditorButtonHandler(view, editor), m_dialog(new LC_MultilineTextEditDialog(&editor)), m_multiline(false),
          m_placeholder(placeholder) {
        m_dialogContainer = connectDialog(m_dialog);
        LC_PropertyQStringMultilineEditBttnHandler::doUpdateEditor();

        const auto lineEdit = editor.getLineEdit();
        lineEdit->installEventFilter(this);

        connect(editor.getToolButton(), &QToolButton::clicked, this, &LC_PropertyQStringMultilineEditBttnHandler::onToolButtonClicked);

        connect(lineEdit, &QLineEdit::editingFinished, this, &LC_PropertyQStringMultilineEditBttnHandler::onEditingFinished);
    }

protected:
    void doUpdateEditor() override {
        const auto le = getEditor()->getLineEdit();
        le->setReadOnly(!isEditableByUser());

        if (isMultiValue()) {
            le->clear();
            le->setPlaceholderText(LC_Property::getMultiValuePlaceholder());
        }
        else {
            const auto text = getPropertyValue();

            if (LC_PropertyQString::isMultilineText(text)) {
                m_multiline = true;
                le->setText(QString());
            }
            else {
                m_multiline = false;
                le->setText(text);
            }

            if (text.isEmpty() && !m_placeholder.isEmpty()) {
                le->setPlaceholderText(m_placeholder);
            }
            else {
                le->setPlaceholderText(LC_PropertyQString::getPlaceholderStr(text, true));
            }
            le->selectAll();
        }
    }

     void doRollbackValue() override {
        m_reverted = true;
    }

    void doOnToolButtonClick() override {
        onToolButtonClicked(false);
    }

private:
    void onEditingFinished() {
        if (doCheckMayApply()) {
            const auto text = getEditor()->getLineEdit()->text();

            if (!m_multiline || !text.isEmpty()) {
                getProperty().setValue(text, changeReasonDueToEdit());
                doUpdateEditor();
            }
        }

        doApplyReset();
    }

    void onToolButtonClicked(bool) {
        auto text = getEditor()->getLineEdit()->text();
        const auto property = &this->getProperty();

        if (text.isEmpty() && m_multiline) {
            text = property->value();
        }

        m_reverted = true;
        const bool readonly = !isEditableByUser();
        const auto dialogContainer = this->m_dialogContainer;
        m_dialog->setReadOnly(readonly);

        if (readonly) {
            m_dialog->setWindowTitle(LC_PropertyQString::getReadOnlyPropertyTitleFormat().arg(property->getDisplayName()));
        }
        else {
            m_dialog->setWindowTitle(property->getDisplayName());
        }

        m_dialog->setText(text);
        m_dialog->show();
        m_dialog->raise();
        volatile bool destroyed = false;
        const auto connection = connect(this, &QObject::destroyed, [&destroyed]() mutable {
            destroyed = true;
        });

        if (m_dialog->exec() == QDialog::Accepted && !destroyed) {
            property->setValue(m_dialog->getText(), changeReasonDueToEdit());
        }
        if (!destroyed) {
            disconnect(connection);
            doUpdateEditor();
        }

        Q_UNUSED(dialogContainer);
    }

    LC_MultilineTextEditDialog* m_dialog;
    PtrDialogContainer m_dialogContainer;
    bool m_multiline;
    QString m_placeholder;
};

class LC_PropertyQStringLineEditViewHandler : public LC_PropertyEditorHandler<LC_PropertyQString, QLineEdit> {
public:
    LC_PropertyQStringLineEditViewHandler(LC_PropertyViewEditable* view, QLineEdit& editor, const QString& placeholder)
        : LC_PropertyEditorHandler(view, editor), m_placeholder(placeholder) {
        LC_PropertyQStringLineEditViewHandler::doUpdateEditor();

        editor.installEventFilter(this);
        connect(&editor, &QLineEdit::editingFinished, this, &LC_PropertyQStringLineEditViewHandler::updateValue);
    }

protected:
    void doUpdateEditor() override {
        const auto le = getEditor();
        le->setReadOnly(!isEditableByUser());
        if (isMultiValue()) {
            le->clear();
            le->setPlaceholderText(LC_Property::getMultiValuePlaceholder());
        }
        else {
            const auto text = getPropertyValue();
            le->setText(text);
            const bool textIsEmpty = text.isEmpty() && !m_placeholder.isEmpty();
            const auto placeholderText = textIsEmpty ? m_placeholder : LC_PropertyQString::getPlaceholderStr(text, false);
            le->setPlaceholderText(placeholderText);
            le->selectAll();
        }
    }

    void updateValue() {
        if (doCheckMayApply()) {
            const auto newValue = toSingleLine(getEditor()->text());
            getProperty().setValue(newValue, changeReasonDueToEdit());
        }
        doApplyReset();
    }

private:
    QString m_placeholder;
};

LC_PropertyQStringLineEditView::LC_PropertyQStringLineEditView(LC_PropertyQString* property)
    : LC_PropertyViewTyped(property), m_maxLength(0x1000000), m_multiline(true) {
}

void LC_PropertyQStringLineEditView::doApplyAttributes(const LC_PropertyViewDescriptor& info) {
    info.load(ATTR_MULTILINE_EDIT, m_multiline);
    info.load(ATTR_PLACEHOLDER, m_placeholder);
    info.load(ATTR_MAX_LENGTH, m_maxLength);
}

bool LC_PropertyQStringLineEditView::doAcceptKeyPressedForInplaceEdit(QKeyEvent* keyEvent) const {
    if (LC_PropertyViewTyped::doAcceptKeyPressedForInplaceEdit(keyEvent)) {
        return true;
    }
    // accept any printable key
    return LC_PropertyEditorUtils::isAcceptableKeyEventForLineEdit(keyEvent);
}

QWidget* LC_PropertyQStringLineEditView::doCreateValueEditor(QWidget* parent, const QRect& rect, const EditActivationContext* ctx) {
    if (isEditableByUser()) {
        if (m_multiline) {
            const auto leB = new LC_PropertyLineEditWithButton(parent);
            leB->setGeometry(rect);
            const auto lineEdit = leB->getLineEdit();
            lineEdit->setMaxLength(m_maxLength);
            lineEdit->setPlaceholderText(m_placeholder);
            new LC_PropertyQStringMultilineEditBttnHandler(this, *leB, m_placeholder);
            LC_PropertyEditorUtils::initializeLineEditor(lineEdit, ctx);
            return leB;
        }
        const auto le = new QLineEdit(parent);
        le->setGeometry(rect);
        le->setMaxLength(m_maxLength);
        le->setPlaceholderText(m_placeholder);
        new LC_PropertyQStringLineEditViewHandler(this, *le, m_placeholder);
        LC_PropertyEditorUtils::initializeLineEditor(le, ctx);
        return le;
    }
    return nullptr;
}

bool LC_PropertyQStringLineEditView::doPropertyValueToStr(QString& strValue) const {
    strValue = propertyValue();
    auto placeholder = isShowPlaceholderForEmptyText(strValue)
                           ? m_placeholder
                           : LC_PropertyQString::getPlaceholderStr(strValue, m_multiline);

    if (!placeholder.isEmpty()) {
        strValue.swap(placeholder);
    }
    return true;
}

bool LC_PropertyQStringLineEditView::isPlaceholderColor() const {
    const auto text = propertyValue();
    if (isShowPlaceholderForEmptyText(text)) {
        return true;
    }
    const auto placeholderStr = LC_PropertyQString::getPlaceholderStr(text, m_multiline);
    return !placeholderStr.isEmpty();
}
