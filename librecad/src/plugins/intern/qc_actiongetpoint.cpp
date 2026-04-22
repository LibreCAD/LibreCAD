/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2011 Rallaz (rallazz@gmail.com)
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
**
**
** This file is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
**
** This copyright notice MUST APPEAR in all copies of the script!  
**
**********************************************************************/

#include "qc_actiongetpoint.h"

#include <QMouseEvent>

#include "rs_coordinateevent.h"
#include "rs_debug.h"
#include "rs_document.h"
#include "rs_line.h"
#include "rs_modification.h"
#include "rs_preview.h"

struct QC_ActionGetPoint::ActionData {
    RS_Vector referencePoint;
    RS_Vector targetPoint;
    QString message;
};

// fixme - sand actiontype is not set???
QC_ActionGetPoint::QC_ActionGetPoint(LC_ActionContext* actionContext)
    : RS_PreviewActionInterface("Get Point", actionContext), m_canceled(false), m_completed{false}, m_setTargetPoint{false},
      m_actionData(std::make_unique<ActionData>()) {
    m_actionData->targetPoint = RS_Vector(0, 0);
}

QC_ActionGetPoint::~QC_ActionGetPoint() = default;

void QC_ActionGetPoint::trigger() {
    RS_DEBUG->print("QC_ActionGetPoint::trigger()");
    m_completed = true;
    updateActionPrompt();
}

void QC_ActionGetPoint::mouseMoveEvent(QMouseEvent* e) {
    deletePreview();
    RS_DEBUG->print("QC_ActionGetPoint::mouseMoveEvent begin");

    const RS_Vector mouse = snapPoint(e);
    if (m_setTargetPoint) {
        if (m_actionData->referencePoint.valid) {
            m_actionData->targetPoint = mouse;
            const auto* line = new RS_Line{m_preview.get(), m_actionData->referencePoint, mouse};
            line->setPen(RS_Pen(RS_Color(0, 0, 0), RS2::Width00, RS2::DotLine));
            m_preview->addEntity(line);
            RS_DEBUG->print("QC_ActionGetPoint::mouseMoveEvent: draw preview");
            QList<RS_Entity*> selection;
            m_document->collectSelected(selection);
            m_preview->addClonesFromList(selection);
        }
    }
    else {
        m_actionData->targetPoint = mouse;
    }

    RS_DEBUG->print("QC_ActionGetPoint::mouseMoveEvent end");
    drawPreview();
}

void QC_ActionGetPoint::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button() == Qt::LeftButton) {
        RS_CoordinateEvent ce(snapPoint(e));
        coordinateEvent(&ce);
    }
    else if (e->button() == Qt::RightButton) {
        m_canceled = true;
        m_completed = true;
        finish();
    }
}

void QC_ActionGetPoint::onCoordinateEvent([[maybe_unused]] int status, [[maybe_unused]] bool isZero, const RS_Vector& pos) {
    m_actionData->targetPoint = pos;
    moveRelativeZero(m_actionData->targetPoint);
    trigger();
}

void QC_ActionGetPoint::updateActionPrompt() {
    if (!m_completed) {
        updatePrompt(m_actionData->message, tr("Cancel"));
    }
    else {
        updatePrompt();
    }
}

RS2::CursorType QC_ActionGetPoint::doGetMouseCursor([[maybe_unused]] int status) {
    return RS2::CadCursor;
}

void QC_ActionGetPoint::setBasepoint(const QPointF* basepoint) {
    m_actionData->referencePoint.x = basepoint->x();
    m_actionData->referencePoint.y = basepoint->y();
    m_setTargetPoint = true;
}

void QC_ActionGetPoint::setMessage(const QString& msg) const {
    m_actionData->message = msg;
}

void QC_ActionGetPoint::getPoint(QPointF* point) const {
    if (m_actionData) {
        point->setX(m_actionData->targetPoint.x);
        point->setY(m_actionData->targetPoint.y);
    }
}
