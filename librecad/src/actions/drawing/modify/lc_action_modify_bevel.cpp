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

#include "lc_action_modify_bevel.h"

#include "lc_actioninfomessagebuilder.h"
#include "lc_bevel_options_filler.h"
#include "lc_bevel_options_widget.h"
#include "rs_atomicentity.h"
#include "rs_document.h"
#include "rs_graphic.h"
#include "rs_information.h"
#include "rs_line.h"
#include "rs_modification.h"
#include "rs_polyline.h"

struct LC_ActionModifyBevel::BevelActionData {
    RS_Vector coord1;
    RS_Vector coord2;
    RS_BevelData data;
    LC_BevelResult bevelResult;
    LC_DocumentModificationBatch triggerContext;
};

LC_ActionModifyBevel::LC_ActionModifyBevel(LC_ActionContext *actionContext)
    :LC_UndoableDocumentModificationAction("ActionModifyBevel",actionContext, RS2::ActionModifyBevel)
    , m_actionData(std::make_unique<BevelActionData>()) {
}

LC_ActionModifyBevel::~LC_ActionModifyBevel() = default;

void LC_ActionModifyBevel::doSaveOptions() {
    save("Length1", getLength1());
    save("Length2", getLength2());
    save("Trim", isTrimOn());
}

void LC_ActionModifyBevel::doLoadOptions() {
    const double len1 = loadDouble("Length1", 10.0);
    setLength1(len1);
    const double len2 = loadDouble("Length2", 10.0);
    setLength2(len2);

    const bool trimOn = loadBool("Trim", true);
    setTrim(trimOn);
}

void LC_ActionModifyBevel::finish(){
    RS_PreviewActionInterface::finish();
}

void LC_ActionModifyBevel::init(const int status) {
    RS_PreviewActionInterface::init(status);
    m_snapMode.restriction = RS2::RestrictNothing;
}

void LC_ActionModifyBevel::doInitWithContextEntity(RS_Entity* contextEntity, const RS_Vector& clickPos) {
    if (isLine(contextEntity)) { // can apply bevel to lines only...
        m_entity1 = static_cast<RS_AtomicEntity *>(contextEntity);
        m_actionData->coord1 = m_entity1->getNearestPointOnEntity(clickPos, true);
        setStatus(SetEntity2);
    }
}

bool LC_ActionModifyBevel::doTriggerModifications(LC_DocumentModificationBatch& ctx) {
    if (isAtomic(m_entity1) && isAtomic(m_entity2)) {
        const LC_BevelResult bevelResult = m_actionData->bevelResult;
        ctx.setActiveLayerAndPen(false, false);
        if (!bevelResult.isPolyline) {
            bevelResult.bevel->setLayer(m_graphic->getActiveLayer());
            bevelResult.bevel->setPen(m_document->getActivePen());
        }
        auto tmpContext = m_actionData->triggerContext;
        ctx += tmpContext.entitiesToAdd;
        ctx -= tmpContext.entitiesToDelete;
        return true;
    }
    return false;
}

void LC_ActionModifyBevel::doTriggerCompletion([[maybe_unused]]bool success) {
    m_actionData->coord1 = {};
    m_actionData->coord2 = {};
    m_entity1            = nullptr;
    m_entity2            = nullptr;
    m_actionData->triggerContext.clear();
    // fixme - decide - should we stay with selected line 1 or go to line selection status??
    setStatus(SetEntity1);
}

bool LC_ActionModifyBevel::doUpdateDistanceByInteractiveInput(const QString& tag, const double distance) {
    if (tag == "length1") {
        setLength1(distance);
        return true;
    }
    if (tag == "length2") {
        setLength2(distance);
        return true;
    }
    return false;
}

void LC_ActionModifyBevel::drawSnapper() {
    // disable snapper
}

