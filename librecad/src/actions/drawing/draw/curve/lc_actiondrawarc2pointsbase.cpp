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

#include <QMouseEvent>

#include "lc_actiondrawarc2pointsbase.h"
#include "lc_actiondrawarc2poptions.h"
#include "lc_linemath.h"
#include "rs_arc.h"
#include "rs_document.h"
#include "rs_graphic.h"
#include "rs_graphicview.h"
#include "rs_math.h"

LC_ActionDrawArc2PointsBase::LC_ActionDrawArc2PointsBase(const char* name, RS_EntityContainer &container, RS_GraphicView &graphicView)
    :RS_PreviewActionInterface(name, container, graphicView)  {
}

void LC_ActionDrawArc2PointsBase::doTrigger() {
    RS_Entity* createdEntity = createArc(getStatus(), endPoint, alternated, true);
    if (createdEntity != nullptr){

        createdEntity->setSelected(true);
        createdEntity->setParent(container);

        setPenAndLayerToActive(createdEntity);

        undoCycleAdd(createdEntity);

        setStatus(SetPoint1);
        alternated = false;
        doAfterTrigger();
    }
    else{
        doOnEntityNotCreated();
    }
}

void LC_ActionDrawArc2PointsBase::mouseMoveEvent(QMouseEvent *e) {
    deletePreview();
    deleteHighlights();
    RS_Vector mouse = snapPoint(e);

    int status = getStatus();
    switch (status){
        case SetPoint1:{
            mouse = getRelZeroAwarePoint(e, mouse);
            if (showRefEntitiesOnPreview) {
                previewRefSelectablePoint(mouse);
            }
            break;
        }
        case SetPoint2:{
            mouse = getSnapAngleAwarePoint(e, startPoint, mouse, true);
            bool alternate = isControl(e);
            RS_Arc* arc = createArc(status, mouse, alternate);
            if (arc != nullptr){
                previewEntityToCreate(arc);
                RS_Vector center = arc->getCenter();
                if (showRefEntitiesOnPreview) {
                    previewRefPoint(startPoint);
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

    drawPreview();
    drawHighlights();
}

void LC_ActionDrawArc2PointsBase::onMouseLeftButtonRelease(int status, QMouseEvent *e) {
    RS_Vector mouse = snapPoint(e);
    switch (status){
        case SetPoint1:{
            mouse = getRelZeroAwarePoint(e, mouse);
            fireCoordinateEvent(mouse);
            break;
        }
        case SetPoint2:{
            mouse = getSnapAngleAwarePoint(e, startPoint, mouse, true);
            if (LC_LineMath::isNotMeaningfulDistance(startPoint, mouse)){
                command(tr("The end point is too close to the start point"));
            }
            else{
                alternated = isControl(e);
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

void LC_ActionDrawArc2PointsBase::onMouseRightButtonRelease(int status, [[maybe_unused]]QMouseEvent *e) {
    initPrevious(status);
}

bool LC_ActionDrawArc2PointsBase::doProcessCommand(int status, const QString &command) {
   bool accept = false;
   QString parameterValueCommand = getParameterCommand();
   if (checkCommand(parameterValueCommand, command)){
       savedState = status;
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
               setStatus(savedState);
           }else {
               commandMessage(tr("Positive value is expected"));
           }
       } else
           commandMessage(tr("Not a valid expression"));
       accept = true;
   }
   return accept;
}

void LC_ActionDrawArc2PointsBase::onCoordinateEvent(int status, [[maybe_unused]] bool isZero, const RS_Vector &pos) {
    switch (status){
        case SetPoint1:{
            startPoint = pos;
            moveRelativeZero(pos);
            setStatus(SetPoint2);
            break;
        }
        case SetPoint2:{
            endPoint = pos;
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
    return reversed;
}

void LC_ActionDrawArc2PointsBase::setReversed(bool r) {
    reversed = r;
}

double LC_ActionDrawArc2PointsBase::getParameter() const {
    return parameterLen;
}

void LC_ActionDrawArc2PointsBase::setParameter(double pL) {
    parameterLen = pL;
}

LC_ActionOptionsWidget *LC_ActionDrawArc2PointsBase::createOptionsWidget() {
    return new LC_ActionDrawArc2POptions(actionType);
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
    parameterLen = r;
}
