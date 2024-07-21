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

#include <QMouseEvent>

#include "qg_printpreviewoptions.h"
#include "rs_actionprintpreview.h"
#include "rs_commandevent.h"
#include "rs_coordinateevent.h"
#include "rs_dialogfactory.h"
#include "rs_graphic.h"
#include "rs_graphicview.h"
#include "rs_math.h"
#include "rs_settings.h"
#include "rs_units.h"

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
                        container, graphicView, RS2::ActionFilePrintPreview)
    , pPoints(std::make_unique<Points>())
{
    RS_SETTINGS->beginGroup("/PrintPreview");
    bool fixed = (RS_SETTINGS->readNumEntry("/PrintScaleFixed", 0) != 0);
    RS_SETTINGS->endGroup();
    if (!fixed)
        fit();
    setPaperScaleFixed(fixed);
}

RS_ActionPrintPreview::~RS_ActionPrintPreview()=default;

void RS_ActionPrintPreview::init(int status) {
    RS_ActionInterface::init(status);
}

void RS_ActionPrintPreview::mouseMoveEvent(QMouseEvent* e) {
    switch (getStatus()) {
        case Moving: {
            pPoints->v2 = toGraph(e);
            // if Shift is pressed the paper moves only horizontally
            if (isShift(e)) {
                pPoints->v2.y = pPoints->v1.y;
            }
            // if Ctrl is pressed the paper moves only vertically
            if (isControl(e)) {
                pPoints->v2.x = pPoints->v1.x;
            }
            if (graphic) {
                RS_Vector pinsbase = graphic->getPaperInsertionBase();
                double scale = graphic->getPaperScale();
                graphic->setPaperInsertionBase(pinsbase - pPoints->v2 * scale + pPoints->v1 * scale);
            }
            pPoints->v1 = pPoints->v2;
            graphicView->redraw(RS2::RedrawGrid); // DRAW Grid also draws paper, background items
            break;
        }
        default:
            break;
    }
}

