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
#include "lc_ucssetoptions.h"

LC_ActionUCSCreate::LC_ActionUCSCreate(LC_ActionContext *actionContext)
    :RS_PreviewActionInterface("UCSCreate", actionContext, RS2::ActionUCSCreate){
}

LC_ActionUCSCreate::~LC_ActionUCSCreate() = default;

void LC_ActionUCSCreate::doTrigger() {
//   LC_ERR << "SET Origin. UCS: " << formatVector(m_originPoint) << " World: "<< formatVectorWCS(m_originPoint) << " Angle: " << formatAngle(m_angle);

   double angle = m_fixedAngle ? toWorldAngle(m_angle) : m_angle;
   m_viewport->createUCS(m_originPoint, angle);
   setStatus(-1);
   finish(false);
}

void LC_ActionUCSCreate::initFromSettings() {
    RS_Snapper::initFromSettings();
    m_ucsMarkOptions.loadSettings();
}

void LC_ActionUCSCreate::showUCSMark(RS_Vector &point, double angle){
    double uiX, uiY;
    m_viewport->toUI(point, uiX, uiY);
    auto *ucsMark = new LC_OverlayUCSMark({uiX, uiY}, angle, false, &m_ucsMarkOptions);
    auto overlayContainer = m_viewport->getOverlaysDrawablesContainer(RS2::OverlayGraphics::ActionPreviewEntity);
    overlayContainer->add(ucsMark);
}

void LC_ActionUCSCreate::onMouseMoveEvent(int status, LC_MouseEvent *event) {
    RS_Vector snap = event->snapPoint;
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
            RS_Vector pos = getSnapAngleAwarePoint(event, m_originPoint, snap, true);
            m_currentAngle = m_originPoint.angleTo(pos);
            updateOptionsUI(1);
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

void LC_ActionUCSCreate::onMouseLeftButtonRelease(int status, LC_MouseEvent *e) {
    RS_Vector snap = e->snapPoint;
    switch (status){
        case SetOrigin:{
            snap = getRelZeroAwarePoint(e, snap);
            break;        }
        case SetAngle:{
            snap = getSnapAngleAwarePoint(e, m_originPoint, snap);
            break;
        }
        default:
            break;
    }
    fireCoordinateEvent(snap);
}


void LC_ActionUCSCreate::onCoordinateEvent(int status, [[maybe_unused]]bool isZero, const RS_Vector &pos) {
    switch (status){
        case SetOrigin:{
            m_originPoint = pos;
            if (m_fixedAngle){
                trigger();
            }
            else {
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

bool LC_ActionUCSCreate::doUpdateAngleByInteractiveInput(const QString& tag, double angle) {
    if (tag == "angle") {
        setAngle(angle);
        return true;
    }
    return false;
}

void LC_ActionUCSCreate::onMouseRightButtonRelease([[maybe_unused]]int status, [[maybe_unused]]LC_MouseEvent *e) {
    setStatus(getStatus() - 1); // fixme - temporary
}

QStringList LC_ActionUCSCreate::getAvailableCommands() {
    return RS_ActionInterface::getAvailableCommands();
}

void LC_ActionUCSCreate::updateMouseButtonHints() {
    switch (getStatus()){
        case SetOrigin:{
            updateMouseWidgetTRCancel("Specify origin point", MOD_SHIFT_RELATIVE_ZERO);
            break;
        }
        case SetAngle:{
            updateMouseWidgetTRCancel("Specify direction point for X axis", MOD_SHIFT_ANGLE_SNAP);
            break;
        }
        default:
            updateMouseWidget();
    }
}

bool LC_ActionUCSCreate::doProcessCommand(int status, const QString &command) {
    return RS_ActionInterface::doProcessCommand(status, command);
}


LC_ActionOptionsWidget *LC_ActionUCSCreate::createOptionsWidget() {
    return new LC_UCSSetOptions();
}

RS2::CursorType LC_ActionUCSCreate::doGetMouseCursor([[maybe_unused]]int status) {
    return RS2::CadCursor;
}
