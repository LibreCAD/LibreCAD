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

#include "lc_action_draw_line_parallel.h"

#include "lc_actioncontext.h"
#include "lc_line_parallel_options_filler.h"
#include "lc_line_parallel_options_widget.h"
#include "rs_creation.h"
#include "rs_document.h"
#include "rs_preview.h"

namespace {
    // fixme - sand - remove, this is not used!
    //this holds a list of entity types which supports tangent
    const auto g_supportedEntityTypes = EntityTypeList{
        {RS2::EntityArc, RS2::EntityCircle, RS2::EntityLine, RS2::EntityParabola/*, RS2::EntitySplinePoints*/}
    };
}

LC_ActionDrawLineParallel::LC_ActionDrawLineParallel(LC_ActionContext* actionContext, const RS2::ActionType actionType)
    : LC_UndoableDocumentModificationAction("ActionDrawParallels", actionContext, actionType), m_distance(1.0),
      m_numberToCreate(1), m_coord(new RS_Vector{}) {
    switch (actionType) {
        case RS2::ActionDrawLineParallel: {
            m_optionsSettingsGroupName = "ActionDrawLineParallel";
            break;
        }
        case  RS2::ActionDrawCircleParallel: {
            m_optionsSettingsGroupName = "ActionDrawCircleParallel";
            break;
        }
        case RS2::ActionDrawArcParallel: {
            m_optionsSettingsGroupName = "ActionDrawArcParallel";
            break;
        }
        default:
            Q_ASSERT_X(false,"LC_ActionDrawLineParallel", "constructor");
    }
}

LC_ActionDrawLineParallel::~LC_ActionDrawLineParallel() = default;

void LC_ActionDrawLineParallel::doSaveOptions() {
    save("Distance", m_distance);
    save("Number", m_numberToCreate);
}

void LC_ActionDrawLineParallel::doLoadOptions() {
    m_distance = loadDouble("Distance", 1.0);
    m_numberToCreate = loadInt("Number", 1);
}

void LC_ActionDrawLineParallel::doInitWithContextEntity(RS_Entity* contextEntity, [[maybe_unused]] const RS_Vector& clickPos) {
    const auto entity = contextEntity;
    if (isPolyline(entity)) {
        // fixme - sand - investigate whether we can create parallel to polyline segment (arc or line)
    }
}

double LC_ActionDrawLineParallel::getDistance() const {
    return m_distance;
}

void LC_ActionDrawLineParallel::setDistance(const double d) {
    m_distance = d;
}

int LC_ActionDrawLineParallel::getNumber() const {
    return m_numberToCreate;
}

void LC_ActionDrawLineParallel::setNumber(const int n) {
    m_numberToCreate = n;
}

bool LC_ActionDrawLineParallel::doUpdateDistanceByInteractiveInput(const QString& tag, const double distance) {
    if (tag == "distance") {
        setDistance(distance);
        return true;
    }
    return false;
}

bool LC_ActionDrawLineParallel::doTriggerModifications(LC_DocumentModificationBatch& ctx) {
    RS_Creation::createParallel(*m_coord, m_distance, m_numberToCreate, m_entity, false, ctx.entitiesToAdd);
    return true;
}

void LC_ActionDrawLineParallel::onMouseMoveEvent([[maybe_unused]] const int status, const LC_MouseEvent* e) {
    *m_coord = {e->graphPoint}; // copy is needed there!

    m_entity = catchAndDescribe(e, RS2::ResolveAll);

    switch (status) {
        case SetEntity: {
            if (m_entity != nullptr) {
                QList<RS_Entity*> parallels;
                RS_Creation::createParallel(*m_coord, m_distance, m_numberToCreate, m_entity, false, parallels);
                if (!parallels.empty()) {
                    m_preview->addAllFromList(parallels);
                    highlightHover(m_entity);
                    if (m_numberToCreate == 1) {
                        prepareEntityDescription(parallels.front(), RS2::EntityDescriptionLevel::DescriptionCreating);
                    }
                    else {
                        appendInfoCursorZoneMessage(QString::number(m_numberToCreate) + tr(" entities will be created"), 2, false);
                    }
                    if (m_showRefEntitiesOnPreview) {
                        const RS_Vector nearest = m_entity->getNearestPointOnEntity(*m_coord, false);
                        previewRefPoint(nearest);
                    }
                }
            }
            break;
        }
        default:
            break;
    }
}

void LC_ActionDrawLineParallel::onMouseLeftButtonRelease([[maybe_unused]] int status, [[maybe_unused]] const LC_MouseEvent* e) {
    trigger();
}

void LC_ActionDrawLineParallel::onMouseRightButtonRelease(const int status, [[maybe_unused]] const LC_MouseEvent* e) {
    initPrevious(status);
}

void LC_ActionDrawLineParallel::updateActionPrompt() {
    switch (getStatus()) {
        case SetEntity:
            updatePromptTRCancel(tr("Specify Distance <%1> or select entity or [%2]").arg(m_distance).arg(command("through")));
            break;
        case SetNumber:
            updatePrompt(tr("Enter number:"));
            break;
        default:
            updatePrompt();
            break;
    }
}

bool LC_ActionDrawLineParallel::doProcessCommand(const int status, const QString& command) {
    bool accept = false;
    switch (status) {
        case SetEntity: {
            // fixme = rework support of throught for simpler UX - add this to UI, probably combine two actions into same implementation
            if (checkCommand("through", command)) {
                finish();
                accept = true;
                switchToAction(RS2::ActionDrawLineParallelThrough);
            }
            else if (checkCommand("number", command)) {
                deletePreview();
                setStatus(SetNumber);
                accept = true;
            }
            else {
                bool ok = false;
                const double d = RS_Math::eval(command, &ok);
                accept = true;
                if (ok && d > RS_TOLERANCE) {
                    m_distance = d;
                }
                else {
                    commandMessage(tr("Not a valid expression"));
                }
                updateOptions();
                updateActionPrompt();
                //setStatus(SetEntity);
            }
            break;
        }
        case SetNumber: {
            bool ok = false;
            const int n = command.toInt(&ok);
            if (ok) {
                accept = true;
                if (n > 0 && n < 100) {
                    m_numberToCreate = n;
                }
                else {
                    commandMessage(tr("Not a valid number. Try 1..99"));
                }
            }
            else {
                commandMessage(tr("Not a valid expression"));
            }
            updateOptions();
            setStatus(SetEntity);
            break;
        }
        default:
            break;
    }
    return accept;
}

QStringList LC_ActionDrawLineParallel::getAvailableCommands() {
    QStringList cmd;
    switch (getStatus()) {
        case SetEntity:
            cmd += command("number");
            cmd += command("through");
            break;
        default:
            break;
    }
    return cmd;
}

RS2::CursorType LC_ActionDrawLineParallel::doGetMouseCursor([[maybe_unused]] int status) {
    return RS2::SelectCursor;
}

LC_ActionOptionsWidget* LC_ActionDrawLineParallel::createOptionsWidget() {
    return new LC_LineParallelOptionsWidget();
}

LC_ActionOptionsPropertiesFiller* LC_ActionDrawLineParallel::createOptionsFiller() {
    return new LC_LineParallelOptionsFiller();
}
