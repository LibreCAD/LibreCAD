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

#ifndef LC_PROPERTY_QSTRING_FILE_LINE_EDIT_BUTTON_VIEW_HANDLER_H
#define LC_PROPERTY_QSTRING_FILE_LINE_EDIT_BUTTON_VIEW_HANDLER_H

#include "lc_property_editor_button_handler.h"
#include "lc_property_lineedit_with_button.h"
#include "lc_property_qstring.h"

class QFileDialog;

class LC_PropertyQStringFileViewLineEditButtonHandler : public LC_PropertyEditorButtonHandler<LC_PropertyQString, LC_PropertyLineEditWithButton> {
    Q_OBJECT
public:
    LC_PropertyQStringFileViewLineEditButtonHandler(LC_PropertyViewEditable* view, LC_PropertyLineEditWithButton& editor);
    void applyAttributes(const LC_PropertyViewDescriptor& attrs);
    void doOnToolButtonClick() override;
    void doUpdateEditor() override;
private:
    void onToolButtonClicked(bool);
    void onEditingFinished();

    QFileDialog* m_dialog;
    PtrDialogContainer m_dialogContainer;
    QString m_defaultDirectory;
};

#endif
