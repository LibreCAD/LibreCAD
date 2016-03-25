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
#include<cmath>
#include <QAction>
#include <QMouseEvent>
#include "rs_actiondimlinear.h"
#include "rs_dimlinear.h"

#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_commandevent.h"
#include "rs_constructionline.h"
#include "rs_line.h"
#include "rs_coordinateevent.h"
#include "rs_math.h"
#include "rs_preview.h"
#include "rs_debug.h"

/**
 * Constructor.
 *
 * @param angle Initial angle in rad.
 * @param fixedAngle true: The user can't change the angle.
 *                   false: The user can change the angle in a option widget.
 */
RS_ActionDimLinear::RS_ActionDimLinear(RS_EntityContainer& container,
                                       RS_GraphicView& graphicView,
                                       double angle,
									   bool _fixedAngle, RS2::ActionType /*type*/)
        :RS_ActionDimension("Draw linear dimensions",
                    container, graphicView)
		,edata(new RS_DimLinearData(RS_Vector(0., 0.), RS_Vector(0., 0.), angle, 0.))
		,fixedAngle(_fixedAngle)
		,lastStatus(SetExtPoint1)
{
	//TODO: fix dim linear type logic: whether it's for linear only, or should cover horizontal/vertical dim types
	actionType=RS2::ActionDimLinear;
	reset();
}



RS_ActionDimLinear::~RS_ActionDimLinear() = default;

void RS_ActionDimLinear::reset() {
    RS_ActionDimension::reset();

	edata.reset(new RS_DimLinearData(RS_Vector(false),
                             RS_Vector(false),
							 (fixedAngle ? edata->angle : 0.0), 0.0)
				);

	RS_DIALOGFACTORY->requestOptions(this, true, true);
}



void RS_ActionDimLinear::trigger() {
    RS_ActionDimension::trigger();

    preparePreview();
	RS_DimLinear* dim = new RS_DimLinear(container, *data, *edata);
    dim->setLayerToActive();
    dim->setPenToActive();
    dim->update();
    container->addEntity(dim);

    // upd. undo list:
    if (document) {
        document->startUndoCycle();
        document->addUndoable(dim);
        document->endUndoCycle();
    }

    RS_Vector rz = graphicView->getRelativeZero();
	graphicView->redraw(RS2::RedrawDrawing);
    graphicView->moveRelativeZero(rz);

    RS_DEBUG->print("RS_ActionDimLinear::trigger():"
                    " dim added: %d", dim->getId());
}


void RS_ActionDimLinear::preparePreview() {
	RS_Vector dirV = RS_Vector::polar(100., edata->angle+M_PI_2);

    RS_ConstructionLine cl(
        NULL,
        RS_ConstructionLineData(
			edata->extensionPoint2,
			edata->extensionPoint2+dirV));

	data->definitionPoint =
		cl.getNearestPointOnEntity(data->definitionPoint);

}



void RS_ActionDimLinear::mouseMoveEvent(QMouseEvent* e) {
    RS_DEBUG->print("RS_ActionDimLinear::mouseMoveEvent begin");

    RS_Vector mouse = snapPoint(e);

    switch (getStatus()) {
    case SetExtPoint1:
        break;

    case SetExtPoint2:
		if (edata->extensionPoint1.valid) {
            deletePreview();
			preview->addEntity(new RS_Line{preview.get(),
										   edata->extensionPoint1, mouse});
            drawPreview();
        }
        break;

    case SetDefPoint:
		if (edata->extensionPoint1.valid && edata->extensionPoint2.valid) {
            deletePreview();
			data->definitionPoint = mouse;

            preparePreview();

			RS_DimLinear* dim = new RS_DimLinear(preview.get(), *data, *edata);
            preview->addEntity(dim);
            dim->update();
            drawPreview();
        }
        break;
    }

    RS_DEBUG->print("RS_ActionDimLinear::mouseMoveEvent end");
}



void RS_ActionDimLinear::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button()==Qt::LeftButton) {
        RS_CoordinateEvent ce(snapPoint(e));
        coordinateEvent(&ce);
    } else if (e->button()==Qt::RightButton) {
        deletePreview();
        init(getStatus()-1);
    }
}



