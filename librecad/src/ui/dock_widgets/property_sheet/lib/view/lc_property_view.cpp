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

#include "lc_property_view.h"

#include <QPainterPath>
#include <QStyleOption>

#include "lc_guardedconnectionslist.h"
#include "lc_properties_sheet.h"
#include "lc_property_event_context.h"
#include "lc_property_view_part.h"

const QByteArray LC_PropertyView::ATTR_VIRTUAL = QByteArrayLiteral("_virtual");

// default indicator seems is missing under Windows11 and t
// LC_PropertyView::ChildExpandingIndicatorViewStyle LC_PropertyView::m_expandedIndicatorStyle = ExpansionStyleQtDefault;
LC_PropertyView::ChildExpandingIndicatorViewStyle LC_PropertyView::m_expandedIndicatorStyle = ExpansionStyleCustom;

LC_PropertyView::LC_PropertyView(LC_Property* property)
    : m_property{property}, m_stateProperty(nullptr) {
}

LC_PropertyView::~LC_PropertyView() {
    m_stateProperty = nullptr;
}

void LC_PropertyView::init() {
    // do nothing
}

LC_PropertyChangeReason LC_PropertyView::changeReasonDueToEdit() const {
    LC_PropertyChangeReason result = PropertyChangeReasonEdit;
    if (isMultiValue()) {
        result |= PropertyChangeReasonMultiEdit;
    }
    return result;
}

void LC_PropertyView::applySubPropertyInfo(const LC_PropertyViewDescriptor& attrs, const SubPropertyInfo& subInfo) {
    LC_PropertyViewDescriptor subPropertyAttrs;
    auto& subAttrs = attrs.attributes;
    const auto key = subInfo.key.toUtf8();
    const auto subAttrValue = subAttrs.value(key);
    switch (subAttrValue.typeId()) {
        case QMetaType::QVariantMap:
        case QMetaType::QVariantHash: {
            const auto vmap = subAttrValue.toMap();
            static const auto NAME_KEY = QStringLiteral("name");
            for (auto it = vmap.cbegin(); it != vmap.cend(); ++it) {
                if (it.key() == NAME_KEY) {
                    auto vname = vmap.value(NAME_KEY);
                    subPropertyAttrs.viewName = vname.typeId() == QMetaType::QByteArray ? vname.toByteArray() : vname.toString().toUtf8();
                }
                else {
                    auto subAttrName = it.key().toUtf8();
                    subPropertyAttrs[subAttrName] = it.value();
                }
            }
            break;
        }

        default:
            break;
    }

    const auto p = getSubProperty(subInfo.id);
    // fixme - sand - review whether name should be actually set!
    // p->setName(subInfo.key);
    attrs.store(subInfo.displayNameAttr, p, &LC_Property::getDisplayName, &LC_Property::setDisplayName);
    attrs.store(subInfo.descriptionAttr, p, &LC_Property::getDescription, &LC_Property::setDescription);
    if (subPropertyAttrs.viewName.isEmpty()) {
        const auto originalViewDescriptor = p->getViewDescriptor();
        if (originalViewDescriptor != nullptr) {
            const auto viewName = originalViewDescriptor->viewName;
            if (!viewName.isEmpty()) {
                subPropertyAttrs.viewName = viewName;
            }
        }
    }
    p->setViewDescriptor(subPropertyAttrs);
}

void LC_PropertyView::applySubPropertyInfos(const LC_PropertyViewDescriptor& info, const std::vector<SubPropertyInfo>& subInfos) {
    for (const auto &subInfo : subInfos) {
        applySubPropertyInfo(info, subInfo);
    }
}

bool LC_PropertyView::isSplittable() const {
    return false;
}

int LC_PropertyView::doGetSubPropertyCount() const {
    return 0;
}

LC_Property* LC_PropertyView::doGetSubProperty([[maybe_unused]] int index) {
    return nullptr;
}

void LC_PropertyView::doApplyAttributes([[maybe_unused]] const LC_PropertyViewDescriptor& info) {
}

QStyle::State LC_PropertyView::getState(const bool isActive, const LC_PropertyViewPart& part) const {
    QStyle::State state = QStyle::State_Active;
    if (isEditableByUser()) {
        state |= QStyle::State_Enabled;
    }
    if (isActive) {
        state |= QStyle::State_Selected;
        state |= QStyle::State_HasFocus;
    }
    if (part.isUnderCursor()) {
        state |= QStyle::State_MouseOver;
    }
    else if (part.isPushed()) {
        state |= QStyle::State_Sunken;
    }
    return state;
}

