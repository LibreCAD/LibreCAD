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

#ifndef LC_PROPERTYQSTRINGINVALIDVIEWBASE_H
#define LC_PROPERTYQSTRINGINVALIDVIEWBASE_H

#include "lc_property_qstring_lineedit_view.h"

class LC_PropertyQStringInvalidViewBase : public LC_PropertyQStringLineEditView {
    Q_DISABLE_COPY(LC_PropertyQStringInvalidViewBase)

protected:
    static const QByteArray ATTR_INVALID_COLOR;
    explicit LC_PropertyQStringInvalidViewBase(LC_PropertyQString* property);
    void doApplyAttributes(const LC_PropertyViewDescriptor& atts) override;
    void doDrawValue(LC_PropertyPaintContext& ctx, QStylePainter& painter, const QRect& rect) override;
    virtual bool doCheckPropertyValid() const = 0;

private:
    QColor m_invalidColor;
};

#endif
