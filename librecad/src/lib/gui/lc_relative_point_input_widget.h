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

#ifndef LC_RELATIVEPOINTINPUTWIDGET_H
#define LC_RELATIVEPOINTINPUTWIDGET_H

#include <QWidget>

#include "lc_graphicviewportlistener.h"
#include "lc_relative_position_editing_widget.h"
#include "rs_vector.h"

class RS_Color;
class LC_GraphicViewport;
class RS_GraphicView;

class LC_RelativePointInputWidget: public QWidget, public LC_GraphicViewPortListener, public LC_LateCompletionRequestor{
    Q_OBJECT
public:
    LC_RelativePointInputWidget(RS_GraphicView* parent, LC_ActionContext* actionContext);
    void completeInteractiveInput(RS2::RelativePointParam paramType, double value);
    void setContentWidget(QWidget* w);
    RS_Vector getGraphPosition() const {return m_graphPosition;}
    void show(const RS_Vector& pos, const RS_Vector& basePoint, bool baseIsRelativePoint, RS2::RelativePointParam activeParam);
    void onViewportChanged();
    void updatePosition(bool resume);
    void setWidgetColors(const RS_Color& bgColor, const RS_Color& textColor);
    void setFont(QString name, int size);
    void onLateRequestCompleted(bool shouldBeSkipped) override;
private:
    RS_Vector m_graphPosition{false};
    RS_GraphicView* m_graphicView {nullptr};
    LC_GraphicViewport* m_viewport{nullptr};
    LC_ActionContext* m_actionContext {nullptr};
    LC_RelativePositionEditingWidget* m_contentWidget {nullptr};
};

#endif
