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

#include <iostream>

#include "lc_actiondimarc.h"

#include "rs_arc.h"
#include "rs_debug.h"
#include "rs_preview.h"

LC_ActionDimArc::LC_ActionDimArc(LC_ActionContext *actionContext):
    RS_ActionDimension("Draw Arc Dimensions", actionContext,RS2::EntityDimArc, RS2::ActionDimArc){
    reset();
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

void LC_ActionDimArc::doTrigger() {
    if (m_selectedArcEntity == nullptr){
        RS_DEBUG->print(RS_Debug::D_ERROR, "LC_ActionDimArc::trigger: selectedArcEntity is nullptr.\n");
        return;
    }

    if (!m_dimArcData.centre.valid){
        RS_DEBUG->print(RS_Debug::D_ERROR, "LC_ActionDimArc::trigger: dimArcData.centre is not valid.\n");
        return;
    }

    auto newEntity= new LC_DimArc(m_container, *m_dimensionData, m_dimArcData);
    setPenAndLayerToActive(newEntity);
    newEntity->update();
    undoCycleAdd(newEntity);
    setStatus(SetEntity);

    RS_Snapper::finish();
}

void LC_ActionDimArc::onMouseMoveEvent(int status, LC_MouseEvent *e) {
    RS_Vector snap = e->snapPoint;
    switch (status) {
        case SetEntity:{
            auto en = catchEntityByEvent(e,RS2::EntityArc, RS2::ResolveAll);
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

            LC_DimArc *temp_dimArc_entity{new LC_DimArc(m_preview.get(), *m_dimensionData, m_dimArcData)};
            previewEntity(temp_dimArc_entity);
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

        auto selectedEntity = static_cast<RS_Arc*>(m_selectedArcEntity);
        m_dimArcData.startAngle = RS_Vector(selectedEntity->getAngle1());
        m_dimArcData.endAngle = RS_Vector(selectedEntity->getAngle2());

        m_dimensionData->definitionPoint = m_selectedArcEntity->getStartpoint();

        if (m_selectedArcEntity->isReversed()){
            const RS_Vector tempAngle = RS_Vector(m_dimArcData.startAngle);
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

void LC_ActionDimArc::onMouseLeftButtonRelease(int status, LC_MouseEvent *e) {
    switch (status) {
        case SetEntity: {
            auto entity = catchEntityByEvent(e, RS2::ResolveAll);
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

void LC_ActionDimArc::onMouseRightButtonRelease(int status, [[maybe_unused]]LC_MouseEvent *e) {
    deletePreview();
    initPrevious(status);
}

void LC_ActionDimArc::onCoordinateEvent(int status, [[maybe_unused]] bool isZero, const RS_Vector &pos) {
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

bool LC_ActionDimArc::doProcessCommand([[maybe_unused]]int status, const QString& c){
    // fixme - support other commands
    bool accept = false;
    if (checkCommand("exit", c)){
        init(-1);
        accept = true;
    }
    return accept;
}

QStringList LC_ActionDimArc::getAvailableCommands(){
    QStringList availableCommandsList{"help", "exit"};
    return availableCommandsList;
}

void LC_ActionDimArc::updateMouseButtonHints(){
    switch (getStatus()) {
        case SetEntity:
            updateMouseWidgetTRCancel(tr("Select arc entity"));
            break;
        case SetPos:
            updateMouseWidgetTRBack(tr("Specify dimension arc location"),MOD_SHIFT_FREE_SNAP);
            break;
        default:
            updateMouseWidget();
            break;
    }
}

void LC_ActionDimArc::setRadius(const RS_Vector &selectedPosition){
    const double minimum_dimArc_gap = 0.0;
    m_dimArcData.radius = selectedPosition.distanceTo(m_dimArcData.centre);
    const double minimumRadius = m_selectedArcEntity->getRadius() + minimum_dimArc_gap;
    if (m_dimArcData.radius < minimumRadius) {
        m_dimArcData.radius = minimumRadius;
    }
}
