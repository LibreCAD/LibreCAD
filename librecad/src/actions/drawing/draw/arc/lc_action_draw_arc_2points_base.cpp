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

#include "lc_action_draw_arc_2points_base.h"

#include "lc_arc_2_points_options_widget_filler.h"
#include "lc_arc_2points_options_widget.h"
#include "lc_linemath.h"
#include "rs_arc.h"
#include "rs_document.h"

LC_ActionDrawArc2PointsBase::LC_ActionDrawArc2PointsBase(const QString& name, LC_ActionContext *actionContext, const RS2::ActionType actionType)
    :LC_SingleEntityCreationAction(name, actionContext,actionType)  {
}

void LC_ActionDrawArc2PointsBase::doSaveOptions() {
    save("Reversed",  m_reversed);
    save("Parameter", m_parameterLen);
}

void LC_ActionDrawArc2PointsBase::doLoadOptions() {
    m_reversed = loadBool("Reversed",  false);
    m_parameterLen = loadDouble("Parameter", 1.0);
}

bool LC_ActionDrawArc2PointsBase::isInVisualSnapStatus(int status) {
    return (status == SetPoint1) || (status == SetPoint2);
}

RS_Entity* LC_ActionDrawArc2PointsBase::doTriggerCreateEntity() {
    RS_Entity* createdEntity = createArc(getStatus(), m_endPoint, m_alternated, true);
    if (createdEntity != nullptr){
        createdEntity->setParent(m_document);
        select(createdEntity);
        return createdEntity;
    }
    return nullptr;
}

void LC_ActionDrawArc2PointsBase::doTriggerCompletion([[maybe_unused]]bool success) {
    setStatus(SetPoint1);
    m_alternated = false;
}

bool LC_ActionDrawArc2PointsBase::doUpdateAngleByInteractiveInput([[maybe_unused]]const QString& tag, const double angle) {
    setParameter(angle);
    return true;
}

bool LC_ActionDrawArc2PointsBase::doUpdateDistanceByInteractiveInput([[maybe_unused]]const QString& tag, const double distance) {
    setParameter(distance);
    return true;
}

void LC_ActionDrawArc2PointsBase::onMouseMoveEvent(const int status, const LC_MouseEvent* e) {
    RS_Vector mouse = e->snapPoint;
    switch (status){
        case SetPoint1:{
            mouse = getRelZeroAwarePoint(e, mouse);
            if (m_showRefEntitiesOnPreview) {
                previewRefSelectablePoint(mouse);
            }
            break;
        }
        case SetPoint2:{
            mouse = getSnapAngleAwarePoint(e, m_startPoint, mouse, true);
            const bool alternate = e->isControl;
            RS_Arc* arc = createArc(status, mouse, alternate);
            if (arc != nullptr){
                previewEntityToCreate(arc);
                if (m_showRefEntitiesOnPreview) {
                    const RS_Vector center = arc->getCenter();
                    previewRefPoint(m_startPoint);
                    previewRefSelectablePoint(arc->getEndpoint());
                    previewRefPoint(center);
                    doPreviewOnPoint2Custom(arc);
                }
            }
            break;
        }
        default:{
            break;
        }
    }
}

void LC_ActionDrawArc2PointsBase::onMouseLeftButtonRelease(const int status, const LC_MouseEvent* e) {
    RS_Vector mouse = e->snapPoint;
    switch (status){
        case SetPoint1:{
            mouse = getRelZeroAwarePoint(e, mouse);
            fireCoordinateEvent(mouse);
            break;
        }
        case SetPoint2:{
            mouse = getSnapAngleAwarePoint(e, m_startPoint, mouse, true);
            if (LC_LineMath::isNotMeaningfulDistance(m_startPoint, mouse)){
                command(tr("The end point is too close to the start point"));
            }
            else{
                m_alternated = e->isControl;
                fireCoordinateEvent(mouse);
            }
            break;
        }
        default:{
            onMouseLeftButtonReleaseForNonPointsStatus(status, mouse, e);
            break;
        }
    }
}

