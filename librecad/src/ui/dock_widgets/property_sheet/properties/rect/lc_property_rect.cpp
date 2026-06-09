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

#include "lc_property_rect.h"

#include "lc_actioncontext.h"
#include "lc_graphicviewport.h"
#include "lc_property_double.h"
#include "lc_property_double_interactivepick_view.h"

LC_PropertyRect::LC_PropertyRect(QObject* parent, bool holdValue)
    : ParentClass(parent, holdValue) {
}

LC_PropertyAtomic* LC_PropertyRect::createLeftProperty() {
    return createFieldProperty(&LC_MarginsRect::getLeft, &LC_MarginsRect::setLeft, "left", getLeftLabel(), getLeftDescriptionFormat(),
                               LC_PropertyDoubleInteractivePickView::VIEW_NAME);
}

LC_PropertyAtomic* LC_PropertyRect::createRightProperty() {
    return createFieldProperty(&LC_MarginsRect::getRight, &LC_MarginsRect::setRight, "right", getRightLabel(), getRightDescriptionFormat(),
                               LC_PropertyDoubleInteractivePickView::VIEW_NAME);
}

LC_PropertyAtomic* LC_PropertyRect::createTopProperty() {
    return createFieldProperty(&LC_MarginsRect::getTop, &LC_MarginsRect::setTop, "top", getTopLabel(), getTopDescriptionFormat(),
                               LC_PropertyDoubleInteractivePickView::VIEW_NAME);
}

LC_PropertyAtomic* LC_PropertyRect::createBottomProperty() {
    return createFieldProperty(&LC_MarginsRect::getBottom, &LC_MarginsRect::setBottom, "bottom", getBottomLabel(),
                               getBottomDescriptionFormat(), LC_PropertyDoubleInteractivePickView::VIEW_NAME);
}

QString LC_PropertyRect::getLeftLabel() const {
    return tr("Left");
}

QString LC_PropertyRect::getRightLabel() const {
    return tr("Right");
}

QString LC_PropertyRect::getTopLabel() const {
    return tr("Top");
}

QString LC_PropertyRect::getBottomLabel() const {
    return tr("Bottom");
}

QString LC_PropertyRect::getLeftDescriptionFormat() const {
    return m_leftDescription;
}

QString LC_PropertyRect::getRightDescriptionFormat() const {
    return m_rightDescription;
}

QString LC_PropertyRect::getTopDescriptionFormat() const {
    return m_topDescription;
}

QString LC_PropertyRect::getBottomDescriptionFormat() const {
    return m_bottomDescription;
}

QString LC_PropertyRect::getToStrValueFormat() {
    return tr("[%1, %2] [%3, %4]");
}

void LC_PropertyRect::setActionContext(LC_ActionContext* actionContext) {
    m_actionContext = actionContext;
    m_formatter = actionContext->getGraphicView()->getViewPort()->getFormatter();
}
