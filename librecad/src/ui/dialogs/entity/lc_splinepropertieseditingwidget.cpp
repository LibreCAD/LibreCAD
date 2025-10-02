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
#include <QMessageBox>

LC_SplinePropertiesEditingWidget::LC_SplinePropertiesEditingWidget(QWidget *parent) :
    LC_EntityPropertiesEditorWidget(parent),
    ui(new Ui::LC_SplinePropertiesEditingWidget)
{
    ui->setupUi(this);
    ui->tableControlPoints->setContextMenuPolicy(Qt::CustomContextMenu);

    // Connect table signals
    connect(ui->tableControlPoints, &QTableWidget::itemChanged,
            this, &LC_SplinePropertiesEditingWidget::onControlPointChanged);
    connect(ui->tableKnots, &QTableWidget::itemChanged,
            this, &LC_SplinePropertiesEditingWidget::onKnotChanged);

    // Connect button signals
    connect(ui->btnAddControlPoint, &QPushButton::clicked,
            this, &LC_SplinePropertiesEditingWidget::onAddControlPoint);
    connect(ui->btnRemoveControlPoint, &QPushButton::clicked,
            this, &LC_SplinePropertiesEditingWidget::onRemoveControlPoint);

    // Enable/disable Remove button based on selection
    connect(ui->tableControlPoints->selectionModel(),
            &QItemSelectionModel::currentRowChanged,
            this, [this]() {
                ui->btnRemoveControlPoint->setEnabled(ui->tableControlPoints->currentRow() >= 0);
            });

    // Initialize Remove button state
    ui->btnRemoveControlPoint->setEnabled(false);

    connect(ui->tableControlPoints, &QTableWidget::customContextMenuRequested,
            this, &LC_SplinePropertiesEditingWidget::showControlPointsContextMenu);
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
    if (!m_entity) {
        ui->tableControlPoints->setRowCount(0);
        ui->tableKnots->setRowCount(0);
        ui->cbClosed->setChecked(false);
        ui->cbDegree->setCurrentIndex(-1);
        return;
    }

    QSignalBlocker cpBlocker(ui->tableControlPoints);
    QSignalBlocker knotBlocker(ui->tableKnots);

    ui->cbClosed->setChecked(m_entity->isClosed());
    ui->cbDegree->setCurrentIndex(m_entity->getDegree() - 1);

    size_t numCP = m_entity->getNumberOfControlPoints();
    ui->tableControlPoints->setRowCount(static_cast<int>(numCP));
    const auto& controlPoints = m_entity->getControlPoints();
    for (size_t i = 0; i < numCP; ++i) {
        QTableWidgetItem *itemNum = new QTableWidgetItem(QString::number(i + 1));
        itemNum->setFlags(itemNum->flags() & ~Qt::ItemIsEditable);
        ui->tableControlPoints->setItem(static_cast<int>(i), 0, itemNum);
        ui->tableControlPoints->setItem(static_cast<int>(i), 1, new QTableWidgetItem(QString::number(controlPoints[i].x)));
        ui->tableControlPoints->setItem(static_cast<int>(i), 2, new QTableWidgetItem(QString::number(controlPoints[i].y)));
        ui->tableControlPoints->setItem(static_cast<int>(i), 3, new QTableWidgetItem(QString::number(m_entity->getWeight(i))));
    }

    size_t numKnots = m_entity->getKnotVector().size();
    std::vector<double> knots = m_entity->getKnotVector();
    ui->tableKnots->setRowCount(static_cast<int>(numKnots));
    for (size_t i = 0; i < numKnots; ++i) {
        QTableWidgetItem *itemNum = new QTableWidgetItem(QString::number(i + 1));
        itemNum->setFlags(itemNum->flags() & ~Qt::ItemIsEditable);
        ui->tableKnots->setItem(static_cast<int>(i), 0, itemNum);
        ui->tableKnots->setItem(static_cast<int>(i), 1, new QTableWidgetItem(QString::number(knots[i])));
    }
}

void LC_SplinePropertiesEditingWidget::onClosedToggled(bool checked) {
    if (m_entity) {
        try {
            m_entity->setClosed(checked);
            m_entity->update();
            updateUI();
        } catch (const std::exception& e) {
            QMessageBox::warning(this, "Error", e.what());
        }
    }
}

void LC_SplinePropertiesEditingWidget::onDegreeIndexChanged(int index) {
    if (m_entity) {
        try {
            m_entity->setDegree(index + 1);
            m_entity->update();
            updateUI();
        } catch (const std::invalid_argument& e) {
            QMessageBox::warning(this, "Error", e.what());
            ui->cbDegree->setCurrentIndex(m_entity->getDegree() - 1);
        }
    }
}

