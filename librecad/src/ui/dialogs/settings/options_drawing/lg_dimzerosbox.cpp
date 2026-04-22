/*
**********************************************************************************
**
** This file is part of the LibreCAD project (librecad.org), a 2D CAD program.
**
** Copyright (C) 2024 LibreCAD.org
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**
**********************************************************************************
 */

#include "lg_dimzerosbox.h"

#include <QListView>
#include <QStandardItemModel>

LG_DimzerosBox::LG_DimzerosBox(QWidget* parent) : QComboBox(parent) {
    m_dimLine = false;
    m_view = new QListView();
    m_model = new QStandardItemModel(3, 1);
    auto item = new QStandardItem(tr("select:"));
    item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    m_model->setItem(0, 0, item);
    item = new QStandardItem(tr("remove left"));
    item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
    item->setData(Qt::Unchecked, Qt::CheckStateRole);
    m_model->setItem(1, 0, item);
    item = new QStandardItem(tr("remove right"));
    item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
    item->setData(Qt::Unchecked, Qt::CheckStateRole);
    m_model->setItem(2, 0, item);

    LG_DimzerosBox::setModel(m_model);
    setView(m_view);
    setEditable(false);
    setEditText("selectar:");
}

LG_DimzerosBox::~LG_DimzerosBox() {
    delete m_model;
    delete m_view;
}

void LG_DimzerosBox::setLinear() {
    m_dimLine = true;
    auto item = new QStandardItem(tr("remove 0'"));
    item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
    item->setData(Qt::Unchecked, Qt::CheckStateRole);
    m_model->appendRow(item);
    item = new QStandardItem(tr("remove 0\""));
    item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
    item->setData(Qt::Unchecked, Qt::CheckStateRole);
    m_model->appendRow(item);
}

void LG_DimzerosBox::setData(const int i) const {
    if (m_dimLine) {
        if (i & 1) {
            if (i & 2) {
                m_model->item(3)->setCheckState(Qt::Checked);
            }
        }
        else {
            m_model->item(4)->setCheckState(Qt::Checked);
            if ((i & 2) == 0) {
                m_model->item(3)->setCheckState(Qt::Checked);
            }
        }
        if (i & 4) {
            m_model->item(1)->setCheckState(Qt::Checked);
        }
        if (i & 8) {
            m_model->item(2)->setCheckState(Qt::Checked);
        }
    }
    else {
        if (i & 1) {
            m_model->item(1)->setCheckState(Qt::Checked);
        }
        if (i & 2) {
            m_model->item(2)->setCheckState(Qt::Checked);
        }
    }
}

int LG_DimzerosBox::getData() const {
    int ret = 0;
    if (m_dimLine) {
        if (m_model->item(1)->checkState() == Qt::Checked) {
            ret |= 4;
        }
        if (m_model->item(2)->checkState() == Qt::Checked) {
            ret |= 8;
        }
        //imperial:
        if (m_model->item(3)->checkState() == Qt::Checked) {
            if (m_model->item(4)->checkState() == Qt::Unchecked) {
                ret |= 3;
            }
        }
        else {
            if (m_model->item(4)->checkState() == Qt::Checked) {
                ret |= 2;
            }
            else {
                ret |= 1;
            }
        }
    }
    else {
        if (m_model->item(1)->checkState() == Qt::Checked) {
            ret |= 1;
        }
        if (m_model->item(2)->checkState() == Qt::Checked) {
            ret |= 2;
        }
    }
    return ret;
}

/**
 * helper function for DIMZIN var.
 */
int LG_DimzerosBox::convertDimZin(const int v, const bool toIdx) {
    if (toIdx) {
        if (v < 5) {
            return 0;
        }
        int res = 0;
        if (v & 4) {
            res = 3;
        }
        if (v & 8) {
            return (res == 3) ? 5 : 4;
        }
    }
    //toIdx false
    switch (v) {
        case 3:
            return 4;
        case 4:
            return 8;
        case 5:
            return 12;
        default:
            break;
    }
    return 1;
}