void RS_ActionDimLinear::coordinateEvent(RS_CoordinateEvent* e) {
    if (e==NULL) {
        return;
    }

    RS_Vector pos = e->getCoordinate();

    switch (getStatus()) {
    case SetExtPoint1:
		edata->extensionPoint1 = pos;
        graphicView->moveRelativeZero(pos);
        setStatus(SetExtPoint2);
        break;

    case SetExtPoint2:
		edata->extensionPoint2 = pos;
        graphicView->moveRelativeZero(pos);
        setStatus(SetDefPoint);
        break;

    case SetDefPoint:
		data->definitionPoint = pos;
        trigger();
        reset();
        setStatus(SetExtPoint1);
        break;

    default:
        break;
    }
}

double RS_ActionDimLinear::getAngle() const{
	return edata->angle;
}

void RS_ActionDimLinear::setAngle(double a) {
	edata->angle = a;
}

bool RS_ActionDimLinear::hasFixedAngle() const{
	return fixedAngle;
}

void RS_ActionDimLinear::commandEvent(RS_CommandEvent* e) {
    QString c = e->getCommand().toLower();

	if (checkCommand("help", c)) {
		RS_DIALOGFACTORY->commandMessage(msgAvailableCommands()
										 + getAvailableCommands().join(", "));
		return;
	}

    switch (getStatus()) {
    case SetText:
        setText(c);
		RS_DIALOGFACTORY->requestOptions(this, true, true);
        graphicView->enableCoordinateInput();
        setStatus(lastStatus);
        break;

    case SetAngle: {
            bool ok;
            double a = RS_Math::eval(c, &ok);
			if (ok) {
                setAngle(RS_Math::deg2rad(a));
            } else {
				RS_DIALOGFACTORY->commandMessage(tr("Not a valid expression"));
            }
			RS_DIALOGFACTORY->requestOptions(this, true, true);
            setStatus(lastStatus);
        }
        break;

    default:
        lastStatus = (Status)getStatus();
        deletePreview();
        if (checkCommand("text", c)) {
            graphicView->disableCoordinateInput();
            setStatus(SetText);
            return;
        } else if (!fixedAngle && (checkCommand("angle", c))) {
            setStatus(SetAngle);
        }
        break;
    }
}



QStringList RS_ActionDimLinear::getAvailableCommands() {
    QStringList cmd;

    switch (getStatus()) {
    case SetExtPoint1:
    case SetExtPoint2:
    case SetDefPoint:
        cmd += command("text");
        if (!fixedAngle) {
            cmd += command("angle");
        }
        break;

    default:
        break;
    }

    return cmd;
}


void RS_ActionDimLinear::updateMouseButtonHints() {
	switch (getStatus()) {
	case SetExtPoint1:
		RS_DIALOGFACTORY->updateMouseWidget(
					tr("Specify first extension line origin"),
					tr("Cancel"));
		break;
	case SetExtPoint2:
		RS_DIALOGFACTORY->updateMouseWidget(
					tr("Specify second extension line origin"),
					tr("Back"));
		break;
	case SetDefPoint:
		RS_DIALOGFACTORY->updateMouseWidget(
					tr("Specify dimension line location"),
					tr("Back"));
		break;
	case SetText:
		RS_DIALOGFACTORY->updateMouseWidget(tr("Enter dimension text:"), "");
		break;
	case SetAngle:
		RS_DIALOGFACTORY->updateMouseWidget(
					tr("Enter dimension line angle:"), "");
		break;
	default:
		RS_DIALOGFACTORY->updateMouseWidget();
		break;
	}
}



void RS_ActionDimLinear::showOptions() {
    RS_ActionInterface::showOptions();

	RS_DIALOGFACTORY->requestOptions(this, true, true);
}



void RS_ActionDimLinear::hideOptions() {
    RS_ActionInterface::hideOptions();

	RS_DIALOGFACTORY->requestOptions(this, false);
}



// EOF