void LC_SplinePropertiesEditingWidget::onControlPointChanged(QTableWidgetItem *item) {
    if (!m_entity || !item) return;

    int row = item->row();
    int col = item->column();

    if (col == 0 || row < 0 || static_cast<size_t>(row) >= m_entity->getNumberOfControlPoints()) return;

    bool ok;
    double val = item->text().toDouble(&ok);
    if (!ok) {
        if (col == 1) {
            item->setText(QString::number(m_entity->getControlPoints()[row].x));
        } else if (col == 2) {
            item->setText(QString::number(m_entity->getControlPoints()[row].y));
        } else if (col == 3) {
            item->setText(QString::number(m_entity->getWeight(row)));
        }
        QMessageBox::warning(this, "Invalid Input", "Please enter a valid number.");
        return;
    }

    try {
        if (col == 1 || col == 2) {
            RS_Vector cp = m_entity->getControlPoints()[row];
            if (col == 1) cp.x = val;
            else cp.y = val;
            m_entity->setControlPoint(static_cast<size_t>(row), cp);
        } else if (col == 3) {
            m_entity->setWeight(static_cast<size_t>(row), val);
        }
        m_entity->update();
    } catch (const std::exception& e) {
        if (col == 1) {
            item->setText(QString::number(m_entity->getControlPoints()[row].x));
        } else if (col == 2) {
            item->setText(QString::number(m_entity->getControlPoints()[row].y));
        } else if (col == 3) {
            item->setText(QString::number(m_entity->getWeight(row)));
        }
        QMessageBox::warning(this, "Error", e.what());
    }
}

void LC_SplinePropertiesEditingWidget::onKnotChanged(QTableWidgetItem *item) {
    if (!m_entity || !item) return;

    int row = item->row();
    int col = item->column();

    if (col != 1 || row < 0) return;

    bool ok;
    double val = item->text().toDouble(&ok);
    if (!ok) {
        item->setText(QString::number(m_entity->getKnotVector()[row]));
        QMessageBox::warning(this, "Invalid Input", "Please enter a valid number.");
        return;
    }

    try {
        std::vector<double> knots = m_entity->getKnotVector();
        if (static_cast<size_t>(row) >= knots.size()) return;
        knots[row] = val;
        m_entity->setKnotVector(knots);
        m_entity->update();
    } catch (const std::invalid_argument& e) {
        item->setText(QString::number(m_entity->getKnotVector()[row]));
        QMessageBox::warning(this, "Invalid Knot", e.what());
    }
}

void LC_SplinePropertiesEditingWidget::onAddControlPoint() {
    if (!m_entity) return;

    int row = ui->tableControlPoints->currentRow();
    size_t insertIndex = (row >= 0) ? static_cast<size_t>(row + 1) : m_entity->getNumberOfControlPoints();

    try {
        m_entity->insertControlPoint(insertIndex, RS_Vector(0.0, 0.0), 1.0);
        m_entity->update();
        updateUI();
    } catch (const std::exception& e) {
        QMessageBox::warning(this, "Error", e.what());
    }
}

void LC_SplinePropertiesEditingWidget::onAddControlPointBefore() {
    if (!m_entity) return;

    int row = ui->tableControlPoints->currentRow();
    if (row < 0) return;

    try {
        RS_Vector cp = m_entity->getControlPoints()[row];
        double w = m_entity->getWeight(row);
        m_entity->insertControlPoint(static_cast<size_t>(row), cp, w);
        m_entity->update();
        updateUI();
    } catch (const std::exception& e) {
        QMessageBox::warning(this, "Error", e.what());
    }
}

void LC_SplinePropertiesEditingWidget::onAddControlPointAfter() {
    if (!m_entity) return;

    int row = ui->tableControlPoints->currentRow();
    if (row < 0) return;

    try {
        RS_Vector cp = m_entity->getControlPoints()[row];
        double w = m_entity->getWeight(row);
        m_entity->insertControlPoint(static_cast<size_t>(row + 1), cp, w);
        m_entity->update();
        updateUI();
    } catch (const std::exception& e) {
        QMessageBox::warning(this, "Error", e.what());
    }
}

void LC_SplinePropertiesEditingWidget::onRemoveControlPoint() {
    if (!m_entity) return;

    int row = ui->tableControlPoints->currentRow();
    if (row < 0) return;

    if (m_entity->getNumberOfControlPoints() <= (m_entity->isClosed() ? 3 : static_cast<size_t>(m_entity->getDegree()) + 1)) {
        QMessageBox::warning(this, "Cannot Remove", "Cannot remove control point: minimum number required.");
        return;
    }

    try {
        m_entity->removeControlPoint(static_cast<size_t>(row));
        m_entity->update();
        updateUI();
    } catch (const std::exception& e) {
        QMessageBox::warning(this, "Error", e.what());
    }
}

void LC_SplinePropertiesEditingWidget::showControlPointsContextMenu(const QPoint &pos) {
    if (!m_entity) return;

    QModelIndex index = ui->tableControlPoints->indexAt(pos);
    if (!index.isValid()) return;

    int row = index.row();
    ui->tableControlPoints->setCurrentIndex(index);

    QMenu menu(this);

    QAction *addBeforeAct = new QAction("Add before", this);
    connect(addBeforeAct, &QAction::triggered, this, &LC_SplinePropertiesEditingWidget::onAddControlPointBefore);

    QAction *addAfterAct = new QAction("Add after", this);
    connect(addAfterAct, &QAction::triggered, this, &LC_SplinePropertiesEditingWidget::onAddControlPointAfter);

    QAction *deleteAct = new QAction("Delete", this);
    connect(deleteAct, &QAction::triggered, this, &LC_SplinePropertiesEditingWidget::onRemoveControlPoint);

    menu.addAction(addBeforeAct);
    menu.addAction(addAfterAct);
    menu.addAction(deleteAct);

    menu.exec(ui->tableControlPoints->viewport()->mapToGlobal(pos));
}
