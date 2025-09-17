/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file gpl-2.0.txt included in the
** packaging of this file.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
**
** This copyright notice MUST APPEAR in all copies of the script!
**
**********************************************************************/
#ifndef QG_DLGSPLINE_H
#define QG_DLGSPLINE_H

#include <vector>
#include <QTimer>
#include "ui_qg_dlgspline.h"
#include "lc_entitypropertiesdlg.h"

class RS_Spline;
class QLabel;
class RS_Entity;

class QG_DlgSpline : public LC_EntityPropertiesDlg, public Ui::QG_DlgSpline {
    Q_OBJECT

public:
    explicit QG_DlgSpline(QWidget *parent, LC_GraphicViewport *pViewport, RS_Spline *spline);

public slots:
    /**
     * @brief Updates the spline entity based on changes in the dialog UI.
     * Calls specialized update methods for each property group.
     */
    void updateEntity() override;

protected slots:
    /**
     * @brief Updates the dialog's text strings to the current language.
     */
    void languageChange();

protected:
    /**
     * @brief Pointer to the spline entity being edited.
     */
    RS_Spline* m_spline = nullptr;

    /**
     * @brief Timer for debounced updates to improve performance.
     */
    QTimer* m_updateTimer = nullptr;

    /**
     * @brief Sets the dialog's entity and populates UI fields, including control points and knots tables.
     * @param e The spline entity to edit.
     */
    void setEntity(RS_Spline *e);

    /**
     * @brief Utility to update entity ID label based on settings.
     * TODO: Move to shared header (e.g., lc_entitypropertiesutils.h) for reuse.
     * @param lId The QLabel to update/hide.
     * @param e The entity providing the ID.
     */
    static void setEntityID(QLabel *lId, RS_Entity *e);

    /**
     * @brief Updates the spline's control points and weights based on table changes.
     * Validates input, reverts invalid values, and applies changes if valid.
     */
    void updateControlPoints();

    /**
     * @brief Updates the spline's knots based on table changes.
     * Validates input, reverts invalid values, and applies changes if valid and sufficient.
     */
    void updateKnots();

    /**
     * @brief Updates the spline's degree if valid.
     * Checks knot sufficiency and reverts if invalid.
     */
    void updateDegree();

    /**
     * @brief Updates the spline's closed status.
     */
    void updateClosed();

    /**
     * @brief Updates the spline's pen attributes.
     */
    void updatePen();

    /**
     * @brief Updates the spline's layer assignment.
     */
    void updateLayer();

    /**
     * @brief Schedules a debounced update of the spline entity.
     */
    void scheduleUpdate();

    /**
     * @brief Performs the actual entity update (called by timer).
     */
    void performUpdate();

    /**
     * @brief Adds a new control point row with default values (0,0,1.0).
     */
    void addControlPoint();

    /**
     * @brief Removes the last control point row if possible.
     */
    void removeControlPoint();

    /**
     * @brief Adds a new knot row with default value (last knot or 1.0).
     */
    void addKnot();

    /**
     * @brief Removes the last knot row if possible.
     */
    void removeKnot();

    /**
     * @brief Updates the enabled state of add/remove buttons based on table sizes.
     */
    void updateButtonStates();
};

#endif // QG_DLGSPLINE_H
