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

#include "lc_action_draw_line_rel_angle.h"

#include "lc_line_rel_angle_options_filler.h"
#include "lc_line_rel_angle_options_widget.h"
#include "rs_creation.h"
#include "rs_document.h"
#include "rs_line.h"
#include "rs_polyline.h"

namespace {
    //list of entity types supported by current action
    const auto g_enTypeList = EntityTypeList{RS2::EntityLine, RS2::EntityArc, RS2::EntityCircle, RS2::EntityEllipse};
}

// fixme - sand - add support of options for line snap point
LC_ActionDrawLineRelAngle::LC_ActionDrawLineRelAngle(LC_ActionContext* actionContext, const double angle, const bool fixedAngle)
    : LC_SingleEntityCreationAction(fixedAngle ? "ActionDrawLineOrthogonal" : "ActionDrawLineRelAngle", actionContext),
      m_pos(std::make_unique<RS_Vector>()), m_fixedAngle(fixedAngle) {
      m_relativeAngleRad = /*RS_Math::rad2deg(ang)*/ angle;
}

LC_ActionDrawLineRelAngle::~LC_ActionDrawLineRelAngle() = default;

void LC_ActionDrawLineRelAngle::doSaveOptions() {
    if (!hasFixedAngle()) {
        save("Angle", m_relativeAngleRad);
    }
    save("Length", m_length);
}

void LC_ActionDrawLineRelAngle::doLoadOptions() {
    if (!hasFixedAngle()) {
        m_relativeAngleRad = loadDouble("Angle", 30);
    }
    m_length = loadDouble("Length", 1.0);
}

void LC_ActionDrawLineRelAngle::doInitWithContextEntity(RS_Entity* contextEntity, const RS_Vector& clickPos) {
    auto entity = contextEntity;
    if (isPolyline(entity)) {
        const auto polyline = static_cast<RS_Polyline*>(contextEntity);
        entity = polyline->getNearestEntity(clickPos);
    }
    const RS2::EntityType rtti = entity->rtti();
    if (g_enTypeList.contains(rtti)) {
        setEntity(entity);
    }
}

void LC_ActionDrawLineRelAngle::setAngle(const double angleDeg) {
    m_relativeAngleRad = adjustRelativeAngleSignByBasis(RS_Math::deg2rad(angleDeg));
}

double LC_ActionDrawLineRelAngle::getAngle() const {
    return adjustRelativeAngleSignByBasis(RS_Math::rad2deg(m_relativeAngleRad));
}

bool LC_ActionDrawLineRelAngle::doUpdateAngleByInteractiveInput(const QString& tag, const double angleRad) {
    if (tag == "angle") {
        m_relativeAngleRad = adjustRelativeAngleSignByBasis(angleRad);
        return true;
    }
    return false;
}

bool LC_ActionDrawLineRelAngle::doUpdateDistanceByInteractiveInput(const QString& tag, const double distance) {
    if (tag == "distance") {
        setLength(distance);
        return true;
    }
    return false;
}

RS2::ActionType LC_ActionDrawLineRelAngle::rtti() const {
    if (m_fixedAngle && RS_Math::getAngleDifference(m_relativeAngleRad, M_PI_2) < RS_TOLERANCE_ANGLE) {
        return RS2::ActionDrawLineOrthogonal;
    }
    return RS2::ActionDrawLineRelAngle;
}

void LC_ActionDrawLineRelAngle::finish() {
    RS_PreviewActionInterface::finish();
}

RS_Entity* LC_ActionDrawLineRelAngle::doTriggerCreateEntity() {
    moveRelativeZero(*m_pos);
    const auto line = RS_Creation::createLineRelAngle(*m_pos, m_entity, m_relativeAngleRad, m_length);
    return line;
}

void LC_ActionDrawLineRelAngle::doTriggerCompletion([[maybe_unused]] bool success) {
}

bool LC_ActionDrawLineRelAngle::isInVisualSnapStatus(int status) {
    return (status == SetPos);
}

