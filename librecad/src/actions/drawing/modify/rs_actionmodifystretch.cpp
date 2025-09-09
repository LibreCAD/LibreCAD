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

#include "rs_actionmodifystretch.h"

#include "lc_actioninfomessagebuilder.h"
#include "lc_modifystretchoptions.h"
#include "rs_debug.h"
#include "rs_modification.h"
#include "rs_preview.h"

struct RS_ActionModifyStretch::StretchActionData {
    RS_Vector firstCorner;
    RS_Vector secondCorner;
    RS_Vector referencePoint;
    RS_Vector targetPoint;
};

RS_ActionModifyStretch::RS_ActionModifyStretch(LC_ActionContext *actionContext)
    :RS_PreviewActionInterface("Stretch Entities", actionContext, RS2::ActionModifyStretch),
    m_actionData(std::make_unique<StretchActionData>()){
}

void RS_ActionModifyStretch::init(int status) {
    RS_PreviewActionInterface::init(status);
}

RS_ActionModifyStretch::~RS_ActionModifyStretch() = default;

void RS_ActionModifyStretch::doTrigger() {
    RS_DEBUG->print("RS_ActionModifyStretch::trigger()");

    RS_Modification m(*m_container, m_viewport);
    m.stretch(m_actionData->firstCorner,
              m_actionData->secondCorner,
              m_actionData->targetPoint - m_actionData->referencePoint, m_removeOriginals);
    if (m_removeOriginals) {
        setStatus(SetFirstCorner);
    }
    else{
        setStatus(SetTargetPoint);
    }
}

void RS_ActionModifyStretch::onMouseMoveEvent(int status, LC_MouseEvent *e) {
    RS_Vector mouse = e->snapPoint;
    switch (status) {
        case SetFirstCorner:{
            break;
        }
        case SetSecondCorner: {
            if (m_actionData->firstCorner.valid) {
                m_actionData->secondCorner = e->snapPoint;
                previewStretchRect(false);                
                if (isInfoCursorForModificationEnabled()){
                    msg(tr("Stretch"))
                        .vector(tr("Start Corner:"), m_actionData->firstCorner)
                        .vector(tr("End Corner:"), m_actionData->secondCorner)
                        .toInfoCursorZone2(false);
                }
            }
            break;
        }
        case SetReferencePoint: {            
            previewStretchRect(true);
            trySnapToRelZeroCoordinateEvent(e);            
            if (isInfoCursorForModificationEnabled()) {
                msg(tr("Stretch"))
                    .vector(tr("Reference Point:"), mouse)
                    .toInfoCursorZone2(false);
            }    
            break;
        }
        case SetTargetPoint: {
            if (m_actionData->referencePoint.valid) {                
                mouse= getSnapAngleAwarePoint(e, m_actionData->referencePoint, mouse, true);
                m_actionData->targetPoint = mouse;
                // fixme - isn't it more reliable to rely on RS_Modification::stretch there?
                m_preview->addStretchablesFrom(*m_container, m_viewport, m_actionData->firstCorner, m_actionData->secondCorner);
                const RS_Vector &offset = m_actionData->targetPoint - m_actionData->referencePoint;
                m_preview->stretch(m_actionData->firstCorner, m_actionData->secondCorner,offset);
                if (m_showRefEntitiesOnPreview) {
                    previewRefPoint(m_actionData->referencePoint);
                    previewRefSelectablePoint(m_actionData->targetPoint);
                    previewRefLine(m_actionData->referencePoint, m_actionData->targetPoint);
                }
                if (isInfoCursorForModificationEnabled()) {
                    msg(tr("Stretch"))
                        .vector(tr("Reference Point:"), m_actionData->referencePoint)
                        .vector(tr("Target Point:"), mouse)
                        .string(tr("Offset:"))
                        .relative(mouse)
                        .relativePolar(mouse)
                        .toInfoCursorZone2(false);
                }                
            }
            break;
        }
        default:
            break;
    }
}

void RS_ActionModifyStretch::previewStretchRect(bool selected) {
    if (m_showRefEntitiesOnPreview){
        RS_Vector v0 = m_actionData->firstCorner;
        RS_Vector v1 = m_actionData->secondCorner;
        previewRefLine(v0, {v1.x, v0.y});
        previewRefLine({v1.x, v0.y}, v1);
        previewRefLine(v1, {v0.x, v1.y});
        previewRefLine({v0.x, v1.y}, v0);
        previewRefPoint(v0);
        if (selected){
            previewRefPoint(v1);
        }
        else{
            previewRefSelectablePoint(v1);
        }
    }
    else{
        m_preview->addRectangle(m_actionData->firstCorner, m_actionData->secondCorner);
    }
}

void RS_ActionModifyStretch::onMouseLeftButtonRelease([[maybe_unused]]int status, LC_MouseEvent *e) {
    if (status == SetTargetPoint){
        RS_Vector mouse= getSnapAngleAwarePoint(e, m_actionData->referencePoint, e->snapPoint);
        fireCoordinateEvent(mouse);
    }
    else {
        fireCoordinateEventForSnap(e);
    }
}
// fixme - default back - remove as well as from other actions and rely to parent class
void RS_ActionModifyStretch::onMouseRightButtonRelease(int status, [[maybe_unused]] LC_MouseEvent *e) {
    deletePreview();
    initPrevious(status);
}

void RS_ActionModifyStretch::onCoordinateEvent(int status,  [[maybe_unused]]bool isZero, const RS_Vector &mouse) {
    switch (status) {
        case SetFirstCorner: {
            m_actionData->firstCorner = mouse;
            setStatus(SetSecondCorner);
            break;
        }
        case SetSecondCorner: {
            m_actionData->secondCorner = mouse;
            deletePreview();
            setStatus(SetReferencePoint);
            break;
        }
        case SetReferencePoint: {
            m_actionData->referencePoint = mouse;
            moveRelativeZero(m_actionData->referencePoint);
            setStatus(SetTargetPoint);
            break;
        }
        case SetTargetPoint: {
            m_actionData->targetPoint = mouse;
            moveRelativeZero(m_actionData->targetPoint);
            trigger();
            break;
        }
        default:
            break;
    }
}

void RS_ActionModifyStretch::updateMouseButtonHints(){
    switch (getStatus()) {
        case SetFirstCorner:
            updateMouseWidgetTRCancel(tr("Specify first corner"));
            break;
        case SetSecondCorner:
            updateMouseWidgetTRBack(tr("Specify second corner"));
            break;
        case SetReferencePoint:
            updateMouseWidgetTRBack(tr("Specify reference point"), MOD_SHIFT_RELATIVE_ZERO);
            break;
        case SetTargetPoint:
            updateMouseWidgetTRBack(tr("Specify target point"), MOD_SHIFT_ANGLE_SNAP);
            break;
        default:
            updateMouseWidget();
            break;
    }
}
RS2::CursorType RS_ActionModifyStretch::doGetMouseCursor([[maybe_unused]] int status){
    return RS2::CadCursor;
}

LC_ActionOptionsWidget* RS_ActionModifyStretch::createOptionsWidget() {
    return new LC_ModifyStretchOptions();
}
