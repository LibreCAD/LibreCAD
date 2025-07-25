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

#ifndef LC_DIMSTYLEITEM_H
#define LC_DIMSTYLEITEM_H


#include <QString>

#include "lc_dimstyle.h"
#include "rs.h"

class LC_DimStyle;

class LC_DimStyleItem : public QObject {
    Q_OBJECT
public:
    LC_DimStyleItem();

    LC_DimStyleItem(LC_DimStyle* dimStyle, int usageCount,
                    bool current):
          m_dimStyle{dimStyle},
          m_usageCount{usageCount},
          m_active{current} {
        updateNameAndType();
    }

    ~LC_DimStyleItem() override;
    void appendChild(LC_DimStyleItem* item);
    int childCount() const;
    LC_DimStyleItem* child(int row) const;
    QString displayName() const { return m_displayName; }
    bool isActive() const { return m_active; }
    bool isFromVariables() const { return m_dimStyle->isFromVars(); }
    int usageCount() const { return m_usageCount; }
    LC_DimStyleItem* parentItem() const;
    void setParentItem(LC_DimStyleItem* root) {m_parentItem = root;}
    void setActive(bool value) {
        m_active = value;
    }
    LC_DimStyle* dimStyle() const { return m_dimStyle; }
    void setName(const QString& name) { m_displayName = name; }
    RS2::EntityType forDimensionType() const { return m_dimType; }
    bool isBaseStyle() const { return m_dimType == RS2::EntityUnknown; }
    QString baseName() const { return m_baseName; }
    void updateNameAndType();
    int row() const;

    LC_DimStyleItem* findByName(const QString& name) const;
    LC_DimStyleItem* findBaseStyleItem(const QString& baseStyleName) const;
    LC_DimStyleItem* findActive() const;
    LC_DimStyleItem* findEntityStyleItem() const;
    void cleanup(bool deleteDimStyles);
    void removeChild(LC_DimStyleItem* item);
    void collectChildren(QList<LC_DimStyleItem*>& items);
    void setNewBaseName(const QString& newBaseName);
    static QString composeDisplayName(QString baseName, RS2::EntityType entityType);
    static QString getDisplayDimStyleName(LC_DimStyle* style);
    bool hasUsedChildren();
    bool isNotUsedInDrawing();
    bool isOverrideItem() const {return m_overrideItem;}
    void setOverrideItem(bool val) {m_overrideItem = val;}
    bool isEntityStyleItem() const {return m_entityStyle;}
    void setEntityStyleItem(bool val) {m_entityStyle = val;}
    bool isUnsaved(){return m_unsavedItem;}
    void setUnsaved(bool value){m_unsavedItem = value;}
private:
    LC_DimStyle* m_dimStyle{nullptr};
    int m_usageCount{0};
    QString m_displayName;
    QString m_baseName;
    RS2::EntityType m_dimType {RS2::EntityUnknown};
    bool m_active{false};
    bool m_overrideItem {false};
    bool m_unsavedItem{false};
    bool m_entityStyle{false};
    // children of this item
    QList<LC_DimStyleItem*> m_childItems;
    LC_DimStyleItem* m_parentItem{nullptr};
};
#endif // LC_DIMSTYLEITEM_H
