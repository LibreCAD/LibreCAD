/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

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
 ******************************************************************************/

#include "lc_dimstyleslist.h"

#include "lc_dimstyle.h"

LC_DimStylesList::LC_DimStylesList() {
    m_fallbackDimStyleFromVars = std::make_unique<LC_DimStyle>();
    m_fallbackDimStyleFromVars->setFromVars(true);
}

LC_DimStylesList::~LC_DimStylesList() {
    qDeleteAll(m_stylesList);
}

LC_DimStyle *LC_DimStylesList::findByName(const QString &name) const {
    //
    for (auto v: m_stylesList){
        if (v->getName().compare(name, Qt::CaseInsensitive) == 0){
            return v;
        }
    }
    return nullptr;
}

LC_DimStyle *LC_DimStylesList::findByBaseNameAndType(const QString &name, RS2::EntityType dimType) const {
    if (dimType == RS2::EntityUnknown) {
        return findByName(name);
    }
    QString nameSuffix = LC_DimStyle::getDimStyleNameSuffixForType(dimType);
    QString nameToFind = name + nameSuffix;
    auto result = findByName(nameToFind);
    if (result == nullptr) {
        if (dimType == RS2::EntityDimAligned) {
            // fall back to style for linear
            nameToFind = LC_DimStyle::getStyleNameForBaseAndType(name, RS2::EntityDimLinear);
            return findByName(nameToFind);
        }
        if (dimType == RS2::EntityTolerance) {
            // fall back to style for linear
            nameToFind = LC_DimStyle::getStyleNameForBaseAndType(name, RS2::EntityDimLeader);
            return findByName(nameToFind);
        }
    }
    return nullptr;
}


LC_DimStyle* LC_DimStylesList::resolveByBaseName(const QString& name, RS2::EntityType dimType) const {
    QString nameSuffix = LC_DimStyle::getDimStyleNameSuffixForType(dimType);
    if (nameSuffix.isEmpty()) {
        return findByName(name);
    }

    QString nameForType = name + nameSuffix;
    LC_DimStyle* res = findByName(nameForType);
    if (res == nullptr) {
        if (dimType == RS2::EntityDimAligned) {
            // fall back to style for linear
            nameForType = LC_DimStyle::getStyleNameForBaseAndType(name, RS2::EntityDimLinear);
            res = findByName(nameForType);
            if (res != nullptr) {
                return res;
            }
        }
        else if (dimType == RS2::EntityTolerance) {
            // fall back to style for leader
            nameForType = LC_DimStyle::getStyleNameForBaseAndType(name, RS2::EntityDimLeader);
            res = findByName(nameForType);
            if (res != nullptr) {
                return res;
            }
        }
        return findByName(name);
    }
    return res;
}

LC_DimStyle* LC_DimStylesList::resolveByName(const QString& name, RS2::EntityType dimType) const {
    LC_DimStyle* res = nullptr;

    // first, try to resolve style by its exact name. AutoCAD stores complete style name for type in DIMENSION
    if (name.contains(LC_DimStyle::NAME_SEPARATOR)) {
        res = findByName(name);
        if (res != nullptr) {
            return res;
        }
        // try to resolve name by base name
        QString baseName;
        RS2::EntityType type;
        LC_DimStyle::parseStyleName(name, baseName, type);
        return resolveByBaseName(baseName, dimType);
    }
    return resolveByBaseName(name, dimType);
}


void LC_DimStylesList::addDimStyle(LC_DimStyle *style) {
    // fixme - sand - dims - check for duplicated name?
    m_stylesList.append(style);
    setModified(true);
}

void LC_DimStylesList::deleteDimStyle([[maybe_unused]]QString &name) {
    // fixme - sand - is it actually needed?
   setModified(true);
}

void LC_DimStylesList::clear() {
    m_stylesList.clear();
    setModified(true);
}

void LC_DimStylesList::mergeStyles() {

    QMap<QString, LC_DimStyle*> baseStyles;
    QList<LC_DimStyle*> entityTypeStyles;
    for (auto ds: m_stylesList) {
        QString baseName;
        RS2::EntityType entityType;
        LC_DimStyle::parseStyleName(ds->getName(), baseName, entityType);
        if (entityType == RS2::EntityUnknown) {
            baseStyles[baseName] = ds;
        }
        else {
            entityTypeStyles.append(ds);
        }
    }

    for (auto typeSpecificStyle: entityTypeStyles) {
        QString baseName;
        RS2::EntityType entityType;
        LC_DimStyle::parseStyleName(typeSpecificStyle->getName(), baseName, entityType);

        if (baseStyles.contains(baseName)) {
            auto baseStyle = baseStyles[baseName];
            auto savedCheckMode = typeSpecificStyle->getModifyCheckMode();

            // merge type-specific and basic style, so the type-specific style for all explicitly modified properties
            // will store own explicit values, and for properties that are not changed - their values will be set from the base style.
            typeSpecificStyle->mergeWith(baseStyle, LC_DimStyle::ModificationAware::UNSET, savedCheckMode);
        }
    }
}

void LC_DimStylesList::replaceStyles(const QList<LC_DimStyle*>& list) {
    qDeleteAll(m_stylesList);
    m_stylesList.clear();
    m_stylesList.append(list);
    mergeStyles();
    setModified(true);
}
