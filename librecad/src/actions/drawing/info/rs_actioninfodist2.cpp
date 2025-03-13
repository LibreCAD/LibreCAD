/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file gpl-2.0.txt included in the
** packaging of this file.
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

#include "rs_coordinateevent.h"
#include "rs_debug.h"
#include "rs_dialogfactory.h"
#include "rs_graphic.h"
#include "rs_graphicview.h"
#include "rs_units.h"
#include "rs_actioninfodist2.h"
#include "lc_infodist2options.h"

RS_ActionInfoDist2::RS_ActionInfoDist2(RS_EntityContainer &container, RS_GraphicView &graphicView, bool fromPoint)
    :RS_PreviewActionInterface("Info Dist2", container, graphicView), entity(nullptr){
    actionType = RS2::ActionInfoDistEntity2Point;
    selectionMode = fromPoint ? FIRST_IS_POINT : FIRST_IS_ENTITY;
}

RS_ActionInfoDist2::~RS_ActionInfoDist2()= default;

void RS_ActionInfoDist2::init(int status){
    RS_PreviewActionInterface::init(status);
    if (status == 0){
        if (selectionMode == FIRST_IS_POINT){
            setStatus(SetPoint);
        } else {
            savedRelZero = getRelativeZero();
        }
    } else if (status < 0){
        restoreRelZero();
    }
}

void RS_ActionInfoDist2::finish(bool updateTB){
    RS_PreviewActionInterface::finish(updateTB);
    restoreRelZero();
}

void RS_ActionInfoDist2::restoreRelZero(){
    if (selectionMode == FIRST_IS_ENTITY){
        moveRelativeZero(savedRelZero);
    }
}

// fixme - consider displaying information in EntityInfo widget
void RS_ActionInfoDist2::doTrigger() {
    RS_DEBUG->print("RS_ActionInfoDist2::trigger()");
    if (point.valid && entity != nullptr){
        RS_Vector dV;
        if (selectionMode == FIRST_IS_ENTITY){
            dV = point - entityNearestPoint;
        } else {
            dV = entityNearestPoint - point;
        }
        QStringList dists;
        for (double a: {dV.magnitude(), dV.x, dV.y, entityNearestPoint.x, entityNearestPoint.y, point.x, point.y}) {
            dists << formatLinear(a);

        }
        double wcsAngle = dV.angle();
        QString angle = formatWCSAngle(wcsAngle);
        commandMessage("---");
        const QString &msgTemplate = tr("Distance: %1\nCartesian: (%2 , %3)\nPolar: (%4 < %5)\nPoint On Entity: (%6 , %7)\nPoint: (%8 , %9)");
        QString message = msgTemplate.arg(dists[0], dists[1], dists[2], dists[0], angle, dists[3], dists[4], dists[5], dists[6]);
        commandMessage(message);
        restoreRelZero();
        deletePreview();
        redrawDrawing();
    }
}

void RS_ActionInfoDist2::onMouseMoveEvent(int status, LC_MouseEvent *e) {
    RS_Vector snap = e->snapPoint;
    switch (status) {
        case SetEntity: {
            deleteSnapper();
            auto en = doCatchEntity(e, true);
            if (en != nullptr){
                highlightHover(en);
                deleteSnapper();
                if (point.valid){
                    if (selectionMode == FIRST_IS_POINT){
                        RS_Vector nearest = en->getNearestPointOnEntity(point, nearestPointShouldBeOnEntity);
                        previewLine(nearest, point);
                        updateInfoCursor(nearest,point);
                        if (showRefEntitiesOnPreview) {
                            previewRefLine(nearest, point);
                            previewRefPoint(point);
                            previewRefSelectablePoint(nearest);
                        }
                    } else {
                        // no more
                    }
                }
            } else {
                if (selectionMode == FIRST_IS_POINT){
                    previewLine(snap, point);
                    updateInfoCursor(snap, point);
                    if (showRefEntitiesOnPreview) {
                        previewRefLine(snap, point);
                        previewRefPoint(point);
                    }
                }
            }
            break;
        }
        case SetPoint: {
            switch (selectionMode) {
                case FIRST_IS_POINT: {
                    trySnapToRelZeroCoordinateEvent(e);
                    if (showRefEntitiesOnPreview) {
                        previewRefPoint(snap);
                    }
                    break;
                }
                case FIRST_IS_ENTITY: {
                    if (entity != nullptr){
                        highlightSelected(entity);
                        snap = getRelZeroAwarePoint(e, snap);
                        RS_Vector nearest = obtainNearestPointOnEntity(snap);
                        previewLine(nearest, snap);
                        updateInfoCursor(snap, nearest);
                        if (showRefEntitiesOnPreview) {
                            previewRefLine(nearest, snap);
                            previewLine(nearest, snap);
                            previewRefPoint(nearest);
                            previewRefSelectablePoint(snap);
                        }

                        // move relative point so we'll have proper distance and angle from entity to point in coordinates widget
                        if (e->isControl){
                            moveRelativeZero(nearest);
                        }
                    }
                    break;
                }
            }
            break;
        }
        default:
            break;
    }
}

