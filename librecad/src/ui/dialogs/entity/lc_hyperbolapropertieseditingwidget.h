// File: lc_hyperbolapropertieseditingwidget.h

/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2025 LibreCAD.org
 * Copyright (C) 2025 Dongxu Li (github.com/dxli)
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

#ifndef LC_HYPERBOLAPROPERTIESEDITINGWIDGET_H
#define LC_HYPERBOLAPROPERTIESEDITINGWIDGET_H

#include "lc_entitypropertieseditorwidget.h"

class LC_Hyperbola;

namespace Ui {
    class LC_HyperbolaPropertiesEditingWidget;
}

/**
 * Properties editor widget for hyperbola entities.
 * Supports both standard parameter editing and alternative foci + point definition.
 */
class LC_HyperbolaPropertiesEditingWidget : public LC_EntityPropertiesEditorWidget
{
    Q_OBJECT

public:
    explicit LC_HyperbolaPropertiesEditingWidget(QWidget *parent = nullptr);
    ~LC_HyperbolaPropertiesEditingWidget() override;

    void setEntity(RS_Entity* entity) override;
    void updateEntityData() override;

private:
    Ui::LC_HyperbolaPropertiesEditingWidget *ui = nullptr;
    LC_Hyperbola* m_entity = nullptr;

    void updateUI();
};

#endif // LC_HYPERBOLAPROPERTIESEDITINGWIDGET_H
