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

#ifndef LC_ENTITYMETAUTILS_H
#define LC_ENTITYMETAUTILS_H

#include <QObject>
#include <QSet>
#include <rs.h>

class QComboBox;

class LC_EntityMetaUIUtils: public QObject{
    Q_OBJECT
public:
    static void setupSelectionEntityTypesCombobox(QComboBox* entityTypeCombobox, const QMap<RS2::EntityType, int>& map, bool addAll = true);
    static void setupSelectionEntityTypesCombobox(QComboBox* entityTypeCombobox, const QMap<RS2::EntityType, int>& map, const std::vector<QPair<QString, RS2::EntityType>>& entityTypes, bool addAll);
    static void setupEntityTypesCombobox(QComboBox* entityTypeCombobox, const QSet<RS2::EntityType>& set, const std::vector<QPair<QString, RS2::EntityType>>& entityTypes);
    static const std::vector<QPair<QString, RS2::EntityType>>& getEntityTypeNamesList();
    static void setupEntitiesTypesList(QComboBox* entityTypeCombobox, const QSet<RS2::EntityType>& set);
};
#endif
