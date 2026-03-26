/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2025 LibreCAD.org
 Copyright (C) 2025 sand1024

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

#include "lc_actionucscreate.h"

#include "lc_graphicviewport.h"
#include "lc_overlayentitiescontainer.h"
#include "lc_ucs_create_options_filler.h"
#include "lc_ucs_create_options_widget.h"

LC_ActionUCSCreate::LC_ActionUCSCreate(LC_ActionContext *actionContext)
    :RS_PreviewActionInterface("ActionUCSCreate", actionContext, RS2::ActionUCSCreate){
}

LC_ActionUCSCreate::~LC_ActionUCSCreate() = default;

void LC_ActionUCSCreate::doSaveOptions() {
    save("Angle", m_angle);
    save("AngleIsFixed", m_fixedAngle);
}

void LC_ActionUCSCreate::doLoadOptions() {
    m_angle = loadDouble("Angle", 0.0);
    m_fixedAngle = loadBool("AngleIsFixed", true);
}

bool LC_ActionUCSCreate::isInVisualSnapStatus(int status) {
    return (status == SetOrigin) || (status == SetAngle);
}

void LC_ActionUCSCreate::doTrigger() {
//   LC_ERR << "SET Origin. UCS: " << formatVector(m_originPoint) << " World: "<< formatVectorWCS(m_originPoint) << " Angle: " << formatAngle(m_angle);

   const double angle = m_fixedAngle ? toWorldAngle(m_angle) : m_angle;
   m_viewport->createUCS(m_originPoint, angle);
   setStatus(-1);
   finish();
}

void LC_ActionUCSCreate::initFromSettings() {
    RS_Snapper::initFromSettings();
    m_ucsMarkOptions.loadSettings();
}

void LC_ActionUCSCreate::showUCSMark(const RS_Vector &point, const double angle){
    double uiX, uiY;
    m_viewport->toUI(point, uiX, uiY);
    auto *ucsMark = new LC_OverlayUCSMark({uiX, uiY}, angle, false, &m_ucsMarkOptions);
    const auto overlayContainer = m_viewport->getOverlaysDrawablesContainer(RS2::OverlayGraphics::ActionPreviewEntity);
    overlayContainer->add(ucsMark);
}

void LC_ActionUCSCreate::onMouseMoveEvent(const int status, const LC_MouseEvent* event) {
    const RS_Vector snap = event->snapPoint;
    switch (status){
        case SetOrigin:{
            if (!trySnapToRelZeroCoordinateEvent(event)) {
                showUCSMark(snap, m_fixedAngle ? -m_angle:0.0);
                if (m_showRefEntitiesOnPreview){
                    previewRefSelectablePoint(snap);
                }
            }
            break;
        }
        case SetAngle:{
            const RS_Vector pos = getSnapAngleAwarePoint(event, m_originPoint, snap, true);
            m_currentAngle = m_originPoint.angleTo(pos);
            if (m_showRefEntitiesOnPreview){
                previewRefPoint(m_originPoint);
                previewRefSelectablePoint(pos);
                previewRefLine(m_originPoint, pos);
            }
            showUCSMark(m_originPoint, -toUCSAngle(m_currentAngle));
            break;
        }
        default:
            break;
    }
}

void LC_ActionUCSCreate::onMouseLeftButtonRelease(const int status, const LC_MouseEvent* e) {
    RS_Vector snap = e->snapPoint;
    switch (status){
        case SetOrigin:{
            snap = getRelZeroAwarePoint(e, snap);
            break;
        }
        case SetAngle:{
            snap = getSnapAngleAwarePoint(e, m_originPoint, snap);
            break;
        }
        default:
            break;
    }
    fireCoordinateEvent(snap);
}


void LC_ActionUCSCreate::onCoordinateEvent(const int status, [[maybe_unused]]bool isZero, const RS_Vector &pos) {
    switch (status){
        case SetOrigin:{
            m_originPoint = pos;
            if (m_fixedAngle){
                trigger();
            }
            else {
                addSnappedPointToVisualSnap(m_originPoint);
                moveRelativeZero(m_originPoint);
                setStatus(SetAngle);
            }
            break;
        }
        case SetAngle:{
            m_angle = m_originPoint.angleTo(pos);
            trigger();
            break;
        }
        default:
            break;
    }
}

bool LC_ActionUCSCreate::doUpdateAngleByInteractiveInput(const QString& tag, const double angle) {
    if (tag == "angle") {
        setAngle(angle);
        return true;
    }
    return false;
}

void LC_ActionUCSCreate::onMouseRightButtonRelease([[maybe_unused]]int status, [[maybe_unused]] const LC_MouseEvent* e) {
    setStatus(getStatus() - 1); // fixme - temporary
}

QStringList LC_ActionUCSCreate::getAvailableCommands() {
    return RS_ActionInterface::getAvailableCommands();
}

void LC_ActionUCSCreate::updateActionPrompt() {
    switch (getStatus()){
        case SetOrigin:{
            updatePromptTRCancel("Specify origin point", MOD_SHIFT_RELATIVE_ZERO);
            break;
        }
        case SetAngle:{
            updatePromptTRCancel("Specify direction point for X axis", MOD_SHIFT_ANGLE_SNAP);
            break;
        }
        default:
            updatePrompt();
    }
}

bool LC_ActionUCSCreate::doProcessCommand(const int status, const QString &command) {
    return RS_ActionInterface::doProcessCommand(status, command);
}

LC_ActionOptionsWidget *LC_ActionUCSCreate::createOptionsWidget() {
    return new LC_UCSCreateOptionsWidget();
}

LC_ActionOptionsPropertiesFiller* LC_ActionUCSCreate::createOptionsFiller() {
    return new LC_UCSCreateOptionsFiller();
}

RS2::CursorType LC_ActionUCSCreate::doGetMouseCursor([[maybe_unused]]int status) {
    return RS2::CadCursor;
}
