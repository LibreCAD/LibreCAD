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


#include "rs_actiondrawlinebisector.h"

#include "qg_linebisectoroptions.h"
#include "rs_creation.h"
#include "rs_line.h"
#include "rs_polyline.h"

namespace {

    //list of entity types supported by current action - only lines so far
    const auto g_enTypeList = EntityTypeList{RS2::EntityLine};
}


struct RS_ActionDrawLineBisector::ActionData {
	/** Mouse pos when choosing the 1st line */
	RS_Vector coord1;
	/** Mouse pos when choosing the 2nd line */
	RS_Vector coord2;
};

RS_ActionDrawLineBisector::RS_ActionDrawLineBisector(LC_ActionContext *actionContext)
    :RS_PreviewActionInterface("Draw Bisectors", actionContext, RS2::ActionDrawLineBisector),
    m_bisector(nullptr), m_line1(nullptr), m_line2(nullptr), m_length(10.), m_numberToCreate(1),
    m_actionData(std::make_unique<ActionData>()), m_lastStatus(SetLine1){
}

RS_ActionDrawLineBisector::~RS_ActionDrawLineBisector() = default;


void RS_ActionDrawLineBisector::setLength(double l){
    m_length = l;
}

double RS_ActionDrawLineBisector::getLength() const{
    return m_length;
}

void RS_ActionDrawLineBisector::setNumber(int n){
    m_numberToCreate = n;
}

int RS_ActionDrawLineBisector::getNumber() const{
    return m_numberToCreate;
}

bool RS_ActionDrawLineBisector::doUpdateDistanceByInteractiveInput(const QString& tag, double distance) {
    if (tag == "length") {
        setLength(distance);
        return true;
    }
    return false;
}

void RS_ActionDrawLineBisector::init(int status){
    if (status >= 0){
        invalidateSnapSpot();
    }
    RS_PreviewActionInterface::init(status);
}

void RS_ActionDrawLineBisector::doInitWithContextEntity(RS_Entity* contextEntity, const RS_Vector& clickPos) {
   auto entity = contextEntity;
   if (isPolyline(contextEntity)) {
       auto polyline = static_cast<RS_Polyline*> (contextEntity);
       entity = polyline->getNearestEntity(clickPos);
   }
   setFirstLine(entity);
}

void RS_ActionDrawLineBisector::setStatus(int status) {
    RS_ActionInterface::setStatus(status);
    invalidateSnapSpot();
}

void RS_ActionDrawLineBisector::doTrigger() {
    RS_Creation creation(m_container, m_viewport);
    creation.createBisector(m_actionData->coord1, m_actionData->coord2, m_length, m_numberToCreate, m_line1, m_line2);
}

void RS_ActionDrawLineBisector::onMouseMoveEvent(int status, LC_MouseEvent *e) {
    RS_Vector mouse = e->graphPoint;
    deleteSnapper();
    switch (status) {
        case SetLine1: {
            RS_Entity *en = catchAndDescribe(e, g_enTypeList, RS2::ResolveAll);
            if (en != nullptr){
                highlightHover(en);
            }
            break;
        }
        case SetLine2: {
            highlightSelected(m_line1);
            m_actionData->coord2 = mouse;
            RS_Entity *en = catchAndDescribe(e, g_enTypeList, RS2::ResolveAll);
            if (en == m_line1){
                m_line2 = nullptr;
            } else if (en != nullptr){
                m_line2 = dynamic_cast<RS_Line *>(en);

                RS_Creation creation(m_preview.get(), nullptr, false);
                auto ent = creation.createBisector(m_actionData->coord1, m_actionData->coord2, m_length, m_numberToCreate, m_line1, m_line2);
                if (ent != nullptr){
                    highlightHover(m_line2);
                    if (m_numberToCreate == 1){
                        prepareEntityDescription(ent, RS2::EntityDescriptionLevel::DescriptionCreating);
                    }
                    else{
                        appendInfoCursorZoneMessage(QString::number(m_numberToCreate) + tr(" entities will be created"), 2, false);
                    }
                    if (m_showRefEntitiesOnPreview) {
                        previewRefPoint(m_line1->getNearestPointOnEntity(m_actionData->coord1));
                        previewRefPoint(ent->getStartpoint());
                        RS_Vector nearest = m_line2->getNearestPointOnEntity(mouse, false);
                        previewRefSelectablePoint(nearest);
                    }
                }
            }
            break;
        }
        case SetLength:
        case SetNumber: {
            if (m_line1 != nullptr){
                highlightSelected(m_line1);
            }
            break;
        }
        default:
            break;
    }
}

void RS_ActionDrawLineBisector::setFirstLine(RS_Entity* en) {
    if (isLine(en)){ // fixme - support of polyline
        m_line1 = dynamic_cast<RS_Line *>(en);
        m_line2 = nullptr;
        setStatus(SetLine2);
    }
}

void RS_ActionDrawLineBisector::onMouseLeftButtonRelease(int status, LC_MouseEvent *e) {
    RS_Vector mouse = e->graphPoint;
    switch (status) {
        case SetLine1: {
            m_actionData->coord1 = mouse;
            RS_Entity *en = catchEntity(mouse,g_enTypeList,RS2::ResolveAll); // fixme - support of polyline
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

void RS_ActionDrawLineBisector::onMouseRightButtonRelease(int status, [[maybe_unused]]LC_MouseEvent *e) {
    deletePreview();
    initPrevious(status);
}

bool RS_ActionDrawLineBisector::doProcessCommand(int status, const QString &c) {
   bool accept = false;
    switch (status) {
        case SetLine1:
        case SetLine2: {
            m_lastStatus = (Status) status;
            if (checkCommand("length", c)){
                deletePreview();
                setStatus(SetLength);
                accept = true;
            } else if (checkCommand("number", c)){
                deletePreview();
                setStatus(SetNumber);
                accept = true;
            }
            break;
        }
        case SetLength: {
            bool ok = false;
            double l = RS_Math::eval(c, &ok);
            if (ok){
                accept = true;
                m_length = l;
            } else {
                commandMessage(tr("Not a valid expression"));
            }
            updateOptions();
            setStatus(m_lastStatus);
            break;
        }
        case SetNumber: {
            bool ok = false;
            int n = std::lround(RS_Math::eval(c, &ok));
            if (ok){
                accept= true;
                if (n > 0 && n <= 200) {
                    m_numberToCreate = n;
                }
                else {
                    commandMessage(
                       tr("Number sector lines not in range: ", "number of bisector to create must be in [1, 200]") + QString::number(n));
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

QStringList RS_ActionDrawLineBisector::getAvailableCommands(){
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

void RS_ActionDrawLineBisector::updateMouseButtonHints(){
    switch (getStatus()) {
        case SetLine1:
            updateMouseWidgetTRCancel(tr("Select first line"));
            break;
        case SetLine2:
            updateMouseWidgetTRBack(tr("Select second line"));
            break;
        case SetLength:
            updateMouseWidgetTRBack(tr("Enter bisector length:"));
            break;
        case SetNumber:
            updateMouseWidgetTRBack(tr("Enter number of bisectors:"));
            break;
        default:
            updateMouseWidget();
            break;
    }
}

LC_ActionOptionsWidget* RS_ActionDrawLineBisector::createOptionsWidget(){
    return new QG_LineBisectorOptions();
}

RS2::CursorType RS_ActionDrawLineBisector::doGetMouseCursor([[maybe_unused]] int status){
    return RS2::SelectCursor;
}
