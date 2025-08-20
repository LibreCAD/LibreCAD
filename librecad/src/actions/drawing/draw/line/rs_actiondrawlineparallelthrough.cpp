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

#include "rs_actiondrawlineparallelthrough.h"

#include "qg_lineparallelthroughoptions.h"
#include "rs_creation.h"
#include "rs_debug.h"
#include "rs_polyline.h"
// fixme - sand - consider relaxing existing restrictions, if any - and use no-restrictions mode for this action.

namespace {
    //this holds a list of entity types which supports tangent
    const auto g_supportedEntityTypes = EntityTypeList{
                {RS2::EntityArc, RS2::EntityCircle, RS2::EntityLine/*, RS2::EntityParabola, RS2::EntitySplinePoints*/}
    };
}

RS_ActionDrawLineParallelThrough::RS_ActionDrawLineParallelThrough(LC_ActionContext *actionContext)
		:RS_PreviewActionInterface("Draw Parallels", actionContext,RS2::ActionDrawLineParallelThrough)
		, m_coord(new RS_Vector{}),m_lastStatus(SetEntity){
    m_SnapDistance=1.;
}

RS_ActionDrawLineParallelThrough::~RS_ActionDrawLineParallelThrough() = default;

void RS_ActionDrawLineParallelThrough::finish(bool updateTB){
    if(m_entity){
        m_entity->setHighlighted(false);
        m_entity=nullptr;
    }
    RS_PreviewActionInterface::finish(updateTB);
}

void RS_ActionDrawLineParallelThrough::doInitWithContextEntity(RS_Entity* contextEntity, const RS_Vector& clickPos) {
    auto entity = contextEntity;
    if (isPolyline(contextEntity)) {
        auto polyline = static_cast<RS_Polyline*>(contextEntity);
        entity = polyline->getNearestEntity(clickPos);
    }
    RS2::EntityType rtti = entity->rtti();
    if (g_supportedEntityTypes.contains(rtti)) {
        m_entity = entity;
        setStatus(SetPos);
    }
}

void RS_ActionDrawLineParallelThrough::doTrigger() {
    if (m_entity != nullptr){
        RS_Creation creation(m_container, m_viewport);
        RS_Entity *e = creation.createParallelThrough(*m_coord,m_numberToCreate,m_entity, m_symmetric);

        if (e == nullptr){
            RS_DEBUG->print("RS_ActionDrawLineParallelThrough::trigger: No parallels added\n");
        }
    }
}

void RS_ActionDrawLineParallelThrough::onMouseMoveEvent([[maybe_unused]]int status, LC_MouseEvent *e) {
    const RS_Vector &snap = e->snapPoint;
    switch (status) {
        case SetEntity: {
            auto entity = catchAndDescribe(e, RS2::ResolveAll);
            if (entity != nullptr && g_supportedEntityTypes.contains(entity->rtti())){
                m_entity = entity;
                highlightHover(m_entity);
                if (m_showRefEntitiesOnPreview) {
                    RS_Vector nearest = m_entity->getNearestPointOnEntity(*m_coord, false);
                    previewRefPoint(nearest);
                }
            }
            break;
        }
        case SetPos: {
            *m_coord = getFreeSnapAwarePoint(e, snap);
            highlightSelected(m_entity);
            RS_Creation creation(m_preview.get(), nullptr, false);
            auto en = creation.createParallelThrough(*m_coord, m_numberToCreate, m_entity, m_symmetric);
            if (en != nullptr){
                RS_Vector nearest = m_entity->getNearestPointOnEntity(*m_coord, false);
                moveRelativeZero(nearest); // fixme - should we restore original relzero?
                if (m_numberToCreate == 1 && !m_symmetric){
                    prepareEntityDescription(en, RS2::EntityDescriptionLevel::DescriptionCreating);
                }
                else{
                    int creatingNumber = m_numberToCreate * (m_symmetric ? 2 : 1);
                    appendInfoCursorEntityCreationMessage(QString::number(creatingNumber) + tr(" entities will be created"));
                }
                if (m_showRefEntitiesOnPreview) {
                    previewRefPoint(nearest);
                    previewRefLine(nearest, *m_coord);

                    if (m_symmetric && isLine(m_entity)){ // fixme - support of polyline
                        RS_Vector otherPoint = *m_coord;
                        otherPoint.mirror(m_entity->getStartpoint(), m_entity->getEndpoint());
                        previewRefPoint(otherPoint);
                        previewRefLine(nearest, otherPoint);
                    }
                }
            }
            break;
        }
        default:
            break;
    }
}

