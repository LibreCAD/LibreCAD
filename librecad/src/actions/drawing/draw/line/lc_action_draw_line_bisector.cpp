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

#include "lc_action_draw_line_bisector.h"

#include "lc_line_bisector_options_filler.h"
#include "lc_line_bisector_options_widget.h"
#include "rs_creation.h"
#include "rs_document.h"
#include "rs_line.h"
#include "rs_polyline.h"
#include "rs_preview.h"

namespace {
    //list of entity types supported by current action - only lines so far
    const auto g_enTypeList = EntityTypeList{RS2::EntityLine};
}

struct LC_ActionDrawLineBisector::ActionData {
    /** Mouse pos when choosing the 1st line */
    RS_Vector coord1;
    /** Mouse pos when choosing the 2nd line */
    RS_Vector coord2;
};

LC_ActionDrawLineBisector::LC_ActionDrawLineBisector(LC_ActionContext* actionContext)
    : LC_UndoableDocumentModificationAction("Draw Bisectors", actionContext, RS2::ActionDrawLineBisector), m_length(10.),
      m_numberToCreate(1), m_actionData(std::make_unique<ActionData>()) {
}

LC_ActionDrawLineBisector::~LC_ActionDrawLineBisector() = default;

void LC_ActionDrawLineBisector::doSaveOptions() {
    save("Length", m_length);
    save("Number", m_numberToCreate);
}

void LC_ActionDrawLineBisector::doLoadOptions() {
    m_length = loadDouble("Length", 1.0);
    m_numberToCreate = loadInt("Number", 1);
}

void LC_ActionDrawLineBisector::setLength(const double l) {
    m_length = l;
}

double LC_ActionDrawLineBisector::getLength() const {
    return m_length;
}

void LC_ActionDrawLineBisector::setNumber(const int n) {
    m_numberToCreate = n;
}

int LC_ActionDrawLineBisector::getNumber() const {
    return m_numberToCreate;
}

bool LC_ActionDrawLineBisector::doUpdateDistanceByInteractiveInput(const QString& tag, const double distance) {
    if (tag == "length") {
        setLength(distance);
        return true;
    }
    return false;
}

void LC_ActionDrawLineBisector::init(const int status) {
    if (status >= 0) {
        invalidateSnapSpot();
    }
    RS_PreviewActionInterface::init(status);
}

void LC_ActionDrawLineBisector::doInitWithContextEntity(RS_Entity* contextEntity, const RS_Vector& clickPos) {
    auto entity = contextEntity;
    if (isPolyline(contextEntity)) {
        const auto polyline = static_cast<RS_Polyline*>(contextEntity);
        entity = polyline->getNearestEntity(clickPos);
    }
    setFirstLine(entity);
}

void LC_ActionDrawLineBisector::setStatus(const int status) {
    RS_ActionInterface::setStatus(status);
    invalidateSnapSpot();
}

bool LC_ActionDrawLineBisector::doTriggerModifications(LC_DocumentModificationBatch& ctx) {
    const bool created = RS_Creation::createBisector(m_actionData->coord1, m_actionData->coord2, m_length, m_numberToCreate, m_line1,
                                                     m_line2, ctx.entitiesToAdd);
    return created;
}

void LC_ActionDrawLineBisector::onMouseMoveEvent(const int status, const LC_MouseEvent* e) {
    const RS_Vector mouse = e->graphPoint;
    deleteSnapper();
    switch (status) {
        case SetLine1: {
            const RS_Entity* en = catchAndDescribe(e, g_enTypeList, RS2::ResolveAll);
            if (en != nullptr) {
                highlightHover(en);
            }
            break;
        }
        case SetLine2: {
            highlightSelected(m_line1);
            m_actionData->coord2 = mouse;
            RS_Entity* en = catchAndDescribe(e, g_enTypeList, RS2::ResolveAll);
            if (en == m_line1) {
                m_line2 = nullptr;
            }
            else if (en != nullptr) {
                m_line2 = dynamic_cast<RS_Line*>(en);

                QList<RS_Entity*> lines;
                const bool created = RS_Creation::createBisector(m_actionData->coord1, m_actionData->coord2, m_length, m_numberToCreate,
                                                                 m_line1, m_line2, lines);
                if (created) {
                    highlightHover(m_line2);
                    m_preview->addAllFromList(lines);
                    const RS_Entity* ent = lines.front();
                    if (m_numberToCreate == 1) {
                        prepareEntityDescription(ent, RS2::EntityDescriptionLevel::DescriptionCreating);
                    }
                    else {
                        appendInfoCursorZoneMessage(QString::number(m_numberToCreate) + tr(" entities will be created"), 2, false);
                    }
                    if (m_showRefEntitiesOnPreview) {
                        previewRefPoint(m_line1->getNearestPointOnEntity(m_actionData->coord1));
                        previewRefPoint(ent->getStartpoint());
                        const RS_Vector nearest = m_line2->getNearestPointOnEntity(mouse, false);
                        previewRefSelectablePoint(nearest);
                    }
                }
            }
            break;
        }
        case SetLength:
        case SetNumber: {
            if (m_line1 != nullptr) {
                highlightSelected(m_line1);
            }
            break;
        }
        default:
            break;
    }
}