void LC_ActionModifyBevel::onMouseMoveEvent(const int status, const LC_MouseEvent* e) {
    const RS_Vector mouse = e->graphPoint;
    // it seems that bevel works properly with lines only... it relies on trimEndpoint/moveEndpoint methods, which
    // have some support for arc and ellipse, yet still...
    RS_Entity *se = catchAndDescribe(e, RS2::EntityLine, RS2::ResolveAll);

    switch (status) {
        case SetEntity1: {
            if (isEntityAccepted(se)){
                highlightHover(se);
            }
            break;
        }
        case SetEntity2: {
            highlightSelected(m_entity1);
            if (areBothEntityAccepted(m_entity1, se)){
                const auto atomicCandidate2 = dynamic_cast<RS_AtomicEntity *>(se);

                LC_DocumentModificationBatch ctx;
                const LC_BevelResult bevelResult = RS_Modification::bevel(m_actionData->coord1,  m_entity1, mouse, atomicCandidate2, m_actionData->data, true, ctx);

                if (bevelResult.error == LC_BevelResult::OK) {
                    highlightHover(se);

                    // bevel
                    if (bevelResult.isPolyline) {
                        previewEntity(bevelResult.polyline);
                    }
                    else {
                        previewEntity(bevelResult.bevel);
                    }

                    if (m_showRefEntitiesOnPreview) {
                        // bevel points
                        previewRefPoint(bevelResult.bevel->getStartpoint());
                        previewRefPoint(bevelResult.bevel->getEndpoint());

                        // lines intersection
                        previewRefPoint(bevelResult.intersectionPoint);

                        // changes in lines
                        if (m_actionData->data.trim) {
                            previewLineModifications(m_entity1, bevelResult.trimmed1, bevelResult.trimStart1);
                            previewLineModifications(atomicCandidate2, bevelResult.trimmed2, bevelResult.trimStart2);
                        }

                        // selection points
                        previewRefSelectablePoint(m_actionData->coord1);
                        previewRefSelectablePoint(se->getNearestPointOnEntity(mouse));
                    }

                    if (isInfoCursorForModificationEnabled()) {
                        msg(tr("Trim"))
                                .vector(tr("Intersection:"),bevelResult.intersectionPoint)
                                .vector(tr("Point 1:"),bevelResult.bevel->getStartpoint())
                                .vector(tr("Point 2:"),bevelResult.bevel->getEndpoint())
                                .toInfoCursorZone2(false);
                    }
                    if (!bevelResult.isPolyline) {
                        delete bevelResult.trimmed1;
                        delete bevelResult.trimmed2;
                    }
                }
            }
            break;
        }
        default:
            break;
    }
}

void LC_ActionModifyBevel::previewLineModifications(const RS_Entity *original, const RS_Entity *trimmed, const bool trimOnStart) const {
    const bool originalIncreased = original->getLength() < trimmed->getLength();
    if (originalIncreased){
        if (trimOnStart){
            previewLine(original->getStartpoint(), trimmed->getStartpoint());
            previewRefPoint(original->getStartpoint());
        }
        else{
            previewLine(original->getEndpoint(), trimmed->getEndpoint());
            previewRefPoint(original->getEndpoint());
        }
    }
    else{
        if (trimOnStart){
            previewRefLine(original->getStartpoint(), trimmed->getStartpoint());
        }
        else{
            previewRefLine(original->getEndpoint(), trimmed->getEndpoint());
        }
    }
}

void LC_ActionModifyBevel::onMouseLeftButtonRelease(const int status, const LC_MouseEvent* e) {
    RS_Entity *se = catchEntityByEvent(e,RS2::EntityLine, RS2::ResolveAll);
    switch (status) {
            case SetEntity1: {
                if (isAtomic(se)){
                    if (RS_Information::isTrimmable(se)){
                        m_entity1 = static_cast<RS_AtomicEntity *>(se);
                        m_actionData->coord1 = m_entity1->getNearestPointOnEntity(e->graphPoint, true);
                        setStatus(SetEntity2);
                    } else {
                        commandMessage(tr("Invalid entity selected (non-trimmable)."));
                    }
                } else {
                    commandMessage(tr("Invalid entity selected (non-atomic)."));
                }
                break;
            }
            case SetEntity2: {
                if (isAtomic(se)){
                    m_entity2 = static_cast<RS_AtomicEntity *>(se);
                    m_actionData->coord2 = e->graphPoint;
                    const LC_BevelResult bevelResult = RS_Modification::bevel(m_actionData->coord1, m_entity1, m_actionData->coord2, m_entity2,
                                            m_actionData->data, false, m_actionData->triggerContext);
                    switch (bevelResult.error) {
                        case LC_BevelResult::OK: {
                            m_actionData->bevelResult = bevelResult;
                            trigger();
                            break;
                        }
                        case LC_BevelResult::ERR_NO_INTERSECTION: {
                            commandMessage(tr("Selected lines are parallel"));
                            break;
                        }
                        case LC_BevelResult::ERR_NOT_THE_SAME_POLYLINE: {
                            commandMessage(tr("Selected lines are not children of the same polyline"));
                            break;
                        }
                        case LC_BevelResult::ERR_NOT_LINES: {
                            commandMessage(tr("Both selected entities should be lines"));
                            break;
                        }
                        case LC_BevelResult::ERR_VISIBILITY: {
                            commandMessage(tr("Invalid entity selected (non-trimmable with first entity)."));
                            break;
                        }
                        default:
                            break;
                    }
                } else {
                    commandMessage(tr("Invalid entity selected (non-atomic)."));
                }
                break;
            }
            default:
                break;
        }

}

