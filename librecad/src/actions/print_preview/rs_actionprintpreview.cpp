/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2024 LibreCAD.org
 Copyright (C) 2024 sand1024

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 ******************************************************************************/

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
    :RS_ActionInterface("Print Preview",container, graphicView, RS2::ActionFilePrintPreview)
    , pPoints(std::make_unique<Points>()){

    bool fixed = LC_GET_ONE_BOOL("PrintPreview", "PrintScaleFixed");

    if (!fixed) {
        fit();
    }
    setPaperScaleFixed(fixed);
}

RS_ActionPrintPreview::~RS_ActionPrintPreview()=default;

void invokeSettingsDialog();

void zoomPageExWithBorder(int borderSize);

void RS_ActionPrintPreview::init(int status) {
    RS_ActionInterface::init(status);
}

void RS_ActionPrintPreview::invokeSettingsDialog(){
    if (graphic) {
        RS_DIALOGFACTORY->requestOptionsDrawingDialog(*graphic);
        updateCoordinateWidgetFormat();
        updateOptionsUI(QG_PrintPreviewOptions::MODE_UPDATE_ORIENTATION);
        zoomToPage();
    }
}

bool RS_ActionPrintPreview::isPortrait(){
    bool landscape;
    graphic->getPaperFormat(&landscape);
    return !landscape;
}

