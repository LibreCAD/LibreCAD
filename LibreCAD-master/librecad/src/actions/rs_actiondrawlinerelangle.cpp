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

#include "rs_actiondrawlinerelangle.h"
#include<cmath>
#include <QAction>
#include <QMouseEvent>
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_commandevent.h"
#include "rs_creation.h"
#include "rs_line.h"
#include "rs_coordinateevent.h"
#include "rs_math.h"
#include "rs_preview.h"
#include "rs_debug.h"

namespace {
auto enTypeList={RS2::EntityLine, RS2::EntityArc, RS2::EntityCircle,
				 RS2::EntityEllipse};
}

RS_ActionDrawLineRelAngle::RS_ActionDrawLineRelAngle(
		RS_EntityContainer& container,
		RS_GraphicView& graphicView,
		double angle,
		bool fixedAngle)
	:RS_PreviewActionInterface("Draw Lines with relative angles",
							   container, graphicView)
	,entity(nullptr)
	, pos(new RS_Vector{})
	,angle(angle)
	,length(10.)
	,fixedAngle(fixedAngle)
{
}

RS_ActionDrawLineRelAngle::~RS_ActionDrawLineRelAngle() = default;


RS2::ActionType RS_ActionDrawLineRelAngle::rtti() const{
	if( fixedAngle &&
			RS_Math::getAngleDifference(angle, M_PI_2) < RS_TOLERANCE_ANGLE)
		return RS2::ActionDrawLineOrthogonal;
	else
		return RS2::ActionDrawLineRelAngle;
}

void RS_ActionDrawLineRelAngle::trigger() {
    RS_PreviewActionInterface::trigger();

    deletePreview();

    RS_Creation creation(container, graphicView);
	creation.createLineRelAngle(*pos,
                                entity,
                                angle,
                                length);

    /*
       if (line) {
		   RS_Entity* newEntity = nullptr;

           newEntity = new RS_Line(container,
                                   line->getData());

           if (newEntity) {
               newEntity->setLayerToActive();
               newEntity->setPenToActive();
               container->addEntity(newEntity);

               // upd. undo list:
               if (document) {
                   document->startUndoCycle();
                   document->addUndoable(newEntity);
                   document->endUndoCycle();
               }
               graphicView->drawEntity(newEntity);
               setStatus(SetEntity);
           }
           //reset();
           delete line;
		   line = nullptr;
       } else {
           RS_DEBUG->print("RS_ActionDrawLineRelAngle::trigger:"
						   " Line is nullptr\n");
       }
    */
}



void RS_ActionDrawLineRelAngle::mouseMoveEvent(QMouseEvent* e) {
    RS_DEBUG->print("RS_ActionDrawLineRelAngle::mouseMoveEvent begin");

    RS_Vector mouse(graphicView->toGraphX(e->x()),
                    graphicView->toGraphY(e->y()));

    switch (getStatus()) {
    case SetEntity:
		entity = catchEntity(e, enTypeList, RS2::ResolveAll);
        break;

    case SetPos: {
            //length = graphicView->toGraphDX(graphicView->getWidth());
            //RS_Vector mouse = snapPoint(e);
			*pos = snapPoint(e);

			/*RS_Creation creation(nullptr, nullptr);
            RS_Line* l = creation.createLineRelAngle(mouse,
                         entity,
                         angle,
                         length);*/

            deletePreview();

			RS_Creation creation(preview.get(), nullptr, false);
			creation.createLineRelAngle(*pos,
                                        entity,
                                        angle,
                                        length);

            drawPreview();

            /*if (l) {
                if (line) {
                    delete line;
                }
                line = (RS_Line*)l->clone();

                deletePreview();
                preview->addEntity(l);
                drawPreview();
        }*/
        }
        break;

    default:
        break;
    }

    RS_DEBUG->print("RS_ActionDrawLineRelAngle::mouseMoveEvent end");
}



