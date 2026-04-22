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

#include "lc_property.h"

#include <QCoreApplication>
#include <QIODevice>

#include "lc_property_utils.h"

class ViewAttributesProvider {
    Q_DISABLE_COPY(ViewAttributesProvider)

public:
    virtual LC_PropertyViewDescriptor* getAttributes() = 0;
    virtual ~ViewAttributesProvider() = default;

protected:
    ViewAttributesProvider() = default;
};

class ValueHolderViewAttributesProvider : public ViewAttributesProvider {
public:
    explicit ValueHolderViewAttributesProvider(const LC_PropertyViewDescriptor& delegate);
    LC_PropertyViewDescriptor* getAttributes() override;

private:
    LC_PropertyViewDescriptor m_viewAttributes;
};

class DelegatedViewAttributesProvider : public ViewAttributesProvider {
public:
    explicit DelegatedViewAttributesProvider(const LC_Property::FunViewDescriptorProvider& callback);
    LC_PropertyViewDescriptor* getAttributes() override;

private:
    LC_Property::FunViewDescriptorProvider m_callback;
    QScopedPointer<LC_PropertyViewDescriptor> m_viewAttrs;
};

LC_Property::LC_Property(QObject* parent)
    : QObject(parent), m_primaryProperty(nullptr), m_stateLocal(PropertyStateNone),
      m_stateInherited(PropertyStateNone), m_changeReasons(0), m_timer(0), m_updateEvent(nullptr), m_viewDescriptorProvider{nullptr} {
}

bool LC_Property::event(QEvent* e) {
    if (e == m_updateEvent) {
        m_updateEvent = nullptr;
        if (m_changeReasons != 0) {
            emit afterPropertyChange(LC_PropertyChangeReason(m_changeReasons));
            m_changeReasons = 0;
        }
        return true;
    }
    if (e->type() == QEvent::Timer && static_cast<QTimerEvent*>(e)->timerId() == m_timer) {
        killTimer(m_timer);
        m_timer = 0;

        if (m_changeReasons != 0) {
            emit afterPropertyChange(LC_PropertyChangeReason(m_changeReasons));
            m_changeReasons = 0;
        }
        return true;
    }
    return QObject::event(e);
}

LC_Property::~LC_Property() {
    disconnectPrimaryState();
    LC_PropertyContainerUtils::removeChildPropertyFromContainer(parent(), this);
}

const QMetaObject* LC_Property::propertyMetaObject() const {
    return metaObject();
}

void LC_Property::setNames(const Names& names) {
    setName(names.name);
    setDisplayName(names.displayName);
    setDescription(names.description);
}

void LC_Property::setName(const QString& name) {
    if (objectName() == name) {
        return;
    }

    LC_PropertyChangeReason reason(PropertyChangeReasonName);
    if (m_displayName.isEmpty() && !name.isEmpty()) {
        m_displayName = name;
        reason |= PropertyChangeReasonDisplayName;
    }

    emit beforePropertyChange(reason, PropertyValuePtr(&name), qMetaTypeId<QString>());
    setObjectName(name);
    emit afterPropertyChange(reason);
}

void LC_Property::setDisplayName(const QString& displayName) {
    if (displayName == m_displayName) {
        return;
    }
    emit beforePropertyChange(PropertyChangeReasonDisplayName, PropertyValuePtr(&displayName), qMetaTypeId<QString>());
    m_displayName = displayName;
    emit afterPropertyChange(PropertyChangeReasonDisplayName);
}

void LC_Property::setDescription(const QString& description) {
    if (m_description == description) {
        return;
    }
    emit beforePropertyChange(PropertyChangeReasonDescription, PropertyValuePtr(&description), qMetaTypeId<QString>());
    m_description = description;
    emit afterPropertyChange(PropertyChangeReasonDescription);
}

QString LC_Property::getViewName() const {
    const auto desc = getViewDescriptor();
    if (desc == nullptr) {
        return QString("<default>");
    }
    return desc->viewName;
}

bool LC_Property::isExpanded() const {
    return 0 == (m_stateLocal & PropertyStateCollapsed);
}

void LC_Property::setState(const LC_PropertyState stateToSet, const bool force) {
    setStateInternal(stateToSet, force);
}

void LC_Property::updatePropertyState() {
}

void LC_Property::setStateInternal(LC_PropertyState stateToSet, const bool force, LC_PropertyChangeReason reason) {
    if (!force && (m_stateLocal == stateToSet)) {
        return;
    }

    reason |= PropertyChangeReasonStateLocal;
    emit beforePropertyChange(reason, PropertyValuePtr(&stateToSet), qMetaTypeId<LC_PropertyState>());
    m_stateLocal = stateToSet;
    updatePropertyState();
    emit afterPropertyChange(reason);
    updateStateInherited(force);
}

