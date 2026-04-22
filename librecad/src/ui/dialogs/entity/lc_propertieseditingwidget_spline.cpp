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

#include "lc_propertieseditingwidget_spline.h"

#include <QAction>
#include <QMenu>
#include <QMessageBox>
#include <QSignalBlocker>
#include <stdexcept>

#include "rs_spline.h"
#include "ui_lc_propertieseditingwidget_spline.h"

LC_PropertiesEditingWidgetSpline::LC_PropertiesEditingWidgetSpline(QWidget *parent) :
    LC_EntityPropertiesEditorWidget(parent),
    ui(new Ui::LC_SplinePropertiesEditingWidget)
{
    ui->setupUi(this);
    ui->tableControlPoints->setContextMenuPolicy(Qt::CustomContextMenu);
    // hide extra index columns
    ui->tableControlPoints->verticalHeader()->setVisible(false);
    ui->tableKnots->verticalHeader()->setVisible(false);

    // Connect table signals
    connect(ui->tableControlPoints, &QTableWidget::itemChanged,
            this, &LC_PropertiesEditingWidgetSpline::onControlPointChanged);
    connect(ui->tableKnots, &QTableWidget::itemChanged,
            this, &LC_PropertiesEditingWidgetSpline::onKnotChanged);

    // Connect button signals
    connect(ui->btnAddControlPoint, &QPushButton::clicked,
            this, &LC_PropertiesEditingWidgetSpline::onAddControlPoint);
    connect(ui->btnRemoveControlPoint, &QPushButton::clicked,
            this, &LC_PropertiesEditingWidgetSpline::onRemoveControlPoint);

    // Enable/disable Remove button based on selection
    connect(ui->tableControlPoints->selectionModel(),
            &QItemSelectionModel::currentRowChanged,
            this, [this] {
                ui->btnRemoveControlPoint->setEnabled(ui->tableControlPoints->currentRow() >= 0);
            });

    // Initialize Remove button state
    ui->btnRemoveControlPoint->setEnabled(false);

    connect(ui->tableControlPoints, &QTableWidget::customContextMenuRequested,
            this, &LC_PropertiesEditingWidgetSpline::showControlPointsContextMenu);
}

LC_PropertiesEditingWidgetSpline::~LC_PropertiesEditingWidgetSpline() = default;

void LC_PropertiesEditingWidgetSpline::setEntity(RS_Entity *entity) {
    m_entity = dynamic_cast<RS_Spline*>(entity);
    if (m_entity != nullptr) {
        updateUI();
    }
}

void LC_PropertiesEditingWidgetSpline::updateUI() const {
    const auto tableControlPoints = ui->tableControlPoints;
    if (m_entity == nullptr) {
        tableControlPoints->setRowCount(0);
        ui->tableKnots->setRowCount(0);
        ui->cbClosed->setChecked(false);
        ui->cbDegree->setCurrentIndex(-1);
        return;
    }

    QSignalBlocker cpBlocker(tableControlPoints);
    QSignalBlocker knotBlocker(ui->tableKnots);

    ui->cbClosed->setChecked(m_entity->isClosed());
    ui->cbDegree->setCurrentIndex(m_entity->getDegree() - 1);

    const size_t numCP = m_entity->getNumberOfControlPoints();
    tableControlPoints->setRowCount(static_cast<int>(numCP));
    const auto& controlPoints = m_entity->getControlPoints();
    for (size_t i = 0; i < numCP; ++i) {
        auto *itemNum = new QTableWidgetItem(QString::number(i + 1));
        itemNum->setFlags(itemNum->flags() & ~Qt::ItemIsEditable);
        tableControlPoints->setItem(static_cast<int>(i), 0, itemNum);
        tableControlPoints->setItem(static_cast<int>(i), 1, new QTableWidgetItem(QString::number(controlPoints[i].x)));
        tableControlPoints->setItem(static_cast<int>(i), 2, new QTableWidgetItem(QString::number(controlPoints[i].y)));
        tableControlPoints->setItem(static_cast<int>(i), 3, new QTableWidgetItem(QString::number(m_entity->getWeight(i))));
    }

    const size_t numKnots = m_entity->getKnotVector().size();
    const std::vector<double> knots = m_entity->getKnotVector();
    ui->tableKnots->setRowCount(static_cast<int>(numKnots));
    for (size_t i = 0; i < numKnots; ++i) {
        auto *itemNum = new QTableWidgetItem(QString::number(i + 1));
        itemNum->setFlags(itemNum->flags() & ~Qt::ItemIsEditable);
        ui->tableKnots->setItem(static_cast<int>(i), 0, itemNum);
        ui->tableKnots->setItem(static_cast<int>(i), 1, new QTableWidgetItem(QString::number(knots[i])));
    }
}

