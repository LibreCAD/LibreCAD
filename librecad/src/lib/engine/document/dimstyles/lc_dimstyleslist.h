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

#ifndef LC_DIMSTYLESLIST_H
#define LC_DIMSTYLESLIST_H

#include <memory>
#include <QList>
#include "rs.h"

class LC_DimStyle;

class LC_DimStylesList{
public:
    LC_DimStylesList();
    virtual ~LC_DimStylesList();
    LC_DimStyle* findByName(const QString& name) const;
    LC_DimStyle* findByBaseNameAndType(const QString &name, RS2::EntityType dimType) const;
    LC_DimStyle* resolveByBaseName(const QString& name, RS2::EntityType dimType) const;
    LC_DimStyle* resolveByName(const QString& name, RS2::EntityType dimType) const;
    void addDimStyle(LC_DimStyle* style);
    void deleteDimStyle(QString &name);
    int size() const {return m_stylesList.size();}
    void clear();
    void mergeStyles();
    const QList<LC_DimStyle*>* getStylesList(){return &m_stylesList;}
    LC_DimStyle* getFallbackDimStyleFromVars() const {return m_fallbackDimStyleFromVars.get();}
    void replaceStyles(const QList<LC_DimStyle*>& list);
    void setModified(bool m) {m_modified = m;}
    bool isEmpty() {return m_stylesList.isEmpty();}
    virtual bool isModified() const { return m_modified;}
protected:
    /** Flag set if the layer list was modified and not yet saved. */
    bool m_modified = false;
    QList<LC_DimStyle*> m_stylesList;
    std::unique_ptr<LC_DimStyle> m_fallbackDimStyleFromVars;
};

#endif // LC_DIMSTYLESLIST_H
