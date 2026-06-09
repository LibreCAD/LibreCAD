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

#ifndef LC_PROPERTYVIEW_H
#define LC_PROPERTYVIEW_H

#include <QStylePainter>

#include "lc_property.h"

struct LC_PropertyViewPart;
struct LC_PropertyPaintContext;
class LC_PropertyViewFactory;

class LC_PropertyView {
    Q_DISABLE_COPY(LC_PropertyView)

public:
    enum ChildExpandingIndicatorViewStyle {
        ExpansionStyleQtDefault,
        ExpansionStyleCustom,
    };

    static void setChildExpansionStyle(const ChildExpandingIndicatorViewStyle style) {
        m_expandedIndicatorStyle = style;
    }

    static const QByteArray ATTR_VIRTUAL;

    struct SubPropertyInfo {
        int id;
        QString key;
        QByteArray displayNameAttr;
        QByteArray descriptionAttr;
    };

    virtual ~LC_PropertyView();
    virtual void init();

    LC_Property* getProperty() const {
        return m_property;
    }

    int getSubPropertyCount() const {
        return doGetSubPropertyCount();
    }

    LC_Property* getSubProperty(const int index) {
        return doGetSubProperty(index);
    }

    void applyAttributes(const LC_PropertyViewDescriptor& info) {
        doApplyAttributes(info);
    }

    void applySubPropertyInfo(const LC_PropertyViewDescriptor& attrs, const SubPropertyInfo& subInfo);
    void applySubPropertyInfos(const LC_PropertyViewDescriptor& info, const std::vector<SubPropertyInfo>& subInfos);

    LC_PropertyChangeReason changeReasonDueToEdit() const;

    LC_Property* getStateProperty() const {
        return (m_stateProperty != nullptr) ? m_stateProperty : m_property;
    }

    void setStateProperty(LC_Property* p) {
        m_stateProperty = p;
    }

    LC_PropertyViewFactory* getFactory() const {
        return m_factory;
    }

    void setFactory(LC_PropertyViewFactory* factory) {
        m_factory = factory;
    }

    void buildViewParts(LC_PropertyPaintContext& context, QList<LC_PropertyViewPart>& parts) {
        doBuildViewParts(context, parts);
    }

    virtual bool isSplittable() const;

    virtual void invalidateCached() {
    }

protected:
    explicit LC_PropertyView(LC_Property* property);
    virtual int doGetSubPropertyCount() const;
    virtual LC_Property* doGetSubProperty(int index);
    virtual void doApplyAttributes(const LC_PropertyViewDescriptor& info);
    virtual void doBuildViewParts(LC_PropertyPaintContext& context, QList<LC_PropertyViewPart>& parts) = 0;
    QStyle::State getState(bool isActive, const LC_PropertyViewPart& part) const;
    void buildPartChildrenExpansion(LC_PropertyPaintContext& ctx, QList<LC_PropertyViewPart>& parts) const;

    bool isMultiValue() const {
        return getStateProperty()->isMultiValue();
    }

    bool isEditableByUser() const {
        return getStateProperty()->isEditableByUser();
    }

    bool isReadOnly() const {
        return !getStateProperty()->isEditableByUser();
    }

    LC_Property* m_property;
    LC_Property* m_stateProperty;

    bool m_shiftHasChildren = true;
    bool m_shiftNoChildren = false;

    static ChildExpandingIndicatorViewStyle m_expandedIndicatorStyle;

    static QColor disabledTextColor(const QStylePainter& painter);
    static QColor activeTextColor(const QStylePainter& painter);

private:
    LC_PropertyViewFactory* m_factory = nullptr;
};

#endif
