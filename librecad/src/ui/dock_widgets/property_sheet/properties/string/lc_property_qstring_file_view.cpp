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

#include "lc_property_qstring_file_view.h"

#include <QDir>
#include <QFileDialog>

#include "lc_property_editor_utils.h"
#include "lc_property_lineedit_with_button.h"
#include "lc_property_qstring_file_view_handler.h"

const QByteArray LC_PropertyQStringFileView::VIEW_NAME = QByteArrayLiteral("File");
const QByteArray LC_PropertyQStringFileView::ATTR_DEFAULT_SUFFIX = QByteArrayLiteral("defaultSuffix");
const QByteArray LC_PropertyQStringFileView::ATTR_DEFAULT_DIR = QByteArrayLiteral("defaultDirectory");
const QByteArray LC_PropertyQStringFileView::ATTR_VIEW_MODE = QByteArrayLiteral("viewMode");
const QByteArray LC_PropertyQStringFileView::ATTR_FILE_MODE = QByteArrayLiteral("fileMode");
const QByteArray LC_PropertyQStringFileView::ATTR_FILENAME_FILTER = QByteArrayLiteral("nameFilter");
const QByteArray LC_PropertyQStringFileView::ATTR_FILENAME_FILTERS = QByteArrayLiteral("nameFilters");
const QByteArray LC_PropertyQStringFileView::ATTR_SHOW_RELATIVE_PATH = QByteArrayLiteral("showRelativePath");
const QByteArray LC_PropertyQStringFileView::ATTR_ACCEPT_MODE = QByteArrayLiteral("acceptMode");
const QByteArray LC_PropertyQStringFileView::ATTR_OPTIONS = QByteArrayLiteral("options");

LC_PropertyQStringFileView::LC_PropertyQStringFileView(LC_PropertyQString* property)
    : LC_PropertyQStringInvalidViewBase(property) {
}

void LC_PropertyQStringFileView::doApplyAttributes(const LC_PropertyViewDescriptor& info) {
    LC_PropertyQStringInvalidViewBase::doApplyAttributes(info);
    m_editorAttributes = info;
}

bool LC_PropertyQStringFileView::doPropertyValueToStr(QString& strValue) const {
    if (!propertyValue().isEmpty()) {
        const auto pathName = isShowRelativePath() ? getRelativeFilePath() : getAbsoluteFilePath();
        strValue = QDir::toNativeSeparators(pathName);
        return true;
    }
    return LC_PropertyQStringInvalidViewBase::doPropertyValueToStr(strValue);
}

bool LC_PropertyQStringFileView::doGetToolTip(QString& strValue) {
    strValue = QDir::toNativeSeparators(getAbsoluteFilePath());
    return true;
}

QWidget* LC_PropertyQStringFileView::doCreateValueEditor(QWidget* parent, const QRect& rect, const EditActivationContext* ctx) {
    auto* editor = new LC_PropertyLineEditWithButton(parent);
    editor->setGeometry(rect);
    const auto handler = new LC_PropertyQStringFileViewLineEditButtonHandler(this, *editor);
    handler->applyAttributes(m_editorAttributes);
    LC_PropertyEditorUtils::initializeLineEditor(editor->getLineEdit(), ctx);
    return editor;
}

// todo - cache property valid status!!!??? its called on draw() But when cached value should be cleared?
bool LC_PropertyQStringFileView::doCheckPropertyValid() const {
    const auto filePath = getAbsoluteFilePath();

    if (filePath.isEmpty()) {
        return true;
    }

    const auto mode = m_editorAttributes.attr(ATTR_FILE_MODE, QFileDialog::AnyFile);
    const auto fileMode = static_cast<QFileDialog::FileMode>(mode);

    switch (fileMode) {
        case QFileDialog::Directory: {
            const bool valid = QFileInfo(filePath).isDir();
            return valid;
        }
        case QFileDialog::ExistingFile:
        case QFileDialog::ExistingFiles: {
            const bool valid = QFileInfo(filePath).isFile();
            return valid;
        }
        case QFileDialog::AnyFile: {
            return true;
        }
    }
    return false;
}

QString LC_PropertyQStringFileView::getDefaultDirectory() const {
    return m_editorAttributes.attr(ATTR_DEFAULT_DIR, QString());
}

QString LC_PropertyQStringFileView::getAbsoluteFilePath() const {
    QString result = propertyValue();
    if (!result.isEmpty() && QDir::isRelativePath(result)) {
        const auto defaultDir = getDefaultDirectory();
        if (!defaultDir.isEmpty()) {
            result = QDir(defaultDir).filePath(result);
        }
    }
    return result;
}

QString LC_PropertyQStringFileView::getRelativeFilePath() const {
    QString result = propertyValue();
    if (!result.isEmpty() && QDir::isAbsolutePath(result)) {
        const auto defaultDir = getDefaultDirectory();
        if (!defaultDir.isEmpty()) {
            return QDir(defaultDir).relativeFilePath(result);
        }
    }
    return result;
}

bool LC_PropertyQStringFileView::isShowRelativePath() const {
    if (getDefaultDirectory().isEmpty()) {
        return false;
    }
    return m_editorAttributes.attr(ATTR_SHOW_RELATIVE_PATH, false);
}