void RS_ActionPrintPreview::mousePressEvent(QMouseEvent* e) {
    if (e->button()==Qt::LeftButton) {
        switch (getStatus()) {
        case Neutral: {
            pPoints->v1 = toGraph(e);
            setStatus(Moving);
            break;
        }
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
        commandMessage(tr("Printout offset in paper coordinates by (%1, %2)").arg(mouse.x).arg(mouse.y));
        mouse *= graphic->getPaperScale();
    }else
        commandMessage(tr("Printout offset in graph coordinates by (%1, %2)").arg(mouse.x).arg(mouse.y));

    //    RS_DIALOGFACTORY->commandMessage(tr("old insertion base (%1, %2)").arg(pinsbase.x).arg(pinsbase.y));
    //    RS_DIALOGFACTORY->commandMessage(tr("new insertion base (%1, %2)").arg((pinsbase-mouse).x).arg((pinsbase-mouse).y));

    graphic->setPaperInsertionBase(pinsbase-mouse);
    graphicView->redraw(RS2::RedrawGrid); // DRAW Grid also draws paper, background items
}

bool RS_ActionPrintPreview::doProcessCommand(int status, const QString &c) {
    bool accept = true;
    //    qDebug()<<"cmd="<<c;
    if (checkCommand("blackwhite", c)) {
        setBlackWhite(true);
        commandMessageTR("Printout in Black/White");
    } else if (checkCommand("color", c)) {
        setBlackWhite(false);
        commandMessageTR("Printout in color");
    } else if (checkCommand("graphoffset", c)) {
        m_bPaperOffset=false;
        commandMessageTR("Printout offset in graph coordinates");
    } else if (checkCommand("paperoffset", c)) {
        m_bPaperOffset=true;
        commandMessageTR("Printout offset in paper coordinates");
    }
    else{
        //coordinate event
        if (c.contains(',')){
            QString coord = c;
            if(c.startsWith('@')) {
                commandMessageTR("Printout offset ignores relative zero. Ignoring '@'");
                coord.remove(0, 1);
            }
            //        qDebug()<<"offset by absolute coordinate: ";

            const int commaPos = coord.indexOf(',');
            bool ok1, ok2;
            double x = RS_Math::eval(coord.left(commaPos), &ok1);
            double y = RS_Math::eval(coord.mid(commaPos+1), &ok2);
            if (ok1 && ok2) {
                RS_CoordinateEvent ce(RS_Vector(x,y));
                coordinateEvent(&ce);
            }
            else{
                accept = false;
            }
        }
        else{
            accept = false;
        }
    }
    return accept;
}

QString RS_ActionPrintPreview::getAdditionalHelpMessage() {
    return tr(": select printout offset coordinates")+
           "\n"+tr("type in offset from command line to offset printout");
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
}

//printout warning in command widget
void RS_ActionPrintPreview::printWarning(const QString& s) {
    commandMessage(s);
}

RS2::CursorType RS_ActionPrintPreview::doGetMouseCursor([[maybe_unused]] int status){
    switch (status) {
        case Moving:
            return RS2::ClosedHandCursor;
        default:
            return RS2::OpenHandCursor;
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
        RS_Vector paperSize=RS_Units::convert(graphic->getPaperSize(),
                                              RS2::Millimeter, getUnit());

        if(std::abs(paperSize.x)<10.|| std::abs(paperSize.y)<10.)
            printWarning("Warning:: Paper size less than 10mm."
                         " Paper is too small for fitting to page\n"
                         "Please set paper size by Menu: Edit->Current Drawing Preferences->Paper");
        //        double f0=graphic->getPaperScale();
        if ( graphic->fitToPage()==false) {
            commandMessageTR("RS_ActionPrintPreview::fit(): Invalid paper size");
        }
        //        if(std::abs(f0-graphic->getPaperScale())>RS_TOLERANCE){
        //only zoomPage when scale changed
        //        }
        graphic->centerToPage();
        graphicView->zoomPage();
        graphicView->redraw();
    }
}

bool RS_ActionPrintPreview::setScale(double f, bool autoZoom) {
    if (graphic) {
        if(std::abs(f - graphic->getPaperScale()) < RS_TOLERANCE )
            return false;
        auto pinBase = graphic->getPaperInsertionBase();
        double oldScale = graphic->getPaperScale();

        graphic->setPaperScale(f);

        // changing scale around the drawing center
        pinBase += graphic->getSize()*(oldScale - f)*0.5;
        graphic->setPaperInsertionBase(pinBase);

        if(autoZoom)
            graphicView->zoomPage();
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

void RS_ActionPrintPreview::setLineWidthScaling(bool state) {
    graphicView->setLineWidthScaling(state);
    graphicView->redraw();
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
void RS_ActionPrintPreview::setPaperScaleFixed(bool fixed){
    graphic->setPaperScaleFixed(fixed);
}

/** get paperscale fixed */
bool RS_ActionPrintPreview::getPaperScaleFixed(){
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
            commandMessageTR("RS_ActionPrintPreview::calcPagesNum(): Limit of pages has been exceeded.");
            return;
        }

        graphic->setPagesNum(pX, pY);
        graphic->centerToPage();
        graphicView->zoomPage();
        graphicView->redraw();
    }
}

void RS_ActionPrintPreview::updateMouseButtonHints() {
    updateMouseWidgetTR("Position Paper", "", LC_ModifiersInfo::SHIFT_AND_CTRL("Move Horizontally", "Move Vertically"));
}

LC_ActionOptionsWidget* RS_ActionPrintPreview::createOptionsWidget() {
    return new QG_PrintPreviewOptions();
}

void RS_ActionPrintPreview::showOptions() {
    RS_ActionInterface::showOptions();
}

void RS_ActionPrintPreview::hideOptions(bool includeSnap) {
    RS_ActionInterface::hideOptions(includeSnap);
}
