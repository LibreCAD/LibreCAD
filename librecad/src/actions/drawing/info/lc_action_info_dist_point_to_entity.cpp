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

#include "lc_action_info_dist_point_to_entity.h"

#include "lc_actioninfomessagebuilder.h"
#include "lc_cursoroverlayinfo.h"
#include "lc_info_dist_point_to_entity_options_filler.h"
#include "lc_info_dist_point_to_entity_options_widget.h"
#include "rs_debug.h"
#include "rs_entity.h"

LC_ActionInfoDistPointToEntity::LC_ActionInfoDistPointToEntity(LC_ActionContext* actionContext, const bool fromPointToEntity)
    : RS_PreviewActionInterface(fromPointToEntity ? "ActionInfoDistPoint2Entity" : "ActionInfoDistEntity2Point", actionContext,
                                fromPointToEntity ? RS2::ActionInfoDistPoint2Entity : RS2::ActionInfoDistEntity2Point) {
    m_selectionMode = fromPointToEntity ? FIRST_IS_POINT : FIRST_IS_ENTITY;
}

LC_ActionInfoDistPointToEntity::~LC_ActionInfoDistPointToEntity() = default;

void LC_ActionInfoDistPointToEntity::doSaveOptions() {
    save("NearestIsOnEntity", m_nearestPointShouldBeOnEntity);
}

void LC_ActionInfoDistPointToEntity::doLoadOptions() {
    m_nearestPointShouldBeOnEntity = loadBool("NearestIsOnEntity", true);
}

bool LC_ActionInfoDistPointToEntity::isInVisualSnapStatus(int status) {
    return (status == SetPoint);
}

void LC_ActionInfoDistPointToEntity::init(const int status) {
    RS_PreviewActionInterface::init(status);
    if (status == 0) {
        if (m_selectionMode == FIRST_IS_POINT) {
            setStatus(SetPoint);
        }
        else {
            m_savedRelZero = getRelativeZero();
            setStatus(SetEntity);
        }
    }
    else if (status < 0) {
        restoreRelZero();
    }
}

void LC_ActionInfoDistPointToEntity::doInitWithContextEntity(RS_Entity* contextEntity, [[maybe_unused]] const RS_Vector& clickPos) {
    m_entity = contextEntity;
    setStatus(SetPoint);
    highlightHover(m_entity);
    drawPreview();
}

void LC_ActionInfoDistPointToEntity::finish() {
    RS_PreviewActionInterface::finish();
    restoreRelZero();
}

void LC_ActionInfoDistPointToEntity::restoreRelZero() {
    if (m_selectionMode == FIRST_IS_ENTITY) {
        moveRelativeZero(m_savedRelZero);
    }
}

// fixme - consider displaying information in EntityInfo widget
void LC_ActionInfoDistPointToEntity::doTrigger() {
    RS_DEBUG->print("RS_ActionInfoDist2::trigger()");
    if (m_point.valid && m_entity != nullptr) {
        RS_Vector dV;
        if (m_selectionMode == FIRST_IS_ENTITY) {
            dV = m_point - m_entityNearestPoint;
        }
        else {
            dV = m_entityNearestPoint - m_point;
        }
        QStringList dists;
        for (const double a : {dV.magnitude(), dV.x, dV.y, m_entityNearestPoint.x, m_entityNearestPoint.y, m_point.x, m_point.y}) {
            dists << formatLinear(a);
        }
        const double wcsAngle = dV.angle();
        QString angle = formatWCSAngle(wcsAngle);
        commandMessage("---");
        const QString& msgTemplate = tr(
            "Distance: %1\nCartesian: (%2 , %3)\nPolar: (%4 < %5)\nPoint On Entity: (%6 , %7)\nPoint: (%8 , %9)");
        const QString message = msgTemplate.arg(dists[0], dists[1], dists[2], dists[0], angle, dists[3], dists[4], dists[5], dists[6]);
        commandMessage(message);
        restoreRelZero();
    }
}

void LC_ActionInfoDistPointToEntity::onMouseMoveEvent(const int status, const LC_MouseEvent* e) {
    RS_Vector snap = e->snapPoint;
    switch (status) {
        case SetEntity: {
            deleteSnapper();
            const auto en = doCatchEntity(e, true);
            if (en != nullptr) {
                highlightHover(en);
                deleteSnapper();
                if (m_point.valid) {
                    if (m_selectionMode == FIRST_IS_POINT) {
                        const RS_Vector nearest = en->getNearestPointOnEntity(m_point, m_nearestPointShouldBeOnEntity);
                        previewLine(nearest, m_point);
                        updateInfoCursor(nearest, m_point);
                        if (m_showRefEntitiesOnPreview) {
                            previewRefLine(nearest, m_point);
                            previewRefPoint(m_point);
                            previewRefSelectablePoint(nearest);
                        }
                    }
                    else {
                        // no more
                    }
                }
            }
            else {
                if (m_selectionMode == FIRST_IS_POINT) {
                    previewLine(snap, m_point);
                    updateInfoCursor(snap, m_point);
                    if (m_showRefEntitiesOnPreview) {
                        previewRefLine(snap, m_point);
                        previewRefPoint(m_point);
                    }
                }
            }
            break;
        }
        case SetPoint: {
            switch (m_selectionMode) {
                case FIRST_IS_POINT: {
                    trySnapToRelZeroCoordinateEvent(e);
                    if (m_showRefEntitiesOnPreview) {
                        previewRefPoint(snap);
                    }
                    break;
                }
                case FIRST_IS_ENTITY: {
                    if (m_entity != nullptr) {
                        highlightSelected(m_entity);
                        snap = getRelZeroAwarePoint(e, snap);
                        const RS_Vector nearest = obtainNearestPointOnEntity(snap);
                        previewLine(nearest, snap);
                        updateInfoCursor(snap, nearest);
                        if (m_showRefEntitiesOnPreview) {
                            previewRefLine(nearest, snap);
                            previewLine(nearest, snap);
                            previewRefPoint(nearest);
                            previewRefSelectablePoint(snap);
                        }

                        // move relative point so we'll have proper distance and angle from entity to point in coordinates widget
                        if (e->isControl) {
                            moveRelativeZero(nearest);
                        }
                    }
                    break;
                }
                default:
                    break;
            }
            break;
        }
        default:
            break;
    }
}

