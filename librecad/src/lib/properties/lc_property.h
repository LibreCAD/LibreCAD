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

#ifndef LC_PROPERTY_H
#define LC_PROPERTY_H

#include <functional>

#include "lc_property_view_descriptor.h"

class ViewAttributesProvider;
class LC_PropertyAtomic;
class LC_PropertyContainer;

template <typename T>
struct PropertyValueTag {
};

enum LC_PropertyChangeReasonFlag {
    PropertyChangeReasonValueNew       = 0x0001,
    PropertyChangeReasonValueLoaded    = 0x0002,
    PropertyChangeReasonName           = 0x0004, //
    PropertyChangeReasonDisplayName    = 0x0008, //
    PropertyChangeReasonDescription    = 0x0010, //
    PropertyChangeReasonStateLocal     = 0x0020,
    PropertyChangeReasonStateInherited = 0x0040,
    PropertyChangeReasonChildAdd       = 0x0080,
    PropertyChangeReasonChildRemove    = 0x0100,
    PropertyChangeReasonEdit           = 0x0200,
    PropertyChangeReasonMultiEdit      = 0x0400,
    PropertyChangeReasonUpdateView     = 0x0800,
    PropertyChangeReasonNewAttribute   = 0x1000, //
    PropertyChangeReasonState          = PropertyChangeReasonStateInherited | PropertyChangeReasonStateLocal,
    PropertyChangeReasonChildren       = PropertyChangeReasonChildRemove | PropertyChangeReasonChildAdd,
    PropertyChangeReasonValue          = PropertyChangeReasonValueLoaded | PropertyChangeReasonValueNew
};

Q_DECLARE_FLAGS(LC_PropertyChangeReason, LC_PropertyChangeReasonFlag)
Q_DECLARE_OPERATORS_FOR_FLAGS (LC_PropertyChangeReason)

enum LC_PropertyStateFlag {
    PropertyStateNone                    = 0x0000,
    PropertyStateInvisible               = 0x0001,
    PropertyStateReadonly                = 0x0002,
    PropertyStateCollapsed               = 0x0004,
    PropertyStateValueMulti              = 0x0008,
    PropertyStateValueModified           = 0x0010,
    PropertyStateIgnoreDirectParentState = 0x80
};

Q_DECLARE_FLAGS(LC_PropertyState, LC_PropertyStateFlag)
Q_DECLARE_OPERATORS_FOR_FLAGS (LC_PropertyState)
Q_DECLARE_METATYPE (LC_PropertyState)
Q_DECLARE_METATYPE (LC_PropertyChangeReason)

class LC_Property : public QObject {
    Q_OBJECT Q_DISABLE_COPY(LC_Property)

    friend class LC_QObjectPropertyConnector;
    friend class LC_PropertyContainer;

public:
    struct Names {
        QString name;
        QString displayName;
        QString description;
    };

    using FunViewDescriptorProvider = std::function<LC_PropertyViewDescriptor()>;
    using PropertyValuePtr = void*;

    ~LC_Property() override;
    virtual const QMetaObject* propertyMetaObject() const;
    void setNames(const Names& names);
    inline QString getName() const;
    void setName(const QString& name);
    inline QString getDisplayName() const;
    void setDisplayName(const QString& displayName);
    inline QString getDescription() const;
    void setDescription(const QString& description);
    void setExpanded(bool expanded);
    void setCollapsed(bool collapsed);
    inline void expand();
    inline void collapse();

    LC_PropertyState state() const;
    inline LC_PropertyState stateLocal() const;
    inline LC_PropertyState stateInherited() const;

    void setState(LC_PropertyState stateToSet, bool force = false);
    void addState(LC_PropertyState stateToAdd, bool force = false);
    void removeState(LC_PropertyState stateToRemove, bool force = false);
    void switchState(LC_PropertyState stateToSwitch, bool switchOn, bool force = false);
    void toggleState(LC_PropertyState stateToSwitch, bool force = false);
    void setReadOnly(const bool readonly = true) {switchState(PropertyStateReadonly, readonly);}

    bool isMultiValue() const;
    bool isExpanded() const;
    bool isCollapsed() const;
    bool isWritable() const;
    bool isEditableByUser() const;
    bool isVisible() const;
    bool isValueUnchanged() const;
    bool isValueModified() const;

    static QString getMultiValuePlaceholder() {
        return tr("(VARIES)");
    }

    // casts
    virtual LC_PropertyAtomic* asAtomic();
    virtual const LC_PropertyAtomic* asAtomic() const;
    virtual LC_PropertyContainer* asContainer();
    virtual const LC_PropertyContainer* asContainer() const;
    virtual bool isContainer() const;

    LC_Property* getRootProperty();
    LC_PropertyContainer* getRootPropertySet();
    inline LC_Property* getPrimaryProperty() const;

    QString getViewName() const;
    const LC_PropertyViewDescriptor* getViewDescriptor() const;
    void setViewDescriptor(const LC_PropertyViewDescriptor& descriptor);
    void setViewDescriptorProvider(const FunViewDescriptorProvider& callback);
    void setViewAttribute(const QByteArray& attributeName, const QVariant& attributeValue,
                          LC_PropertyChangeReason reason = LC_PropertyChangeReason());
    void connectPrimaryState(LC_Property* primaryProperty);

    void disconnectPrimaryState();
    bool event(QEvent* e) override;
    void postUpdateEvent(LC_PropertyChangeReason reason, int postDelayMS = 0);
signals :
    void afterPropertyChange(LC_PropertyChangeReason reason);
    void beforePropertyChange(LC_PropertyChangeReason reason, PropertyValuePtr newValue, int typeId);

protected:
    explicit LC_Property(QObject* parent);
    virtual void updatePropertyState();
    virtual void updateStateInherited(bool force);
    void setStateInherited(LC_PropertyState stateToSet, bool force = false);
    void setStateInternal(LC_PropertyState stateToSet, bool force = false, LC_PropertyChangeReason reason = LC_PropertyChangeReason());

    virtual void doOnBeforeMasterPropertyChange(LC_PropertyChangeReason reason);
    virtual void doOnAfterMasterPropertyChange(LC_PropertyChangeReason reason);
private:
    LC_PropertyState masterPropertyState() const;
    void onMasterPropertyDestroyed(const QObject* object);
    void beforeUpdateStateFromMasterProperty();
    void doUpdateStateFromMasterProperty();

    LC_Property* m_primaryProperty;

    QString m_displayName;
    QString m_description;
    LC_PropertyState m_stateLocal;
    LC_PropertyState m_stateInherited;

    int m_changeReasons;
    int m_timer;
    QEvent* m_updateEvent;
    std::unique_ptr<ViewAttributesProvider> m_viewDescriptorProvider;
};

QString LC_Property::getName() const {
    return objectName();
}

QString LC_Property::getDisplayName() const {
    return m_displayName;
}

QString LC_Property::getDescription() const {
    return m_description;
}

void LC_Property::expand() {
    removeState(PropertyStateCollapsed);
}

void LC_Property::collapse() {
    addState(PropertyStateCollapsed);
}

LC_PropertyState LC_Property::stateLocal() const {
    return m_stateLocal;
}

LC_PropertyState LC_Property::stateInherited() const {
    return m_stateInherited;
}

LC_Property* LC_Property::getPrimaryProperty() const {
    return m_primaryProperty;
}

Q_DECLARE_METATYPE(const LC_Property*)

Q_DECLARE_METATYPE(LC_Property*)

Q_DECLARE_METATYPE(LC_Property::PropertyValuePtr)
#endif
