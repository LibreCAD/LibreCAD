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

#ifndef LC_PROPERTIESSHEETWIDGET_H
#define LC_PROPERTIESSHEETWIDGET_H

#include <QSplitter>
#include <QWidget>

#include "lc_inplace_property_editing_stopper.h"

enum LC_PropertyWidgetArea {
    PROPERTY_WIDGET_AREA_NONE    = 0x0000,
    PROPERTY_WIDGET_AREA_INFO    = 0x0001,
    PROPERTY_WIDGET_AREA_OTHER = 0x0002
};

class LC_PropertiesSheet;
class LC_PropertyContainer;
class LC_Property;
class QVBoxLayout;
class QLabel;

Q_DECLARE_FLAGS(LC_PropertyWidgetAreas, LC_PropertyWidgetArea)
Q_DECLARE_OPERATORS_FOR_FLAGS (LC_PropertyWidgetAreas)

class LC_PropertiesSheetPanel : public QWidget, public LC_InplacePropertyEditingStopper {
    Q_OBJECT Q_DISABLE_COPY(LC_PropertiesSheetPanel)

public:
    explicit LC_PropertiesSheetPanel(QWidget* parent = nullptr);
    ~LC_PropertiesSheetPanel() override;
    inline LC_PropertyWidgetAreas parts() const;
    void setParts(LC_PropertyWidgetAreas newAreas);
    const LC_PropertyContainer* propertyContainer() const;
    LC_PropertyContainer* propertyContainer();
    void setPropertyContainer(LC_PropertyContainer* newPropertiesContainer);
    inline LC_PropertiesSheet* propertiesSheet() const;
    bool stopInplaceEdit(bool deleteLater = true, bool restoreParentFocus = true) override;
    void createDescriptionLabel(QSplitter* splitter);
signals  :
    void propertySetChanged();

private:
    void updateParts();
    void setActiveProperty(const LC_Property* activeProperty) const;

    LC_PropertyWidgetAreas m_areas;

    QVBoxLayout* m_layout;
    QLabel* m_toolbar;
    LC_PropertiesSheet* m_propertiesSheet;
    QWidget* m_descriptionSplitter;
    QLabel* m_propertyInfoLabel;
};

LC_PropertyWidgetAreas LC_PropertiesSheetPanel::parts() const {
    return m_areas;
}

LC_PropertiesSheet* LC_PropertiesSheetPanel::propertiesSheet() const {
    return m_propertiesSheet;
}

#endif
