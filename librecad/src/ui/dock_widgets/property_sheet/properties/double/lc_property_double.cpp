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

#include "lc_property_double.h"

#include "lc_graphicviewport.h"
#include "rs_graphicview.h"

LC_PropertyDouble::LC_PropertyDouble(QObject* parent, const bool holdValue)
    : LC_PropertyNumeric(parent, holdValue) {
}

void LC_PropertyDouble::setActionContextAndLaterRequestor(LC_ActionContext* actionContext, LC_LateCompletionRequestor* requestor) {
    m_actionContext = actionContext;
    m_lateCompletionRequestor = requestor;
    m_formatter = actionContext->getGraphicView()->getViewPort()->getFormatter();
}

void LC_PropertyDouble::requestInteractiveInput() const {
    QString propertyName;
    if (m_interactiveInputType == LC_ActionContext::InteractiveInputInfo::POINT_X || m_interactiveInputType ==
        LC_ActionContext::InteractiveInputInfo::POINT_Y) {
        const auto masterProperty = getPrimaryProperty();
        if (masterProperty != nullptr) {
            propertyName = masterProperty->getName();
        }
    }
    else {
        propertyName = getName();
    }
    m_actionContext->interactiveInputStart(getInteractiveInputType(), m_lateCompletionRequestor, propertyName);
}
