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

#ifndef LC_PROPERTYVIEWEDITABLE_H
#define LC_PROPERTYVIEWEDITABLE_H

#include <QEvent>
#include <QLineEdit>

#include "lc_property_view_with_value.h"

class LC_PropertyEditorHandlerBase;
struct LC_InplacePropertyEditingStopper;

class LC_PropertyViewEditable : public LC_PropertyViewWithValue {
    Q_DISABLE_COPY(LC_PropertyViewEditable)

public:
    struct EditActivationContext {
        QEvent* activationEvent{nullptr};

        explicit EditActivationContext(QEvent* event) : activationEvent{event} {
        }

        bool isActivatedByKeyPress() const {
            return activationEvent->type() == QEvent::KeyPress;
        }
    };

    ~LC_PropertyViewEditable() override;

    QWidget* createValueEditor(QWidget* parent, const QRect& rect, const EditActivationContext* ctx) {
        return doCreateValueEditor(parent, rect, ctx);
    }

    bool acceptKeyPressedForInplaceEdit(QKeyEvent* keyEvent) const {
        return doAcceptKeyPressedForInplaceEdit(keyEvent);
    }

    bool propertyValueToStr(QString& strValue) const;
    void stopInplaceEditing(bool deleteLater = true, bool restoreParentFocus = true) const;

protected:
    explicit LC_PropertyViewEditable(LC_Property* property);
    bool doBuildPartValue(LC_PropertyPaintContext& ctx, LC_PropertyViewPart& partValue) override;
    virtual bool doPropertyValueToStr(QString& strValue) const;
    virtual bool doPropertyValueToStrForView(QString& strValue);
    virtual bool doPropertyValueToStrForEdit(QString& strValue) const;
    virtual bool doGetToolTip(QString& strValue);
    virtual bool isPlaceholderColor() const;
    virtual void doDrawValue(LC_PropertyPaintContext& ctx, QStylePainter& painter, const QRect& rect);
    virtual bool doAcceptKeyPressedForInplaceEdit(QKeyEvent* keyEvent) const;
    virtual QWidget* doCreateValueEditor(QWidget* parent, const QRect& rect, const EditActivationContext* ctx) = 0;
    QLineEdit* createValueEditorLineEdit(QWidget* parent, const QRect& rect, bool readOnly, const EditActivationContext* ctx) const;

    bool isNormalPainterState(const QStylePainter& painter) const;

    bool isInplaceEditAllowed(const EditActivationContext* ctx) const {
        return ctx != nullptr && getStateProperty()->isEditableByUser();
    }

    // void showPopupForInplace(EditActivationContext* inplaceInfo, QComboBox* combo); // todo - either create method or remove  { isInplaceEditAllowed(inplaceInfo) ? comboBox->showPopup() :; }
private:
    friend class LC_PropertyEditorHandlerBase;
    LC_PropertyEditorHandlerBase* m_editorHandler{nullptr};
    LC_InplacePropertyEditingStopper* m_inplaceEditStopper{nullptr};
};

#endif
