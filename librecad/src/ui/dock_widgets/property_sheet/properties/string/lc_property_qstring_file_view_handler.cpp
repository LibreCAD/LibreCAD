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

#include "lc_property_qstring_file_view_handler.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QToolButton>

#include "lc_property_qstring_file_view.h"

LC_PropertyQStringFileViewLineEditButtonHandler::LC_PropertyQStringFileViewLineEditButtonHandler(
    LC_PropertyViewEditable* view, LC_PropertyLineEditWithButton& editor) : LC_PropertyEditorButtonHandler(view, editor),
                                                                            m_dialog(new QFileDialog(&editor)) {
    m_dialogContainer = connectDialog(m_dialog);

    LC_PropertyQStringFileViewLineEditButtonHandler::doUpdateEditor();
    const auto sender = editor.getLineEdit();
    sender->installEventFilter(this);
    const auto toolButton = editor.getToolButton();
    connect(toolButton, &QToolButton::clicked, this, &LC_PropertyQStringFileViewLineEditButtonHandler::onToolButtonClicked);
    connect(sender, &QLineEdit::editingFinished, this, &LC_PropertyQStringFileViewLineEditButtonHandler::onEditingFinished);

    toolButton->setText("");
    toolButton->setToolTip(tr("Select file"));
    toolButton->setIcon(QIcon(":/icons/fileopen.lci"));
}

void LC_PropertyQStringFileViewLineEditButtonHandler::applyAttributes(const LC_PropertyViewDescriptor& attrs) {
    int option = 0;
    if (attrs.load(LC_PropertyQStringFileView::ATTR_ACCEPT_MODE, option)) {
        m_dialog->setAcceptMode(static_cast<QFileDialog::AcceptMode>(option));
    }

    QString str;

    if (attrs.load(LC_PropertyQStringFileView::ATTR_DEFAULT_SUFFIX, str)) {
        m_dialog->setDefaultSuffix(str);
    }
    if (attrs.load(LC_PropertyQStringFileView::ATTR_DEFAULT_DIR, str)) {
        m_defaultDirectory = str;
    }
    if (attrs.load(LC_PropertyQStringFileView::ATTR_FILE_MODE, option)) {
        m_dialog->setFileMode(static_cast<QFileDialog::FileMode>(option));
    }
    if (attrs.load(LC_PropertyQStringFileView::ATTR_OPTIONS, option)) {
        m_dialog->setOptions(QFileDialog::Options(QFlag(option)));
    }
    if (attrs.load(LC_PropertyQStringFileView::ATTR_VIEW_MODE, option)) {
        m_dialog->setViewMode(static_cast<QFileDialog::ViewMode>(option));
    }
    if (attrs.load(LC_PropertyQStringFileView::ATTR_FILENAME_FILTER, str)) {
        m_dialog->setNameFilter(str);
    }

    QStringList list;
    if (attrs.load(LC_PropertyQStringFileView::ATTR_FILENAME_FILTERS, list)) {
        m_dialog->setNameFilters(list);
    }
}

void LC_PropertyQStringFileViewLineEditButtonHandler::doOnToolButtonClick() {
    onToolButtonClicked(false);
}

void LC_PropertyQStringFileViewLineEditButtonHandler::doUpdateEditor() {
    const bool editable = isEditableByUser();
    const auto leB = getEditor();
    const auto lineEdit = leB->getLineEdit();
    lineEdit->setReadOnly(true);
    const auto toolButton = leB->getToolButton();
    toolButton->setEnabled(editable);

    const auto pathName = getPropertyValue();
    const auto path = QDir::toNativeSeparators(pathName);
    leB->setTextForProperty(getStateProperty(), path);

    if (!isMultiValue()) {
        const auto edit = lineEdit;
        edit->setPlaceholderText(LC_PropertyQString::getPlaceholderStr(edit->text(), false));
        edit->selectAll();
    }
}

void LC_PropertyQStringFileViewLineEditButtonHandler::onToolButtonClicked(bool) {
    const auto property = &this->getProperty();
    volatile bool destroyed = false;
    const auto connection = connect(property, &QObject::destroyed, [&destroyed]() mutable {
        destroyed = true;
    });
    m_reverted = true;
    const auto dialogContainer = m_dialogContainer;
    QString filePath = property->value();
    QString dirPath = m_defaultDirectory;

    if (!filePath.isEmpty()) {
        QFileInfo fileInfo(filePath);
        filePath = QDir(dirPath).filePath(filePath);
        fileInfo.setFile(filePath);
        dirPath = fileInfo.path();
    }

    m_dialog->setDirectory(dirPath);
    m_dialog->selectFile(filePath);

    if (m_dialog->exec() == QDialog::Accepted && !destroyed) {
        QStringList files = m_dialog->selectedFiles();
        if (files.size() == 1) {
            property->setValue(QDir::toNativeSeparators(files.first()), changeReasonDueToEdit());
        }
    }
    if (!destroyed) {
        disconnect(connection);
    }

    Q_UNUSED(dialogContainer);
}

void LC_PropertyQStringFileViewLineEditButtonHandler::onEditingFinished() {
    if (doCheckMayApply()) {
        const auto newValue = getEditor()->getLineEdit()->text();
        if (!newValue.isEmpty()) {
            const QFileInfo fileInfo(newValue);
            if (fileInfo.isFile()) {
                getProperty().setValue(newValue, changeReasonDueToEdit());
            }
            else {
                // ?
            }
        }
    }
    doApplyReset();
}
