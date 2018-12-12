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

#include <QAction>
#include <QMouseEvent>
#include "rs_actiondrawlineparallel.h"

#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_creation.h"
#include "rs_commands.h"
#include "rs_commandevent.h"
#include "rs_actiondrawlineparallelthrough.h"
#include "rs_math.h"
#include "rs_preview.h"
#include "rs_debug.h"

RS_ActionDrawLineParallel::RS_ActionDrawLineParallel(
		RS_EntityContainer& container,
		RS_GraphicView& graphicView)
	:RS_PreviewActionInterface("Draw Parallels", container, graphicView)
	,parallel(nullptr)
	,distance(1.0)
	,number(1)
	, coord(new RS_Vector{})
	,entity(nullptr)
{
	actionType=RS2::ActionDrawLineParallel;
}

RS_ActionDrawLineParallel::~RS_ActionDrawLineParallel() = default;

double RS_ActionDrawLineParallel::getDistance() const{
	return distance;
}

void RS_ActionDrawLineParallel::setDistance(double d) {
	distance = d;
}

int RS_ActionDrawLineParallel::getNumber() const{
	return number;
}

void RS_ActionDrawLineParallel::setNumber(int n) {
	number = n;
}

void RS_ActionDrawLineParallel::trigger() {
    RS_PreviewActionInterface::trigger();

    RS_Creation creation(container, graphicView);
	RS_Entity* e = creation.createParallel(*coord,
                                           distance, number,
                                           entity);

	if (!e) {
        RS_DEBUG->print("RS_ActionDrawLineParallel::trigger:"
                        " No parallels added\n");
    }
}

void RS_ActionDrawLineParallel::mouseMoveEvent(QMouseEvent* e) {
    RS_DEBUG->print("RS_ActionDrawLineParallel::mouseMoveEvent begin");

	*coord = {graphicView->toGraphX(e->x()), graphicView->toGraphY(e->y())};

    entity = catchEntity(e, RS2::ResolveAll);

    switch (getStatus()) {
    case SetEntity: {
            deletePreview();

			RS_Creation creation(preview.get(), nullptr, false);
			creation.createParallel(*coord,
                                    distance, number,
                                    entity);

            drawPreview();
        }
        break;

    default:
        break;
    }

    RS_DEBUG->print("RS_ActionDrawLineParallel::mouseMoveEvent end");
}

void RS_ActionDrawLineParallel::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button()==Qt::RightButton) {
        init(getStatus()-1);
    } else {
        trigger();
    }
}

void RS_ActionDrawLineParallel::updateMouseButtonHints() {
	switch (getStatus()) {
	case SetEntity:
		RS_DIALOGFACTORY->updateMouseWidget(
					tr("Specify Distance <%1> or select entity or [%2]")
					.arg(distance).arg(RS_COMMANDS->command("through")),
					tr("Cancel"));
		break;

	case SetNumber:
		RS_DIALOGFACTORY->updateMouseWidget(tr("Enter number:"), "");
		break;

	default:
		RS_DIALOGFACTORY->updateMouseWidget();
		break;
	}
}

void RS_ActionDrawLineParallel::showOptions() {
	RS_ActionInterface::showOptions();

	RS_DIALOGFACTORY->requestOptions(this, true);
	updateMouseButtonHints();
}

void RS_ActionDrawLineParallel::hideOptions() {
    RS_ActionInterface::hideOptions();

	RS_DIALOGFACTORY->requestOptions(this, false);
}

void RS_ActionDrawLineParallel::commandEvent(RS_CommandEvent* e) {
    QString c = e->getCommand().toLower();

	if (checkCommand("help", c)) {
		RS_DIALOGFACTORY->commandMessage(msgAvailableCommands()
										 + getAvailableCommands().join(", "));
		return;
	}

    switch (getStatus()) {
    case SetEntity: {
            if (checkCommand("through", c)) {
                finish(false);
                graphicView->setCurrentAction(
                    new RS_ActionDrawLineParallelThrough(*container,
                                                         *graphicView));
            } else if (checkCommand("number", c)) {
                deletePreview();
                setStatus(SetNumber);
            } else {
                bool ok;
                double d = RS_Math::eval(c, &ok);
                if(ok) e->accept();
                if (ok && d>1.0e-10) {
                    distance = d;
				} else
					RS_DIALOGFACTORY->commandMessage(tr("Not a valid expression"));
				RS_DIALOGFACTORY->requestOptions(this, true, true);
                updateMouseButtonHints();
                //setStatus(SetEntity);
            }
        }
        break;

    case SetNumber: {
            bool ok;
            int n = c.toInt(&ok);
            if (ok) {
                e->accept();
                if (n>0 && n<100) {
                    number = n;
				} else
					RS_DIALOGFACTORY->commandMessage(tr("Not a valid number. "
														"Try 1..99"));
			} else
				RS_DIALOGFACTORY->commandMessage(tr("Not a valid expression"));
			RS_DIALOGFACTORY->requestOptions(this, true, true);
            setStatus(SetEntity);
        }
        break;

    default:
        break;
    }
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

void RS_ActionDrawLineParallel::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::SelectCursor);
}

// EOF