void LC_ActionDrawLineRelAngle::onMouseMoveEvent(const int status, const LC_MouseEvent* e) {
    const RS_Vector snap = e->snapPoint;
    switch (status) {
        case SetEntity: {
            m_entity = catchAndDescribe(e, g_enTypeList, RS2::ResolveAll);
            if (m_entity != nullptr) {
                highlightHover(m_entity);
            }
            break;
        }
        case SetPos: {
            highlightSelected(m_entity);
            *m_pos = getRelZeroAwarePoint(e, snap);
            const auto lineToCreate = RS_Creation::createLineRelAngle(*m_pos, m_entity, m_relativeAngleRad, m_length);
            if (lineToCreate != nullptr) {
                previewEntityToCreate(lineToCreate, true);
                if (m_showRefEntitiesOnPreview) {
                    const auto vp = m_entity->getNearestPointOnEntity(*m_pos, false);
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

void LC_ActionDrawLineRelAngle::setEntity(RS_Entity* en) {
    m_entity = en;
    setStatus(SetPos);
}

void LC_ActionDrawLineRelAngle::onMouseLeftButtonRelease(const int status, const LC_MouseEvent* e) {
    switch (status) {
        case SetEntity: {
            RS_Entity* en = catchEntityByEvent(e, g_enTypeList, RS2::ResolveAll);
            if (en != nullptr) {
                setEntity(en);
            }
            break;
        }
        case SetPos: {
            const RS_Vector& snap = e->snapPoint;
            const RS_Vector position = getRelZeroAwarePoint(e, snap);
            fireCoordinateEvent(position);
            break;
        }
        default:
            break;
    }
}

void LC_ActionDrawLineRelAngle::onMouseRightButtonRelease(const int status, [[maybe_unused]] const LC_MouseEvent* e) {
    deletePreview();
    initPrevious(status);
}

void LC_ActionDrawLineRelAngle::onCoordinateEvent(const int status, [[maybe_unused]] bool isZero, const RS_Vector& coord) {
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

bool LC_ActionDrawLineRelAngle::doProcessCommand(const int status, const QString& command) {
    bool accept = false;

    switch (status) {
        case SetEntity:
        case SetPos: {
            if (!m_fixedAngle && checkCommand("angle", command)) {
                deletePreview();
                setStatus(SetAngle);
                accept = true;
            }
            else if (checkCommand("length", command)) {
                deletePreview();
                setStatus(SetLength);
                accept = true;
            }
            break;
        }
        case SetAngle: {
            double angle;
            const bool ok = parseToRelativeAngle(command, angle);
            if (ok) {
                accept = true;
                m_relativeAngleRad = angle;
            }
            else {
                commandMessage(tr("Not a valid expression"));
            }
            updateOptions();
            setStatus(SetPos);
            break;
        }
        case SetLength: {
            bool ok = false;
            const double l = RS_Math::eval(command, &ok);
            if (ok) {
                m_length = l;
                accept = true;
            }
            else {
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

QStringList LC_ActionDrawLineRelAngle::getAvailableCommands() {
    QStringList cmd;
    switch (getStatus()) {
        case SetPos:
        case SetLength: {
            if (!m_fixedAngle) {
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

void LC_ActionDrawLineRelAngle::updateActionPrompt() {
    switch (getStatus()) {
        case SetEntity:
            updatePromptTRCancel(tr("Select base entity"));
            break;
        case SetPos:
            updatePromptTRBack(tr("Specify position"), MOD_SHIFT_RELATIVE_ZERO);
            break;
        case SetAngle:
            updatePromptTRBack(tr("Specify angle"));
            break;
        case SetLength:
            updatePromptTRBack(tr("Specify length"));
            break;
        default:
            updatePrompt();
            break;
    }
}

RS2::CursorType LC_ActionDrawLineRelAngle::doGetMouseCursor([[maybe_unused]] const int status) {
    switch (status) {
        case SetEntity:
            return RS2::SelectCursor;
        case SetPos:
            return RS2::CadCursor;
        default:
            return RS2::NoCursorChange;
    }
}

LC_ActionOptionsWidget* LC_ActionDrawLineRelAngle::createOptionsWidget() {
    return new LC_LineRelAngleOptionsWidget();
}

LC_ActionOptionsPropertiesFiller* LC_ActionDrawLineRelAngle::createOptionsFiller() {
    return new LC_LineRelAngleOptionsFiller();
}
