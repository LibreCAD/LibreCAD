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
#include <QImage>
#include <QMouseEvent>
#include "rs_image.h"
#include "rs_line.h"
#include "rs_units.h"

#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_commandevent.h"
#include "rs_creation.h"
#include "rs_actiondrawimage.h"
#include "rs_coordinateevent.h"
#include "rs_math.h"
#include "rs_preview.h"

struct RS_ActionDrawImage::ImageData {
	RS_ImageData data;
	QImage img;
};

/**
 * Constructor.
 */
RS_ActionDrawImage::RS_ActionDrawImage(RS_EntityContainer& container,
                                       RS_GraphicView& graphicView)
    :RS_PreviewActionInterface("Image",
							   container, graphicView)
	, pImg(new ImageData{})
	, lastStatus(ShowDialog)
{
	actionType=RS2::ActionDrawImage;
}

RS_ActionDrawImage::~RS_ActionDrawImage() = default;


void RS_ActionDrawImage::init(int status) {
    RS_PreviewActionInterface::init(status);

    reset();

	pImg->data.file = RS_DIALOGFACTORY->requestImageOpenDialog();
    // RVT_PORT should we really redarw here?? graphicView->redraw();

	if (!pImg->data.file.isEmpty()) {
		//std::cout << "file: " << pImg->data.file << "\n";
		//qDebug() << "file: " << pImg->data.file;

		pImg->img = QImage(pImg->data.file);

        setStatus(SetTargetPoint);
    } else {
        setFinished();
        //RS_DIALOGFACTORY->requestToolBar(RS2::ToolBarMain);
    }
}



void RS_ActionDrawImage::reset() {
	pImg->data = {
				   0,
				   {0.0,0.0},
				   {1.0,0.0},
				   {0.0,1.0},
				   {1.0,1.0},
				   "",
				   50, 50, 0
			   };
}



void RS_ActionDrawImage::trigger() {
    deletePreview();

	if (!pImg->data.file.isEmpty()) {
        RS_Creation creation(container, graphicView);
		creation.createImage(& pImg->data);
    }

    graphicView->redraw(RS2::RedrawDrawing);
    finish(false);
}


void RS_ActionDrawImage::mouseMoveEvent(QMouseEvent* e) {
	if (getStatus() == SetTargetPoint) {
		pImg->data.insertionPoint = snapPoint(e);

        deletePreview();
		//RS_Creation creation(preview, nullptr, false);
        //creation.createInsert(data);
		double const w=pImg->img.width();
		double const h=pImg->img.height();
		RS_Line* line = new RS_Line{preview.get(), {0., 0.}, {w, 0.}};
        preview->addEntity(line);
		line = new RS_Line{preview.get(), {w, 0.}, {w, h}};
        preview->addEntity(line);
		line = new RS_Line{preview.get(), {w, h}, {0., h}};
        preview->addEntity(line);
		line = new RS_Line{preview.get(), {0., h}, {0., 0.}};
        preview->addEntity(line);
		preview->scale({0., 0.},
			{pImg->data.uVector.magnitude(), pImg->data.uVector.magnitude()});
		preview->rotate({0.,0.}, pImg->data.uVector.angle());
		preview->move(pImg->data.insertionPoint);
		drawPreview();
    }
}



void RS_ActionDrawImage::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button()==Qt::LeftButton) {
        RS_CoordinateEvent ce(snapPoint(e));
        coordinateEvent(&ce);
    } else if (e->button()==Qt::RightButton) {
        //init(getStatus()-1);
        finish(false);
    }
}



void RS_ActionDrawImage::coordinateEvent(RS_CoordinateEvent* e) {
	if (e==nullptr) {
        return;
    }

	pImg->data.insertionPoint = e->getCoordinate();
    trigger();
}



void RS_ActionDrawImage::commandEvent(RS_CommandEvent* e) {
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
        } else if (checkCommand("dpi",c)) {
            deletePreview();
            lastStatus =(Status)getStatus();
            setStatus(SetDPI);
        }
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

    case SetDPI : {
        bool ok;
        double dpi = RS_Math::eval(c, &ok);

		if(ok) {
            setFactor(RS_Units::dpiToScale(dpi, document->getGraphicUnit()));
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


double RS_ActionDrawImage::getAngle() const {
	return pImg->data.uVector.angle();
}

void RS_ActionDrawImage::setAngle(double a) const{
	double l = pImg->data.uVector.magnitude();
	pImg->data.uVector.setPolar(l, a);
	pImg->data.vVector.setPolar(l, a+M_PI_2);
}

double RS_ActionDrawImage::getFactor() const {
	return pImg->data.uVector.magnitude();
}

void RS_ActionDrawImage::setFactor(double f) const {
	double a = pImg->data.uVector.angle();
	pImg->data.uVector.setPolar(f, a);
	pImg->data.vVector.setPolar(f, a+M_PI_2);
}

double RS_ActionDrawImage::dpiToScale(double dpi) const {
	return RS_Units::dpiToScale(dpi, document->getGraphicUnit());
}



double RS_ActionDrawImage::scaleToDpi(double scale) const {
	return RS_Units::scaleToDpi(scale, document->getGraphicUnit());
}



void RS_ActionDrawImage::updateMouseCursor() {
	graphicView->setMouseCursor(RS2::CadCursor);
}

QStringList RS_ActionDrawImage::getAvailableCommands() {
    QStringList cmd;

    switch (getStatus()) {
    case SetTargetPoint:
        cmd += command("angle");
        cmd += command("factor");
        cmd += command("dpi");
        break;
    default:
        break;
    }

    return cmd;
}



void RS_ActionDrawImage::showOptions() {
    RS_ActionInterface::showOptions();

	RS_DIALOGFACTORY->requestOptions(this, true);
}



void RS_ActionDrawImage::hideOptions() {
    RS_ActionInterface::hideOptions();
	RS_DIALOGFACTORY->requestOptions(this, false);
}


void RS_ActionDrawImage::updateMouseButtonHints() {
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
    case SetDPI:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Enter dpi:"),
                                            "");
        break;
    default:
		RS_DIALOGFACTORY->updateMouseWidget();
        break;
    }
}




// EOF
