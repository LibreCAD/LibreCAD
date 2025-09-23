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
#include "lc_splinepropertieseditingwidget.h"
#include "rs_math.h"
#include "rs_spline.h"
#include "ui_lc_splinepropertieseditingwidget.h"
#include <QTableWidgetItem>
#include <QMessageBox>
#include <cmath> // For std::isfinite

LC_SplinePropertiesEditingWidget::LC_SplinePropertiesEditingWidget(QWidget *parent)
    : LC_EntityPropertiesEditorWidget(parent)
    , ui(new Ui::LC_SplinePropertiesEditingWidget){
    ui->setupUi(this);
    connect(ui->cbClosed, &QCheckBox::toggled, this, &LC_SplinePropertiesEditingWidget::onClosedToggled);
    connect(ui->cbDegree, &QComboBox::currentIndexChanged, this, &LC_SplinePropertiesEditingWidget::onDegreeIndexChanged);
    connect(ui->tableControlPoints, &QTableWidget::itemChanged, this, &LC_SplinePropertiesEditingWidget::onControlPointChanged);
    connect(ui->tableKnots, &QTableWidget::itemChanged, this, &LC_SplinePropertiesEditingWidget::onKnotChanged);
}
LC_SplinePropertiesEditingWidget::~LC_SplinePropertiesEditingWidget(){
    delete ui;
}
void LC_SplinePropertiesEditingWidget::setEntity(RS_Entity *entity) {
    m_entity = dynamic_cast<RS_Spline*>(entity);
    if (!m_entity) {
        // Clear UI or disable
        ui->cbDegree->setCurrentIndex(2); // Default to 3
        ui->cbClosed->setChecked(false);
        ui->tableControlPoints->clearContents();
        ui->tableControlPoints->setRowCount(0);
        ui->tableKnots->clearContents();
        ui->tableKnots->setRowCount(0);
        return;
    }
    updateUI();
}
void LC_SplinePropertiesEditingWidget::updateUI() {
    QString s = QString::number(m_entity->getDegree());
    ui->cbDegree->setCurrentIndex(ui->cbDegree->findText(s));toUIBool(m_entity->isClosed(), ui->cbClosed);

    // Populate control points table with X, Y, Weight
    ui->tableControlPoints->blockSignals(true);
    ui->tableControlPoints->clearContents();
    size_t numPoints = m_entity->getNumberOfControlPoints();
    ui->tableControlPoints->setRowCount(static_cast<int>(numPoints));
    const auto& cps = m_entity->getControlPoints();
    const auto& weights = m_entity->getEffectiveWeights();
    QStringList verticalLabelsCP;
    for (size_t i = 0; i < numPoints; ++i) {
        QTableWidgetItem *itemX = new QTableWidgetItem(QString::number(cps[i].x));
        QTableWidgetItem *itemY = new QTableWidgetItem(QString::number(cps[i].y));
        QTableWidgetItem *itemW = new QTableWidgetItem(QString::number(weights[i]));
        ui->tableControlPoints->setItem(static_cast<int>(i), 0, itemX);
        ui->tableControlPoints->setItem(static_cast<int>(i), 1, itemY);
        ui->tableControlPoints->setItem(static_cast<int>(i), 2, itemW);
        verticalLabelsCP << QString::number(i + 1);
    }
    ui->tableControlPoints->setVerticalHeaderLabels(verticalLabelsCP);
    ui->tableControlPoints->blockSignals(false);

    // Populate knots table
    ui->tableKnots->blockSignals(true);
    ui->tableKnots->clearContents();
    std::vector<double> knots = m_entity->getKnotVector(); // Effective knots
    size_t numKnots = knots.size();
    ui->tableKnots->setRowCount(static_cast<int>(numKnots));
    QStringList verticalLabelsKnots;
    for (size_t i = 0; i < numKnots; ++i) {
        QTableWidgetItem *item = new QTableWidgetItem(QString::number(knots[i]));
        ui->tableKnots->setItem(static_cast<int>(i), 0, item);
        verticalLabelsKnots << QString::number(i);
    }
    ui->tableKnots->setVerticalHeaderLabels(verticalLabelsKnots);
    ui->tableKnots->blockSignals(false);}
void LC_SplinePropertiesEditingWidget::onClosedToggled([[maybe_unused]]bool checked) {
    if (m_entity) {
        m_entity->setClosed(ui->cbClosed->isChecked());
        updateUI(); // Refresh tables, as closed affects knot generation
    }
}
void LC_SplinePropertiesEditingWidget::onDegreeIndexChanged([[maybe_unused]]int index) {
    if (m_entity) {
        m_entity->setDegree(RS_Math::round(RS_Math::eval(ui->cbDegree->currentText())));
        updateUI(); // Refresh tables, as degree affects knot count
    }
}
void LC_SplinePropertiesEditingWidget::onControlPointChanged(QTableWidgetItem *item) {
    if (!m_entity) return;
    int row = item->row();
    int col = item->column();
    double newVal = RS_Math::eval(item->text());
    if (!std::isfinite(newVal)) {
        // Invalid, revert
        const auto& cps = m_entity->getControlPoints();
        const auto& weights = m_entity->getEffectiveWeights();
        if (col == 0) item->setText(QString::number(cps[row].x));
        else if (col == 1) item->setText(QString::number(cps[row].y));
        else if (col == 2) item->setText(QString::number(weights[row]));
        return;
    }
    if (col == 0 || col == 1) {
        auto cps = m_entity->getControlPoints();
        if (col == 0) cps[row].x = newVal;
        else cps[row].y = newVal;
        m_entity->setControlPoints(cps);
    } else if (col == 2 && newVal > 0.0) {
        auto weights = m_entity->getEffectiveWeights();
        weights[row] = newVal;
        m_entity->setWeights(weights);
    } else {
        // Invalid weight, revert
        item->setText(QString::number(m_entity->getEffectiveWeights()[row]));
    }
    // No need for updateUI, as only value changed
}
void LC_SplinePropertiesEditingWidget::onKnotChanged(QTableWidgetItem *item) {
    if (!m_entity) return;
    int row = item->row();
    double newKnot = RS_Math::eval(item->text());
    if (!std::isfinite(newKnot)) {
        // Invalid, revert
        item->setText(QString::number(m_entity->getKnotVector()[row]));
        return;
    }
    std::vector<double> knots = m_entity->getKnotVector();
    double oldKnot = knots[row];
    knots[row] = newKnot;
    // Validate entire non-decreasing
    bool valid = true;
    for (size_t i = 1; i < knots.size(); ++i) {
        if (knots[i] < knots[i - 1]) {
            valid = false;
            break;
        }
    }
    if (!valid) {
        item->setText(QString::number(oldKnot));
        // Optional: QMessageBox::warning(this, "Invalid Knot", "Knot vector must be non-decreasing.");
        return;
    }
    m_entity->setKnotVector(knots);
    // No need for updateUI, as only value changed
}