void LC_ActionModifyBevel::onMouseRightButtonRelease(const int status, [[maybe_unused]] const LC_MouseEvent* e) {
    deletePreview();
    int newStatus = -1;
    switch (status){
        case SetEntity1:
            break;
        case SetEntity2:
            newStatus = SetEntity1;
            break;
        case SetLength1:
        case SetLength2:
            newStatus = m_lastStatus;
            break;
        default:
            break;
    }
    setStatus(newStatus);
}

bool LC_ActionModifyBevel::isEntityAccepted(const RS_Entity *en) const{
    return isAtomic(en) && RS_Information::isTrimmable(en);
}

bool LC_ActionModifyBevel::areBothEntityAccepted(const RS_Entity *en1, const RS_Entity *en2) const{
    return isAtomic(en2) && en2 != en1 /* && RS_Information::isTrimmable(en1,en2)*/;
}

bool LC_ActionModifyBevel::doProcessCommand(const int status, const QString &command) {
    bool accept = false;
    switch (status) {
        case SetEntity1:
        case SetEntity2: {
            if (checkCommand("length1", command)){
                deletePreview();
                m_lastStatus = static_cast<Status>(getStatus());
                setStatus(SetLength1);
                accept = true;
            } else if (checkCommand("length2", command)){
                deletePreview();
                m_lastStatus = static_cast<Status>(getStatus());
                setStatus(SetLength2);
                accept = true;
            } else if (checkCommand("trim", command)){
                m_actionData->data.trim = !m_actionData->data.trim;
                updateOptions();
                accept = true;
            }
            break;
        }
        case SetLength1: {
            bool ok = false;
            const double l = RS_Math::eval(command, &ok);
            if (ok){
                accept = true;
                m_actionData->data.length1 = l;
            } else {
                commandMessage(tr("Not a valid expression"));
            }
            updateOptions();
            setStatus(m_lastStatus);
            break;
        }
        case SetLength2: {
            bool ok = false;
            const double l = RS_Math::eval(command, &ok);
            if (ok){
                m_actionData->data.length2 = l;
                accept = true;
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

void LC_ActionModifyBevel::setLength1(const double l1) const {
    m_actionData->data.length1 = l1;
}

double LC_ActionModifyBevel::getLength1() const{
    return m_actionData->data.length1;
}

void LC_ActionModifyBevel::setLength2(const double l2) const {
    m_actionData->data.length2 = l2;
}

double LC_ActionModifyBevel::getLength2() const{
    return m_actionData->data.length2;
}

void LC_ActionModifyBevel::setTrim(const bool t) const {
    m_actionData->data.trim = t;
}

bool LC_ActionModifyBevel::isTrimOn() const{
    return m_actionData->data.trim;
}

QStringList LC_ActionModifyBevel::getAvailableCommands(){
    QStringList cmd;
    switch (getStatus()) {
        case SetEntity1:
        case SetEntity2:
            cmd += command("length1");
            cmd += command("length2");
            cmd += command("trim");
            break;
        default:
            break;
    }
    return cmd;
}

void LC_ActionModifyBevel::updateActionPrompt() {
    switch (getStatus()) {
        case SetEntity1:
            updatePromptTRCancel(tr("Select first entity"));
            break;
        case SetEntity2:
            updatePromptTRBack(tr("Select second entity"));
            break;
        case SetLength1:
            updatePromptTRBack(tr("Enter length 1:"));
            break;
        case SetLength2:
            updatePromptTRBack(tr("Enter length 2:"));
            break;
        default:
            updatePrompt();
            break;
    }
}

RS2::CursorType LC_ActionModifyBevel::doGetMouseCursor([[maybe_unused]] int status){
    return RS2::SelectCursor;
}

LC_ActionOptionsWidget* LC_ActionModifyBevel::createOptionsWidget(){
    return new LC_BevelOptionsWidget();
}

LC_ActionOptionsPropertiesFiller* LC_ActionModifyBevel::createOptionsFiller() {
    return new LC_BevelOptionsFiller();
}
