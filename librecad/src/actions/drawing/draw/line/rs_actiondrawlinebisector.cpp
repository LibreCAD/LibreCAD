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

#include "rs_actiondrawlinebisector.h"
#include "rs_commandevent.h"
#include "rs_creation.h"
#include "rs_debug.h"
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_line.h"
#include "rs_math.h"
#include "rs_preview.h"
#include "qg_linebisectoroptions.h"
#include "rs_actioninterface.h"

namespace {

    //list of entity types supported by current action - only lines so far
    const auto enTypeList = EntityTypeList{RS2::EntityLine};
}


struct RS_ActionDrawLineBisector::Points {
	/** Mouse pos when choosing the 1st line */
	RS_Vector coord1;
	/** Mouse pos when choosing the 2nd line */
	RS_Vector coord2;
};

RS_ActionDrawLineBisector::RS_ActionDrawLineBisector(
    RS_EntityContainer &container,
    RS_GraphicView &graphicView)
    :RS_PreviewActionInterface("Draw Bisectors", container, graphicView), bisector(nullptr), line1(nullptr), line2(nullptr), length(10.), number(1),
     pPoints(std::make_unique<Points>()), lastStatus(SetLine1){
    actionType = RS2::ActionDrawLineBisector;
}

RS_ActionDrawLineBisector::~RS_ActionDrawLineBisector() = default;


void RS_ActionDrawLineBisector::setLength(double l){
    length = l;
}

double RS_ActionDrawLineBisector::getLength() const{
    return length;
}

void RS_ActionDrawLineBisector::setNumber(int n){
    number = n;
}

int RS_ActionDrawLineBisector::getNumber() const{
    return number;
}

void RS_ActionDrawLineBisector::init(int status){
    RS_PreviewActionInterface::init(status);
    if (status >= 0){
        RS_Snapper::suspend();
    }

    if (status < SetLine2){
        if (line2 && line2->isHighlighted()){
            line2->setHighlighted(false);
        }
        if (status < 0 && line1 && line1->isHighlighted()){
            line1->setHighlighted(false);
        }
        graphicView->redraw(RS2::RedrawDrawing);
    }
}

void RS_ActionDrawLineBisector::trigger(){
    RS_PreviewActionInterface::trigger();

    for (auto p: {line1, line2}) {
        if (p && p->isHighlighted()){
            p->setHighlighted(false);
        }
    }
    graphicView->redraw(RS2::RedrawDrawing);

    RS_Creation creation(container, graphicView);
    creation.createBisector(pPoints->coord1,
                            pPoints->coord2,
                            length,
                            number,
                            line1,
                            line2);
}


void RS_ActionDrawLineBisector::mouseMoveEvent(QMouseEvent *e){
    RS_DEBUG->print("RS_ActionDrawLineBisector::mouseMoveEvent begin");

    snapPoint(e); // update coordinates widget
    RS_Vector mouse = toGraph(e);

    deleteHighlights();
    switch (getStatus()) {
        case SetLine1: {
            RS_Entity *en = catchEntity(e, enTypeList, RS2::ResolveAll);
            if (en != nullptr){
                highlightHover(en);
            }
            break;
        }
        case SetLine2: {
            highlightSelected(line1);
            pPoints->coord2 = mouse;
            RS_Entity *en = catchEntity(e, enTypeList, RS2::ResolveAll);
            deletePreview();
            if (en == line1){
                line2 = nullptr;
            } else if (en != nullptr){
                line2 = dynamic_cast<RS_Line *>(en);

                RS_Creation creation(preview.get(), nullptr, false);
                auto ent = creation.createBisector(pPoints->coord1,
                                                   pPoints->coord2,
                                                   length,
                                                   number,
                                                   line1,
                                                   line2);
                if (ent != nullptr){
                    highlightHover(line2);
                    if (showRefEntitiesOnPreview) {
                        previewRefPoint(line1->getNearestPointOnEntity(pPoints->coord1));
                        previewRefPoint(ent->getStartpoint());
                        RS_Vector nearest = line2->getNearestPointOnEntity(mouse, false);
                        previewRefSelectablePoint(nearest);
                    }
                }
                drawPreview();
            }
            break;
        }
        case SetLength:
        case SetNumber: {
            if (line1 != nullptr){
                highlightSelected(line1);
            }
            break;
        }
        default:
            break;
    }
    drawHighlights();

    RS_DEBUG->print("RS_ActionDrawLineBisector::mouseMoveEvent end");
}

void RS_ActionDrawLineBisector::onMouseLeftButtonRelease(int status, QMouseEvent *e) {
    RS_Vector mouse = toGraph(e);
    switch (status) {
        case SetLine1: {
            pPoints->coord1 = mouse;
            RS_Entity *en = catchEntity(e,enTypeList,RS2::ResolveAll);
            if (isLine(en)){
                line1 = dynamic_cast<RS_Line *>(en);
                line2 = nullptr;
                setStatus(SetLine2);
            }
            break;
        }
        case SetLine2:
            pPoints->coord2 = mouse;
            trigger();
            setStatus(SetLine1);
            break;
        default:
            break;
    }

}

void RS_ActionDrawLineBisector::onMouseRightButtonRelease(int status, [[maybe_unused]]QMouseEvent *e) {
    deletePreview();
    initPrevious(status);
}

bool RS_ActionDrawLineBisector::doProcessCommand(int status, const QString &c) {
   bool accept = false;
    switch (status) {
        case SetLine1:
        case SetLine2: {
            lastStatus = (Status) status;
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
            bool ok;
            double l = RS_Math::eval(c, &ok);
            if (ok){
                accept = true;
                length = l;
            } else {
                commandMessage(tr("Not a valid expression"));
            }
            updateOptions();
            setStatus(lastStatus);
            break;
        }
        case SetNumber: {
            bool ok;
            int n = (int) RS_Math::eval(c, &ok);
            if (ok){
                accept= true;
                if (n > 0 && n <= 200)
                    number = n;
                else
                    commandMessage(
                        tr("Number sector lines not in range: ", "number of bisector to create must be in [1, 200]") + QString::number(n));
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
