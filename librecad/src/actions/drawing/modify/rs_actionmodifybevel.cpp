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


#include <QMouseEvent>

#include "rs_actionmodifybevel.h"
#include "rs_commandevent.h"
#include "rs_debug.h"
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_information.h"
#include "rs_math.h"
#include "rs_modification.h"
#include "qg_beveloptions.h"

struct RS_ActionModifyBevel::Points {
    RS_Vector coord1;
    RS_Vector coord2;
    RS_BevelData data;
};

RS_ActionModifyBevel::RS_ActionModifyBevel(RS_EntityContainer& container,
                                           RS_GraphicView& graphicView)
    :RS_PreviewActionInterface("Bevel Entities",
                               container, graphicView)
    , pPoints(std::make_unique<Points>())
    ,lastStatus(SetEntity1){
    setActionType(RS2::ActionModifyBevel);
}

RS_ActionModifyBevel::~RS_ActionModifyBevel() = default;


void RS_ActionModifyBevel::finish(bool updateTB){
    RS_PreviewActionInterface::finish(updateTB);
}

void RS_ActionModifyBevel::init(int status) {
    RS_PreviewActionInterface::init(status);

    //snapMode = RS2::SnapFree;
    snapMode.restriction = RS2::RestrictNothing;
}

void RS_ActionModifyBevel::trigger(){

    RS_DEBUG->print("RS_ActionModifyBevel::trigger()");

    if (entity1 && entity1->isAtomic() &&
        entity2 && entity2->isAtomic()){

        RS_Modification m(*container, graphicView);
        LC_BevelResult* bevelResult = m.bevel(pPoints->coord1, entity1, pPoints->coord2, entity2, pPoints->data, false);
        if (bevelResult != nullptr){
            switch (bevelResult->error) {
                case LC_BevelResult::OK:
                    break;
                case LC_BevelResult::ERR_NO_INTERSECTION:
                    commandMessage(tr("Selected lines are parallel"));
                    break;
                case LC_BevelResult::ERR_NOT_THE_SAME_POLYLINE:
                    commandMessage(tr("Selected lines are not children of the same polyline"));
                    break;
            }
        }

        delete bevelResult;

        // fixme - decide stay with selected line 1 or go to line selection status??

        pPoints->coord1 = {};
        pPoints->coord2 = {};
        entity1 = nullptr;

        setStatus(SetEntity1);

        updateSelectionWidget();
        deletePreview();
        deleteHighlights();
        graphicView->redraw();
    }
}

void RS_ActionModifyBevel::mouseMoveEvent(QMouseEvent *e){
    RS_DEBUG->print("RS_ActionModifyBevel::mouseMoveEvent begin");
    snapPoint(e);
    RS_Vector mouse = toGraph(e);
    // it seems that bevel works properly with lines only... it relies on trimEndpoint/moveEndpoint methods, which
    // have some support for arc and ellipse, yet still...
    RS_Entity *se = catchEntity(e, RS2::EntityLine, RS2::ResolveAllButTextImage);

    deleteHighlights();

    switch (getStatus()) {
        case SetEntity1: {
            if (isEntityAccepted(se)){
                highlightHover(se);
            }
            break;
        }
        case SetEntity2: {
            deletePreview();
            highlightSelected(entity1);
            if (se != entity1 && areBothEntityAccepted(entity1, se)){

                auto atomicCandidate2 = dynamic_cast<RS_AtomicEntity *>(se);

                RS_Modification m(*container, nullptr);
                LC_BevelResult* bevelResult = m.bevel(pPoints->coord1,  entity1, mouse, atomicCandidate2, pPoints->data, true);

                if (bevelResult != nullptr){
                    if (bevelResult->error == LC_BevelResult::OK){
                        highlightHover(se);

                        // bevel
                        previewEntity(bevelResult->bevel);

                        if (showRefEntitiesOnPreview) {
                            // bevel points
                            previewRefPoint(bevelResult->bevel->getStartpoint());
                            previewRefPoint(bevelResult->bevel->getEndpoint());

                            // lines intersection
                            previewRefPoint(bevelResult->intersectionPoint);

                            // changes in lines
                            if (pPoints->data.trim) {
                                previewLineModifications(entity1, bevelResult->trimmed1, bevelResult->trimStart1);
                                previewLineModifications(atomicCandidate2, bevelResult->trimmed2,
                                                         bevelResult->trimStart2);
                            }

                            // selection points
                            previewRefSelectablePoint(pPoints->coord1);
                            previewRefSelectablePoint(se->getNearestPointOnEntity(mouse));
                        }
                    }
                    delete bevelResult;
                }
            }
            drawPreview();
            break;
        }
        default:
            break;
    }

    drawHighlights();

    RS_DEBUG->print("RS_ActionModifyBevel::mouseMoveEvent end");
}

