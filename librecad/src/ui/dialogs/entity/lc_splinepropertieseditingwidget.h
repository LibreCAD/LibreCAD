/*This file is part of the LibreCAD project, a 2D CAD program

Copyright (C) 2025 LibreCAD.org
Copyright (C) 2025 sand1024

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef LC_SPLINEPROPERTIESEDITINGWIDGET_H
#define LC_SPLINEPROPERTIESEDITINGWIDGET_H

#include <memory>
#include <QWidget>
#include "lc_entitypropertieseditorwidget.h"

class RS_Spline;
class QTableWidgetItem;

namespace Ui {
class LC_SplinePropertiesEditingWidget;
}

/**
 * @brief Widget for editing properties of a spline entity in LibreCAD.
 */
class LC_SplinePropertiesEditingWidget : public LC_EntityPropertiesEditorWidget {
    Q_OBJECT
public:
    /**
     * @brief Constructs the spline properties editing widget.
     * @param parent Optional parent widget.
     */
    explicit LC_SplinePropertiesEditingWidget(QWidget* parent = nullptr);

    /**
     * @brief Destroys the widget and its UI resources.
     */
    ~LC_SplinePropertiesEditingWidget() override;

    /**
     * @brief Sets the spline entity to be edited and updates the UI.
     * @param entity Pointer to the RS_Entity (expected to be RS_Spline).
     */
    void setEntity(RS_Entity* entity) override;

private slots:
    /**
     * @brief Handles toggling of the spline's closed state.
     * @param checked True if the spline is closed, false otherwise.
     */
    void onClosedToggled(bool checked);

    /**
     * @brief Handles changes to the spline's degree.
     * @param index Combo box index (maps to degree = index + 1).
     */
    void onDegreeIndexChanged(int index);

    /**
     * @brief Handles changes to control point coordinates or weights.
     * @param item The changed table item.
     */
    void onControlPointChanged(QTableWidgetItem* item);

    /**
     * @brief Handles changes to knot vector values.
     * @param item The changed table item.
     */
    void onKnotChanged(QTableWidgetItem* item);

    /**
     * @brief Adds a new control point at the end or after the selected row.
     */
    void onAddControlPoint();

    /**
     * @brief Adds a control point before the selected row.
     */
    void onAddControlPointBefore();

    /**
     * @brief Adds a control point after the selected row.
     */
    void onAddControlPointAfter();

    /**
     * @brief Removes the selected control point.
     */
    void onRemoveControlPoint();

    /**
     * @brief Shows the context menu for control points table.
     * @param pos The position where the context menu was triggered.
     */
    void showControlPointsContextMenu(const QPoint& pos);

private:
    /**
     * @brief Updates the UI based on the current spline entity.
     */
    void updateUI();

    std::unique_ptr<Ui::LC_SplinePropertiesEditingWidget> ui; // Manages UI elements
    RS_Spline* m_entity{nullptr}; // Non-owning pointer to the spline entity
};

#endif // LC_SPLINEPROPERTIESEDITINGWIDGET_H
