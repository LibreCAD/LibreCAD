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
// File: qg_dlgspline.cpp

#include <vector>
#include <cmath>  // for std::abs
#include <QTableWidgetItem>
#include <QMessageBox>  // For validation errors
#include <QTimer>
#include <QHeaderView>
#include <QPushButton>

#include "qg_dlgspline.h"
#include "qg_layerbox.h"
#include "qg_widgetpen.h"
#include "rs_debug.h"
#include "rs_graphic.h"
#include "rs_layer.h"
#include "rs_math.h"
#include "rs_settings.h"
#include "rs_spline.h"

/*
 *  Constructs a QG_DlgSpline as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */
QG_DlgSpline::QG_DlgSpline(QWidget *parent, LC_GraphicViewport *pViewport, RS_Spline * spline)
    :LC_EntityPropertiesDlg(parent, "SplineProperties", pViewport){
    setupUi(this);

    // Debounce updates with a timer to improve performance during rapid edits
    m_updateTimer = new QTimer(this);
    m_updateTimer->setSingleShot(true);
    m_updateTimer->setInterval(150);  // 150ms delay
    connect(m_updateTimer, &QTimer::timeout, this, &QG_DlgSpline::performUpdate);

    // Connect table changes to delayed update
    connect(tableControlPoints, &QTableWidget::cellChanged, this, &QG_DlgSpline::scheduleUpdate);
    connect(tableKnots, &QTableWidget::cellChanged, this, &QG_DlgSpline::scheduleUpdate);

    // Connect closed and degree changes for immediate updates (structural changes)
    connect(cbClosed, &QCheckBox::toggled, this, &QG_DlgSpline::updateClosed);
    connect(cbDegree, QOverload<const QString &>::of(&QComboBox::currentTextChanged), this, &QG_DlgSpline::updateDegree);

    // Connect add/remove buttons
    connect(pbAddControlPoint, &QPushButton::clicked, this, &QG_DlgSpline::addControlPoint);
    connect(pbRemoveControlPoint, &QPushButton::clicked, this, &QG_DlgSpline::removeControlPoint);
    connect(pbAddKnot, &QPushButton::clicked, this, &QG_DlgSpline::addKnot);
    connect(pbRemoveKnot, &QPushButton::clicked, this, &QG_DlgSpline::removeKnot);

    setEntity(spline);
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_DlgSpline::languageChange(){
    retranslateUi(this);
}

void QG_DlgSpline::setEntityID(QLabel* lId, RS_Entity* e) {
    if (!lId || !e) return;
    bool showIds = LC_GET_ONE_BOOL("Appearance", "ShowEntityIDs", false);
    if (showIds) {
        lId->setText(tr("ID: %1").arg(e->getId()));
        lId->setVisible(true);
    } else {
        lId->setVisible(false);
    }
}

void QG_DlgSpline::setEntity(RS_Spline* e) {
    m_spline = e;

    RS_Graphic* graphic = m_spline->getGraphic();
    if (graphic) {
        cbLayer->init(*(graphic->getLayerList()), false, false);
    }
    RS_Layer* lay = m_spline->getLayer(false);
    if (lay) {
        cbLayer->setLayer(*lay);
    }

    wPen->setPen(m_spline, lay, tr("Pen"));

    // Fixed QComboBox API: Clamp degree to valid range (1-3) and ensure index is set
    int degree = m_spline->getDegree();
    if (degree < 1) degree = 1;
    if (degree > 3) degree = 3;
    QString s;
    s.setNum(degree);
    int index = cbDegree->findText(s);
    if (index == -1) {
        // Fallback: should not happen after clamping, but set to default (degree 3)
        index = cbDegree->findText("3");
    }
    cbDegree->setCurrentIndex(index);

    toUIBool(m_spline->isClosed(), cbClosed);

    // Refactored: Use common function for entity ID display
    setEntityID(lId, m_spline);

    // Set table tooltips for better UX
    tableControlPoints->setToolTip(tr("Edit spline control points (X, Y coordinates and weights). Weights must be positive."));
    tableKnots->setToolTip(tr("Edit knot vector values. Knots must be non-decreasing and of sufficient length."));

    // Configure table headers for better readability
    tableControlPoints->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    tableKnots->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

    // Temporarily disconnect to avoid triggering updates during population
    disconnect(tableControlPoints, &QTableWidget::cellChanged, this, &QG_DlgSpline::scheduleUpdate);
    disconnect(tableKnots, &QTableWidget::cellChanged, this, &QG_DlgSpline::scheduleUpdate);

    // Populate control points table
    std::vector<RS_Vector> cps = m_spline->getControlPoints();
    std::vector<double> weights = m_spline->getWeights();
    tableControlPoints->clearContents();
    tableControlPoints->setRowCount(static_cast<int>(cps.size()));
    for (size_t i = 0; i < cps.size(); ++i) {
        // Column 0: X
        QTableWidgetItem* itemX = new QTableWidgetItem(QString::number(cps[i].x, 'g', 6));
        tableControlPoints->setItem(static_cast<int>(i), 0, itemX);

        // Column 1: Y
        QTableWidgetItem* itemY = new QTableWidgetItem(QString::number(cps[i].y, 'g', 6));
        tableControlPoints->setItem(static_cast<int>(i), 1, itemY);

        // Column 2: Weight (editable)
        double w = (i < weights.size()) ? weights[i] : 1.0;
        QTableWidgetItem* itemW = new QTableWidgetItem(QString::number(w, 'g', 6));
        tableControlPoints->setItem(static_cast<int>(i), 2, itemW);
    }

    // Populate knots table
    std::vector<double> knots = m_spline->getKnots();
    tableKnots->clearContents();
    tableKnots->setRowCount(static_cast<int>(knots.size()));
    for (size_t i = 0; i < knots.size(); ++i) {
        QTableWidgetItem* itemK = new QTableWidgetItem(QString::number(knots[i], 'g', 6));
        tableKnots->setItem(static_cast<int>(i), 0, itemK);
    }

    // Update button states
    updateButtonStates();

    // Reconnect after population for delayed updates
    connect(tableControlPoints, &QTableWidget::cellChanged, this, &QG_DlgSpline::scheduleUpdate);
    connect(tableKnots, &QTableWidget::cellChanged, this, &QG_DlgSpline::scheduleUpdate);
}

/**
 * @brief Schedules a debounced update of the spline entity.
 */
void QG_DlgSpline::scheduleUpdate() {
    if (!m_updateTimer->isActive())
        m_updateTimer->start();
}

/**
 * @brief Performs the actual entity update (called by timer).
 */
void QG_DlgSpline::performUpdate() {
    updateControlPoints();
    updateKnots();
    updatePen();
    updateLayer();
    updateButtonStates();
}

/**
 * @brief Updates the spline's control points and weights based on table changes.
 * Validates input, reverts invalid values (including non-positive weights), and applies changes if valid.
 */
void QG_DlgSpline::updateControlPoints() {
    int numRows = tableControlPoints->rowCount();
    if (numRows < 2) {
        return;
    }

    const std::vector<RS_Vector>& currentCPs = m_spline->getControlPoints();
    const std::vector<double>& currentWeights = m_spline->getWeights();

    std::vector<RS_Vector> newCPs = currentCPs;
    std::vector<double> newWeights(currentWeights.size(), 1.0);  // Ensure size matches
    bool hasChanges = false;

    for (int row = 0; row < numRows; ++row) {
        QTableWidgetItem* itemX = tableControlPoints->item(row, 0);
        QTableWidgetItem* itemY = tableControlPoints->item(row, 1);
        QTableWidgetItem* itemW = tableControlPoints->item(row, 2);

        if (!itemX || !itemY || !itemW) {
            continue;
        }

        bool okX, okY, okW;
        double x = itemX->text().toDouble(&okX);
        double y = itemY->text().toDouble(&okY);
        double w = itemW->text().toDouble(&okW);

        double currentX = currentCPs[row].x;
        double currentY = currentCPs[row].y;
        double currentW = (static_cast<size_t>(row) < currentWeights.size()) ? currentWeights[row] : 1.0;

        bool isValidRow = okX && okY && okW && w > 0.0;  // Weights must be positive
        if (isValidRow) {
            if (std::abs(x - currentX) > 1e-6 || std::abs(y - currentY) > 1e-6 || std::abs(w - currentW) > 1e-6) {
                newCPs[row] = RS_Vector(x, y);
                newWeights[row] = w;
                hasChanges = true;
            }
        }

        // Revert invalid fields to current values
        if (!okX) {
            itemX->setText(QString::number(currentX, 'g', 6));
        }
        if (!okY) {
            itemY->setText(QString::number(currentY, 'g', 6));
        }
        if (!okW || w <= 0.0) {
            itemW->setText(QString::number(currentW, 'g', 6));
            if (w <= 0.0 && okW) {
                QMessageBox::warning(this, tr("Invalid Weight"), tr("Weight must be positive. Reverted to %1.").arg(currentW));
            }
        }
    }

    if (hasChanges) {
        // Resize weights if necessary
        if (newWeights.size() < newCPs.size()) {
            newWeights.resize(newCPs.size(), 1.0);
        }
        m_spline->setControlPoints(newCPs);
        m_spline->setWeights(newWeights);
        m_spline->update();
    }
}

/**
 * @brief Updates the spline's knots based on table changes.
 * Validates input, checks monotonicity, reverts invalid values, and applies changes if valid and sufficient.
 */
void QG_DlgSpline::updateKnots() {
    int numRows = tableKnots->rowCount();
    if (numRows <= 0) {
        return;
    }

    const std::vector<double>& currentKnots = m_spline->getKnots();
    std::vector<double> newKnots(currentKnots);
    bool hasChanges = false;
    bool allValid = true;

    // First pass: parse and check for changes/invalid parses
    for (int row = 0; row < numRows; ++row) {
        QTableWidgetItem* itemK = tableKnots->item(row, 0);
        if (!itemK) {
            allValid = false;
            continue;
        }

        bool okK;
        double k = itemK->text().toDouble(&okK);
        double currentK = (static_cast<size_t>(row) < currentKnots.size()) ? currentKnots[row] : 0.0;

        if (okK) {
            if (std::abs(k - currentK) > 1e-6) {
                newKnots[row] = k;
                hasChanges = true;
            }
        } else {
            // Revert invalid to current value
            itemK->setText(QString::number(currentK, 'g', 6));
            allValid = false;
        }
    }

    if (!allValid || !hasChanges) {
        return;
    }

    // Second pass: validate monotonicity (non-decreasing)
    bool isMonotonic = true;
    for (size_t i = 1; i < newKnots.size(); ++i) {
        if (newKnots[i] < newKnots[i - 1]) {
            isMonotonic = false;
            break;
        }
    }

    if (!isMonotonic) {
        QMessageBox::warning(this, tr("Invalid Knots"),
                             tr("Knot vector must be non-decreasing. Changes reverted."));
        // Revert all table cells to current knots
        for (int row = 0; row < numRows; ++row) {
            double currentK = (static_cast<size_t>(row) < currentKnots.size()) ? currentKnots[row] : 0.0;
            QTableWidgetItem* itemK = tableKnots->item(row, 0);
            if (itemK) {
                itemK->setText(QString::number(currentK, 'g', 6));
            }
        }
        return;
    }

    // Validate knot vector size: must be at least num_control_points + degree + 1
    size_t numCPs = m_spline->getControlPoints().size();
    int degree = m_spline->getDegree();
    size_t requiredKnots = numCPs + static_cast<size_t>(degree) + 1;

    if (newKnots.size() >= requiredKnots) {
        m_spline->setKnots(newKnots);
        m_spline->update();
    } else {
        QMessageBox::warning(this, tr("Invalid Knots"),
                             tr("Knot vector size insufficient: requires at least %1 knots for %2 control points and degree %3 (have %4).")
                                 .arg(requiredKnots).arg(numCPs).arg(degree).arg(newKnots.size()));
        // Revert table
        for (int row = 0; row < numRows; ++row) {
            double currentK = (static_cast<size_t>(row) < currentKnots.size()) ? currentKnots[row] : 0.0;
            QTableWidgetItem* itemK = tableKnots->item(row, 0);
            if (itemK) {
                itemK->setText(QString::number(currentK, 'g', 6));
            }
        }
    }
}

/**
 * @brief Updates the spline's degree if valid.
 * Checks knot sufficiency and reverts if invalid.
 */
void QG_DlgSpline::updateDegree() {
    QString degText = cbDegree->currentText();
    bool ok = false;
    int newDegree = degText.toInt(&ok);
    if (!ok || newDegree < 1 || newDegree > 3) {
        LC_ERR << __func__ << "() invalid degree for a spline, degree=" << degText;
        return;
    }

    int currentDegree = m_spline->getDegree();
    if (newDegree == currentDegree) {
        return;
    }

    // Temporarily apply to validate
    m_spline->setDegree(newDegree);
    size_t numCPs = m_spline->getControlPoints().size();
    const std::vector<double>& currentKnots = m_spline->getKnots();
    size_t requiredKnots = numCPs + static_cast<size_t>(newDegree) + 1;

    if (currentKnots.size() < requiredKnots) {
        // Revert degree
        m_spline->setDegree(currentDegree);
        QMessageBox::warning(this, tr("Invalid Degree"),
                             tr("Cannot set degree to %1: requires at least %2 knots (have %3) for %4 control points.")
                                 .arg(newDegree).arg(requiredKnots).arg(currentKnots.size()).arg(numCPs));

        // Reset combo box to current degree
        QString currentText;
        currentText.setNum(currentDegree);
        int index = cbDegree->findText(currentText);
        if (index != -1) {
            cbDegree->setCurrentIndex(index);
        }
    } else {
        m_spline->update();
    }
}

/**
 * @brief Updates the spline's closed status and handles control point wrapping.
 */
void QG_DlgSpline::updateClosed() {
    bool newClosed = cbClosed->isChecked();
    if (newClosed == m_spline->isClosed()) {
        return;
    }

    m_spline->setClosed(newClosed);

    m_spline->update();
    // Repopulate tables to reflect changes
    setEntity(m_spline);  // Safe to recall as it updates UI
}

/**
 * @brief Updates the spline's pen attributes.
 */
void QG_DlgSpline::updatePen() {
    RS_Pen newPen = wPen->getPen();
    if (newPen != m_spline->getPen()) {
        m_spline->setPen(newPen);
        m_spline->update();
    }
}

/**
 * @brief Updates the spline's layer assignment.
 */
void QG_DlgSpline::updateLayer() {
    RS_Layer* newLayer = cbLayer->getLayer();
    RS_Layer* currentLayer = m_spline->getLayer();
    if (newLayer != nullptr && currentLayer != nullptr && newLayer->getName() != currentLayer->getName()) {
        m_spline->setLayer(newLayer);
        m_spline->update();
    }
}

/**
 * @brief Adds a new control point row with default values (0,0,1.0).
 */
void QG_DlgSpline::addControlPoint() {
    int row = tableControlPoints->rowCount();
    tableControlPoints->insertRow(row);

    QTableWidgetItem* itemX = new QTableWidgetItem("0.000000");
    tableControlPoints->setItem(row, 0, itemX);

    QTableWidgetItem* itemY = new QTableWidgetItem("0.000000");
    tableControlPoints->setItem(row, 1, itemY);

    QTableWidgetItem* itemW = new QTableWidgetItem("1.000000");
    tableControlPoints->setItem(row, 2, itemW);

    scheduleUpdate();
    updateButtonStates();
}

/**
 * @brief Removes the last control point row if possible.
 */
void QG_DlgSpline::removeControlPoint() {
    int row = tableControlPoints->rowCount();
    if (row > 2) {  // At least 2 points required
        tableControlPoints->removeRow(row - 1);
        scheduleUpdate();
        updateButtonStates();
    } else {
        QMessageBox::warning(this, tr("Invalid Operation"), tr("Spline requires at least 2 control points."));
    }
}

/**
 * @brief Adds a new knot row with default value (last knot or 1.0).
 */
void QG_DlgSpline::addKnot() {
    int row = tableKnots->rowCount();
    double defaultK = (row > 0) ? tableKnots->item(row - 1, 0)->text().toDouble() : 1.0;
    tableKnots->insertRow(row);

    QTableWidgetItem* itemK = new QTableWidgetItem(QString::number(defaultK, 'g', 6));
    tableKnots->setItem(row, 0, itemK);

    scheduleUpdate();
    updateButtonStates();
}

/**
 * @brief Removes the last knot row if possible.
 */
void QG_DlgSpline::removeKnot() {
    int row = tableKnots->rowCount();
    if (row > m_spline->getDegree() + 1) {  // Basic min knots
        tableKnots->removeRow(row - 1);
        scheduleUpdate();
        updateButtonStates();
    } else {
        QMessageBox::warning(this, tr("Invalid Operation"), tr("Insufficient knots remaining."));
    }
}

/**
 * @brief Updates the enabled state of add/remove buttons based on table sizes.
 */
void QG_DlgSpline::updateButtonStates() {
    int numCPs = tableControlPoints->rowCount();
    pbRemoveControlPoint->setEnabled(numCPs > 2);
    pbAddControlPoint->setEnabled(true);  // Always allow adding

    int numKnots = tableKnots->rowCount();
    int minKnots = m_spline->getNumberOfControlPoints() + m_spline->getDegree() + 1;
    pbRemoveKnot->setEnabled(numKnots > minKnots);
    pbAddKnot->setEnabled(true);
}

/**
 * @brief Updates the spline entity based on changes in the dialog UI.
 * Calls specialized update methods for each property group.
 */
void QG_DlgSpline::updateEntity() {
    performUpdate();  // Ensure immediate update for override calls
}