void RS_ActionDrawLineRelAngle::mouseReleaseEvent(QMouseEvent* e) {

    if (e->button()==Qt::LeftButton) {
        switch (getStatus()) {
        case SetEntity: {
				RS_Entity* en = catchEntity(e, enTypeList, RS2::ResolveAll);
				if (en) {
                    entity = en;

                    entity->setHighlighted(true);
                    graphicView->drawEntity(entity);

                    setStatus(SetPos);
                }
            }
            break;

        case SetPos: {
                RS_CoordinateEvent ce(snapPoint(e));
                coordinateEvent(&ce);
            }
            break;

        default:
            break;
        }
    } else if (e->button()==Qt::RightButton) {
        deletePreview();
        if (entity) {
            entity->setHighlighted(false);
            graphicView->drawEntity(entity);
        }
        init(getStatus()-1);
    }
}



void RS_ActionDrawLineRelAngle::coordinateEvent(RS_CoordinateEvent* e) {
	if (!e) {
        return;
    }

    switch (getStatus()) {
    case SetPos:
		*pos = e->getCoordinate();
        trigger();
        break;

    default:
        break;
    }
}



void RS_ActionDrawLineRelAngle::commandEvent(RS_CommandEvent* e) {
    QString c = e->getCommand().toLower();

	if (checkCommand("help", c)) {
		RS_DIALOGFACTORY->commandMessage(msgAvailableCommands()
										 + getAvailableCommands().join(", "));
        return;
    }

    switch (getStatus()) {
    case SetEntity:
    case SetPos:
        if (!fixedAngle && checkCommand("angle", c)) {
            deletePreview();
            setStatus(SetAngle);
        } else if (checkCommand("length", c)) {
            deletePreview();
            setStatus(SetLength);
        }
        break;

    case SetAngle: {
            bool ok;
            double a = RS_Math::eval(c, &ok);
            if (ok) {
                e->accept();
                angle = RS_Math::deg2rad(a);
			} else
                    RS_DIALOGFACTORY->commandMessage(tr("Not a valid expression"));
			RS_DIALOGFACTORY->requestOptions(this, true, true);
            setStatus(SetPos);
        }
        break;

    case SetLength: {
            bool ok;
            double l = RS_Math::eval(c, &ok);
            if (ok) {
                length = l;
			} else
				RS_DIALOGFACTORY->commandMessage(tr("Not a valid expression"));
			RS_DIALOGFACTORY->requestOptions(this, true, true);
            setStatus(SetPos);
        }
        break;

    default:
        break;
    }
}



QStringList RS_ActionDrawLineRelAngle::getAvailableCommands() {
    QStringList cmd;

    switch (getStatus()) {
    case SetPos:
    case SetLength:
        if (!fixedAngle) {
            cmd += command("angle");
        }
        cmd += command("length");
        break;
    default:
        break;
    }

    return cmd;
}


void RS_ActionDrawLineRelAngle::updateMouseButtonHints() {
	switch (getStatus()) {
	case SetEntity:
		RS_DIALOGFACTORY->updateMouseWidget(tr("Select base entity"),
											tr("Cancel"));
		break;
	case SetPos:
		RS_DIALOGFACTORY->updateMouseWidget(tr("Specify position"),
											tr("Back"));
		break;
	default:
		RS_DIALOGFACTORY->updateMouseWidget();
		break;
	}
}



void RS_ActionDrawLineRelAngle::showOptions() {
    RS_ActionInterface::showOptions();

	RS_DIALOGFACTORY->requestOptions(this, true);
}



void RS_ActionDrawLineRelAngle::hideOptions() {
    RS_ActionInterface::hideOptions();

	RS_DIALOGFACTORY->requestOptions(this, false);
}



void RS_ActionDrawLineRelAngle::updateMouseCursor()
{
    switch (getStatus())
    {
        case SetEntity:
            graphicView->setMouseCursor(RS2::SelectCursor);
            break;
        case SetPos:
            graphicView->setMouseCursor(RS2::CadCursor);
            break;
        default:
            break;
    }
}

// EOF
