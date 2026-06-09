/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2021 Melwyn Francis Carlo <carlo.melwyn@outlook.com>
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

#include "lc_actiondimarc.h"

#include <iostream>

#include "rs_arc.h"
#include "rs_debug.h"
#include "rs_preview.h"

LC_ActionDimArc::LC_ActionDimArc(LC_ActionContext *actionContext):
    RS_ActionDimension("Draw Arc Dimensions", actionContext,RS2::EntityDimArc, RS2::ActionDimArc){
    LC_ActionDimArc::reset();
}

LC_ActionDimArc::~LC_ActionDimArc() = default;

void LC_ActionDimArc::reset(){
    RS_ActionDimension::reset();
    m_dimArcData.radius = 0.0;
    m_dimArcData.arcLength = 0.0;

    m_dimArcData.centre = RS_Vector(false);
    m_dimArcData.endAngle = RS_Vector(false);
    m_dimArcData.startAngle = RS_Vector(false);

    m_selectedArcEntity = nullptr;

    updateOptions(); // fixme - check whether it's necessary there
}

void LC_ActionDimArc::doInitWithContextEntity(RS_Entity* contextEntity, [[maybe_unused]]const RS_Vector& clickPos) {
   setArcEntity(contextEntity);
}

RS_Entity* LC_ActionDimArc::doTriggerCreateEntity() {
    if (m_selectedArcEntity == nullptr){
        RS_DEBUG->print(RS_Debug::D_ERROR, "LC_ActionDimArc::trigger: selectedArcEntity is nullptr.\n");
        return nullptr;
    }
    if (!m_dimArcData.centre.valid){
        RS_DEBUG->print(RS_Debug::D_ERROR, "LC_ActionDimArc::trigger: dimArcData.centre is not valid.\n");
        return nullptr;
    }
    const auto newEntity = new LC_DimArc(m_document, *m_dimensionData, m_dimArcData);
    newEntity->update();
    return newEntity;
}

void LC_ActionDimArc::doTriggerCompletion([[maybe_unused]]bool success) {
    setStatus(SetEntity);
    RS_Snapper::finish();
}

void LC_ActionDimArc::onMouseMoveEvent(const int status, const LC_MouseEvent* e) {
    RS_Vector snap = e->snapPoint;
    switch (status) {
        case SetEntity:{
            const auto en = catchEntityByEvent(e,RS2::EntityArc, RS2::ResolveAll);
            if (en != nullptr){
                highlightHover(en);
            }
            break;
        }
        case SetPos: {
            snap = getFreeSnapAwarePoint(e, snap);
            highlightSelected(m_selectedArcEntity);
            setRadius(snap);

            // fixme - determine why DimArc is drawn on preview by preview pen, while other dimension entities - using normal pen...

            const LC_DimArc *tempDimArcEntity{new LC_DimArc(m_preview.get(), *m_dimensionData, m_dimArcData)};
            previewEntity(tempDimArcEntity);
            break;
        }
        default:
            break;
    }
}

void LC_ActionDimArc::setArcEntity(RS_Entity* entity) {
    if (entity->is(RS2::EntityArc)){
        m_selectedArcEntity = static_cast<RS_Arc*>(entity);
        m_dimArcData.centre = m_selectedArcEntity->getCenter();
        m_dimArcData.arcLength = m_selectedArcEntity->getLength();

        const auto selectedEntity = m_selectedArcEntity;
        m_dimArcData.startAngle = RS_Vector(selectedEntity->getAngle1());
        m_dimArcData.endAngle = RS_Vector(selectedEntity->getAngle2());

        m_dimensionData->definitionPoint = m_selectedArcEntity->getStartpoint();

        if (m_selectedArcEntity->isReversed()){
            const auto tempAngle = RS_Vector(m_dimArcData.startAngle);
            m_dimArcData.startAngle = m_dimArcData.endAngle;
            m_dimArcData.endAngle = tempAngle;
            m_dimensionData->definitionPoint = m_selectedArcEntity->getEndpoint();
        }

        setStatus(SetPos);
    } else {
        RS_DEBUG->print(RS_Debug::D_ERROR,
                        "LC_ActionDimArc::mouseReleaseEvent: selectedArcEntity is not an arc.");

        m_selectedArcEntity = nullptr;
    }
}

void LC_ActionDimArc::onMouseLeftButtonRelease(const int status, const LC_MouseEvent* e) {
    switch (status) {
        case SetEntity: {
            const auto entity = catchEntityByEvent(e, RS2::ResolveAll);
            if (entity != nullptr){
                setArcEntity(entity);
            }
            break;
        }
        case SetPos: {
            RS_Vector snap = e->snapPoint;
            snap = getFreeSnapAwarePoint(e, snap);
            fireCoordinateEvent(snap);
            break;
        }
        default:
            break;
    }
}

void LC_ActionDimArc::onMouseRightButtonRelease(const int status, [[maybe_unused]] const LC_MouseEvent* e) {
    deletePreview();
    initPrevious(status);
}

void LC_ActionDimArc::onCoordinateEvent(const int status, [[maybe_unused]] bool isZero, const RS_Vector &pos) {
    switch (status) {
        case SetPos: {
            setRadius(pos);
            trigger();
            reset();
            setStatus(SetEntity);
            break;
        }
        default:
            break;
    }
}

bool LC_ActionDimArc::doProcessCommand([[maybe_unused]]int status, const QString& command){
    // fixme - support other commands
    bool accept = false;
    if (checkCommand("exit", command)){
        init(-1);
        accept = true;
    }
    return accept;
}

QStringList LC_ActionDimArc::getAvailableCommands(){
    QStringList availableCommandsList{"help", "exit"};
    return availableCommandsList;
}

void LC_ActionDimArc::updateActionPrompt(){
    switch (getStatus()) {
        case SetEntity:
            updatePromptTRCancel(tr("Select arc entity"));
            break;
        case SetPos:
            updatePromptTRBack(tr("Specify dimension arc location"),MOD_SHIFT_FREE_SNAP);
            break;
        default:
            updatePrompt();
            break;
    }
}

void LC_ActionDimArc::setRadius(const RS_Vector &selectedPosition){
    constexpr double minimumDimArcGap = 0.0;
    m_dimArcData.radius = selectedPosition.distanceTo(m_dimArcData.centre);
    const double minimumRadius = m_selectedArcEntity->getRadius() + minimumDimArcGap;
    if (m_dimArcData.radius < minimumRadius) {
        m_dimArcData.radius = minimumRadius;
    }
}
