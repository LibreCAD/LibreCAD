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
#include "rs_actionprintpreview.h"

#include <QAction>
#include <QMouseEvent>
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_graphic.h"
#include "rs_commandevent.h"
#include "rs_coordinateevent.h"
#include "rs_math.h"
#include "rs_preview.h"
#include "rs_settings.h"

struct RS_ActionPrintPreview::Points {
	RS_Vector v1;
	RS_Vector v2;
};

/**
 * Constructor.
 */
RS_ActionPrintPreview::RS_ActionPrintPreview(RS_EntityContainer& container,
                                             RS_GraphicView& graphicView)
    :RS_ActionInterface("Print Preview",
						container, graphicView)
	, hasOptions(false)
	, scaleFixed(false)
	, m_bPaperOffset(false)
	, pPoints(new Points{})
{
    showOptions();
	actionType=RS2::ActionFilePrintPreview;
    RS_SETTINGS->beginGroup("/PrintPreview");
    bool fixed = (RS_SETTINGS->readNumEntry("/PrintScaleFixed", 0) != 0);
    RS_SETTINGS->endGroup();
    setPaperScaleFixed(fixed);
}

RS_ActionPrintPreview::~RS_ActionPrintPreview()=default;

void RS_ActionPrintPreview::init(int status) {
    RS_ActionInterface::init(status);
    showOptions();
}

void RS_ActionPrintPreview::mouseMoveEvent(QMouseEvent* e) {
    switch (getStatus()) {
    case Moving:
		pPoints->v2 = graphicView->toGraph(e->x(), e->y());
        if (graphic) {
            RS_Vector pinsbase = graphic->getPaperInsertionBase();

            double scale = graphic->getPaperScale();

			graphic->setPaperInsertionBase(pinsbase-pPoints->v2*scale+pPoints->v1*scale);
        }
		pPoints->v1 = pPoints->v2;
        graphicView->redraw(RS2::RedrawGrid); // DRAW Grid also draws paper, background items
        break;

    default:
        break;
    }
}



void RS_ActionPrintPreview::mousePressEvent(QMouseEvent* e) {
    if (e->button()==Qt::LeftButton) {
        switch (getStatus()) {
        case Neutral:
			pPoints->v1 = graphicView->toGraph(e->x(), e->y());
            setStatus(Moving);
            break;

        default:
            break;
        }
    }
}


void RS_ActionPrintPreview::mouseReleaseEvent(QMouseEvent* e) {
    switch (getStatus()) {
    case Moving:
        setStatus(Neutral);
        break;

    default:
        e->accept();
        break;
    }
}



void RS_ActionPrintPreview::coordinateEvent(RS_CoordinateEvent* e) {
    RS_Vector pinsbase = graphic->getPaperInsertionBase();
    RS_Vector mouse = e->getCoordinate();
//    qDebug()<<"coordinateEvent= ("<<mouse.x<<", "<<mouse.y<<")";

    if(m_bPaperOffset) {
        RS_DIALOGFACTORY->commandMessage(tr("Printout offset in paper coordinates by (%1, %2)").arg(mouse.x).arg(mouse.y));
        mouse *= graphic->getPaperScale();
    }else
        RS_DIALOGFACTORY->commandMessage(tr("Printout offset in graph coordinates by (%1, %2)").arg(mouse.x).arg(mouse.y));

//    RS_DIALOGFACTORY->commandMessage(tr("old insertion base (%1, %2)").arg(pinsbase.x).arg(pinsbase.y));
//    RS_DIALOGFACTORY->commandMessage(tr("new insertion base (%1, %2)").arg((pinsbase-mouse).x).arg((pinsbase-mouse).y));

    graphic->setPaperInsertionBase(pinsbase-mouse);
    graphicView->redraw(RS2::RedrawGrid); // DRAW Grid also draws paper, background items

}



void RS_ActionPrintPreview::commandEvent(RS_CommandEvent*  e) {
    QString c = e->getCommand().trimmed().toLower();
//    qDebug()<<"cmd="<<c;
	if (checkCommand("blackwhite", c)) {
		setBlackWhite(true);
		RS_DIALOGFACTORY->commandMessage(tr("Printout in Black/White"));
		e->accept();
		return;
	} else if (checkCommand("color", c)) {
		setBlackWhite(false);
		RS_DIALOGFACTORY->commandMessage(tr("Printout in color"));
		e->accept();
		return;
	} else if (checkCommand("graphoffset", c)) {
        m_bPaperOffset=false;
        RS_DIALOGFACTORY->commandMessage(tr("Printout offset in graph coordinates"));
        e->accept();
        return;
    } else if (checkCommand("paperoffset", c)) {
        m_bPaperOffset=true;
        RS_DIALOGFACTORY->commandMessage(tr("Printout offset in paper coordinates"));
        e->accept();
        return;
    }else if (checkCommand("help", c)) {
        RS_DIALOGFACTORY->commandMessage(msgAvailableCommands()
                                         + getAvailableCommands().join(", ")+tr(": select printout offset coordinates")+
                                         "\n"+tr("type in offset from command line to offset printout")
                                         );
        e->accept();
        return;
    }
    //coordinate event
    if (c.contains(',')){
        if(c.startsWith('@')) {
            RS_DIALOGFACTORY->commandMessage(tr("Printout offset ignores relative zero. Ignoring '@'"));
            c.remove(0, 1);
        }
//        qDebug()<<"offset by absolute coordinate: ";

        const int commaPos = c.indexOf(',');
        bool ok1, ok2;
        double x = RS_Math::eval(c.left(commaPos), &ok1);
        double y = RS_Math::eval(c.mid(commaPos+1), &ok2);
        if (ok1 && ok2) {
            RS_CoordinateEvent ce(RS_Vector(x,y));
            this->coordinateEvent(&ce);
            e->accept();
        }
    }
}



