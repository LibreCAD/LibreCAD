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

#include "lc_entitymetauiutils.h"

#include <QComboBox>
#include <QSet>

#include "rs.h"

void LC_EntityMetaUIUtils::setupSelectionEntityTypesCombobox(QComboBox* entityTypeCombobox, const QMap<RS2::EntityType, int>& map, const bool addAll) {
    setupSelectionEntityTypesCombobox(entityTypeCombobox, map, getEntityTypeNamesList(), addAll);
}

void LC_EntityMetaUIUtils::setupSelectionEntityTypesCombobox(QComboBox* entityTypeCombobox, const QMap<RS2::EntityType, int>& map,
                                                             const std::vector<QPair<QString, RS2::EntityType>>& entityTypes, const bool addAll) {
    int itemIndexToSelect = 0;
    if (map.empty()) {
        if (addAll) {
            entityTypeCombobox->addItem(tr("No Selection"), RS2::EntityType::EntityUnknown);
        }
    }
    else {
        int totalCount = 0;
        for (const auto& [name, type] : entityTypes) {
            const bool contains = map.contains(type);
            if (contains) {
                const int count = map[type];
                totalCount += count;
                QString label = name + " (" + QString::number(count) + ")";
                entityTypeCombobox->addItem(label, type);
            }
        }
        if (addAll) {
            entityTypeCombobox->insertItem(0, tr("All") + " (" + QString::number(totalCount) + ")", RS2::EntityType::EntityContainer);
        }
        if (totalCount == 1) {
            itemIndexToSelect = 1; // select the item for the only entity type selected
        }
    }
    if (addAll) {
        entityTypeCombobox->setCurrentIndex(itemIndexToSelect);
    }
}

void LC_EntityMetaUIUtils::setupEntityTypesCombobox(QComboBox* entityTypeCombobox, const QSet<RS2::EntityType>& set,
                                                    const std::vector<QPair<QString, RS2::EntityType>>& entityTypes) {
    for (const auto& p : entityTypes) {
        auto type = p.second;
        if (set.contains(type)) {
            QString name = p.first;
            entityTypeCombobox->addItem(name, type);
        }
    }
}

const std::vector<QPair<QString, RS2::EntityType>>& LC_EntityMetaUIUtils::getEntityTypeNamesList() {
    static const std::vector<QPair<QString, RS2::EntityType>>& ENTITY_TYPES = {
        {tr("Line"), RS2::EntityType::EntityLine},
        {tr("Circle"), RS2::EntityType::EntityCircle},
        {tr("Arc"), RS2::EntityType::EntityArc},
        {tr("Polyline"), RS2::EntityType::EntityPolyline},
        {tr("Block"), RS2::EntityType::EntityBlock},
        {tr("Point"), RS2::EntityType::EntityPoint},
        {tr("Spline"), RS2::EntityType::EntitySpline},
        {tr("Spline By Points"), RS2::EntityType::EntitySplinePoints},
        {tr("Ellipse"), RS2::EntityType::EntityEllipse},
        {tr("Hyperbola"), RS2::EntityType::EntityHyperbola},
        {tr("Text"), RS2::EntityType::EntityText},
        {tr("MText"), RS2::EntityType::EntityMText},
        {tr("Dimension Aligned"), RS2::EntityType::EntityDimAligned},
        {tr("Dimension Linear"), RS2::EntityType::EntityDimLinear},
        {tr("Dimension Ordinate"), RS2::EntityType::EntityDimOrdinate},
        {tr("Dimension Angular"), RS2::EntityType::EntityDimAngular},
        {tr("Dimension Radial"), RS2::EntityType::EntityDimRadial},
        {tr("Dimension Diametric"), RS2::EntityType::EntityDimDiametric},
        {tr("Dimension Arc"), RS2::EntityType::EntityDimArc},
        {tr("Hatch"), RS2::EntityType::EntityHatch},
        {tr("Leader"), RS2::EntityType::EntityDimLeader},
        {tr("Parabola"), RS2::EntityType::EntityParabola},
        {tr("Image"), RS2::EntityType::EntityImage},
        {tr("Insert"), RS2::EntityType::EntityInsert},
    };
    return ENTITY_TYPES;
}

void LC_EntityMetaUIUtils::setupEntitiesTypesList(QComboBox* entityTypeCombobox, const QSet<RS2::EntityType>& set) {
    setupEntityTypesCombobox(entityTypeCombobox, set, getEntityTypeNamesList());
}
