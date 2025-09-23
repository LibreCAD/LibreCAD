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
#include "ui_lc_splinepropertieseditingwidget.h"
#include "rs_spline.h"
#include <QTableWidgetItem>
#include <QMenu>
#include <QAction>
#include <QModelIndex>

LC_SplinePropertiesEditingWidget::LC_SplinePropertiesEditingWidget(QWidget *parent) :
    LC_EntityPropertiesEditorWidget(parent),
    ui(new Ui::LC_SplinePropertiesEditingWidget)
{
    ui->setupUi(this);
    ui->tableControlPoints->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->tableControlPoints, SIGNAL(customContextMenuRequested(const QPoint&)),
            this, SLOT(showControlPointsContextMenu(const QPoint&)));
}

LC_SplinePropertiesEditingWidget::~LC_SplinePropertiesEditingWidget()
{
}

void LC_SplinePropertiesEditingWidget::setEntity(RS_Entity *entity) {
    RS_Spline* spline = dynamic_cast<RS_Spline*>(entity);
    if (spline) {
        m_entity = spline;
        updateUI();
    } else {
        m_entity = nullptr;
    }
}

void LC_SplinePropertiesEditingWidget::updateUI() {
    if (!m_entity) return;

    ui->cbClosed->setChecked(m_entity->isClosed());

    ui->cbDegree->setCurrentIndex(m_entity->getDegree() - 1);

    size_t numCP = m_entity->getNumberOfControlPoints();

    ui->tableControlPoints->clearContents();
    ui->tableControlPoints->setRowCount(static_cast<int>(numCP));

    for (size_t i = 0; i < numCP; ++i) {
        QTableWidgetItem *itemNum = new QTableWidgetItem(QString::number(i + 1));
        itemNum->setFlags(itemNum->flags() & ~Qt::ItemIsEditable);
        ui->tableControlPoints->setItem(static_cast<int>(i), 0, itemNum);

        const RS_Vector& cp = m_entity->getControlPoints()[i];

        QTableWidgetItem *itemX = new QTableWidgetItem(QString::number(cp.x));
        ui->tableControlPoints->setItem(static_cast<int>(i), 1, itemX);

        QTableWidgetItem *itemY = new QTableWidgetItem(QString::number(cp.y));
        ui->tableControlPoints->setItem(static_cast<int>(i), 2, itemY);

        QTableWidgetItem *itemW = new QTableWidgetItem(QString::number(m_entity->getWeight(i)));
        ui->tableControlPoints->setItem(static_cast<int>(i), 3, itemW);
    }

    size_t numKnots = static_cast<size_t>(m_entity->getNumberOfKnots());

    std::vector<double> knots = m_entity->getKnotVector();
    if (knots.empty()) {
        knots = m_entity->isClosed() ? m_entity->knotu(numCP, static_cast<size_t>(m_entity->getDegree()) + 1) : m_entity->knot(numCP, static_cast<size_t>(m_entity->getDegree()) + 1);
    }

    ui->tableKnots->clearContents();
    ui->tableKnots->setRowCount(static_cast<int>(numKnots));

    for (size_t i = 0; i < numKnots; ++i) {
        QTableWidgetItem *itemNum = new QTableWidgetItem(QString::number(i + 1));
        itemNum->setFlags(itemNum->flags() & ~Qt::ItemIsEditable);
        ui->tableKnots->setItem(static_cast<int>(i), 0, itemNum);

        QTableWidgetItem *itemV = new QTableWidgetItem(QString::number(knots[i]));
        ui->tableKnots->setItem(static_cast<int>(i), 1, itemV);
    }
}

void LC_SplinePropertiesEditingWidget::onClosedToggled(bool checked) {
    if (m_entity) {
        m_entity->setClosed(checked);
        m_entity->update();
        updateUI();
    }
}

void LC_SplinePropertiesEditingWidget::onDegreeIndexChanged(int index) {
    if (m_entity) {
        m_entity->setDegree(index + 1);
        m_entity->update();
        updateUI();
    }
}

void LC_SplinePropertiesEditingWidget::onControlPointChanged(QTableWidgetItem *item) {
    if (!m_entity || !item) return;

    int row = item->row();
    int col = item->column();

    if (row < 0 || static_cast<size_t>(row) >= m_entity->getNumberOfControlPoints()) return;

    bool ok;
    double val = item->text().toDouble(&ok);
    if (!ok) return;

    if (col == 1 || col == 2) {
        RS_Vector cp = m_entity->getControlPoints()[row];
        if (col == 1) cp.x = val;
        else cp.y = val;
        m_entity->setControlPoint(static_cast<size_t>(row), cp);
    } else if (col == 3) {
        m_entity->setWeight(static_cast<size_t>(row), val);
    }
    m_entity->update();
}

void LC_SplinePropertiesEditingWidget::onKnotChanged(QTableWidgetItem *item) {
    if (!m_entity || !item) return;

    int row = item->row();
    int col = item->column();

    if (col != 1 || row < 0) return;

    bool ok;
    double val = item->text().toDouble(&ok);
    if (!ok) return;

    size_t numKnots = static_cast<size_t>(m_entity->getNumberOfKnots());

    std::vector<double> knots = m_entity->getKnotVector();
    if (knots.size() != numKnots) {
        size_t numCP = m_entity->getNumberOfControlPoints();
        size_t order = static_cast<size_t>(m_entity->getDegree()) + 1;
        knots = m_entity->isClosed() ? m_entity->knotu(numCP, order) : m_entity->knot(numCP, order);
        m_entity->setKnotVector(knots);
    }

    m_entity->setKnot(static_cast<size_t>(row), val);
    m_entity->update();
}

void LC_SplinePropertiesEditingWidget::onAddControlPoint() {
    if (!m_entity) return;

    int row = ui->tableControlPoints->currentRow();
    size_t insertIndex = (row >= 0) ? static_cast<size_t>(row + 1) : m_entity->getNumberOfControlPoints();

    m_entity->insertControlPoint(insertIndex, RS_Vector(0.0, 0.0), 1.0);
    m_entity->update();
    updateUI();
}

void LC_SplinePropertiesEditingWidget::onAddControlPointBefore() {
    if (!m_entity) return;

    int row = ui->tableControlPoints->currentRow();
    if (row < 0) return;

    m_entity->insertControlPoint(static_cast<size_t>(row), RS_Vector(0.0, 0.0), 1.0);
    m_entity->update();
    updateUI();
}

void LC_SplinePropertiesEditingWidget::onRemoveControlPoint() {
    if (!m_entity) return;

    int row = ui->tableControlPoints->currentRow();
    if (row < 0) return;

    m_entity->removeControlPoint(static_cast<size_t>(row));
    m_entity->update();
    updateUI();
}

void LC_SplinePropertiesEditingWidget::showControlPointsContextMenu(const QPoint &pos) {
    if (!m_entity) return;

    QModelIndex index = ui->tableControlPoints->indexAt(pos);
    if (!index.isValid()) return;

    int row = index.row();
    ui->tableControlPoints->setCurrentIndex(index);

    QMenu menu(this);

    QAction *addBeforeAct = new QAction("Add before", this);
    connect(addBeforeAct, SIGNAL(triggered()), this, SLOT(onAddControlPointBefore()));

    QAction *deleteAct = new QAction("Delete", this);
    connect(deleteAct, SIGNAL(triggered()), this, SLOT(onRemoveControlPoint()));

    menu.addAction(addBeforeAct);
    menu.addAction(deleteAct);

    menu.exec(ui->tableControlPoints->viewport()->mapToGlobal(pos));
}