void RS_ActionModifyBevel::previewLineModifications(const RS_Entity *original, const RS_Entity *trimmed, bool trimOnStart){
    bool originalIncreased = original->getLength() < trimmed->getLength();
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

void RS_ActionModifyBevel::onMouseLeftButtonRelease(int status, QMouseEvent *e) {
    RS_Entity *se = catchEntity(e,RS2::EntityLine, RS2::ResolveAllButTextImage);
    if (se != nullptr){
        switch (status) {
            case SetEntity1: {
                if (se->isAtomic()){
                    if (RS_Information::isTrimmable(se)){
                        entity1 = dynamic_cast<RS_AtomicEntity *>(se);
                        pPoints->coord1 = entity1->getNearestPointOnEntity(toGraph(e), true);
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
                if (se->isAtomic()){
                    if (RS_Information::isTrimmable(entity1, se)){
                        entity2 = dynamic_cast<RS_AtomicEntity *>(se);
                        pPoints->coord2 = toGraph(e);
                        trigger();
                    }
                    else{
                        commandMessage(tr("Invalid entity selected (non-trimmable with first entity)."));
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
}

void RS_ActionModifyBevel::onMouseRightButtonRelease(int status, [[maybe_unused]] QMouseEvent *e) {
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
            newStatus = lastStatus;
            break;
        default:
            break;
    }
    setStatus(newStatus);
}

bool RS_ActionModifyBevel::isEntityAccepted(RS_Entity *en) const{
    return en != nullptr && en->isAtomic() && RS_Information::isTrimmable(en);
}

bool RS_ActionModifyBevel::areBothEntityAccepted(RS_Entity *en1, RS_Entity *en2) const{
    return en2 != nullptr && en2 != en1 && en2->isAtomic() && RS_Information::isTrimmable(en1,en2);
}

bool RS_ActionModifyBevel::doProcessCommand(int status, const QString &c) {
    bool accept = false;
    switch (status) {
        case SetEntity1:
        case SetEntity2: {
            if (checkCommand("length1", c)){
                deletePreview();
                lastStatus = (Status) getStatus();
                setStatus(SetLength1);
                accept = true;
            } else if (checkCommand("length2", c)){
                deletePreview();
                lastStatus = (Status) getStatus();
                setStatus(SetLength2);
                accept = true;
            } else if (checkCommand("trim", c)){
                pPoints->data.trim = !pPoints->data.trim;
                updateOptions();
                accept = true;
            }
            break;
        }
        case SetLength1: {
            bool ok;
            double l = RS_Math::eval(c, &ok);
            if (ok){
                accept = true;
                pPoints->data.length1 = l;
            } else {
                commandMessage(tr("Not a valid expression"));
            }
            updateOptions();
            setStatus(lastStatus);
            break;
        }
        case SetLength2: {
            bool ok;
            double l = RS_Math::eval(c, &ok);
            if (ok){
                pPoints->data.length2 = l;
                accept = true;
            } else {
                commandMessage(tr("Not a valid expression"));
            }
            updateOptions();
            setStatus(lastStatus);
            break;
        }
        default:
            break;
    }
    return accept;
}

void RS_ActionModifyBevel::setLength1(double l1){
    pPoints->data.length1 = l1;
}

double RS_ActionModifyBevel::getLength1() const{
    return pPoints->data.length1;
}

void RS_ActionModifyBevel::setLength2(double l2){
    pPoints->data.length2 = l2;
}

double RS_ActionModifyBevel::getLength2() const{
    return pPoints->data.length2;
}

void RS_ActionModifyBevel::setTrim(bool t){
    pPoints->data.trim = t;
}

bool RS_ActionModifyBevel::isTrimOn() const{
    return pPoints->data.trim;
}

QStringList RS_ActionModifyBevel::getAvailableCommands(){
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

void RS_ActionModifyBevel::updateMouseButtonHints() {
    switch (getStatus()) {
        case SetEntity1:
            updateMouseWidgetTRCancel(tr("Select first entity"));
            break;
        case SetEntity2:
            updateMouseWidgetTRBack(tr("Select second entity"));
            break;
        case SetLength1:
            updateMouseWidgetTRBack(tr("Enter length 1:"));
            break;
        case SetLength2:
            updateMouseWidgetTRBack(tr("Enter length 2:"));
            break;
        default:
            updateMouseWidget();
            break;
    }
}

RS2::CursorType RS_ActionModifyBevel::doGetMouseCursor([[maybe_unused]] int status){
    return RS2::SelectCursor;
}

LC_ActionOptionsWidget* RS_ActionModifyBevel::createOptionsWidget(){
    return new QG_BevelOptions();
}