void RS_ActionPrintPreview::setPaperOrientation(bool portrait) {
    bool landscape;
    RS2::PaperFormat format = graphic->getPaperFormat(&landscape);
    if (landscape != !portrait) {
        graphic->setPaperFormat(format, !portrait);
        zoomToPage();
    }
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
                
#ifdef DEBUG_PAPER_INSERTION_BASE
                const RS_Vector &pib = graphic->getPaperInsertionBase();
                LC_ERR << "PIB:" <<  pib.x << " , " << pib.y;
#endif
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

void RS_ActionPrintPreview::onCoordinateEvent( [[maybe_unused]]int status,  [[maybe_unused]]bool isZero, const RS_Vector &pos) {
    RS_Vector pinsbase = graphic->getPaperInsertionBase();
    RS_Vector mouse = pos;
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

bool RS_ActionPrintPreview::doProcessCommand( [[maybe_unused]]int status, const QString &c) {
    bool accept = true;
    //    qDebug()<<"cmd="<<c;
    if (checkCommand("blackwhite", c)) {
        setBlackWhite(true);
        commandMessage(tr("Printout in Black/White"));
        updateOptions();
    } else if (checkCommand("color", c)) {
        setBlackWhite(false);
        commandMessage(tr("Printout in color"));
        updateOptions();
    } else if (checkCommand("graphoffset", c)) {
        m_bPaperOffset=false;
        commandMessage(tr("Printout offset in graph coordinates"));
        updateOptions();
    } else if (checkCommand("paperoffset", c)) {
        m_bPaperOffset=true;
        commandMessage(tr("Printout offset in paper coordinates"));
        updateOptions();
    }
    else{
        //coordinate event
        if (c.contains(',')){
            QString coord = c;
            if(c.startsWith('@')) {
                commandMessage(tr("Printout offset ignores relative zero. Ignoring '@'"));
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
    }
}

void RS_ActionPrintPreview::zoomToPage(){
    if (graphic) {
        graphicView->zoomPageEx();
    }
}

void RS_ActionPrintPreview::fit() {
    if (graphic) {
        RS_Vector paperSize=RS_Units::convert(graphic->getPaperSize(),
                                              RS2::Millimeter, getUnit());

        if(std::abs(paperSize.x)<10.|| std::abs(paperSize.y)<10.)
            printWarning("Warning:: Paper size less than 10mm."
                         " Paper is too small for fitting to page\n"
                         "Please set paper size by Menu: Options->Current Drawing Preferences->Paper");
        //        double f0=graphic->getPaperScale();
        if ( graphic->fitToPage()==false) {
            commandMessage(tr("RS_ActionPrintPreview::fit(): Invalid paper size"));
        }
        else{
            graphic->setPagesNum(1,1);
            updateOptionsUI(QG_PrintPreviewOptions::MODE_UPDATE_PAGE_NUMBERS);
        }
        graphic->centerToPage();
        graphicView->zoomPage();
        graphicView->redraw();
    }
}

bool RS_ActionPrintPreview::setScale(double f, bool autoZoom) {
    if (graphic) {
        if(std::abs(f - graphic->getPaperScale()) < RS_TOLERANCE )
            return false;
//        auto pinBase = graphic->getPaperInsertionBase();
        // double oldScale = graphic->getPaperScale();

        graphic->setPaperScale(f);

        // changing scale around the drawing center
//        pinBase += graphic->getSize()*(oldScale - f)*0.5;
//        graphic->setPaperInsertionBase(pinBase);

        // pinBase *= f;

        if(autoZoom) {
            zoomPageExWithBorder(100);
        }
        graphicView->redraw();
        return true;
    }
    return false;
}

void RS_ActionPrintPreview::zoomPageExWithBorder(int borderSize) {
    int bBottom = this->graphicView->getBorderBottom();
    int bTop = this->graphicView->getBorderTop();
    int bLeft = this->graphicView->getBorderLeft();
    int bRight = this->graphicView->getBorderRight();
    // just a small usability improvement - we set additional borders on zoom to let the user
// see that there might be drawing elements around paper
    this->graphicView->setBorders(borderSize, borderSize, borderSize, borderSize);
    this->graphicView->zoomPageEx();
    this->graphicView->setBorders(bLeft, bTop, bRight, bBottom);
}

double RS_ActionPrintPreview::getScale() const{
    double ret = 1.0;
    if (graphic) {
        ret = graphic->getPaperScale();
    }
    return ret;
}

bool RS_ActionPrintPreview::isLineWidthScaling(){
    return graphicView->getLineWidthScaling();
}

void RS_ActionPrintPreview::setLineWidthScaling(bool state) {
    graphicView->setLineWidthScaling(state);
    graphicView->redraw();
}

bool RS_ActionPrintPreview::isBlackWhite() {
    return graphicView->getDrawingMode() == RS2::ModeBW;
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
bool RS_ActionPrintPreview::isPaperScaleFixed(){
    return graphic->getPaperScaleFixed();
}

/** calculate number of pages needed to contain a drawing */
void RS_ActionPrintPreview::calcPagesNum(bool multiplePages) {
    if (graphic) {
        if (multiplePages) {
            RS_Vector printArea = graphic->getPrintAreaSize(false);
            RS_Vector graphicSize = graphic->getSize() * graphic->getPaperScale();
            int pX = ceil(graphicSize.x / printArea.x);
            int pY = ceil(graphicSize.y / printArea.y);

            if (pX > 99 || pY > 99) { // fixme - why such limit? Why hardcoded?
                commandMessage(tr("Limit of pages has been exceeded."));
                return;
            }

            graphic->setPagesNum(pX, pY);
            graphic->centerToPage();
            graphicView->zoomPage();
        }
        else {
            graphic->setPagesNum(1, 1);
        }
        updateOptionsUI(QG_PrintPreviewOptions::MODE_UPDATE_PAGE_NUMBERS);
        updateOptions();
    }
}
// fixme - sand -  review and check why subtle rounding issues occur on some pages values
void RS_ActionPrintPreview::setPagesNumHorizontal(int pagesCount) {
    RS_Vector printArea = graphic->getPrintAreaSize(false);
    RS_Vector graphicSize = graphic->getSize();
    double paperScale = pagesCount * printArea.x / (graphicSize.x + 5);
    int vertPagesCount = ceil(graphicSize.y * paperScale / printArea.y);
    graphic->setPagesNum(pagesCount, vertPagesCount);
    graphic->setPaperScale(paperScale);

//    zoomPageExWithBorder(100);
    graphic->centerToPage();
    graphicView->zoomPage();
    updateOptionsUI(QG_PrintPreviewOptions::MODE_UPDATE_PAGE_NUMBERS);
    updateOptions();
}

void RS_ActionPrintPreview::setPagesNumVertical(int pagesCount) {
    RS_Vector printArea = graphic->getPrintAreaSize(false);
    RS_Vector graphicSize = graphic->getSize();
    double paperScale = pagesCount * printArea.y / (graphicSize.y + 5);

    int horPagesCount = ceil(graphicSize.x * paperScale / printArea.x);

    double paperScaleHor = horPagesCount * printArea.x / graphicSize.x;

    paperScale = std::min(paperScaleHor, paperScale);

    graphic->setPagesNum(horPagesCount, pagesCount);
    graphic->setPaperScale(paperScale);

//    zoomPageExWithBorder(100);
    graphic->centerToPage();
    graphicView->zoomPage();
    updateOptionsUI(QG_PrintPreviewOptions::MODE_UPDATE_PAGE_NUMBERS);
    updateOptions();
}

int RS_ActionPrintPreview::getPagesNumHorizontal() {
    return graphic->getPagesNumHoriz();
}

int RS_ActionPrintPreview::getPagesNumVertical() {
    return graphic->getPagesNumVert();
}
    void RS_ActionPrintPreview::updateMouseButtonHints() {
        updateMouseWidget(tr("Drag with Left Button to Position Paper or with Middle Button to Pan" ), "", MOD_SHIFT_AND_CTRL(tr("Move Horizontally"), tr("Move Vertically")));
    }

LC_ActionOptionsWidget* RS_ActionPrintPreview::createOptionsWidget() {
    return new QG_PrintPreviewOptions();
}