void RS_ActionInfoDist2::onMouseLeftButtonRelease(int status, LC_MouseEvent *e) {
    switch (status) {
        case SetEntity: {
            entity = doCatchEntity(e, false);
            if (entity != nullptr){
                switch (selectionMode) {
                    case FIRST_IS_POINT: {
                        entityNearestPoint = obtainNearestPointOnEntity(point);
                        trigger();
                        setStatus(SetPoint);
                        break;
                    }
                    case FIRST_IS_ENTITY: {
                        setStatus(SetPoint);
                        break;
                    }
                }
            }
            invalidateSnapSpot();
            break;
        }
        case SetPoint: {
            RS_Vector snap = e->snapPoint;
            snap = getRelZeroAwarePoint(e, snap);
            switch (selectionMode) {
                case FIRST_IS_POINT: {
                    point = snap;
                    moveRelativeZero(point);
                    setStatus(SetEntity);
                    break;
                }
                case FIRST_IS_ENTITY: {
                    entityNearestPoint = obtainNearestPointOnEntity(snap);
                    fireCoordinateEvent(snap);
                    break;
                }
            }
        }
        default:
            break;
    }
}

void RS_ActionInfoDist2::onMouseRightButtonRelease(int status, [[maybe_unused]]LC_MouseEvent *e) {
    deletePreview();
    int newStatus = -1;
    bool firstIsPoint = selectionMode == FIRST_IS_POINT;
    switch (status) {
        case SetEntity: {
            newStatus = firstIsPoint ? SetPoint : -1;
            break;
        }
        case SetPoint: {
            newStatus = firstIsPoint ? -1 : SetEntity;
            restoreRelZero();
            break;
        }
        default:
            break;
    }
    setStatus(newStatus);
}

RS_Vector RS_ActionInfoDist2::obtainNearestPointOnEntity(const RS_Vector &snap) const{
    return entity->getNearestPointOnEntity(snap, nearestPointShouldBeOnEntity);
}

RS_Entity *RS_ActionInfoDist2::doCatchEntity(LC_MouseEvent *e, bool preview){
    RS2::ResolveLevel level = RS2::ResolveAll;
    if (e->isControl){
        level = RS2::ResolveNone;
    }
    if (preview) {
        return catchAndDescribe(e, level);
    }
    else{
        return catchEntityByEvent(e, level);
    }
}

void RS_ActionInfoDist2::onCoordinateEvent(int status, [[maybe_unused]]bool isZero, const RS_Vector &pos) {
    if (status == SetPoint){
        point = pos;
        moveRelativeZero(point);
        switch (selectionMode) {
            case FIRST_IS_POINT: {
                setStatus(SetEntity);
                break;
            }
            case FIRST_IS_ENTITY: {
                trigger();
                setStatus(SetEntity);
                break;
            }
        }
    }
}

void RS_ActionInfoDist2::updateMouseButtonHints(){
    switch (getStatus()) {
        case SetEntity:
            updateMouseWidgetTRCancel(tr("Specify entity"), MOD_CTRL(tr("Do not snap to child entities in container")));
            break;
        case SetPoint: {
            if (selectionMode == FIRST_IS_ENTITY){
                updateMouseWidgetTRCancel(tr("Specify point"), MOD_SHIFT_AND_CTRL(MSG_REL_ZERO, tr("Relative point is on entity")));
            }
            else{
                updateMouseWidgetTRCancel(tr("Specify point"), MOD_SHIFT_RELATIVE_ZERO);
            }

            break;
        }
        default:
            updateMouseWidget();
            break;
    }
}
RS2::CursorType RS_ActionInfoDist2::doGetMouseCursor([[maybe_unused]] int status){
    switch (status) {
        case SetEntity: {
            return RS2::SelectCursor;
        }
        default:{
            return RS2::CadCursor;
        }
    }
}

// todo - refactor, this is a copy from RS_ActionInfoDist
void RS_ActionInfoDist2::updateInfoCursor(const RS_Vector &mouse, const RS_Vector &startPoint) {
    if (infoCursorOverlayPrefs->enabled){
        double distance = startPoint.distanceTo(mouse);
        LC_InfoMessageBuilder msg(tr("Info"));
        msg.add(tr("Distance:"), formatLinear(distance));
        msg.add(tr("Angle:"), formatWCSAngle(startPoint.angleTo(mouse)));
        msg.add(tr("From:"), formatVector(startPoint));
        msg.add(tr("To:"), formatVector(mouse));
        appendInfoCursorZoneMessage(msg.toString(), 2, false);
    }
}

LC_ActionOptionsWidget* RS_ActionInfoDist2::createOptionsWidget(){
    return new LC_InfoDist2Options();
}
