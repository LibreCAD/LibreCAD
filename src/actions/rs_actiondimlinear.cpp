/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by 
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
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

#include "rs_actiondimlinear.h"
#include "rs_snapper.h"
#include "rs_constructionline.h"
#include "rs_dialogfactory.h"

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
                                       bool fixedAngle)
        :RS_ActionDimension("Draw linear dimensions",
                    container, graphicView) {

    edata.angle = angle;
    this->fixedAngle = fixedAngle;

    lastStatus = SetExtPoint1;

    reset();
}



RS_ActionDimLinear::~RS_ActionDimLinear() {}


QAction* RS_ActionDimLinear::createGUIAction(RS2::ActionType type, QObject* /*parent*/) {
    QAction* action;

    switch (type) {
    default:
    case RS2::ActionDimLinear:
		// tr("Linear")
		action = new QAction(tr("&Linear"),  NULL);
                action->setIcon(QIcon(":/extui/dimlinear.png"));
        //action->zetStatusTip(tr("Linear Dimension"));
        break;

    case RS2::ActionDimLinearHor:
		// tr("Horizontal")
		action = new QAction(tr("&Horizontal"), NULL);
		action->setIcon(QIcon(":/extui/dimhor.png"));
        //action->zetStatusTip(tr("Horizontal Dimension"));
        break;

    case RS2::ActionDimLinearVer:
		// tr("Vertical")
		action = new QAction(tr("&Vertical"), NULL);
		action->setIcon(QIcon(":/extui/dimver.png"));
        //action->zetStatusTip(tr("Vertical Dimension"));
        break;
    }

    return action;
}


void RS_ActionDimLinear::reset() {
    RS_ActionDimension::reset();

    edata = RS_DimLinearData(RS_Vector(false),
                             RS_Vector(false),
                             (fixedAngle ? edata.angle : 0.0), 0.0);

    if (RS_DIALOGFACTORY!=NULL) {
        RS_DIALOGFACTORY->requestOptions(this, true, true);
    }
}



void RS_ActionDimLinear::trigger() {
    RS_ActionDimension::trigger();

    preparePreview();
    RS_DimLinear* dim = new RS_DimLinear(container, data, edata);
    dim->setLayerToActive();
    dim->setPenToActive();
    dim->update();
    container->addEntity(dim);

    // upd. undo list:
    if (document!=NULL) {
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
    RS_Vector dirV;
    dirV.setPolar(100.0, edata.angle+M_PI/2.0);

    RS_ConstructionLine cl(
        NULL,
        RS_ConstructionLineData(
            edata.extensionPoint2,
            edata.extensionPoint2+dirV));

    data.definitionPoint =
        cl.getNearestPointOnEntity(data.definitionPoint);

}



void RS_ActionDimLinear::mouseMoveEvent(RS_MouseEvent* e) {
    RS_DEBUG->print("RS_ActionDimLinear::mouseMoveEvent begin");

    RS_Vector mouse = snapPoint(e);

    switch (getStatus()) {
    case SetExtPoint1:
        break;

    case SetExtPoint2:
        if (edata.extensionPoint1.valid) {
            deletePreview();
            preview->addEntity(new RS_Line(preview,
                                           RS_LineData(edata.extensionPoint1,
                                                       mouse)));
            drawPreview();
        }
        break;

    case SetDefPoint:
        if (edata.extensionPoint1.valid && edata.extensionPoint2.valid) {
            deletePreview();
            data.definitionPoint = mouse;

            preparePreview();

            RS_DimLinear* dim = new RS_DimLinear(preview, data, edata);
            dim->update();
            preview->addEntity(dim);
            drawPreview();
        }
        break;
    }

    RS_DEBUG->print("RS_ActionDimLinear::mouseMoveEvent end");
}



void RS_ActionDimLinear::mouseReleaseEvent(RS_MouseEvent* e) {
    if (RS2::qtToRsButtonState(e->button())==RS2::LeftButton) {
        RS_CoordinateEvent ce(snapPoint(e));
        coordinateEvent(&ce);
    } else if (RS2::qtToRsButtonState(e->button())==RS2::RightButton) {
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
        edata.extensionPoint1 = pos;
        graphicView->moveRelativeZero(pos);
        setStatus(SetExtPoint2);
        break;

    case SetExtPoint2:
        edata.extensionPoint2 = pos;
        graphicView->moveRelativeZero(pos);
        setStatus(SetDefPoint);
        break;

    case SetDefPoint:
        data.definitionPoint = pos;
        trigger();
        reset();
        setStatus(SetExtPoint1);
        break;

    default:
        break;
    }
}



void RS_ActionDimLinear::commandEvent(RS_CommandEvent* e) {
    RS_String c = e->getCommand().lower();

    if (checkCommand("help", c)) {
        if (RS_DIALOGFACTORY!=NULL) {
            RS_DIALOGFACTORY->commandMessage(msgAvailableCommands()
                                             + getAvailableCommands().join(", "));
        }
        return;
    }

    switch (getStatus()) {
    case SetText:
        setText(c);
        if (RS_DIALOGFACTORY!=NULL) {
            RS_DIALOGFACTORY->requestOptions(this, true, true);
        }
        graphicView->enableCoordinateInput();
        setStatus(lastStatus);
        break;

    case SetAngle: {
            bool ok;
            double a = RS_Math::eval(c, &ok);
            if (ok==true) {
                setAngle(RS_Math::deg2rad(a));
            } else {
                if (RS_DIALOGFACTORY!=NULL) {
                    RS_DIALOGFACTORY->commandMessage(tr("Not a valid expression"));
                }
            }
            if (RS_DIALOGFACTORY!=NULL) {
                RS_DIALOGFACTORY->requestOptions(this, true, true);
            }
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



RS_StringList RS_ActionDimLinear::getAvailableCommands() {
    RS_StringList cmd;

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
    if (RS_DIALOGFACTORY!=NULL) {
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
            RS_DIALOGFACTORY->updateMouseWidget("", "");
            break;
        }
    }
}



void RS_ActionDimLinear::showOptions() {
    RS_ActionInterface::showOptions();

    if (RS_DIALOGFACTORY!=NULL) {
        RS_DIALOGFACTORY->requestOptions(this, true, true);
    }
}



void RS_ActionDimLinear::hideOptions() {
    RS_ActionInterface::hideOptions();

    if (RS_DIALOGFACTORY!=NULL) {
        RS_DIALOGFACTORY->requestOptions(this, false);
    }
}



// EOF