void LC_ActionDrawArc2PointsBase::onMouseRightButtonRelease(const int status, [[maybe_unused]] const LC_MouseEvent* e) {
    initPrevious(status);
}

bool LC_ActionDrawArc2PointsBase::doProcessCommand(const int status, const QString &command) {
   bool accept = false;
   const QString parameterValueCommand = getParameterCommand();
   if (checkCommand(parameterValueCommand, command)){
       m_savedState = status;
       setStatus(SetParameterValue);
       accept = true;
   }
   else if (checkCommand("reversed", command)){
       accept = true;
       setReversed(!isReversed());
       updateOptions();
   }
   else {
       bool ok;
       const double r = RS_Math::eval(command, &ok);
       if (ok) {
           if (r > RS_TOLERANCE) {
               setParameterValue(r);
               updateOptions();
               setStatus(m_savedState);
           }else {
               commandMessage(tr("Positive value is expected"));
           }
       } else {
           commandMessage(tr("Not a valid expression"));
       }
       accept = true;
   }
   return accept;
}

void LC_ActionDrawArc2PointsBase::onCoordinateEvent(const int status, [[maybe_unused]] bool isZero, const RS_Vector &pos) {
    switch (status){
        case SetPoint1:{
            m_startPoint = pos;
            addSnappedPointToVisualSnap(pos);
            moveRelativeZero(pos);
            setStatus(SetPoint2);
            break;
        }
        case SetPoint2:{
            m_endPoint = pos;
            addSnappedPointToVisualSnap(pos);
            moveRelativeZero(pos);
            proceedFromSetPoint2();
            break;
        }
        default:
            break;
    }
}

void LC_ActionDrawArc2PointsBase::proceedFromSetPoint2() {
    trigger();
}

QStringList LC_ActionDrawArc2PointsBase::getAvailableCommands() {
    return {command(getParameterCommand()), command("reversed")};
}

void LC_ActionDrawArc2PointsBase::updateActionPrompt() {
    const int status = getStatus();
    switch (status){
        case SetPoint1:{
            updatePromptTRCancel(tr("Specify first point of arc"), MOD_SHIFT_RELATIVE_ZERO);
            break;
        }
        case SetPoint2:{
            updatePromptTRCancel(tr("Specify second point of arc"), MOD_SHIFT_AND_CTRL_ANGLE(getAlternativePoint2Prompt()));
            break;
        }
        case SetParameterValue:{
            updatePromptTRCancel(getParameterPromptValue());
            break;
        }
        default:
            break;
    }
}

QString LC_ActionDrawArc2PointsBase::getAlternativePoint2Prompt() const {
return tr("Opposite Arc direction");
}

RS2::CursorType LC_ActionDrawArc2PointsBase::doGetMouseCursor([[maybe_unused]] int status) {
    return RS2::CadCursor;
}

bool LC_ActionDrawArc2PointsBase::isReversed() const {
    return m_reversed;
}

void LC_ActionDrawArc2PointsBase::setReversed(const bool r) {
    m_reversed = r;
}

double LC_ActionDrawArc2PointsBase::getParameter() const {
    return m_parameterLen;
}

void LC_ActionDrawArc2PointsBase::setParameter(const double val) {
    m_parameterLen = val;
}

LC_ActionOptionsWidget *LC_ActionDrawArc2PointsBase::createOptionsWidget() {
    return new LC_Arc2PointsOptionsWidget(m_actionType);
}

LC_ActionOptionsPropertiesFiller* LC_ActionDrawArc2PointsBase::createOptionsFiller() {
    return new LC_Arc2PointsOptionsWidgetFiller();
}

RS_Arc *LC_ActionDrawArc2PointsBase::createArc(const int status, const RS_Vector& pos, const bool reverse, const bool reportErrors) {
    auto arcData = RS_ArcData();
    if (createArcData(arcData, status, pos, reverse, reportErrors)) {
        auto *result = new RS_Arc(nullptr, arcData);
        return result;
    }
    return nullptr;
}

void LC_ActionDrawArc2PointsBase::setParameterValue(const double r) {
    m_parameterLen = r;
}