void LC_PropertiesEditingWidgetSpline::onClosedToggled(const bool checked){
    if (m_entity != nullptr) {
        try {
            m_entity->setClosed(checked);
            m_entity->update();
            updateUI();
        } catch (const std::exception& e) {
            QMessageBox::warning(this, "Error", e.what());
        }
    }
}

void LC_PropertiesEditingWidgetSpline::onDegreeIndexChanged(const int index) {
    if (m_entity != nullptr) {
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

void LC_PropertiesEditingWidgetSpline::onControlPointChanged(QTableWidgetItem *item) {
    if (nullptr == m_entity || nullptr == item) {
        return;
    }

    const int row = item->row();
    const int col = item->column();

    // Ignore index column (non-editable)
    if (col == 0 || row < 0 || static_cast<size_t>(row) >= m_entity->getNumberOfControlPoints()) {
        return;
    }

    bool ok = false;
    const double val = item->text().toDouble(&ok);
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
            if (col == 1) {
                cp.x = val;
            }
            else {
                cp.y = val;
            }
            m_entity->setControlPoint(static_cast<size_t>(row), cp);
        } else if (col == 3) {
            m_entity->setWeight(static_cast<size_t>(row), val);
        }
        m_entity->update();
        // Validate knot vector size after change (though editing doesn't change size)
        const size_t expected_knots = m_entity->getNumberOfKnots();
        if (m_entity->getKnotVector().size() != expected_knots) {
            std::vector<double> new_knots(expected_knots);
            const double min_u = 0.0; // fixme - sand -  why there values are never changed??
            const double max_u = 1.0;
            for (size_t i = 0; i < expected_knots; ++i) {
                new_knots[i] = min_u + (max_u - min_u) * static_cast<double>(i) / static_cast<double>(expected_knots - 1);
            }
            m_entity->setKnotVector(new_knots);
            m_entity->update();
            QMessageBox::information(this, "Knot Vector Adjusted", "Knot vector size mismatch detected. Regenerated uniform knot vector.");
        }
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

void LC_PropertiesEditingWidgetSpline::onKnotChanged(QTableWidgetItem *item) {
    if (nullptr == m_entity || nullptr == item) {
        return;
    }

    const int row = item->row();
    const int col = item->column();

    // Ignore index column
    if (col != 1 || row < 0) {
        return;
    }

    bool ok = false;
    const double val = item->text().toDouble(&ok);
    if (!ok) {
        item->setText(QString::number(m_entity->getKnotVector()[row]));
        QMessageBox::warning(this, "Invalid Input", "Please enter a valid number.");
        return;
    }

    try {
        std::vector<double> knots = m_entity->getKnotVector();
        if (static_cast<size_t>(row) >= knots.size()) {
            return;
        }
        knots[row] = val;
        // Validate the knot vector is non-decreasing
        bool valid = true;
        for (size_t i = 1; i < knots.size(); ++i) {
            if (knots[i] < knots[i-1]) {
                valid = false;
                break;
            }
        }
        if (valid) {
            m_entity->setKnotVector(knots);
            m_entity->update();
        } else {
            item->setText(QString::number(m_entity->getKnotVector()[row]));
            QMessageBox::warning(this, "Invalid Knot", "Knot vector must be non-decreasing");
        }
    } catch (const std::invalid_argument& e) {
        item->setText(QString::number(m_entity->getKnotVector()[row]));
        QMessageBox::warning(this, "Invalid Knot", e.what());
    }
}

void LC_PropertiesEditingWidgetSpline::onAddControlPoint() {
    if (nullptr == m_entity) {
        return;
    }

    const int row = ui->tableControlPoints->currentRow();
    const size_t insertIndex = (row >= 0) ? (row + 1) : m_entity->getNumberOfControlPoints();

    // Get old knot vector
    const std::vector<double> old_knots = m_entity->getKnotVector();
    const bool was_custom = !m_entity->getData().knotslist.empty();

    try {
        m_entity->insertControlPoint(insertIndex, RS_Vector(0.0, 0.0), 1.0);
        m_entity->update();
        if (was_custom) {
            const size_t new_size = m_entity->getNumberOfKnots();
            std::vector<double> new_knots(new_size);
            const double min_u = old_knots.front();
            const double max_u = old_knots.back();
            for (size_t i = 0; i < new_size; ++i) {
                new_knots[i] = min_u + (max_u - min_u) * static_cast<double>(i) / static_cast<double>(new_size - 1);
            }
            m_entity->setKnotVector(new_knots);
            m_entity->update();
        }
        updateUI();
    } catch (const std::exception& e) {
        QMessageBox::warning(this, "Error", e.what());
    }
}

void LC_PropertiesEditingWidgetSpline::onAddControlPointBefore() {
    if (nullptr == m_entity) {
        return;
    }

    const int row = ui->tableControlPoints->currentRow();
    if (row < 0) {
        return;
    }

    // Get old knot vector
    const std::vector<double> old_knots = m_entity->getKnotVector();
    const bool was_custom = !m_entity->getData().knotslist.empty();

    try {
        const RS_Vector cp = m_entity->getControlPoints()[row];
        const double w = m_entity->getWeight(row);
        m_entity->insertControlPoint(static_cast<size_t>(row), cp, w);
        m_entity->update();
        if (was_custom) {
            const size_t new_size = m_entity->getNumberOfKnots();
            std::vector<double> new_knots(new_size);
            const double min_u = old_knots.front();
            const double max_u = old_knots.back();
            for (size_t i = 0; i < new_size; ++i) {
                new_knots[i] = min_u + (max_u - min_u) * static_cast<double>(i) / static_cast<double>(new_size - 1);
            }
            m_entity->setKnotVector(new_knots);
            m_entity->update();
        }
        updateUI();
    } catch (const std::exception& e) {
        QMessageBox::warning(this, "Error", e.what());
    }
}

void LC_PropertiesEditingWidgetSpline::onAddControlPointAfter() {
    if (nullptr == m_entity) {
        return;
    }
    const int row = ui->tableControlPoints->currentRow();
    if (row < 0) {
        return;
    }

    // Get old knot vector
    const std::vector<double> old_knots = m_entity->getKnotVector();
    const bool was_custom = !m_entity->getData().knotslist.empty();

    try {
        const RS_Vector cp = m_entity->getControlPoints()[row];
        const double w = m_entity->getWeight(row);
        m_entity->insertControlPoint(row + 1, cp, w);
        m_entity->update();
        if (was_custom) {
            const size_t new_size = m_entity->getNumberOfKnots();
            std::vector<double> new_knots(new_size);
            const double min_u = old_knots.front();
            const double max_u = old_knots.back();
            for (size_t i = 0; i < new_size; ++i) {
                new_knots[i] = min_u + (max_u - min_u) * static_cast<double>(i) / static_cast<double>(new_size - 1);
            }
            m_entity->setKnotVector(new_knots);
            m_entity->update();
        }
        updateUI();
    } catch (const std::exception& e) {
        QMessageBox::warning(this, "Error", e.what());
    }
}

void LC_PropertiesEditingWidgetSpline::onRemoveControlPoint() {
    if (nullptr == m_entity) {
        return;
    }

    const int row = ui->tableControlPoints->currentRow();
    if (row < 0) {
        return;
    }

    if (m_entity->getNumberOfControlPoints() <= (m_entity->isClosed() ? 3 : static_cast<size_t>(m_entity->getDegree()) + 1)) {
        QMessageBox::warning(this, "Cannot Remove", "Cannot remove control point: minimum number required.");
        return;
    }

    // Get old knot vector
    const std::vector<double> old_knots = m_entity->getKnotVector();
    const bool was_custom = !m_entity->getData().knotslist.empty();

    try {
        m_entity->removeControlPoint(static_cast<size_t>(row));
        m_entity->update();
        if (was_custom) {
            const size_t new_size = m_entity->getNumberOfKnots();
            std::vector<double> new_knots(new_size);
            const double min_u = old_knots.front();
            const double max_u = old_knots.back();
            for (size_t i = 0; i < new_size; ++i) {
                new_knots[i] = min_u + (max_u - min_u) * static_cast<double>(i) / static_cast<double>(new_size - 1);
            }
            m_entity->setKnotVector(new_knots);
            m_entity->update();
        }
        updateUI();
    } catch (const std::exception& e) {
        QMessageBox::warning(this, "Error", e.what());
    }
}

void LC_PropertiesEditingWidgetSpline::showControlPointsContextMenu(const QPoint &pos) {
    if (nullptr == m_entity) {
        return;
    }

    const QModelIndex index = ui->tableControlPoints->indexAt(pos);
    if (!index.isValid()) {
        return;
    }

    ui->tableControlPoints->setCurrentIndex(index);

    QMenu menu(this);

    auto *addBeforeAct = new QAction("Add before", this);
    connect(addBeforeAct, &QAction::triggered, this, &LC_PropertiesEditingWidgetSpline::onAddControlPointBefore);

    const auto addAfterAct = new QAction("Add after", this);
    connect(addAfterAct, &QAction::triggered, this, &LC_PropertiesEditingWidgetSpline::onAddControlPointAfter);

    const auto deleteAct = new QAction("Delete", this);
    connect(deleteAct, &QAction::triggered, this, &LC_PropertiesEditingWidgetSpline::onRemoveControlPoint);

    menu.addAction(addBeforeAct);
    menu.addAction(addAfterAct);
    menu.addAction(deleteAct);

    menu.exec(ui->tableControlPoints->viewport()->mapToGlobal(pos));
}
