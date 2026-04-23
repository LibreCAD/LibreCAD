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

#ifndef LC_PROPERTYQSTRINGLINEEDITVIEW_H
#define LC_PROPERTYQSTRINGLINEEDITVIEW_H

#include "lc_property_qstring.h"
#include "lc_property_view_typed.h"

class LC_PropertyQStringLineEditView : public LC_PropertyViewTyped<LC_PropertyQString> {
    Q_DISABLE_COPY(LC_PropertyQStringLineEditView)

public:
    static const QByteArray VIEW_NAME;
    static const QByteArray ATTR_MULTILINE_EDIT;
    static const QByteArray ATTR_MAX_LENGTH;
    static const QByteArray ATTR_PLACEHOLDER;

    explicit LC_PropertyQStringLineEditView(LC_PropertyQString* property);

protected:
    using Inherited = LC_PropertyViewTyped;
    void doApplyAttributes(const LC_PropertyViewDescriptor& info) override;
    bool doAcceptKeyPressedForInplaceEdit(QKeyEvent* keyEvent) const override;
    QWidget* doCreateValueEditor(QWidget* parent, const QRect& rect, const EditActivationContext* ctx) override;
    bool doPropertyValueToStr(QString& strValue) const override;
    bool isPlaceholderColor() const override;

    bool isShowPlaceholderForEmptyText(const QString& strValue) const {
        return strValue.isEmpty() && !m_placeholder.isEmpty();
    }

    int m_maxLength;
    bool m_multiline;
    QString m_placeholder;
};

#endif
