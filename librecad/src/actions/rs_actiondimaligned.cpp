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
#include "rs_actiondimaligned.h"
#include "rs_dimaligned.h"

#include "rs_dialogfactory.h"
#include "rs_line.h"
#include "rs_graphicview.h"
#include "rs_commandevent.h"
#include "rs_constructionline.h"
#include "rs_coordinateevent.h"
#include "rs_preview.h"
#include "rs_debug.h"

RS_ActionDimAligned::RS_ActionDimAligned(RS_EntityContainer& container,
        RS_GraphicView& graphicView)
        :RS_ActionDimension("Draw aligned dimensions",
                    container, graphicView) {
	actionType=RS2::ActionDimAligned;
	reset();
}



RS_ActionDimAligned::~RS_ActionDimAligned()  = default;

void RS_ActionDimAligned::reset() {
    RS_ActionDimension::reset();

	edata.reset(new RS_DimAlignedData(RS_Vector(false),
							  RS_Vector(false))
				);
    lastStatus = SetExtPoint1;
	RS_DIALOGFACTORY->requestOptions(this, true, true);
}



void RS_ActionDimAligned::trigger() {
    RS_ActionDimension::trigger();

    preparePreview();
    graphicView->moveRelativeZero(data->definitionPoint);

		//data->text = getText();
    RS_DimAligned* dim =
		new RS_DimAligned(container, *data, *edata);
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

    RS_DEBUG->print("RS_ActionDimAligned::trigger():"
                    " dim added: %d", dim->getId());
}



void RS_ActionDimAligned::preparePreview() {
	RS_Vector dirV = RS_Vector::polar(100.,
				  edata->extensionPoint1.angleTo(
					  edata->extensionPoint2)
				  +M_PI_2);
	RS_ConstructionLine cl(nullptr,
                           RS_ConstructionLineData(
							   edata->extensionPoint2,
							   edata->extensionPoint2+dirV));

    data->definitionPoint =
        cl.getNearestPointOnEntity(data->definitionPoint);
}



void RS_ActionDimAligned::mouseMoveEvent(QMouseEvent* e) {
    RS_DEBUG->print("RS_ActionDimAligned::mouseMoveEvent begin");

    RS_Vector mouse = snapPoint(e);

    switch (getStatus()) {
    case SetExtPoint1:
        break;

    case SetExtPoint2:
		if (edata->extensionPoint1.valid) {
            deletePreview();
            preview->addEntity(
				new RS_Line{preview.get(),edata->extensionPoint1, mouse}
            );
            drawPreview();
        }
        break;

    case SetDefPoint:
		if (edata->extensionPoint1.valid && edata->extensionPoint2.valid) {
            deletePreview();
			data->definitionPoint = mouse;

            preparePreview();

						//data->text = getText();
			RS_DimAligned* dim = new RS_DimAligned(preview.get(), *data, *edata);
            preview->addEntity(dim);
            dim->update();
            drawPreview();
        }
        break;

        default:
                break;
    }

    RS_DEBUG->print("RS_ActionDimAligned::mouseMoveEvent end");
}



void RS_ActionDimAligned::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button()==Qt::LeftButton) {
        RS_CoordinateEvent ce(snapPoint(e));
        coordinateEvent(&ce);
    } else if (e->button()==Qt::RightButton) {
        deletePreview();
        init(getStatus()-1);
    }
}



void RS_ActionDimAligned::coordinateEvent(RS_CoordinateEvent* e) {
	if (!e) return;

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



void RS_ActionDimAligned::commandEvent(RS_CommandEvent* e) {
    QString c = e->getCommand().toLower();

    if (checkCommand("help", c)) {
		RS_DIALOGFACTORY->commandMessage(msgAvailableCommands()
										 + getAvailableCommands().join(", "));
        return;
    }

    switch (getStatus()) {
    case SetText: {
            setText(c);
			RS_DIALOGFACTORY->requestOptions(this, true, true);
            setStatus(lastStatus);
            graphicView->enableCoordinateInput();
        }
        break;

    default:
        if (checkCommand("text", c)) {
            lastStatus = (Status)getStatus();
            graphicView->disableCoordinateInput();
            setStatus(SetText);
        }
        break;
    }
}



QStringList RS_ActionDimAligned::getAvailableCommands() {
    QStringList cmd;

    switch (getStatus()) {
    case SetExtPoint1:
    case SetExtPoint2:
    case SetDefPoint:
        cmd += command("text");
        break;

    default:
        break;
    }

    return cmd;
}



void RS_ActionDimAligned::updateMouseButtonHints() {
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
	default:
		RS_DIALOGFACTORY->updateMouseWidget();
		break;
	}
}



void RS_ActionDimAligned::hideOptions() {
	RS_DIALOGFACTORY->requestOptions(this, false);

    RS_ActionDimension::hideOptions();
}



void RS_ActionDimAligned::showOptions() {
    RS_ActionDimension::showOptions();

	RS_DIALOGFACTORY->requestOptions(this, true);
}

// EOF
