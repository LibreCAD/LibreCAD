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
#include "rs_actioninfodist.h"

#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_line.h"
#include "rs_graphic.h"
#include "rs_coordinateevent.h"
#include "rs_preview.h"
#include "rs_debug.h"

struct RS_ActionInfoDist::Points {
	RS_Vector point1;
	RS_Vector point2;
};


RS_ActionInfoDist::RS_ActionInfoDist(RS_EntityContainer& container,
                                     RS_GraphicView& graphicView)
        :RS_PreviewActionInterface("Info Dist",
						   container, graphicView)
		, pPoints(new Points{})
{
	actionType=RS2::ActionInfoDist;
}

RS_ActionInfoDist::~RS_ActionInfoDist()=default;

void RS_ActionInfoDist::init(int status) {
    RS_ActionInterface::init(status);
}



void RS_ActionInfoDist::trigger() {

    RS_DEBUG->print("RS_ActionInfoDist::trigger()");

	if (pPoints->point1.valid && pPoints->point2.valid) {
		auto dV = pPoints->point2 - pPoints->point1;
		QStringList dists;
		for(double a: {dV.magnitude(), dV.x, dV.y}){
			dists<<RS_Units::formatLinear(a, graphic->getUnit(),
										  graphic->getLinearFormat(), graphic->getLinearPrecision());
		}

		QString&& angle = RS_Units::formatAngle(dV.angle(),
												graphic->getAngleFormat(), graphic->getAnglePrecision());

		RS_DIALOGFACTORY->commandMessage(
                    tr("Distance: %1 Cartesian: (%2 , %3), Polar: (%4<%5)").arg(dists[0])
				.arg(dists[1]).arg(dists[2])
				.arg(dists[0])
				.arg(angle)
				);
	}
}



void RS_ActionInfoDist::mouseMoveEvent(QMouseEvent* e) {
    RS_DEBUG->print("RS_ActionInfoDist::mouseMoveEvent begin");

    if (getStatus()==SetPoint1 ||
            getStatus()==SetPoint2) {

        RS_Vector mouse = snapPoint(e);
        switch (getStatus()) {
        case SetPoint1:
            break;

        case SetPoint2:
			if (pPoints->point1.valid) {
				pPoints->point2 = mouse;

                deletePreview();

				preview->addEntity(new RS_Line{preview.get(), pPoints->point1, pPoints->point2});

                drawPreview();
            }
            break;

        default:
            break;
        }
    }

    RS_DEBUG->print("RS_ActionInfoDist::mouseMoveEvent end");
}



void RS_ActionInfoDist::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button()==Qt::LeftButton) {
        RS_CoordinateEvent ce(snapPoint(e));
        coordinateEvent(&ce);
    } else if (e->button()==Qt::RightButton) {
        deletePreview();
        init(getStatus()-1);
    }
}



void RS_ActionInfoDist::coordinateEvent(RS_CoordinateEvent* e) {
    if (e==NULL) {
        return;
    }

    RS_Vector mouse = e->getCoordinate();

    switch (getStatus()) {
    case SetPoint1:
		pPoints->point1 = mouse;
		graphicView->moveRelativeZero(pPoints->point1);
        setStatus(SetPoint2);
        break;

    case SetPoint2:
		if (pPoints->point1.valid) {
			pPoints->point2 = mouse;
            deletePreview();
				graphicView->moveRelativeZero(pPoints->point2);
            trigger();
            setStatus(SetPoint1);
        }
        break;

    default:
        break;
    }
}


void RS_ActionInfoDist::updateMouseButtonHints() {
    switch (getStatus()) {
    case SetPoint1:
        RS_DIALOGFACTORY->updateMouseWidget(
            tr("Specify first point of distance"),
            tr("Cancel"));
        break;
    case SetPoint2:
        RS_DIALOGFACTORY->updateMouseWidget(
            tr("Specify second point of distance"),
            tr("Back"));
        break;
    default:
        RS_DIALOGFACTORY->updateMouseWidget();
        break;
    }
}



void RS_ActionInfoDist::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::CadCursor);
}

// EOF
