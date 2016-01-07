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
#include "rs_actiondimradial.h"

#include "rs_dimradial.h"
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_commandevent.h"
#include "rs_arc.h"
#include "rs_circle.h"
#include "rs_coordinateevent.h"
#include "rs_math.h"
#include "rs_preview.h"
#include "rs_debug.h"


RS_ActionDimRadial::RS_ActionDimRadial(
    RS_EntityContainer& container,
    RS_GraphicView& graphicView)
        :RS_ActionDimension("Draw Radial Dimensions",
					container, graphicView)
		,entity(nullptr)
		,pos(new RS_Vector{})
		,lastStatus(SetEntity)
{
	actionType=RS2::ActionDimRadial;
    reset();
}

RS_ActionDimRadial::~RS_ActionDimRadial() = default;

void RS_ActionDimRadial::reset() {
    RS_ActionDimension::reset();

	edata.reset(new RS_DimRadialData{{}, 0.0});
	entity = nullptr;
	*pos = {};
	lastStatus = SetEntity;
    RS_DIALOGFACTORY->requestOptions(this, true, true);
}



void RS_ActionDimRadial::trigger() {
    RS_ActionDimension::trigger();

    preparePreview();
    if (entity) {
		RS_DimRadial* newEntity = nullptr;

        newEntity = new RS_DimRadial(container,
									 *data,
									 *edata);

        newEntity->setLayerToActive();
        newEntity->setPenToActive();
        newEntity->update();
        container->addEntity(newEntity);

        // upd. undo list:
        if (document) {
            document->startUndoCycle();
            document->addUndoable(newEntity);
            document->endUndoCycle();
        }
        RS_Vector rz = graphicView->getRelativeZero();
		graphicView->redraw(RS2::RedrawDrawing);
        graphicView->moveRelativeZero(rz);
		RS_Snapper::finish();

    }
    else {
        RS_DEBUG->print("RS_ActionDimRadial::trigger:"
						" Entity is nullptr\n");
    }
}


void RS_ActionDimRadial::preparePreview() {
    if (entity) {
		double angle = data->definitionPoint.angleTo(*pos);
        double radius=0.0;
        if (entity->rtti()==RS2::EntityArc) {
            radius = ((RS_Arc*)entity)->getRadius();
        } else if (entity->rtti()==RS2::EntityCircle) {
            radius = ((RS_Circle*)entity)->getRadius();
        }

		edata->definitionPoint.setPolar(radius, angle);
		edata->definitionPoint += data->definitionPoint;
    }
}



void RS_ActionDimRadial::mouseMoveEvent(QMouseEvent* e) {
    RS_DEBUG->print("RS_ActionDimRadial::mouseMoveEvent begin");

    //RS_Vector mouse(graphicView->toGraphX(e->x()),
    //                graphicView->toGraphY(e->y()));

	switch (getStatus()) {

    case SetPos:
        if (entity) {
			*pos = snapPoint(e);

            preparePreview();

			RS_DimRadial* d = new RS_DimRadial(preview.get(), *data, *edata);

            deletePreview();
            preview->addEntity(d);
            d->update();
            drawPreview();
        }
        break;

    default:
        break;
    }

    RS_DEBUG->print("RS_ActionDimRadial::mouseMoveEvent end");
}



void RS_ActionDimRadial::mouseReleaseEvent(QMouseEvent* e) {

    if (e->button()==Qt::LeftButton) {
        switch (getStatus()) {
        case SetEntity: {
                RS_Entity* en = catchEntity(e, RS2::ResolveAll);
                if (en) {
                    if (en->rtti()==RS2::EntityArc ||
                            en->rtti()==RS2::EntityCircle) {
                        entity = en;
                        if (entity->rtti()==RS2::EntityArc) {
							data->definitionPoint =
								static_cast<RS_Arc*>(entity)->getCenter();
                        } else if (entity->rtti()==RS2::EntityCircle) {
							data->definitionPoint =
								static_cast<RS_Circle*>(entity)->getCenter();
                        }
						graphicView->moveRelativeZero(data->definitionPoint);
                        setStatus(SetPos);
                    } else {
                        RS_DIALOGFACTORY->commandMessage(tr("Not a circle "
                                                            "or arc entity"));
                    }
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
        init(getStatus()-1);
    }
}



void RS_ActionDimRadial::coordinateEvent(RS_CoordinateEvent* e) {
	if (!e) return;

    switch (getStatus()) {
    case SetPos:
		*pos = e->getCoordinate();
        trigger();
        reset();
        setStatus(SetEntity);
        break;

    default:
        break;
    }
}



void RS_ActionDimRadial::commandEvent(RS_CommandEvent* e) {
    QString c = e->getCommand().toLower();

    if (checkCommand("help", c)) {
        RS_DIALOGFACTORY->commandMessage(msgAvailableCommands()
                                         + getAvailableCommands().join(", "));
        return;
    }

    // setting new text label:
    if (getStatus()==SetText) {
        setText(c);
        RS_DIALOGFACTORY->requestOptions(this, true, true);
        graphicView->enableCoordinateInput();
        setStatus(lastStatus);
        return;
    }

    // command: text
    if (checkCommand("text", c)) {
        lastStatus = (Status)getStatus();
        graphicView->disableCoordinateInput();
        setStatus(SetText);
    }

    // setting angle
    if (getStatus()==SetPos) {
        bool ok;
        double a = RS_Math::eval(c, &ok);
		if (ok) {
			pos->setPolar(1.0, RS_Math::deg2rad(a));
			*pos += data->definitionPoint;
            trigger();
            reset();
            setStatus(SetEntity);
        } else {
            RS_DIALOGFACTORY->commandMessage(tr("Not a valid expression"));
        }
        return;
    }
}



QStringList RS_ActionDimRadial::getAvailableCommands() {
    QStringList cmd;

    switch (getStatus()) {
    case SetEntity:
    case SetPos:
        cmd += command("text");
        break;

    default:
        break;
    }

    return cmd;
}


void RS_ActionDimRadial::updateMouseButtonHints() {
    switch (getStatus()) {
    case SetEntity:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Select arc or circle entity"),
                                            tr("Cancel"));
        break;
    case SetPos:
        RS_DIALOGFACTORY->updateMouseWidget(
            tr("Specify dimension line position or enter angle:"),
            tr("Cancel"));
        break;
    case SetText:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Enter dimension text:"), "");
        break;
    default:
		RS_DIALOGFACTORY->updateMouseWidget();
        break;
    }
}



void RS_ActionDimRadial::showOptions() {
    RS_ActionInterface::showOptions();

    RS_DIALOGFACTORY->requestOptions(this, true);
    //RS_DIALOGFACTORY->requestDimRadialOptions(edata, true);
}



void RS_ActionDimRadial::hideOptions() {
    RS_ActionInterface::hideOptions();

    //RS_DIALOGFACTORY->requestDimRadialOptions(edata, false);
    RS_DIALOGFACTORY->requestOptions(this, false);
}



// EOF
