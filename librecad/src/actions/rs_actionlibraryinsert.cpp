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

#include "rs_actionlibraryinsert.h"

#include <QAction>
#include <QMouseEvent>
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_commandevent.h"
#include "rs_coordinateevent.h"
#include "rs_math.h"
#include "rs_preview.h"

struct RS_ActionLibraryInsert::Points {
	RS_Graphic prev;
	RS_LibraryInsertData data;
};

/**
 * Constructor.
 */
RS_ActionLibraryInsert::RS_ActionLibraryInsert(RS_EntityContainer& container,
        RS_GraphicView& graphicView)
        :RS_PreviewActionInterface("Library Insert",
						   container, graphicView)
		, pPoints(new Points{})
		,lastStatus(SetTargetPoint)
{
	actionType=RS2::ActionLibraryInsert;
}

RS_ActionLibraryInsert::~RS_ActionLibraryInsert() = default;

QAction* RS_ActionLibraryInsert::createGUIAction(RS2::ActionType /*type*/, QObject* /*parent*/) {
   QAction* action = new QAction(tr("Insert Library Object"), NULL);
    return action;
}


void RS_ActionLibraryInsert::init(int status) {
    RS_PreviewActionInterface::init(status);

    reset();

}



void RS_ActionLibraryInsert::setFile(const QString& file) {
	pPoints->data.file = file;

	if (!pPoints->prev.open(file, RS2::FormatUnknown)) {
        RS_DIALOGFACTORY->commandMessage(tr("Cannot open file '%1'").arg(file));
    }
}


void RS_ActionLibraryInsert::reset() {

	pPoints->data.insertionPoint = {};
	pPoints->data.factor = 1.0;
	pPoints->data.angle = 0.0;
}



void RS_ActionLibraryInsert::trigger() {
    deletePreview();

    RS_Creation creation(container, graphicView);
	creation.createLibraryInsert(pPoints->data);

	graphicView->redraw(RS2::RedrawDrawing); 

}


void RS_ActionLibraryInsert::mouseMoveEvent(QMouseEvent* e) {
    switch (getStatus()) {
    case SetTargetPoint:
		pPoints->data.insertionPoint = snapPoint(e);

        //if (block) {
        deletePreview();
		preview->addAllFrom(pPoints->prev);
		preview->move(pPoints->data.insertionPoint);
		preview->scale(pPoints->data.insertionPoint,
					   RS_Vector(pPoints->data.factor, pPoints->data.factor));
        // unit conversion:
        if (graphic) {
			double const uf = RS_Units::convert(1.0, pPoints->prev.getUnit(),
                                          graphic->getUnit());
			preview->scale(pPoints->data.insertionPoint,
			{uf, uf});
        }
		preview->rotate(pPoints->data.insertionPoint, pPoints->data.angle);
        // too slow:
        //RS_Creation creation(preview, NULL, false);
        //creation.createInsert(data);
        drawPreview();
        //}
        break;

    default:
        break;
    }
}



void RS_ActionLibraryInsert::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button()==Qt::LeftButton) {
        RS_CoordinateEvent ce(snapPoint(e));
        coordinateEvent(&ce);
    } else if (e->button()==Qt::RightButton) {
        init(getStatus()-1);
    }
}



void RS_ActionLibraryInsert::coordinateEvent(RS_CoordinateEvent* e) {
    if (e==NULL) {
        return;
    }

	pPoints->data.insertionPoint = e->getCoordinate();
    trigger();
}



void RS_ActionLibraryInsert::commandEvent(RS_CommandEvent* e) {
    QString c = e->getCommand().toLower();

    if (checkCommand("help", c)) {
        RS_DIALOGFACTORY->commandMessage(msgAvailableCommands()
                                         + getAvailableCommands().join(", "));
        return;
    }

    switch (getStatus()) {
    case SetTargetPoint:
        if (checkCommand("angle", c)) {
            deletePreview();
            lastStatus = (Status)getStatus();
            setStatus(SetAngle);
        } else if (checkCommand("factor", c)) {
            deletePreview();
            lastStatus = (Status)getStatus();
            setStatus(SetFactor);
        }
        break;

    case SetAngle: {
            bool ok;
            double a = RS_Math::eval(c, &ok);
            if (ok) {
				pPoints->data.angle = RS_Math::deg2rad(a);
            } else {
                RS_DIALOGFACTORY->commandMessage(tr("Not a valid expression"));
            }
            RS_DIALOGFACTORY->requestOptions(this, true, true);
            setStatus(lastStatus);
        }
        break;

    case SetFactor: {
            bool ok;
            double f = RS_Math::eval(c, &ok);
            if (ok) {
                setFactor(f);
            } else {
                RS_DIALOGFACTORY->commandMessage(tr("Not a valid expression"));
            }
            RS_DIALOGFACTORY->requestOptions(this, true, true);
            setStatus(lastStatus);
        }
        break;

    default:
        break;
    }
}



QStringList RS_ActionLibraryInsert::getAvailableCommands() {
    QStringList cmd;

    switch (getStatus()) {
    case SetTargetPoint:
        cmd += command("angle");
        cmd += command("factor");
        ;
        break;
    default:
        break;
    }

    return cmd;
}


void RS_ActionLibraryInsert::showOptions() {
    RS_ActionInterface::showOptions();

    RS_DIALOGFACTORY->requestOptions(this, true);
}



void RS_ActionLibraryInsert::hideOptions() {
    RS_ActionInterface::hideOptions();

    RS_DIALOGFACTORY->requestOptions(this, false);
}


void RS_ActionLibraryInsert::updateMouseButtonHints() {
    switch (getStatus()) {
    case SetTargetPoint:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Specify reference point"),
                                            tr("Cancel"));
        break;
    case SetAngle:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Enter angle:"),
                                            "");
        break;
    case SetFactor:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Enter factor:"),
                                            "");
        break;
    default:
		RS_DIALOGFACTORY->updateMouseWidget();
        break;
    }
}

double RS_ActionLibraryInsert::getAngle() const{
	return pPoints->data.angle;
}

void RS_ActionLibraryInsert::setAngle(double a) {
	pPoints->data.angle = a;
}

double RS_ActionLibraryInsert::getFactor() const{
	return pPoints->data.factor;
}

void RS_ActionLibraryInsert::setFactor(double f) {
	pPoints->data.factor = f;
}

void RS_ActionLibraryInsert::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::CadCursor);
}

// EOF
