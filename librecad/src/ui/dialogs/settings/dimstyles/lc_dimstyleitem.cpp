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

#include "lc_dimstyleitem.h"

#include "lc_dimstyle.h"

namespace RS2
{
    enum EntityType : unsigned;
}

LC_DimStyleItem::LC_DimStyleItem() : m_dimType{} {
}

LC_DimStyleItem::~LC_DimStyleItem() {
    m_childItems.clear();
}

void LC_DimStyleItem::appendChild(LC_DimStyleItem *item){
    m_childItems.append(item);
    item->setParentItem(this);
}

LC_DimStyleItem *LC_DimStyleItem::child(int row) const {
    return m_childItems.value(row);
}

int LC_DimStyleItem::childCount() const{
    return m_childItems.count();
}

LC_DimStyleItem *LC_DimStyleItem::parentItem() const {
    return m_parentItem;
}

void LC_DimStyleItem::updateNameAndType() {
    QString name = m_dimStyle->getName();
    LC_DimStyle::parseStyleName(name, m_baseName, m_dimType);
    m_displayName = composeDisplayName(m_baseName, m_dimType);
}

QString LC_DimStyleItem::getDisplayDimStyleName(LC_DimStyle* style) {
    QString name = style->getName();
    QString baseName;
    RS2::EntityType entityType;
    LC_DimStyle::parseStyleName(name, baseName, entityType);
    return composeDisplayName(baseName, entityType);
}

bool LC_DimStyleItem::hasUsedChildren() {
    int childCount = m_childItems.count();
    for (int i = 0; i < childCount; ++i) {
        LC_DimStyleItem* child = m_childItems.at(i);
        if (child->usageCount() > 0) {
            return true;
        }
    }
    return false;
}

bool LC_DimStyleItem::isNotUsedInDrawing() {
    return m_usageCount == 0 && !hasUsedChildren();
}

QString LC_DimStyleItem::composeDisplayName(QString baseName, RS2::EntityType entityType) {
    QString result = baseName;
    QString suffix = "";
    switch (entityType) {
        case RS2::EntityDimLinear:
            suffix = tr("Linear");
            result.append(":").append(suffix);
            break;
        case RS2::EntityDimAngular:
            suffix = tr("Angular");
            result.append(":").append(suffix);
            break;
        case RS2::EntityDimDiametric:
            suffix = tr("Diametric");
            result.append(":").append(suffix);
            break;
        case RS2::EntityDimRadial:
            suffix = tr("Radial");
            result.append(":").append(suffix);
            break;
        case RS2::EntityDimOrdinate:
            suffix = tr("Ordinate");
            result.append(":").append(suffix);
            break;
        case RS2::EntityDimLeader:
            suffix = tr("Leader and Tolerance");
            result.append(":").append(suffix);
            break;
        default:
            break;
    }
    return result;
}

int LC_DimStyleItem::row() const {
    if (m_parentItem != nullptr) {
        return m_parentItem->m_childItems.indexOf(const_cast<LC_DimStyleItem*>(this));
    }
    return 0;
}

LC_DimStyleItem* LC_DimStyleItem::findByName(const QString& name) const {
    int childCount = m_childItems.count();
    for (int i = 0; i < childCount; ++i) {
        LC_DimStyleItem* child = m_childItems.at(i);
        if (child->dimStyle()->getName().compare(name, Qt::CaseInsensitive) == 0) {
            return child;
        }
        LC_DimStyleItem* foundChild = child->findByName(name);
        if (foundChild != nullptr) {
            return foundChild;
        }
    }
    return nullptr;
}

LC_DimStyleItem* LC_DimStyleItem::findBaseStyleItem(const QString& baseStyleName) const {
    int childCount = m_childItems.count();
    for (int i = 0; i < childCount; ++i) {
        LC_DimStyleItem* child = m_childItems.at(i);
        if (child->baseName().compare(baseStyleName, Qt::CaseInsensitive) == 0) {
            return child;
        }
    }
    return nullptr;
}

// fixme - sand - rework later, create more uniform generic
// tree model, with search based on acceptors etc..
LC_DimStyleItem* LC_DimStyleItem::findActive() const {
    int childCount = m_childItems.count();
    for (int i = 0; i < childCount; ++i) {
        LC_DimStyleItem* child = m_childItems.at(i);
        if (child->isActive()) {
            return child;
        }
        LC_DimStyleItem* foundChild = child->findActive();
        if (foundChild != nullptr) {
            return foundChild;
        }
    }
    return nullptr;
}

LC_DimStyleItem* LC_DimStyleItem::findEntityStyleItem() const {
    int childCount = m_childItems.count();
    for (int i = 0; i < childCount; ++i) {
        LC_DimStyleItem* child = m_childItems.at(i);
        if (child->isEntityStyleItem()) {
            return child;
        }
        LC_DimStyleItem* foundChild = child->findEntityStyleItem();
        if (foundChild != nullptr) {
            return foundChild;
        }
    }
    return nullptr;
}

void LC_DimStyleItem::cleanup(bool deleteDimStyles) {
    int childCount = m_childItems.count();
    for (int i = 0; i < childCount; ++i) {
        LC_DimStyleItem* child = m_childItems.at(i);
        child->cleanup(deleteDimStyles);
        auto dimStyle = child->dimStyle();
        if (deleteDimStyles) {
            delete dimStyle;
        }
        delete child;
    }
    m_childItems.clear();
}

void LC_DimStyleItem::removeChild(LC_DimStyleItem* item) {
    m_childItems.removeAll(item);
    item->setParentItem(nullptr);
}

void LC_DimStyleItem::collectChildren(QList<LC_DimStyleItem*>& items) {
    int childCount = m_childItems.count();
    for (int i = 0; i < childCount; ++i) {
        LC_DimStyleItem* child = m_childItems.at(i);
        items.push_back(child);
        child->collectChildren(items);
    }
}

void LC_DimStyleItem::setNewBaseName(const QString& newBaseName) {
    if (m_dimType == RS2::EntityUnknown) {
        m_dimStyle->setName(newBaseName);
    }
    else {
        QString newName = LC_DimStyle::getStyleNameForBaseAndType(newBaseName, m_dimType);
        m_dimStyle->setName(newName);
    }
    m_baseName = newBaseName;
    m_displayName = composeDisplayName(m_baseName, m_dimType);
    int childCount = m_childItems.count();
    for (int i = 0; i < childCount; ++i) {
        LC_DimStyleItem* child = m_childItems.at(i);
        child->setNewBaseName(newBaseName);
    }
}