void LC_ActionDrawLineBisector::setFirstLine(RS_Entity* en) {
    if (isLine(en)) {
        // fixme - support of polyline
        m_line1 = dynamic_cast<RS_Line*>(en);
        m_line2 = nullptr;
        setStatus(SetLine2);
    }
}

void LC_ActionDrawLineBisector::onMouseLeftButtonRelease(const int status, const LC_MouseEvent* e) {
    const RS_Vector mouse = e->graphPoint;
    switch (status) {
        case SetLine1: {
            m_actionData->coord1 = mouse;
            RS_Entity* en = catchEntity(mouse, g_enTypeList, RS2::ResolveAll); // fixme - support of polyline
            setFirstLine(en);
            break;
        }
        case SetLine2:
            m_actionData->coord2 = mouse;
            trigger();
            setStatus(SetLine1);
            break;
        default:
            break;
    }
}

void LC_ActionDrawLineBisector::onMouseRightButtonRelease(const int status, [[maybe_unused]] const LC_MouseEvent* e) {
    deletePreview();
    initPrevious(status);
}

bool LC_ActionDrawLineBisector::doProcessCommand(int status, const QString& command) {
    bool accept = false;
    switch (status) {
        case SetLine1:
        case SetLine2: {
            m_lastStatus = static_cast<Status>(status);
            if (checkCommand("length", command)) {
                deletePreview();
                setStatus(SetLength);
                accept = true;
            }
            else if (checkCommand("number", command)) {
                deletePreview();
                setStatus(SetNumber);
                accept = true;
            }
            break;
        }
        case SetLength: {
            bool ok = false;
            const double l = RS_Math::eval(command, &ok);
            if (ok) {
                accept = true;
                m_length = l;
            }
            else {
                commandMessage(tr("Not a valid expression"));
            }
            updateOptions();
            setStatus(m_lastStatus);
            break;
        }
        case SetNumber: {
            bool ok = false;
            const int n = std::lround(RS_Math::eval(command, &ok));
            if (ok) {
                accept = true;
                if (n > 0 && n <= 200) {
                    m_numberToCreate = n;
                }
                else {
                    commandMessage(
                        tr("Number sector lines not in range: ", "number of bisector to create must be in [1, 200]") + QString::number(n));
                }
            }
            else {
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

QStringList LC_ActionDrawLineBisector::getAvailableCommands() {
    QStringList cmd;

    switch (getStatus()) {
        case SetLine1:
        case SetLine2:
            cmd += command("length");
            cmd += command("number");
            break;
        default:
            break;
    }
    return cmd;
}

void LC_ActionDrawLineBisector::updateActionPrompt() {
    switch (getStatus()) {
        case SetLine1:
            updatePromptTRCancel(tr("Select first line"));
            break;
        case SetLine2:
            updatePromptTRBack(tr("Select second line"));
            break;
        case SetLength:
            updatePromptTRBack(tr("Enter bisector length:"));
            break;
        case SetNumber:
            updatePromptTRBack(tr("Enter number of bisectors:"));
            break;
        default:
            updatePrompt();
            break;
    }
}

LC_ActionOptionsWidget* LC_ActionDrawLineBisector::createOptionsWidget() {
    return new LC_LineBisectorOptionsWidget();
}

LC_ActionOptionsPropertiesFiller* LC_ActionDrawLineBisector::createOptionsFiller() {
    return new LC_LineBisectorOptionsFiller();
}

RS2::CursorType LC_ActionDrawLineBisector::doGetMouseCursor([[maybe_unused]] int status) {
    return RS2::SelectCursor;
}
