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

#include "rs_actiondrawlineparallel.h"
#include "rs_actiondrawlineparallelthrough.h"
#include "rs_commandevent.h"
#include "rs_commands.h"
#include "rs_creation.h"
#include "rs_debug.h"
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_math.h"
#include "rs_preview.h"
#include "qg_lineparalleloptions.h"

RS_ActionDrawLineParallel::RS_ActionDrawLineParallel(
		RS_EntityContainer& container,
		RS_GraphicView& graphicView, RS2::ActionType actionType)
	:RS_PreviewActionInterface("Draw Parallels", container, graphicView)
	,parallel(nullptr)
	,distance(1.0)
	,number(1)
	, coord(new RS_Vector{})
	,entity(nullptr){
    this->actionType= actionType;
}

RS_ActionDrawLineParallel::~RS_ActionDrawLineParallel() = default;

double RS_ActionDrawLineParallel::getDistance() const{
    return distance;
}

void RS_ActionDrawLineParallel::setDistance(double d){
    distance = d;
}

int RS_ActionDrawLineParallel::getNumber() const{
    return number;
}

void RS_ActionDrawLineParallel::setNumber(int n){
    number = n;
}

void RS_ActionDrawLineParallel::trigger(){
    RS_PreviewActionInterface::trigger();

    RS_Creation creation(container, graphicView);
    RS_Entity *e = creation.createParallel(*coord,
                                           distance, number,
                                           entity);

    if (!e){
        RS_DEBUG->print("RS_ActionDrawLineParallel::trigger:"
                        " No parallels added\n");
    }
}

void RS_ActionDrawLineParallel::mouseMoveEvent(QMouseEvent *e){
    RS_DEBUG->print("RS_ActionDrawLineParallel::mouseMoveEvent begin");

    *coord = {toGraph(e)};

    entity = catchEntity(e, RS2::ResolveAll);

    switch (getStatus()) {
        case SetEntity: {
            deletePreview();
            deleteHighlights();
            if (entity != nullptr){
                RS_Creation creation(preview.get(), nullptr, false);
                RS_Entity* createdParallel = creation.createParallel(*coord,
                                        distance, number,
                                        entity);
                if (createdParallel != nullptr){
                    highlightHover(entity);
                    if (showRefEntitiesOnPreview) {
                        RS_Vector nearest = entity->getNearestPointOnEntity(*coord, false);
                        previewRefPoint(nearest);
                    }
                }
                drawHighlights();
            }
            drawPreview();
            break;
        }
        default:
            break;
    }

    RS_DEBUG->print("RS_ActionDrawLineParallel::mouseMoveEvent end");
}

void RS_ActionDrawLineParallel::onMouseLeftButtonRelease([[maybe_unused]]int status, [[maybe_unused]]QMouseEvent *e) {
    trigger();
}

void RS_ActionDrawLineParallel::onMouseRightButtonRelease(int status, [[maybe_unused]]QMouseEvent *e) {
    initPrevious(status);
}

void RS_ActionDrawLineParallel::updateMouseButtonHints(){
    switch (getStatus()) {
        case SetEntity:
            updateMouseWidgetTRCancel(tr("Specify Distance <%1> or select entity or [%2]").arg(distance).arg(command("through")));
            break;
        case SetNumber:
            updateMouseWidget(tr("Enter number:"));
            break;
        default:
            updateMouseWidget();
            break;
    }
}

bool RS_ActionDrawLineParallel::doProcessCommand(int status, const QString &c) {
    bool accept = false;
    switch (status) {
        case SetEntity: {
            // fixme = rework support of throught for simpler UX - add this to UI, probably combine two actions into same implementation
            if (checkCommand("through", c)){
                finish(false);
                accept = true;
                graphicView->setCurrentAction(
                    new RS_ActionDrawLineParallelThrough(*container,
                                                         *graphicView));
            } else if (checkCommand("number", c)){
                deletePreview();
                setStatus(SetNumber);
                accept = true;
            } else {
                bool ok;
                double d = RS_Math::eval(c, &ok);
                accept = true;
                if (ok && d > 1.0e-10){
                    distance = d;
                } else {
                    commandMessage(tr("Not a valid expression"));
                }
                updateOptions();
                updateMouseButtonHints();
                //setStatus(SetEntity);
            }
            break;
        }
        case SetNumber: {
            bool ok;
            int n = c.toInt(&ok);
            if (ok){
                accept = true;
                if (n > 0 && n < 100){
                    number = n;
                } else
                    commandMessage(tr("Not a valid number. Try 1..99"));
            } else {
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

QStringList RS_ActionDrawLineParallel::getAvailableCommands() {
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

RS2::CursorType RS_ActionDrawLineParallel::doGetMouseCursor([[maybe_unused]] int status){
    return RS2::SelectCursor;
}

LC_ActionOptionsWidget* RS_ActionDrawLineParallel::createOptionsWidget(){
    return new QG_LineParallelOptions(actionType);
}