void LC_ActionInfoDistPointToEntity::onMouseLeftButtonRelease(const int status, const LC_MouseEvent* e) {
    switch (status) {
        case SetEntity: {
            m_entity = doCatchEntity(e, false);
            if (m_entity != nullptr) {
                switch (m_selectionMode) {
                    case FIRST_IS_POINT: {
                        m_entityNearestPoint = obtainNearestPointOnEntity(m_point);
                        trigger();
                        setStatus(SetPoint);
                        break;
                    }
                    case FIRST_IS_ENTITY: {
                        setStatus(SetPoint);
                        break;
                    }
                    default:
                        break;
                }
            }
            invalidateSnapSpot();
            break;
        }
        case SetPoint: {
            RS_Vector snap = e->snapPoint;
            snap = getRelZeroAwarePoint(e, snap);
            switch (m_selectionMode) {
                case FIRST_IS_POINT: {
                    m_point = snap;
                    addSnappedPointToVisualSnap(snap);
                    moveRelativeZero(m_point);
                    setStatus(SetEntity);
                    break;
                }
                case FIRST_IS_ENTITY: {
                    m_entityNearestPoint = obtainNearestPointOnEntity(snap);
                    fireCoordinateEvent(snap);
                    break;
                }
                default:
                    break;
            }
            break;
        }
        default:
            break;
    }
}

void LC_ActionInfoDistPointToEntity::onMouseRightButtonRelease(const int status, [[maybe_unused]] const LC_MouseEvent* e) {
    deletePreview();
    int newStatus = -1;
    const bool firstIsPoint = m_selectionMode == FIRST_IS_POINT;
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

RS_Vector LC_ActionInfoDistPointToEntity::obtainNearestPointOnEntity(const RS_Vector& snap) const {
    return m_entity->getNearestPointOnEntity(snap, m_nearestPointShouldBeOnEntity);
}

RS_Entity* LC_ActionInfoDistPointToEntity::doCatchEntity(const LC_MouseEvent* e, const bool preview) const {
    RS2::ResolveLevel level = RS2::ResolveAll;
    if (e->isControl) {
        level = RS2::ResolveNone;
    }
    if (preview) {
        return catchAndDescribe(e, level);
    }
    return catchEntityByEvent(e, level);
}

void LC_ActionInfoDistPointToEntity::onCoordinateEvent(const int status, [[maybe_unused]] bool isZero, const RS_Vector& pos) {
    if (status == SetPoint) {
        addSnappedPointToVisualSnap(m_point);
        m_point = pos;
        moveRelativeZero(m_point);
        switch (m_selectionMode) {
            case FIRST_IS_POINT: {
                setStatus(SetEntity);
                break;
            }
            case FIRST_IS_ENTITY: {
                trigger();
                setStatus(SetEntity);
                break;
            }
            default:
                break;
        }
    }
}

void LC_ActionInfoDistPointToEntity::updateActionPrompt() {
    switch (getStatus()) {
        case SetEntity:
            updatePromptTRCancel(tr("Specify entity"), MOD_CTRL(tr("Do not snap to child entities in container")));
            break;
        case SetPoint: {
            if (m_selectionMode == FIRST_IS_ENTITY) {
                updatePromptTRCancel(tr("Specify point"), MOD_SHIFT_AND_CTRL(MSG_REL_ZERO, tr("Relative point is on entity")));
            }
            else {
                updatePromptTRCancel(tr("Specify point"), MOD_SHIFT_RELATIVE_ZERO);
            }

            break;
        }
        default:
            updatePrompt();
            break;
    }
}

RS2::CursorType LC_ActionInfoDistPointToEntity::doGetMouseCursor([[maybe_unused]] const int status) {
    switch (status) {
        case SetEntity: {
            return RS2::SelectCursor;
        }
        default: {
            return RS2::CadCursor;
        }
    }
}

// todo - refactor, this is a copy from RS_ActionInfoDist
void LC_ActionInfoDistPointToEntity::updateInfoCursor(const RS_Vector& mouse, const RS_Vector& startPoint) const {
    if (m_infoCursorOverlayPrefs->enabled) {
        msg(tr("Info")).linear(tr("Distance:"), startPoint.distanceTo(mouse)).wcsAngle(tr("Angle:"), startPoint.angleTo(mouse)).
                        vector(tr("From:"), startPoint).vector(tr("To:"), mouse).toInfoCursorZone2(false);
    }
}

LC_ActionOptionsWidget* LC_ActionInfoDistPointToEntity::createOptionsWidget() {
    return new LC_InfoDistPointToEntityOptionsWidget();
}

LC_ActionOptionsPropertiesFiller* LC_ActionInfoDistPointToEntity::createOptionsFiller() {
    return new LC_InfoDistPointToEntityOptionsFiller();
}
