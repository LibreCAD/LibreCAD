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

#ifndef LC_ARCPROPERTIESEDITINGWIDGET_H
#define LC_ARCPROPERTIESEDITINGWIDGET_H

#include "lc_entitypropertieseditorwidget.h"

class RS_Arc;

namespace Ui {
class LC_ArcPropertiesEditingWidget;
}

class LC_ArcPropertiesEditingWidget : public LC_EntityPropertiesEditorWidget {
    Q_OBJECT
public:
    explicit LC_ArcPropertiesEditingWidget(QWidget* parent = nullptr);
    ~LC_ArcPropertiesEditingWidget() override;
    void setEntity(RS_Entity* entity) override;
protected slots:
    void onCenterEditingFinished();
    void onRadiusEditingFinished();
    void onDiameterEditingFinished();
    void onArcLengthEditingFinished();
    void onSweepAngleEditingFinished();
    void onBulgeEditingFinished();
    void onAngle1EditingFinished();
    void onAngle2EditingFinished();
    void onReversedToggled(bool checked);
    void onRadiusTextChanged(const QString &text);
    void onDiameterTextChanged(const QString &text);
    void onArcLengthTextChanged(const QString &text);
    void onSweepAngleTextChanged(const QString &text);
    void onBulgeTextChanged(const QString &text);
    void onAngle1TextChanged(const QString &text);
    void onAngle2TextChanged(const QString &text);
public:
    void setupInteractiveInputWidgets() override;
private:
    void updateDerivedFields(double radius, double theta);
    void updateUI();
    Ui::LC_ArcPropertiesEditingWidget* ui = nullptr;
    RS_Arc* m_entity = nullptr;
};

#endif // LC_ARCPROPERTIESEDITINGWIDGET_H