void LC_Property::addState(const LC_PropertyState stateToAdd, const bool force) {
    setState(m_stateLocal | stateToAdd, force);
}

void LC_Property::removeState(const LC_PropertyState stateToRemove, const bool force) {
    setState(m_stateLocal & ~stateToRemove, force);
}

void LC_Property::switchState(const LC_PropertyState stateToSwitch, const bool switchOn, const bool force) {
    if (switchOn) {
        addState(stateToSwitch, force);
    }
    else {
        removeState(stateToSwitch, force);
    }
}

void LC_Property::toggleState(const LC_PropertyState stateToSwitch, const bool force) {
    switchState(stateToSwitch, !(stateLocal() & stateToSwitch), force);
}

bool LC_Property::isEditableByUser() const {
    return !(state() & (PropertyStateReadonly | PropertyStateInvisible));
}

bool LC_Property::isVisible() const {
    return !(state() & PropertyStateInvisible);
}

bool LC_Property::isMultiValue() const {
    return 0 != (m_stateLocal & PropertyStateValueMulti);
}

bool LC_Property::isValueUnchanged() const {
    const bool result = !m_stateLocal.testFlag(PropertyStateValueModified);
    return result;
}

bool LC_Property::isValueModified() const {
    const bool result = m_stateLocal.testFlag(PropertyStateValueModified);
    return result;
}


LC_PropertyAtomic* LC_Property::asAtomic() {
    return nullptr;
}

const LC_PropertyAtomic* LC_Property::asAtomic() const {
    return nullptr;
}

LC_PropertyContainer* LC_Property::asContainer() {
    return nullptr;
}

const LC_PropertyContainer* LC_Property::asContainer() const {
    return nullptr;
}

bool LC_Property::isContainer() const {
    return false;
}

LC_Property* LC_Property::getRootProperty() {
    auto result = this;
    do {
        const auto mp = result->getPrimaryProperty();
        if (nullptr == mp) {
            break;
        }
        result = mp;
    }
    while (true);
    return result;
}

LC_PropertyContainer* LC_Property::getRootPropertySet() {
    auto p = this;
    while (p != nullptr) {
        const auto container = p->asContainer();
        const auto mp = p->getRootProperty();
        if (container != nullptr && mp == p && qobject_cast<LC_PropertyContainer*>(container->parent()) == nullptr) {
            return container;
        }
        if (mp != p) {
            p = mp;
        }
        else {
            p = qobject_cast<LC_Property*>(p->parent());
        }
    }
    return nullptr;
}

void LC_Property::updateStateInherited([[maybe_unused]] bool force) {
}

void LC_Property::connectPrimaryState(LC_Property* primaryProperty) {
    Q_ASSERT(nullptr != primaryProperty);
    disconnectPrimaryState();
    m_primaryProperty = primaryProperty;
    beforeUpdateStateFromMasterProperty();
    doUpdateStateFromMasterProperty();
    connect(primaryProperty, &QObject::destroyed, this, &LC_Property::onMasterPropertyDestroyed);
    connect(primaryProperty, &LC_Property::beforePropertyChange, this, &LC_Property::doOnBeforeMasterPropertyChange);
    connect(primaryProperty, &LC_Property::afterPropertyChange, this, &LC_Property::doOnAfterMasterPropertyChange);
}

void LC_Property::disconnectPrimaryState() {
    if (nullptr != m_primaryProperty) {
        disconnect(m_primaryProperty, &QObject::destroyed, this, &LC_Property::onMasterPropertyDestroyed);
        disconnect(m_primaryProperty, &LC_Property::beforePropertyChange, this, &LC_Property::doOnBeforeMasterPropertyChange);
        disconnect(m_primaryProperty, &LC_Property::afterPropertyChange, this, &LC_Property::doOnAfterMasterPropertyChange);
        m_primaryProperty = nullptr;
    }
}

void LC_Property::postUpdateEvent(const LC_PropertyChangeReason reason, const int postDelayMS) {
    m_changeReasons |= reason;

    if (postDelayMS > 0) {
        if (m_timer == 0) {
            m_timer = startTimer(postDelayMS);
        }
    }
    else if (nullptr == m_updateEvent) {
        m_updateEvent = new QEvent(QEvent::User);
        QCoreApplication::postEvent(this, m_updateEvent);
    }
}

