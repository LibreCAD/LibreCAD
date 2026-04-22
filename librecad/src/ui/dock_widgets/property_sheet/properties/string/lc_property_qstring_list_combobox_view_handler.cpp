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

#include "lc_property_qstring_list_combobox_view_handler.h"

#include <QLineEdit>

#include "lc_property_combobox.h"
#include "lc_property_qstring_list_combobox_view.h"

namespace{
// fixme - to namespace or class
 QString toSingleLine(const QString& str) {
    const int n = str.indexOf('\n');
    const int r = str.indexOf('\r');
    const int len = n < 0 ? r : (r < 0 ? n : qMin(n, r));
    return QString(str.data(), len);
}
}

void LC_PropertyQStringListComboBoxViewHandler::connectCombobox(LC_PropertyComboBox& editor) {
    connect(&editor, &QComboBox::currentTextChanged, [this](const QString &val)-> void {
        getEditor()->disablePaint(true);
        LC_PropertyQStringListComboBoxViewHandler::onValueChanged(val);
    });
}

LC_PropertyQStringListComboBoxViewHandler::LC_PropertyQStringListComboBoxViewHandler(
    LC_PropertyViewEditable* view, LC_PropertyComboBox& editor, const LC_PropertyViewDescriptor& descriptor)
    : LC_PropertyEditorHandlerValueTyped(view, editor) {
    bool editable = false;
    descriptor.load(LC_PropertyQStringListComboBoxView::ATTR_EDITABLE, editable);

    editor.clear();

    if (descriptor.attributes.contains(LC_PropertyQStringListComboBoxView::ATTR_ITEMS_ICON_NAMES)) {
        QStringList iconNames;
        descriptor.load(LC_PropertyQStringListComboBoxView::ATTR_ITEMS_ICON_NAMES, iconNames);

        QStringList itemData;
        descriptor.load(LC_PropertyQStringListComboBoxView::ATTR_ITEMS_DATA, itemData);

        QStringList items;
        descriptor.load(LC_PropertyQStringListComboBoxView::ATTR_ITEMS, items);

        const qsizetype count = iconNames.count();
        for (int i = 0; i < count; i++) {
            const QString& iconName = iconNames.at(i);
            const QString& displayName = items.at(i);
            const QString& data = itemData.at(i);
            editor.addItem(QIcon(iconName), displayName, data);
        }
    }
    else if (descriptor.attributes.contains(LC_PropertyQStringListComboBoxView::ATTR_ITEMS)) {
        QStringList items;
        descriptor.load(LC_PropertyQStringListComboBoxView::ATTR_ITEMS, items);
        editor.addItems(items);
    }
    editor.setEditable(editable);
    editor.setCompleter(nullptr);

    if (editable) {
        editor.installEventFilter(this);
    }
}

void LC_PropertyQStringListComboBoxViewHandler::doUpdateEditor() {
    m_updating++;

    const auto cb = getEditor();
    cb->setEnabled(isEditableByUser());

    const auto lineEdit = cb->lineEdit();
    if (isMultiValue()) {
        cb->clearEditText();
        if (lineEdit != nullptr) {
            lineEdit->setPlaceholderText(LC_Property::getMultiValuePlaceholder());
        }
    }
    else {
        const auto text = getPropertyValue();
        cb->setCurrentText(text);
        if (cb->currentText() != getPropertyValue()) {
            cb->setCurrentIndex(-1);
        }
        if (lineEdit != nullptr) {
            lineEdit->setPlaceholderText(QString());
        }
    }
    if (lineEdit != nullptr) {
        lineEdit->selectAll();
    }
    m_updating--;
}

void LC_PropertyQStringListComboBoxViewHandler::doUpdatePropertyValue() {
    if (!getEditor()->isEditable() || doCheckMayApply()) {
        getProperty().setValue(toSingleLine(m_newValue), changeReasonDueToEdit());
    }
    doApplyReset();
}
