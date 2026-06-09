/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2026 LibreCAD.org
 * Copyright (C) 2026 sand1024
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

#include "lc_property_container.h"

#include <QJsonObject>
#include <QRegularExpression>

#include "lc_property.h"

LC_PropertyContainer::LC_PropertyContainer(QObject* parent)
    : LC_Property(parent) {
}

LC_PropertyContainer::~LC_PropertyContainer() {
    clearChildProperties(false);
}

QList<LC_Property*> LC_PropertyContainer::findChildProperties(QString name, const Qt::FindChildOptions options) const {
    QList<LC_Property*> result;
    name = name.trimmed();

    // if name is dot separated property path
    if (name.contains('.')) {
        const QString nameRoot = name.section('.', 0, 0);
        if (nameRoot.isEmpty()) {
            return result;
        }
        const QString nameTail = name.section('.', 1);
        if (nameTail.isEmpty()) {
            return result;
        }
        QList<LC_Property*> rootResult = findChildProperties(nameRoot, options);
        for (const auto prop : std::as_const(rootResult)) {
            const auto container = prop->asContainer();
            if (container == nullptr) {
                continue;
            }
            result.append(container->findChildProperties(nameTail, options));
        }
    }
    else {
        for (const auto child : m_childProperties) {
            if (child->getName() == name) {
                result.append(child);
            }
        }
        if ((options & Qt::FindChildrenRecursively) != 0u) {
            for (const auto child : m_childProperties) {
                const auto container = child->asContainer();
                if (container != nullptr) {
                    container->findChildPropertiesRecursive(name, result);
                }
            }
        }
    }
    return result;
}

QList<LC_Property*> LC_PropertyContainer::findChildProperties(const QRegularExpression& re, const Qt::FindChildOptions options) const {
    QList<LC_Property*> result;

    for (const auto child : m_childProperties) {
        if (re.match(child->getName()).isValid()) {
            result.append(child);
        }
    }
    if ((options & Qt::FindChildrenRecursively) != 0u) {
        for (const auto child : m_childProperties) {
            const auto container = child->asContainer();
            if (container != nullptr) {
                container->findChildPropertiesRecursive(re, result);
            }
        }
    }
    return result;
}

void LC_PropertyContainer::clearChildProperties(bool emitSignals) {
    if (m_childProperties.isEmpty()) {
        return;
    }
    if (emitSignals) {
        emit beforePropertyChange(PropertyChangeReasonChildRemove, nullptr, 0);
    }

    // Original list is cleared to avoid interference with property destructors,
    // where properties are removed from the parent's list.
    auto childProperties = std::move(m_childProperties);
    for (const auto child : childProperties) {
        if (child->parent() == this) {
            delete child;
        }
    }

    if (emitSignals) {
        emit afterPropertyChange(PropertyChangeReasonChildRemove);
    }
}

bool LC_PropertyContainer::addChildProperty(LC_Property* childProperty, const bool moveOwnership, int index) {
    Q_CHECK_PTR(childProperty);

    emit beforePropertyChange(PropertyChangeReasonChildAdd, PropertyValuePtr(childProperty), qMetaTypeId<LC_Property*>());

    if (index < 0) {
        m_childProperties.append(childProperty);
    }
    else {
        m_childProperties.insert(index, childProperty);
    }

    if (moveOwnership) {
        childProperty->setParent(this);
    }

    emit afterPropertyChange(PropertyChangeReasonChildAdd);
    childProperty->setStateInherited(state());
    return true;
}

bool LC_PropertyContainer::removeChildProperty(LC_Property* childProperty) {
    Q_CHECK_PTR(childProperty);

    const int childPropertyIndex = m_childProperties.indexOf(childProperty);
    if (childPropertyIndex < 0) {
        return false;
    }

    emit beforePropertyChange(PropertyChangeReasonChildRemove, PropertyValuePtr(childProperty), qMetaTypeId<LC_Property*>());

    m_childProperties.erase(m_childProperties.begin() + childPropertyIndex);

    if (childProperty->parent() == this) {
        childProperty->setParent(nullptr);
    }

    emit afterPropertyChange(PropertyChangeReasonChildRemove);
    return true;
}

LC_PropertyContainer* LC_PropertyContainer::asContainer() {
    return this;
}

const LC_PropertyContainer* LC_PropertyContainer::asContainer() const {
    return this;
}

void LC_PropertyContainer::updateStateInherited(const bool force) {
    for (const auto child : std::as_const(m_childProperties)) {
        child->setStateInherited(state(), force);
    }
}

void LC_PropertyContainer::findChildPropertiesRecursive(const QString& name, QList<LC_Property*>& result) {
    for (const auto child : std::as_const(m_childProperties)) {
        if (child->getName() == name) {
            result.append(child);
        }
        const auto container = child->asContainer();
        if (container != nullptr) {
            container->findChildPropertiesRecursive(name, result);
        }
    }
}

void LC_PropertyContainer::findChildPropertiesRecursive(const QRegularExpression& re, QList<LC_Property*>& result) {
    for (const auto child : std::as_const(m_childProperties)) {
        if (re.match(child->getName()).isValid()) {
            result.append(child);
        }
        const auto container = child->asContainer();
        if (container != nullptr) {
            container->findChildPropertiesRecursive(re, result);
        }
    }
}