QColor LC_PropertyView::activeTextColor(const QStylePainter& painter) {
    const auto palette = painter.style()->standardPalette();
    return palette.color(QPalette::Active, QPalette::PlaceholderText);
}

QColor LC_PropertyView::disabledTextColor(const QStylePainter& painter) {
    const auto palette = painter.style()->standardPalette();
    return palette.color(QPalette::Disabled, QPalette::PlaceholderText);
}

void LC_PropertyView::buildPartChildrenExpansion(LC_PropertyPaintContext& ctx, QList<LC_PropertyViewPart>& parts) const {
    LC_PropertyViewPart part(ctx.rect.marginsRemoved(ctx.margins));
    part.rect.setWidth(part.rect.height());

    if (ctx.hasChildren) {
        if (m_shiftHasChildren) {
            ctx.margins.setLeft(ctx.margins.left() + part.rect.height());
        }
    }
    else {
        if (m_shiftNoChildren) {
            ctx.margins.setLeft(ctx.margins.left() + part.rect.height());
        }
        return;
    }
    part.trackState();

    if (!part.rect.isValid()) {
        return;
    }
    if (m_expandedIndicatorStyle == ExpansionStyleCustom) {
        part.funPaint = [this](const LC_PropertyPaintContext& paintContext, const LC_PropertyViewPart& p) {
            auto& painter = *paintContext.painter;
            QRectF indicatorRect = p.rect;
            indicatorRect = indicatorRect.adjusted(2, 2, -2, -2); // make it smaller
            const qreal side = indicatorRect.height() / 3.5;
            const QColor fillClr = paintContext.getPalette().color(QPalette::Text);
            const QColor outlineClr = (p.isPushedOrUnderCursor()) ? Qt::blue : paintContext.getPalette().color(QPalette::Text);

            painter.save();
            painter.setPen(outlineClr);

            QPainterPath expandIndicatorPath;
            if (getStateProperty()->isCollapsed()) {
                expandIndicatorPath.moveTo(indicatorRect.left() + side, indicatorRect.top() + side);
                expandIndicatorPath.lineTo(indicatorRect.right() - side - 1, indicatorRect.top() + indicatorRect.height() * 0.5);
                expandIndicatorPath.lineTo(indicatorRect.left() + side, indicatorRect.bottom() - side);
                expandIndicatorPath.closeSubpath();
            }
            else {
                expandIndicatorPath.moveTo(indicatorRect.left() + side, indicatorRect.top() + side);
                expandIndicatorPath.lineTo(indicatorRect.right() - side, indicatorRect.top() + side);
                expandIndicatorPath.lineTo(indicatorRect.left() + indicatorRect.width() * 0.5, indicatorRect.bottom() - side - 1);
                expandIndicatorPath.closeSubpath();
            }

            if (painter.testRenderHint(QPainter::Antialiasing)) {
                painter.fillPath(expandIndicatorPath, fillClr);
                painter.drawPath(expandIndicatorPath);
            }
            else {
                painter.setRenderHint(QPainter::Antialiasing, true);
                painter.fillPath(expandIndicatorPath, fillClr);
                painter.drawPath(expandIndicatorPath);
                painter.setRenderHint(QPainter::Antialiasing, false);
            }

            painter.restore();
        };
    }
    else {
        part.funPaint = [this](const LC_PropertyPaintContext& paintContext, const LC_PropertyViewPart& p) {
            auto& painter = *paintContext.painter;

            QStyleOption expandIndicatorOption;
            expandIndicatorOption.rect = p.rect;
            expandIndicatorOption.palette = paintContext.getPalette();
            expandIndicatorOption.state = QStyle::State_Children;

            if (getStateProperty()->isExpanded()) {
                expandIndicatorOption.state |= QStyle::State_Open;
            }

            if (p.isPushedOrUnderCursor()) {
                expandIndicatorOption.state |= QStyle::State_MouseOver;
            }

            painter.drawPrimitive(QStyle::PE_IndicatorBranch, expandIndicatorOption);
        };
    }

    part.funHandleEvent = [this](const LC_PropertyEventContext& ectx, const LC_PropertyViewPart&, LC_PropertyEditContext*) -> bool {
        const int eventType = ectx.eventType();
        if ((eventType == QEvent::MouseButtonPress) || (eventType == QEvent::MouseButtonDblClick)) {
            getStateProperty()->toggleState(PropertyStateCollapsed);
            return true;
        }
        return false;
    };

    part.funGetTooltip = [this](LC_PropertyEventContext&, const LC_PropertyViewPart&) -> QString {
        return (getStateProperty()->isCollapsed())
                   ? LC_PropertiesSheet::tr("Click to expand")
                   : LC_PropertiesSheet::tr("Click to collapse");
    };

    parts.append(part);
}
