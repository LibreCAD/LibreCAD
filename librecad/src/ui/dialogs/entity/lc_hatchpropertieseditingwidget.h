/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2026 LibreCAD.org
 * Copyright (C) 2026 Dongxu Li github.com/dxli
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

#ifndef LC_HATCHPROPERTIESEDITINGWIDGET_H
#define LC_HATCHPROPERTIESEDITINGWIDGET_H

#include "lc_entitypropertieseditorwidget.h"

class RS_Hatch;

namespace Ui {
    class LC_HatchPropertiesEditingWidget;
}

class LC_HatchPropertiesEditingWidget : public LC_EntityPropertiesEditorWidget {
    Q_OBJECT
public:
    explicit LC_HatchPropertiesEditingWidget(QWidget *parent = nullptr);
    ~LC_HatchPropertiesEditingWidget() override;
    void setEntity(RS_Entity* entity) override;
protected slots:
    void onScaleEditingFinished();
    void onAngleEditingFinished();
    void onSolidToggled(bool checked);
private:
    void updateMomentFields();
    void saveSettings();
    Ui::LC_HatchPropertiesEditingWidget *ui;
    RS_Hatch* m_entity{nullptr};
};

#endif // LC_HATCHPROPERTIESEDITINGWIDGET_H