void RS_ActionDrawLineParallelThrough::onMouseLeftButtonRelease(int status, LC_MouseEvent *e) {
    switch (status) {
        case SetEntity: {
            auto entity = catchEntityByEvent(e, RS2::ResolveAll);
            if (entity != nullptr && g_supportedEntityTypes.contains(entity->rtti())){
                m_entity = entity;
                setStatus(SetPos);
            }
            break;
        }
        case SetPos: {
            fireCoordinateEventForSnap(e);
            break;
        }
        default:
            break;
    }
}

void RS_ActionDrawLineParallelThrough::onMouseRightButtonRelease(int status, [[maybe_unused]]LC_MouseEvent *e) {
    deletePreview();
    if (m_entity != nullptr){
        m_entity = nullptr;
    }
    initPrevious(status);
}

void RS_ActionDrawLineParallelThrough::onCoordinateEvent(int status,[[maybe_unused]] bool isZero, const RS_Vector &mouse) {
    switch (status) {
        case SetPos: {
            auto pos = mouse;
            *m_coord = pos;
            trigger();
            break;
        }
        default:
            break;
    }
}

void RS_ActionDrawLineParallelThrough::updateMouseButtonHints(){
    switch (getStatus()) {
        case SetEntity:
            updateMouseWidgetTRCancel(tr("Select entity"));
            break;
        case SetPos:
            updateMouseWidgetTRBack(tr("Specify through point"), MOD_SHIFT_FREE_SNAP);
            break;
        case SetNumber:
            updateMouseWidgetTRBack(tr("Number:"));
            break;
        default:
            updateMouseWidget();
            break;
    }
}

bool RS_ActionDrawLineParallelThrough::doProcessCommand(int status, const QString &c) {
    bool accept = false;

    switch (status) {
        case SetEntity:
        case SetPos: {
            if (checkCommand("number", c)){
                deletePreview();
                m_lastStatus = (Status) getStatus();
                setStatus(SetNumber);
                accept = true;
            }
            break;
        }
        case SetNumber: {
            bool ok;
            int n = c.toInt(&ok);
            if (ok){
                accept = true;
                if (n > 0 && n < 100){
                    m_numberToCreate = n;
                } else {
                    commandMessage(tr("Not a valid number. Try 1..99"));
                }
            } else {
                commandMessage(tr("Not a valid expression"));
            }
            updateOptions();
            setStatus(m_lastStatus);
            break;
        }
        default:
            break;
    }
    return accept;
}

QStringList RS_ActionDrawLineParallelThrough::getAvailableCommands(){
    QStringList cmd;
    switch (getStatus()) {
        case SetEntity:
            cmd += command("number");
            break;
        default:
            break;
    }
    return cmd;
}
RS2::CursorType RS_ActionDrawLineParallelThrough::doGetMouseCursor([[maybe_unused]] int status){
    switch (status) {
        case SetEntity:
            return RS2::SelectCursor;
        case SetNumber:
        case SetPos:
            return RS2::CadCursor;
        default:
            return RS2::NoCursorChange;
    }
}

int RS_ActionDrawLineParallelThrough::getNumber() const{
    return m_numberToCreate;
}

void RS_ActionDrawLineParallelThrough::setNumber(int n) {
    m_numberToCreate = n;
}

LC_ActionOptionsWidget* RS_ActionDrawLineParallelThrough::createOptionsWidget(){
    return new QG_LineParallelThroughOptions();
}