void LC_Property::setStateInherited(LC_PropertyState stateToSet, const bool force) {
    if (!force && (m_stateInherited == stateToSet)) {
        return;
    }
    emit beforePropertyChange(PropertyChangeReasonStateInherited, PropertyValuePtr(&stateToSet), qMetaTypeId<LC_PropertyState>());
    m_stateInherited = stateToSet;
    emit afterPropertyChange(PropertyChangeReasonStateInherited);
    updateStateInherited(force);
}

LC_PropertyState LC_Property::masterPropertyState() const {
    return !(stateLocal() & PropertyStateIgnoreDirectParentState) ? m_primaryProperty->state() : m_primaryProperty->stateInherited();
}

void LC_Property::doOnBeforeMasterPropertyChange(const LC_PropertyChangeReason reason) {
    if ((reason & PropertyChangeReasonState) != 0u) {
        Q_ASSERT(sender() == m_primaryProperty);
        beforeUpdateStateFromMasterProperty();
    }
}

void LC_Property::doOnAfterMasterPropertyChange(const LC_PropertyChangeReason reason) {
    if ((reason & PropertyChangeReasonState) != 0u) {
        Q_ASSERT(sender() == m_primaryProperty);
        doUpdateStateFromMasterProperty();
    }
}

void LC_Property::beforeUpdateStateFromMasterProperty() {
    auto newState = masterPropertyState();
    if (m_stateInherited == newState) {
        return;
    }
    emit beforePropertyChange(PropertyChangeReasonStateInherited, PropertyValuePtr(&newState), qMetaTypeId<LC_PropertyState>());
}

void LC_Property::doUpdateStateFromMasterProperty() {
    const auto newState = masterPropertyState();
    if (m_stateInherited == newState) {
        return;
    }
    m_stateInherited = newState;
    emit afterPropertyChange(PropertyChangeReasonStateInherited);
    updateStateInherited(false);
}

void LC_Property::onMasterPropertyDestroyed([[maybe_unused]] const QObject* object) {
    Q_ASSERT(object == m_primaryProperty);
    m_primaryProperty = nullptr;
}

void LC_Property::setExpanded(const bool expanded) {
    if (expanded) {
        expand();
    }
    else {
        collapse();
    }
}

bool LC_Property::isWritable() const {
    return 0 == (state() & PropertyStateReadonly);
}

bool LC_Property::isCollapsed() const {
    return 0 != (m_stateLocal & PropertyStateCollapsed);
}

void LC_Property::setCollapsed(const bool collapsed) {
    if (collapsed) {
        collapse();
    }
    else {
        expand();
    }
}

LC_PropertyState LC_Property::state() const {
    return m_stateLocal | m_stateInherited;
}

const LC_PropertyViewDescriptor* LC_Property::getViewDescriptor() const {
    if (m_viewDescriptorProvider == nullptr) {
        return nullptr;
    }
    return m_viewDescriptorProvider->getAttributes();
}

void LC_Property::setViewDescriptor(const LC_PropertyViewDescriptor& descriptor) {
    m_viewDescriptorProvider = std::make_unique<ValueHolderViewAttributesProvider>(descriptor);
}

void LC_Property::setViewDescriptorProvider(const FunViewDescriptorProvider& callback) {
    m_viewDescriptorProvider = std::make_unique<DelegatedViewAttributesProvider>(callback);
}

void LC_Property::setViewAttribute(const QByteArray& attributeName, const QVariant& attributeValue, const LC_PropertyChangeReason reason) {
    if (m_viewDescriptorProvider == nullptr) {
        setViewDescriptor(LC_PropertyViewDescriptor());
    }
    Q_ASSERT(m_viewDescriptorProvider != nullptr);

    const auto attrs = m_viewDescriptorProvider->getAttributes();

    Q_ASSERT(attrs != nullptr);
    attrs->attributes[attributeName] = attributeValue;

    if (reason) {
        emit afterPropertyChange(reason);
    }
}

ValueHolderViewAttributesProvider::ValueHolderViewAttributesProvider(const LC_PropertyViewDescriptor& delegate)
    : m_viewAttributes(delegate) {
}

LC_PropertyViewDescriptor* ValueHolderViewAttributesProvider::getAttributes() {
    return &m_viewAttributes;
}

DelegatedViewAttributesProvider::DelegatedViewAttributesProvider(const LC_Property::FunViewDescriptorProvider& callback)
    : m_callback(callback) {
    Q_ASSERT(callback != nullptr);
}

LC_PropertyViewDescriptor* DelegatedViewAttributesProvider::getAttributes() {
    if (m_viewAttrs.isNull()) {
        m_viewAttrs.reset(new LC_PropertyViewDescriptor(m_callback()));
    }
    return m_viewAttrs.data();
}
