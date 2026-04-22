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

#ifndef LC_PROPERTYDOUBLECOMBOBOXEDITVIEW_H
#define LC_PROPERTYDOUBLECOMBOBOXEDITVIEW_H

#include "lc_property_double.h"
#include "lc_property_view_typed.h"

class LC_PropertyDoubleLineEditView: public LC_PropertyViewTyped<LC_PropertyDouble> {
    Q_OBJECT Q_DISABLE_COPY(LC_PropertyDoubleLineEditView)
public:
    static const QByteArray VIEW_NAME;
    static const QByteArray ATTR_POSITIVIE_VALUES_ONLY;
    static const QByteArray ATTR_MAX_LENGTH;
    static const QByteArray ATTR_ZERO_PLACEHOLDER;

    explicit LC_PropertyDoubleLineEditView(LC_PropertyDouble& owner)
        : LC_PropertyViewTyped<LC_PropertyDouble>(owner) {
    }
    bool getPropertyValueFromEditString(const QString& text, double& val);
protected:
    QString m_cachedStrValue;
    bool m_positiveOnly = true;
    QString m_zeroPlaceholder;
    int m_maxLength = 6;

    bool doAcceptKeyPressedForInplaceEdit(QKeyEvent* keyEvent) const override;
    QWidget* doCreateValueEditor(QWidget* parent, const QRect& rect, const EditActivationContext* ctx) override;
    void doApplyAttributes(const LC_PropertyViewDescriptor& info) override;
    bool doPropertyValueToStrForView(QString& strValue) override;
    bool doPropertyValueToStrForEdit(QString& strValue) const override;

    friend class LC_PropertyDoubleLineEditViewHandler;
};

#endif
