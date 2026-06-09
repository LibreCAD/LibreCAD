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

#include "lc_relative_point_input_widget.h"

#include <QCheckBox>
#include <QLabel>
#include <QLineEdit>
#include <QTimer>
#include <QVBoxLayout>

#include "lc_graphicviewport.h"
#include "lc_relative_position_editing_widget.h"
#include "rs_color.h"
#include "rs_graphicview.h"



LC_RelativePointInputWidget::LC_RelativePointInputWidget(RS_GraphicView* parent, LC_ActionContext* actionContext) : QWidget(parent),
    m_actionContext{actionContext} {
    m_graphicView = parent;
    m_viewport = m_graphicView->getViewPort();
    m_viewport->addViewportListener(this);
    m_contentWidget = new LC_RelativePositionEditingWidget(this, m_viewport, actionContext, this); // fixme - isn't this bad dependency?
    setContentWidget(m_contentWidget);
    setCursor(Qt::ArrowCursor);
}

void LC_RelativePointInputWidget::completeInteractiveInput( RS2::RelativePointParam paramType, double value) {
    m_contentWidget->updateByInteractiveInput(paramType, value);
    layout()->activate();
    QWidget::show();
}

void LC_RelativePointInputWidget::onLateRequestCompleted(bool shouldBeSkipped) {
    if (!shouldBeSkipped) {
        const auto inputInfo = m_actionContext->getInteractiveInputInfo();
        const auto requestorTag = inputInfo->requestorTag;
        RS2::RelativePointParam paramType;
        double value;
        bool hasValue = true;
        switch (inputInfo->inputType) {
            case LC_ActionContext::InteractiveInputInfo::ANGLE: {
                paramType = RS2::REL_POINT_ANGLE;
                value = inputInfo->angleRad;
                break;
            }
            case LC_ActionContext::InteractiveInputInfo::DISTANCE: {
                bool ok;
                int intValue = requestorTag.toInt(&ok);
                paramType = static_cast<RS2::RelativePointParam>(intValue);
                value = inputInfo->distance;
                break;
            }
            case LC_ActionContext::InteractiveInputInfo::POINT_X: {
                const RS_Vector ucsPoint = m_viewport->toUCS(inputInfo->wcsPoint);
                paramType = RS2::REL_POINT_X;
                value = ucsPoint.x;
                break;
            }
            case LC_ActionContext::InteractiveInputInfo::POINT_Y: {
                const RS_Vector ucsPoint = m_viewport->toUCS(inputInfo->wcsPoint);
                value = ucsPoint.y;
                paramType = RS2::REL_POINT_Y;
                break;
            }
            default:
                hasValue = false;
                break;
        }
        if (hasValue) {
            QTimer::singleShot(100, this, [this, paramType, value]() {
                 completeInteractiveInput(paramType, value);
             });
        }
    }
    m_actionContext->interactiveInputRequestCancel();
}

void LC_RelativePointInputWidget::show(const RS_Vector& pos, const RS_Vector& basePoint, bool baseIsRelativePoint, RS2::RelativePointParam activeParam) {
    m_graphPosition = pos;
    updatePosition(false);
    m_contentWidget->updateForPoints(pos, basePoint, baseIsRelativePoint);
    m_contentWidget->activateParamEditor(activeParam, true);
    layout()->setSizeConstraint(QLayout::SetFixedSize);
    layout()->activate();
    QWidget::show();
}

void LC_RelativePointInputWidget::onViewportChanged() {
    updatePosition(false);
}

void LC_RelativePointInputWidget::updatePosition(bool resume) {
    double x;
    double y;
    m_viewport->toUI(m_graphPosition, x, y);
    move(x, y);
    if (resume) {
        m_contentWidget->focusCurrentParam();
    }
}

void LC_RelativePointInputWidget::setWidgetColors(const RS_Color& bgColor, const RS_Color& textColor) {
    QPalette p = palette();
    p.setColor(QPalette::Window, bgColor);
    p.setColor(QPalette::WindowText, textColor);

    auto edits = findChildren<QLineEdit*>();
    for (const QLineEdit* e : std::as_const(edits)) {
        QPalette palette = e->palette();
        palette.setColor(QPalette::Base, bgColor);
        palette.setColor(QPalette::QPalette::WindowText, bgColor);
        palette.setColor(QPalette::QPalette::Shadow, bgColor);
    }

    setAutoFillBackground(true);
    setPalette(p);
}

void LC_RelativePointInputWidget::setFont(const QString& name, int size) const {
    const QFont font(name, size);
    auto labels = findChildren<QLabel*>();
    for (const auto lbl : std::as_const(labels)) {
        lbl->setFont(font);
    }

    auto checkboxes = findChildren<QCheckBox*>();
    for (const auto cb : std::as_const(checkboxes)) {
        cb->setFont(font);
    }

    auto edits = findChildren<QLineEdit*>();
    for (const auto e : std::as_const(edits)) {
        e->setFont(font);
    }
}

void LC_RelativePointInputWidget::setContentWidget(QWidget* w) {
    const auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(5, 5, 5, 5);
    layout->addWidget(w, 0, Qt::AlignCenter);
    setLayout(layout);
}
