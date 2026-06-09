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

#ifndef LC_IMAGEPROPERTIESEDITINGWIDGET_H
#define LC_IMAGEPROPERTIESEDITINGWIDGET_H

#include "lc_entitypropertieseditorwidget.h"

class RS_Image;

namespace Ui {
    class LC_ImagePropertiesEditingWidget;
}

class LC_PropertiesEditingWidgetImage : public LC_EntityPropertiesEditorWidget{
    Q_OBJECT
public:
    explicit LC_PropertiesEditingWidgetImage(QWidget *parent = nullptr);
    ~LC_PropertiesEditingWidgetImage() override;
    void updateUIbyEntity();
    void setupInteractiveInputWidgets() override;

    void setEntity(RS_Entity* entity) override;
protected slots:
    void onInsertionPointEditingFinished() const;
    void onAngleEditingFinished();
    void onImageFileClick() const;
    void onWidthChanged();
    void onHeightChanged();
    void onScaleXChanged();
    void onScaleYChanged();
    void onPathChanged(const QString &) const;
private:
    void updateDPI(const RS_Vector& scale) const;
    void updateSizeInPixels();
    Ui::LC_ImagePropertiesEditingWidget *ui;
    RS_Image* m_entity = nullptr;
    double m_scale = 1.;
};

#endif
