/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2024 LibreCAD.org
 Copyright (C) 2024 sand1024

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 ******************************************************************************/

#include "lc_actiondrawarc2pointsbase.h"

#include "lc_actiondrawarc2poptions.h"
#include "lc_linemath.h"
#include "rs_arc.h"

LC_ActionDrawArc2PointsBase::LC_ActionDrawArc2PointsBase(const char* name, LC_ActionContext *actionContext, RS2::ActionType actionType)
    :RS_PreviewActionInterface(name, actionContext,actionType)  {
}

void LC_ActionDrawArc2PointsBase::doTrigger() {
    RS_Entity* createdEntity = createArc(getStatus(), m_endPoint, m_alternated, true);
    if (createdEntity != nullptr){

        createdEntity->setSelected(true);
        createdEntity->setParent(m_container);
        setPenAndLayerToActive(createdEntity);
        undoCycleAdd(createdEntity);

        setStatus(SetPoint1);
        m_alternated = false;
        doAfterTrigger();
    }
    else{
        doOnEntityNotCreated();
    }
}

bool LC_ActionDrawArc2PointsBase::doUpdateAngleByInteractiveInput([[maybe_unused]]const QString& tag, double angle) {
    setParameter(angle);
    return true;
}

bool LC_ActionDrawArc2PointsBase::doUpdateDistanceByInteractiveInput([[maybe_unused]]const QString& tag, double distance) {
    setParameter(distance);
    return true;
}

void LC_ActionDrawArc2PointsBase::onMouseMoveEvent(int status, LC_MouseEvent *e) {
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
            bool alternate = e->isControl;
            RS_Arc* arc = createArc(status, mouse, alternate);
            if (arc != nullptr){
                previewEntityToCreate(arc);
                RS_Vector center = arc->getCenter();
                if (m_showRefEntitiesOnPreview) {
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

void LC_ActionDrawArc2PointsBase::onMouseLeftButtonRelease(int status, LC_MouseEvent *e) {
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

void LC_ActionDrawArc2PointsBase::onMouseRightButtonRelease(int status, [[maybe_unused]]LC_MouseEvent *e) {
    initPrevious(status);
}

bool LC_ActionDrawArc2PointsBase::doProcessCommand(int status, const QString &command) {
   bool accept = false;
   QString parameterValueCommand = getParameterCommand();
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
       double r = RS_Math::eval(command, &ok);
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

void LC_ActionDrawArc2PointsBase::onCoordinateEvent(int status, [[maybe_unused]] bool isZero, const RS_Vector &pos) {
    switch (status){
        case SetPoint1:{
            m_startPoint = pos;
            moveRelativeZero(pos);
            setStatus(SetPoint2);
            break;
        }
        case SetPoint2:{
            m_endPoint = pos;
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

void LC_ActionDrawArc2PointsBase::updateMouseButtonHints() {
    int status = getStatus();
    switch (status){
        case SetPoint1:{
            updateMouseWidgetTRCancel(tr("Specify first point of arc"), MOD_SHIFT_RELATIVE_ZERO);
            break;
        }
        case SetPoint2:{
            updateMouseWidgetTRCancel(tr("Specify second point of arc"), MOD_SHIFT_AND_CTRL_ANGLE(getAlternativePoint2Prompt()));
            break;
        }
        case SetParameterValue:{
            updateMouseWidgetTRCancel(getParameterPromptValue());
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

void LC_ActionDrawArc2PointsBase::setReversed(bool r) {
    m_reversed = r;
}

double LC_ActionDrawArc2PointsBase::getParameter() const {
    return m_parameterLen;
}

void LC_ActionDrawArc2PointsBase::setParameter(double pL) {
    m_parameterLen = pL;
}

LC_ActionOptionsWidget *LC_ActionDrawArc2PointsBase::createOptionsWidget() {
    return new LC_ActionDrawArc2POptions(m_actionType);
}

RS_Arc *LC_ActionDrawArc2PointsBase::createArc(int status, RS_Vector pos, bool reverse, bool reportErrors) {
    RS_ArcData arcData = RS_ArcData();
    if (createArcData(arcData, status, pos, reverse, reportErrors)) {
        auto *result = new RS_Arc(nullptr, arcData);
        return result;
    }
    else{
        return nullptr;
    }
}

void LC_ActionDrawArc2PointsBase::setParameterValue(double r) {
    m_parameterLen = r;
}
