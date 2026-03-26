// *********************************************************************************
// This file is part of the LibreCAD project, a 2D CAD program
//
// Copyright (C) 2025 LibreCAD.org
// Copyright (C) 2025 sand1024
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
// *********************************************************************************

#ifndef LC_PROPERTYCONTAINER_H
#define LC_PROPERTYCONTAINER_H

#include "lc_property_atomic.h"

class LC_PropertyContainer : public LC_Property {
    Q_OBJECT Q_DISABLE_COPY(LC_PropertyContainer)

public:
    explicit LC_PropertyContainer(QObject* parent = nullptr);
    ~LC_PropertyContainer() override;

    inline bool hasChildProperties() const;
    inline const QList<LC_Property*>& childProperties() const;
    QList<LC_Property*> findChildProperties(QString name, Qt::FindChildOptions options = Qt::FindChildrenRecursively) const;
    QList<LC_Property*> findChildProperties(const QRegularExpression& re, Qt::FindChildOptions options = Qt::FindChildrenRecursively) const;

    void clearChildProperties(bool emitSignals = true);
    bool addChildProperty(LC_Property* childProperty, bool moveOwnership = true, int index = -1);
    bool removeChildProperty(LC_Property* childProperty);
    LC_PropertyContainer* asContainer() override;
    const LC_PropertyContainer* asContainer() const override;

    bool isContainer() const override {
        return true;
    }

    int getTag() const {return m_tag;}
    void setTag(int value){m_tag = value;}

protected:
    void updateStateInherited(bool force) override;
private:
    void findChildPropertiesRecursive(const QString& name, QList<LC_Property*>& result);
    void findChildPropertiesRecursive(const QRegularExpression& re, QList<LC_Property*>& result);

    QList<LC_Property*> m_childProperties;

    int m_tag = -1;
};

bool LC_PropertyContainer::hasChildProperties() const {
    return !m_childProperties.empty();
}

const QList<LC_Property*>& LC_PropertyContainer::childProperties() const {
    return m_childProperties;
}

Q_DECLARE_METATYPE(LC_PropertyContainer*)

#endif
