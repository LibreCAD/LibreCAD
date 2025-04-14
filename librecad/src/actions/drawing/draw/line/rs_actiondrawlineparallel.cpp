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

RS_ActionDrawLineParallel::RS_ActionDrawLineParallel(LC_ActionContext *actionContext, RS2::ActionType actionType)
	:RS_PreviewActionInterface("Draw Parallels", actionContext, actionType)
	,parallel(nullptr)
	,distance(1.0)
	,number(1)
	, coord(new RS_Vector{})
	,entity(nullptr){
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

void RS_ActionDrawLineParallel::doTrigger() {
    RS_Creation creation(m_container, m_viewport);
    RS_Entity *e = creation.createParallel(*coord,distance, number,entity);

    if (!e){
        RS_DEBUG->print("RS_ActionDrawLineParallel::trigger:No parallels added\n");
    }
}

void RS_ActionDrawLineParallel::onMouseMoveEvent([[maybe_unused]]int status, LC_MouseEvent *e) {
    *coord = {e->graphPoint};

    entity = catchAndDescribe(e, RS2::ResolveAll);

    switch (getStatus()) {
        case SetEntity: {
            if (entity != nullptr){
                RS_Creation creation(m_preview.get(), nullptr, false);
                RS_Entity* createdParallel = creation.createParallel(*coord,distance, number,entity);
                if (createdParallel != nullptr){
                    highlightHover(entity);
                    if (number == 1){
                        prepareEntityDescription(createdParallel, RS2::EntityDescriptionLevel::DescriptionCreating);
                    }
                    else{
                       appendInfoCursorZoneMessage(QString::number(number) + tr(" entities will be created"), 2, false);
                    }
                    if (m_showRefEntitiesOnPreview) {
                        RS_Vector nearest = entity->getNearestPointOnEntity(*coord, false);
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

void RS_ActionDrawLineParallel::onMouseLeftButtonRelease([[maybe_unused]]int status, [[maybe_unused]]LC_MouseEvent *e) {
    trigger();
}

void RS_ActionDrawLineParallel::onMouseRightButtonRelease(int status, [[maybe_unused]]LC_MouseEvent *e) {
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
                // fixme - sand - files - direct action creation
                m_graphicView->setCurrentAction(std::make_shared<RS_ActionDrawLineParallelThrough>(m_actionContext));
            } else if (checkCommand("number", c)){
                deletePreview();
                setStatus(SetNumber);
                accept = true;
            } else {
                bool ok = false;
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
            bool ok = false;
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
    return new QG_LineParallelOptions(m_actionType);
}