QStringList RS_ActionPrintPreview::getAvailableCommands() {
    QStringList cmd;
	cmd +=command("blackwhite");
	cmd +=command("color");
	cmd +=command("graphoffset");
    cmd +=command("paperoffset");
    cmd +=command("help");
    return cmd;
}

void RS_ActionPrintPreview::resume() {
    RS_ActionInterface::resume();
    showOptions();
}

//printout warning in command widget
void RS_ActionPrintPreview::printWarning(const QString& s) {
	RS_DIALOGFACTORY->commandMessage(s);
}

void RS_ActionPrintPreview::showOptions() {
    RS_ActionInterface::showOptions();
	if (!isFinished()) {
        RS_DIALOGFACTORY->requestOptions(this, true,hasOptions);
        hasOptions=true;
    }
}



void RS_ActionPrintPreview::hideOptions() {
    RS_ActionInterface::hideOptions();

    RS_DIALOGFACTORY->requestOptions(this, false);
}

void RS_ActionPrintPreview::updateMouseCursor() {
    switch (getStatus()){
    case Moving:
        graphicView->setMouseCursor(RS2::ClosedHandCursor);
        break;
    default:
        graphicView->setMouseCursor(RS2::OpenHandCursor);
    }
}

void RS_ActionPrintPreview::center() {
    if (graphic) {
        graphic->centerToPage();
        graphicView->zoomPage();
        graphicView->redraw();
    }
}


void RS_ActionPrintPreview::fit() {
    if (graphic) {
        RS_Vector&& paperSize=RS_Units::convert(graphic->getPaperSize(),
                                                RS2::Millimeter, getUnit());

        if(fabs(paperSize.x)<10.|| fabs(paperSize.y)<10.)
            printWarning("Warning:: Paper size less than 10mm."
                         " Paper is too small for fitting to page\n"
                         "Please set paper size by Menu: Edit->Current Drawing Preferences->Paper");
        //        double f0=graphic->getPaperScale();
		if ( graphic->fitToPage()==false) {
            RS_DIALOGFACTORY->commandMessage(
                        tr("RS_ActionPrintPreview::fit(): Invalid paper size")
                        );
        }
        //        if(fabs(f0-graphic->getPaperScale())>RS_TOLERANCE){
        //only zoomPage when scale changed
        //        }
        graphic->centerToPage();
        graphicView->zoomPage();
        graphicView->redraw();
    }
}

bool RS_ActionPrintPreview::setScale(double f, bool autoZoom) {
    if (graphic) {
        if( fabs(f - graphic->getPaperScale()) < RS_TOLERANCE ) return false;
        graphic->setPaperScale(f);
//        graphic->centerToPage();
        if(autoZoom) graphicView->zoomPage();
        graphicView->redraw();
        return true;
    }
    return false;
}



double RS_ActionPrintPreview::getScale() const{
    double ret = 1.0;
    if (graphic) {
        ret = graphic->getPaperScale();
    }
    return ret;
}



void RS_ActionPrintPreview::setBlackWhite(bool bw) {
    if (bw) {
        graphicView->setDrawingMode(RS2::ModeBW);
    }
    else {
        graphicView->setDrawingMode(RS2::ModeFull);
    }
    graphicView->redraw();
}


RS2::Unit RS_ActionPrintPreview::getUnit() {
    if (graphic) {
        return graphic->getUnit();
    }
    else {
        return RS2::None;
    }
}

/** set paperscale fixed */
void RS_ActionPrintPreview::setPaperScaleFixed(bool fixed)
{
    graphic->setPaperScaleFixed(fixed);
}


/** get paperscale fixed */
bool RS_ActionPrintPreview::getPaperScaleFixed()
{
    return graphic->getPaperScaleFixed();
}

/** calculate number of pages needed to contain a drawing */
void RS_ActionPrintPreview::calcPagesNum() {
    if (graphic) {
        RS_Vector printArea = graphic->getPrintAreaSize(false);
        RS_Vector graphicSize = graphic->getSize() * graphic->getPaperScale();
        int pX = ceil(graphicSize.x / printArea.x);
        int pY = ceil(graphicSize.y / printArea.y);

        if ( pX > 99 || pY > 99) {
            RS_DIALOGFACTORY->commandMessage(tr("RS_ActionPrintPreview::calcPagesNum(): "
                                                "Limit of pages has been exceeded."));
            return;
        }

        graphic->setPagesNum(pX, pY);
        graphic->centerToPage();
        graphicView->zoomPage();
        graphicView->redraw();
    }
}

// EOF
