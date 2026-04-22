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

#ifndef LC_PROPERTYVIEWMULTIPLE_H
#define LC_PROPERTYVIEWMULTIPLE_H

#include "lc_property_multi.h"
#include "lc_property_view_typed_compound.h"

class LC_PropertyViewMultiple : public LC_PropertyViewTypedCompound<LC_PropertyMulti, LC_PropertyView> {
    Q_DISABLE_COPY(LC_PropertyViewMultiple)

public:
    static const QByteArray VIEW_NAME;
    explicit LC_PropertyViewMultiple(LC_PropertyMulti& owner);
    ~LC_PropertyViewMultiple() override;
    bool isSplittable() const override;

protected:
    void doApplyAttributes(const LC_PropertyViewDescriptor& descriptor) override;
    void doBuildViewParts(LC_PropertyPaintContext& ctx, QList<LC_PropertyViewPart>& parts) override;

private:
    std::vector<std::unique_ptr<LC_PropertyView>> m_propertiesViewsList;
    void init() override;
    struct PropertyToEdit;
    static void onEditorDestroyed(const PropertyToEdit* data);
    static void onEditedPropertyDestroyed(PropertyToEdit* data);
};

#endif
