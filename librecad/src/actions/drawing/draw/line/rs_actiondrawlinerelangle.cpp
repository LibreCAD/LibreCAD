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

#include "rs_actiondrawlinerelangle.h"

#include "qg_linerelangleoptions.h"
#include "rs_creation.h"
#include "rs_line.h"
#include "rs_polyline.h"

namespace {

//list of entity types supported by current action
const auto g_enTypeList = EntityTypeList{RS2::EntityLine, RS2::EntityArc, RS2::EntityCircle,RS2::EntityEllipse};
}

// fixme - sand - add support of options for line snap point
RS_ActionDrawLineRelAngle::RS_ActionDrawLineRelAngle(LC_ActionContext *actionContext,double ang,bool fixedAngle)
    :RS_PreviewActionInterface("Draw Lines with relative angles",actionContext)
    , m_pos(std::make_unique<RS_Vector>())
    , m_fixedAngle(fixedAngle){
    m_relativeAngleRad = /*RS_Math::rad2deg(ang)*/ ang;
}

RS_ActionDrawLineRelAngle::~RS_ActionDrawLineRelAngle() = default;

void RS_ActionDrawLineRelAngle::doInitWithContextEntity(RS_Entity* contextEntity, const RS_Vector& clickPos) {
    auto entity = contextEntity;
    if (isPolyline(entity)) {
        auto polyline = static_cast<RS_Polyline*> (contextEntity);
        entity = polyline->getNearestEntity(clickPos);
    }
    RS2::EntityType rtti = entity->rtti();
    if (g_enTypeList.contains(rtti)) {
        setEntity(entity);
    }
}

void RS_ActionDrawLineRelAngle::setAngle(double angleDeg) {
    m_relativeAngleRad = adjustRelativeAngleSignByBasis(RS_Math::deg2rad(angleDeg));
}

double RS_ActionDrawLineRelAngle::getAngle() const {
    return adjustRelativeAngleSignByBasis(RS_Math::rad2deg(m_relativeAngleRad));
}

bool RS_ActionDrawLineRelAngle::doUpdateAngleByInteractiveInput(const QString& tag, double angleRad) {
    if (tag == "angle") {
        m_relativeAngleRad = adjustRelativeAngleSignByBasis(angleRad);
        return true;
    }
    return false;
}

bool RS_ActionDrawLineRelAngle::doUpdateDistanceByInteractiveInput(const QString& tag, double distance) {
    if (tag == "distance") {
        setLength(distance);
        return true;
    }
    return false;
}

RS2::ActionType RS_ActionDrawLineRelAngle::rtti() const{
    if( m_fixedAngle && RS_Math::getAngleDifference(m_relativeAngleRad, M_PI_2) < RS_TOLERANCE_ANGLE) {
        return RS2::ActionDrawLineOrthogonal;
    }
    else {
        return RS2::ActionDrawLineRelAngle;
    }
}

void RS_ActionDrawLineRelAngle::finish(bool updateTB) {
    RS_PreviewActionInterface::finish(updateTB);
}

void RS_ActionDrawLineRelAngle::doTrigger() {
    RS_Creation creation(m_container, m_viewport);
    moveRelativeZero(*m_pos); // fixme - to undoable?
    // the created line is added to undo and the view automatically by RS_Creation
    creation.createLineRelAngle(*m_pos,m_entity,m_relativeAngleRad,m_length);
}

void RS_ActionDrawLineRelAngle::onMouseMoveEvent(int status, LC_MouseEvent *e) {
    RS_Vector snap = e->snapPoint;
    switch (status) {
        case SetEntity: {
            m_entity = catchAndDescribe(e, g_enTypeList, RS2::ResolveAll);
            if (m_entity != nullptr){
                highlightHover(m_entity);
            }
            break;
        }
        case SetPos: {
            highlightSelected(m_entity);
            *m_pos = getRelZeroAwarePoint(e, snap);
            //fixme - sand - files - MERGE REGRESSION?
            RS_Creation creation(nullptr, nullptr, false);
            std::unique_ptr<RS_Line> lineToCreate = creation.createLineRelAngle(*m_pos, m_entity, m_relativeAngleRad, m_length);
            if (lineToCreate != nullptr){
                previewEntityToCreate(lineToCreate.get()->clone(), true);
                if (m_showRefEntitiesOnPreview) {
                    auto const vp = m_entity->getNearestPointOnEntity(*m_pos, false);
                    previewRefPoint(vp);
                    previewRefPoint(lineToCreate->getEndpoint());
                }
            }
            break;
        }
        default:
            break;
    }
}

