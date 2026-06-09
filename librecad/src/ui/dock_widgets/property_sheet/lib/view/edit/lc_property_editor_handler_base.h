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

#ifndef LC_PROPERTYEDITORHANDLERBASE_H
#define LC_PROPERTYEDITORHANDLERBASE_H

#include "lc_property_view.h"
#include "lc_property_view_editable.h"

class LC_PropertyEditorHandlerBase : public QObject {
public:
    void cleanup();
    ~LC_PropertyEditorHandlerBase() override;
    virtual void doUpdateEditor() = 0;

protected:
    LC_PropertyEditorHandlerBase(LC_PropertyViewEditable* view, QWidget& editor);

    LC_PropertyView* view() const {
        return m_view;
    }

    LC_PropertyChangeReason changeReasonDueToEdit() const {
        return m_view->changeReasonDueToEdit();
    }

    bool isEditableByUser() const;
    bool isMultiValue() const;

    LC_Property* getBaseProperty() const;
    LC_Property* getStateProperty() const;

    QWidget* editorBase() const {
        return m_editor;
    }

    virtual void doRollbackValue();
    virtual bool doCheckMayApply() const;
    virtual void doApplyReset();
    bool eventFilter(QObject* obj, QEvent* event) override;
    void stopInplaceEdit() const;

    struct DialogRAIIGuard {
        QDialog* dialog;

        explicit DialogRAIIGuard(QDialog* dialog);
        ~DialogRAIIGuard();
    };

    using PtrDialogContainer = std::shared_ptr<DialogRAIIGuard>;

    static PtrDialogContainer connectDialog(QDialog* dialog);
    static void connectDialog(const PtrDialogContainer& containerPtr);

    bool m_reverted : 1;
    bool m_returned : 1;

private:
    LC_PropertyViewEditable* m_view{nullptr};
    QWidget* m_editor{nullptr};

    void onPropertyDestroyed();
    void onPropertyDidChange(LC_PropertyChangeReason reason);
};
#endif