void RS_ActionDrawLineRelAngle::setEntity(RS_Entity* en) {
    m_entity = en;
    setStatus(SetPos);
}

void RS_ActionDrawLineRelAngle::onMouseLeftButtonRelease(int status, LC_MouseEvent *e) {
    switch (status) {
        case SetEntity: {
            RS_Entity *en = catchEntityByEvent(e, g_enTypeList, RS2::ResolveAll);
            if (en != nullptr){
                setEntity(en);
            }
            break;
        }
        case SetPos: {
            const RS_Vector& snap = e->snapPoint;
            RS_Vector position = getRelZeroAwarePoint(e, snap);
            fireCoordinateEvent(position);
            break;
        }
        default:
            break;
    }
}

void RS_ActionDrawLineRelAngle::onMouseRightButtonRelease(int status, [[maybe_unused]]LC_MouseEvent *e) {
    deletePreview();
    initPrevious(status);
}

void RS_ActionDrawLineRelAngle::onCoordinateEvent(int status, [[maybe_unused]]bool isZero, const RS_Vector &coord) {
    switch (status) {
        case SetPos: {
            *m_pos = coord;
            trigger();
            break;
        }
        default:
            break;
    }
}

bool RS_ActionDrawLineRelAngle::doProcessCommand(int status, const QString &c) {
    bool accept = false;

    switch (status) {
        case SetEntity:
        case SetPos: {
            if (!m_fixedAngle && checkCommand("angle", c)){
                deletePreview();
                setStatus(SetAngle);
                accept = true;
            } else if (checkCommand("length", c)){
                deletePreview();
                setStatus(SetLength);
                accept = true;
            }
            break;
        }
        case SetAngle: {
            double angle;
            bool ok = parseToRelativeAngle(c, angle);
            if (ok){
                accept = true;
                m_relativeAngleRad = angle;
            } else {
                commandMessage(tr("Not a valid expression"));
            }
            updateOptions();
            setStatus(SetPos);
            break;
        }
        case SetLength: {
            bool ok = false;
            double l = RS_Math::eval(c, &ok);
            if (ok){
                m_length = l;
                accept = true;
            } else {
                commandMessage(tr("Not a valid expression"));
            }
            updateOptions();
            setStatus(SetPos);
            break;
        }
        default:
            break;
    }
    return accept;
}

QStringList RS_ActionDrawLineRelAngle::getAvailableCommands(){
    QStringList cmd;
    switch (getStatus()) {
        case SetPos:
        case SetLength: {
            if (!m_fixedAngle){
                cmd += command("angle");
            }
            cmd += command("length");
            break;
        }
        default:
            break;
    }
    return cmd;
}

void RS_ActionDrawLineRelAngle::updateMouseButtonHints(){
    switch (getStatus()) {
        case SetEntity:
            updateMouseWidgetTRCancel(tr("Select base entity"));
            break;
        case SetPos:
            updateMouseWidgetTRBack(tr("Specify position"), MOD_SHIFT_RELATIVE_ZERO);
            break;
        case SetAngle:
            updateMouseWidgetTRBack(tr("Specify angle"));
            break;
        case SetLength:
            updateMouseWidgetTRBack(tr("Specify length"));
            break;
        default:
            updateMouseWidget();
            break;
    }
}

RS2::CursorType RS_ActionDrawLineRelAngle::doGetMouseCursor([[maybe_unused]] int status){
    switch (status) {
        case SetEntity:
            return RS2::SelectCursor;
        case SetPos:
            return RS2::CadCursor;
        default:
            return RS2::NoCursorChange;
    }
}

LC_ActionOptionsWidget* RS_ActionDrawLineRelAngle::createOptionsWidget(){
    return new QG_LineRelAngleOptions();
}
