/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010-2011 R. van Twisk (librecad@rvt.dds.nl)
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

#include "qg_graphicview.h"

#include <qbitmap.h>
#include <qcursor.h>
#include <qlabel.h>
//Added by qt3to4:
#include <QKeyEvent>
#include <Q3GridLayout>
#include <QPixmap>
#include <QResizeEvent>
#include <QTabletEvent>
#include <QFocusEvent>
#include <QMouseEvent>
#include <QEvent>
#include <QPaintEvent>
#include <QWheelEvent>

#include "rs_actionzoomin.h"
#include "rs_actionzoompan.h"
#include "rs_actionzoomscroll.h"
#include "rs_actionmodifydelete.h"
#include "rs_actionselectsingle.h"
#include "rs_fileinfo.h"
#include "rs_graphic.h"
#include "rs_settings.h"
#include "rs_system.h"
#include "rs_painterqt.h"
#include "rs_patternlist.h"

#include "qg_cadtoolbar.h"

#include "qg_dialogfactory.h"
#include "qg_qt2rs.h"


#define QG_SCROLLMARGIN 400


/**
 * Constructor.
 */
QG_GraphicView::QG_GraphicView(QWidget* parent, const char* name, Qt::WFlags f)
        : QWidget(parent, name, f), RS_GraphicView() {

    setBackground(background);
    //coordinateWidget = NULL;
    //mouseWidget = NULL;
    //optionWidget = NULL;
    //cadToolBar = NULL;
    //commandWidget = NULL;
			
	redrawMethod=RS2::RedrawAll;
			
	PixmapLayer1=PixmapLayer2=PixmapLayer3=NULL;

    layout = new Q3GridLayout(this, 3, 2);
    layout->setColStretch(0, 1);
    layout->setColStretch(1, 0);
    layout->setColStretch(2, 0);
    layout->setRowStretch(0, 1);
    layout->setRowStretch(1, 0);

    hScrollBar = new QG_ScrollBar(Qt::Horizontal, this);
    hScrollBar->setLineStep(50);
    layout->addMultiCellWidget(hScrollBar, 1, 1, 0, 0);
    layout->addRowSpacing(1, hScrollBar->sizeHint().height());
    connect(hScrollBar, SIGNAL(valueChanged(int)),
            this, SLOT(slotHScrolled(int)));

    vScrollBar = new QG_ScrollBar(Qt::Vertical, this);
    vScrollBar->setLineStep(50);
    layout->addMultiCellWidget(vScrollBar, 0, 0, 2, 2);
    layout->addColSpacing(2, vScrollBar->sizeHint().width());
    connect(vScrollBar, SIGNAL(valueChanged(int)),
            this, SLOT(slotVScrolled(int)));

#ifndef __APPLE__
    // Mouse Cursors:
    QPixmap cur1(":ui/cur_cad_bmp.png");
    curCad = new QCursor(cur1, 15, 15);

    QPixmap cur2(":ui/cur_glass_bmp.png");
    curMagnifier = new QCursor(cur2, 12, 12);

    QPixmap cur3(":ui/cur_del_bmp.png");
    curDel = new QCursor(cur3, 15, 15);

    QPixmap cur4(":ui/cur_select_bmp.png");
    curSelect = new QCursor(cur4, 15, 15);

    QPixmap cur5(":ui/cur_hand_bmp.png");
    curHand = new QCursor(cur5, 15, 15);
#else
    // No individual cursors for the Mac
    curCad = NULL;
    curMagnifier = NULL;
    curDel = NULL;
    curSelect = NULL;
    curHand = NULL;
#endif

    // Dummy widgets for scrollbar corners:
    //layout->addWidget(new QWidget(this), 1, 1);
    //QWidget* w = new QWidget(this);
    //w->setEraseColor(QColor(255,0,0));
    gridStatus = new QLabel("-", this);
    gridStatus->setAlignment(Qt::AlignRight);
    layout->addMultiCellWidget(gridStatus, 1, 1, 1, 2);
    layout->addColSpacing(1, 50);

	
    setMouseTracking(true);
	// flickering under win:
    //setFocusPolicy(WheelFocus);
	
    setFocusPolicy(Qt::NoFocus);

    // See https://sourceforge.net/tracker/?func=detail&aid=3289298&group_id=342582&atid=1433844 (Left-mouse drag shrinks window)
    setAttribute(Qt::WA_NoMousePropagation);
}



/**
 * Destructor
 */
QG_GraphicView::~QG_GraphicView() {
    cleanUp();
	delete PixmapLayer1;
	delete PixmapLayer2;
	delete PixmapLayer3;
}



/**
 * @return width of widget.
 */
int QG_GraphicView::getWidth() {
    return width() - vScrollBar->sizeHint().width();
}



/**
 * @return height of widget.
 */
int QG_GraphicView::getHeight() {
    return height() - hScrollBar->sizeHint().height();
}


/**
 * Changes the current background color of this view.
 */
void QG_GraphicView::setBackground(const RS_Color& bg) {
    RS_GraphicView::setBackground(bg);

    setBackgroundColor(bg);
}



/**
 * Sets the mouse cursor to the given type.
 */
void QG_GraphicView::setMouseCursor(RS2::CursorType c) {

    switch (c) {
    default:
    case RS2::ArrowCursor:
        setCursor(Qt::ArrowCursor);
        break;
    case RS2::UpArrowCursor:
        setCursor(Qt::UpArrowCursor);
        break;
    case RS2::CrossCursor:
        setCursor(Qt::CrossCursor);
        break;
    case RS2::WaitCursor:
        setCursor(Qt::WaitCursor);
        break;
    case RS2::IbeamCursor:
        setCursor(Qt::IBeamCursor);
        break;
    case RS2::SizeVerCursor:
        setCursor(Qt::SizeVerCursor);
        break;
    case RS2::SizeHorCursor:
        setCursor(Qt::SizeHorCursor);
        break;
    case RS2::SizeBDiagCursor:
        setCursor(Qt::SizeBDiagCursor);
        break;
    case RS2::SizeFDiagCursor:
        setCursor(Qt::SizeFDiagCursor);
        break;
    case RS2::SizeAllCursor:
        setCursor(Qt::SizeAllCursor);
        break;
    case RS2::BlankCursor:
        setCursor(Qt::BlankCursor);
        break;
    case RS2::SplitVCursor:
        setCursor(Qt::SplitVCursor);
        break;
    case RS2::SplitHCursor:
        setCursor(Qt::SplitHCursor);
        break;
    case RS2::PointingHandCursor:
        setCursor(Qt::PointingHandCursor);
        break;
    case RS2::ForbiddenCursor:
        setCursor(Qt::ForbiddenCursor);
        break;
    case RS2::WhatsThisCursor:
        setCursor(Qt::WhatsThisCursor);
        break;

#ifndef __APPLE__

    case RS2::CadCursor:
        setCursor(*curCad);
        break;
    case RS2::DelCursor:
        setCursor(*curDel);
        break;
    case RS2::SelectCursor:
        setCursor(*curSelect);
        break;
    case RS2::MagnifierCursor:
        setCursor(*curMagnifier);
        break;
    case RS2::MovingHandCursor:
        setCursor(*curHand);
        break;
#else
        // Reduced cursor selection for the Mac:
    case RS2::CadCursor:
        setCursor(Qt::CrossCursor);
        break;
    case RS2::DelCursor:
        setCursor(Qt::CrossCursor);
        break;
    case RS2::SelectCursor:
        setCursor(Qt::CrossCursor);
        break;
    case RS2::MagnifierCursor:
        setCursor(Qt::CrossCursor);
        break;
    case RS2::MovingHandCursor:
        setCursor(Qt::PointingHandCursor);
        break;
#endif
    }

}



/**
 * Sets the text for the grid status widget in the left bottom corner.
 */
void QG_GraphicView::updateGridStatusWidget(const RS_String& text) {
    gridStatus->setText(text);
}



/**
 * Redraws the widget.
 */
void QG_GraphicView::redraw(RS2::RedrawMethod method) { 
	redrawMethod=(RS2::RedrawMethod ) (redrawMethod | method);
	update(); // Paint when reeady to pain
//	repaint(); //Paint immediate
}



void QG_GraphicView::resizeEvent(QResizeEvent* /*e*/) {
    RS_DEBUG->print("QG_GraphicView::resizeEvent begin");
    adjustOffsetControls();
    adjustZoomControls();
//     updateGrid();
	// Small hack, delete teh snapper during resizes
	getOverlayContainer(RS2::Snapper)->clear();
	redraw();
    RS_DEBUG->print("QG_GraphicView::resizeEvent end");
}



void QG_GraphicView::emulateMouseMoveEvent() {
    RS_MouseEvent e(QEvent::MouseMove, QPoint(mx, my),
                    Qt::NoButton, Qt::NoButton);
    //mouseMoveEvent(&e);
}



void QG_GraphicView::mousePressEvent(QMouseEvent* e) {
    // pan zoom with middle mouse button
    if (e->button()==Qt::MidButton /*|| (e->state()==Qt::LeftButton|Qt::AltButton)*/) {
        setCurrentAction(new RS_ActionZoomPan(*container, *this));
    }

    RS_GraphicView::mousePressEvent(e);
    QWidget::mousePressEvent(e);
}


void QG_GraphicView::mouseReleaseEvent(QMouseEvent* e) {
	RS_DEBUG->print("QG_GraphicView::mouseReleaseEvent");
    RS_GraphicView::mouseReleaseEvent(e);
    //QWidget::mouseReleaseEvent(e);

    if (!e->isAccepted()) {
        if (QG_DIALOGFACTORY!=NULL && QG_DIALOGFACTORY->getCadToolBar()!=NULL) {
			RS_DEBUG->print("QG_GraphicView::mouseReleaseEvent: "
				"fwd to cadtoolbar");
            QG_DIALOGFACTORY->getCadToolBar()->mouseReleaseEvent(e);
        }
    }
	RS_DEBUG->print("QG_GraphicView::mouseReleaseEvent: OK");
}


void QG_GraphicView::mouseMoveEvent(QMouseEvent* e) {
    //RS_DEBUG->print("QG_GraphicView::mouseMoveEvent begin");
    //RS_MouseEvent rsm = QG_Qt2Rs::mouseEvent(e);

    RS_GraphicView::mouseMoveEvent(e);
    QWidget::mouseMoveEvent(e);

#ifdef Q_OS_WIN32
	// make sure that we can still use hotkeys and the mouse wheel
	if (parent()!=NULL) {
		((QWidget*)parent())->setFocus();
	}
#endif
	
    //RS_DEBUG->print("QG_GraphicView::mouseMoveEvent end");
}


/**
 * support for the wacom graphic tablet.
 */
void QG_GraphicView::tabletEvent(QTabletEvent* e) {
    if (hasMouse()) {
        switch (e->device()) {
        case QTabletEvent::Eraser:
            if (e->type()==QEvent::TabletRelease) {
                if (container!=NULL) {

                    RS_ActionSelectSingle* a =
                        new RS_ActionSelectSingle(*container, *this);
                    setCurrentAction(a);
                    QMouseEvent ev(QEvent::MouseButtonRelease, e->pos(),
                                   Qt::LeftButton, Qt::LeftButton);
                    mouseReleaseEvent(&ev);
                    a->finish();

                    if (container->countSelected()>0) {
                        setCurrentAction(
                            new RS_ActionModifyDelete(*container, *this));
                    }
                }
            }
            break;

        case QTabletEvent::Stylus:
        case QTabletEvent::Puck:
            if (e->type()==QEvent::TabletPress) {
                QMouseEvent ev(QEvent::MouseButtonPress, e->pos(),
                               Qt::LeftButton, Qt::LeftButton);
                mousePressEvent(&ev);
            } else if (e->type()==QEvent::TabletRelease) {
                QMouseEvent ev(QEvent::MouseButtonRelease, e->pos(),
                               Qt::LeftButton, Qt::LeftButton);
                mouseReleaseEvent(&ev);
            } else if (e->type()==QEvent::TabletMove) {
                QMouseEvent ev(QEvent::MouseMove, e->pos(),
                               Qt::NoButton, 0);
                mouseMoveEvent(&ev);
            }
            break;

        default:
            break;
        }
    }

    // a 'mouse' click:
    /*if (e->pressure()>10 && lastPressure<10) {
    	QMouseEvent e(QEvent::MouseButtonPress, e->pos(), 
    	   Qt::LeftButton, Qt::LeftButton);
    	mousePressEvent(&e);
}
    else if (e->pressure()<10 && lastPressure>10) {
    	QMouseEvent e(QEvent::MouseButtonRelease, e->pos(), 
    	   Qt::LeftButton, Qt::LeftButton);
    	mouseReleaseEvent(&e);
}	else if (lastPos!=e->pos()) {
    	QMouseEvent e(QEvent::MouseMove, e->pos(), 
    	   Qt::NoButton, 0);
    	mouseMoveEvent(&e);
}

    lastPressure = e->pressure();
    lastPos = e->pos();
    */
}

void QG_GraphicView::leaveEvent(QEvent* e) {
    RS_GraphicView::mouseLeaveEvent();
    QWidget::leaveEvent(e);
}


void QG_GraphicView::enterEvent(QEvent* e) {
    RS_GraphicView::mouseEnterEvent();
    QWidget::enterEvent(e);
}


void QG_GraphicView::focusOutEvent(QFocusEvent* e) {
    QWidget::focusOutEvent(e);
}


void QG_GraphicView::focusInEvent(QFocusEvent* e) {
    RS_GraphicView::mouseEnterEvent();
    QWidget::focusInEvent(e);
}


/**
 * mouse wheel event. zooms in/out or scrolls when
 * shift or ctrl is pressed.
 */
void QG_GraphicView::wheelEvent(QWheelEvent *e) {
    //RS_DEBUG->print("wheel: %d", e->delta());

    //printf("state: %d\n", e->state());
    //printf("ctrl: %d\n", Qt::ControlButton);

    if (container==NULL) {
        return;
    }

	RS_Vector mouse = toGraph(RS_Vector(e->x(), e->y()));

    bool scroll = false;
    RS2::Direction direction = RS2::Up;

    // scroll up / down:
    if (e->state()==Qt::ControlModifier) {
        scroll = true;
        if (e->delta()>0) {
            direction = RS2::Up;
        } else {
            direction = RS2::Down;
        }
    }

    // scroll left / right:
    else if	(e->state()==Qt::ShiftModifier) {
        scroll = true;
        if (e->delta()>0) {
            direction = RS2::Right;
        } else {
            direction = RS2::Left;
        }
    }

    if (scroll) {
        setCurrentAction(new RS_ActionZoomScroll(direction,
                         *container, *this));
    }

    // zoom in / out:
    else if (e->state()==0) {
        if (e->delta()>0) {
            setCurrentAction(new RS_ActionZoomIn(*container, *this,
                                                 RS2::In, RS2::Both,
												 mouse));
        } else {
            setCurrentAction(new RS_ActionZoomIn(*container, *this,
                                                 RS2::Out, RS2::Both,
												 mouse));
        }
    }
	
	redraw();

    e->accept();
}


void QG_GraphicView::keyPressEvent(QKeyEvent* e) {
    //if (e->key()==Qt::Key_Control) {
    //	setCtrlPressed(true);
    //}


    if (container==NULL) {
        return;
    }

    bool scroll = false;
    RS2::Direction direction = RS2::Up;

    switch (e->key()) {
    case Qt::Key_Left:
        scroll = true;
        direction = RS2::Right;
        break;
    case Qt::Key_Right:
        scroll = true;
        direction = RS2::Left;
        break;
    case Qt::Key_Up:
        scroll = true;
        direction = RS2::Up;
        break;
    case Qt::Key_Down:
        scroll = true;
        direction = RS2::Down;
        break;
    default:
        scroll = false;
        break;
    }

    if (scroll) {
        setCurrentAction(new RS_ActionZoomScroll(direction,
                         *container, *this));
    }

    RS_GraphicView::keyPressEvent(e);
}


void QG_GraphicView::keyReleaseEvent(QKeyEvent* e) {
    //if (e->key()==Qt::Key_Control) {
    //	setCtrlPressed(false);
    //}
    RS_GraphicView::keyReleaseEvent(e);
}


/**
 * Called whenever the graphic view has changed.
 * Adjusts the scrollbar ranges / steps.
 */
void QG_GraphicView::adjustOffsetControls() {
	static bool running = false;

	if (running) {
		return;
	}

	running = true;

    RS_DEBUG->print("QG_GraphicView::adjustOffsetControls() begin");

    if (container==NULL || hScrollBar==NULL || vScrollBar==NULL) {
        return;
    }

    int ox = getOffsetX();
    int oy = getOffsetY();
	
    RS_Vector min = container->getMin();
    RS_Vector max = container->getMax();

    // no drawing yet - still allow to scroll
    if (max.x < min.x+1.0e-6 || 
	    max.y < min.y+1.0e-6 ||
		max.x > RS_MAXDOUBLE || 
		max.x < RS_MINDOUBLE || 
		min.x > RS_MAXDOUBLE || 
		min.x < RS_MINDOUBLE ||
		max.y > RS_MAXDOUBLE || 
		max.y < RS_MINDOUBLE || 
		min.y > RS_MAXDOUBLE || 
		min.y < RS_MINDOUBLE ) {
        min = RS_Vector(-10,-10);
        max = RS_Vector(100,100);
    }
	
	int minVal = (int)(min.x * getFactor().x 
			- QG_SCROLLMARGIN - getBorderLeft());
	int maxVal = (int)(max.x * getFactor().x 
			- getWidth() + QG_SCROLLMARGIN + getBorderRight());

	hScrollBar->setValue(0);
	if (minVal<=maxVal) {
		hScrollBar->setRange(minVal, maxVal);
	}
    //hScrollBar->setMinValue(minVal);
    
	//hScrollBar->setMaxValue(maxVal);

	minVal = (int)(getHeight() - max.y * getFactor().y 
			- QG_SCROLLMARGIN - getBorderTop());
	maxVal = (int)(QG_SCROLLMARGIN + getBorderBottom() 
			- (min.y * getFactor().y));

	if (minVal<=maxVal) {
		vScrollBar->setRange(minVal, maxVal);
	}
    //vScrollBar->setMaxValue((int)(QG_SCROLLMARGIN + getBorderBottom()
     //                             - (min.y * getFactor().y)));
								  
	
    //vScrollBar->setMinValue((int)(getHeight() -
     //                             max.y * getFactor().y
     //                             - QG_SCROLLMARGIN - getBorderTop()));
								  

    hScrollBar->setPageStep((int)(getWidth()));
    vScrollBar->setPageStep((int)(getHeight()));

    hScrollBar->setValue(-ox);
    vScrollBar->setValue(oy);
	

    slotHScrolled(-ox);
    slotVScrolled(oy);
	

    RS_DEBUG->print("H min: %d / max: %d / step: %d / value: %d\n",
                    hScrollBar->minValue(), hScrollBar->maxValue(),
                    hScrollBar->pageStep(), ox);
    RS_DEBUG->print("V min: %d / max: %d / step: %d / value: %d\n",
                    vScrollBar->minValue(), vScrollBar->maxValue(),
                    vScrollBar->pageStep(), oy);

    RS_DEBUG->print("QG_GraphicView::adjustOffsetControls() end");

	running = false;
}


/**
 * override this to adjust controls and widgets that
 * control the zoom factor of the graphic.
 */
void QG_GraphicView::adjustZoomControls() {}


/**
 * Slot for horizontal scroll events.
 */
void QG_GraphicView::slotHScrolled(int value) {
    // Scrollbar behaviour tends to change with every Qt version..
    // so let's keep old code in here for now

    //static int running = false;
    //if (!running) {
    //running = true;
    ////RS_DEBUG->print("value x: %d\n", value);
    if (hScrollBar->maxValue()==hScrollBar->minValue()) {
        centerOffsetX();
    } else {
        setOffsetX(-value);
    }
    //if (isUpdateEnabled()) {
//         updateGrid();
    redraw();
}


/**
 * Slot for vertical scroll events.
 */
void QG_GraphicView::slotVScrolled(int value) {
    // Scrollbar behaviour tends to change with every Qt version..
    // so let's keep old code in here for now

    //static int running = false;
    //if (!running) {
    //running = true;
    ////RS_DEBUG->print("value y: %d\n", value);
    if (vScrollBar->maxValue()==vScrollBar->minValue()) {
        centerOffsetY();
    } else {
        setOffsetY(value);
    }
    //if (isUpdateEnabled()) {
  //  updateGrid();
    redraw();
}

QPixmap* QG_GraphicView::getPixmapForView(QPixmap *pm)
{
	
	if (pm==NULL) {
		return new QPixmap(getWidth(), getHeight());
	} else if (pm->width()!=getWidth() || pm->height()!=getHeight()) {
		delete pm;
		return new QPixmap(getWidth(), getHeight());
	} else {
		return pm;
	}
}


/**
 * Handles paint events by redrawing the graphic in this view.
 * usually that's very fast since we only paint the buffer we
 * have from the last call..
 */
void QG_GraphicView::paintEvent(QPaintEvent *) {
    RS_DEBUG->print("QG_GraphicView::paintEvent begin");
	
	RS_SETTINGS->beginGroup("/Appearance");
    bool draftMode = (bool)RS_SETTINGS->readNumEntry("/DraftMode", 0);
    RS_SETTINGS->endGroup();

	
	// Re-Create or get the layering pixmaps
	PixmapLayer1=getPixmapForView(PixmapLayer1);
	PixmapLayer2=getPixmapForView(PixmapLayer2);
	PixmapLayer3=getPixmapForView(PixmapLayer3);

	// Draw Layer 1
	if (redrawMethod & RS2::RedrawGrid) {
		PixmapLayer1->fill(background);
		RS_PainterQt painter1(PixmapLayer1);
		//painter1->setBackgroundMode(Qt::OpaqueMode);
		//painter1->setBackgroundColor(background);
		//painter1->eraseRect(0,0,getWidth(), getHeight());
		drawLayer1((RS_Painter*)&painter1);
		painter1.end();
	}
	

	if (redrawMethod & RS2::RedrawDrawing) {
		// DRaw layer 2
		PixmapLayer2->fill(Qt::transparent);
		RS_PainterQt painter2(PixmapLayer2);
		painter2.setDrawingMode(drawingMode);
		setDraftMode(draftMode);
		drawLayer2((RS_Painter*)&painter2);
		setDraftMode(false);
		painter2.end();
	}
	
	if (redrawMethod & RS2::RedrawOverlay) {
		PixmapLayer3->fill(Qt::transparent);
		RS_PainterQt painter3(PixmapLayer3);
		drawLayer3((RS_Painter*)&painter3);
		painter3.end();
	} 
		
	// Finally paint the layers back on the screen, bitblk to the rescue!
	RS_PainterQt wPainter(this);
	//wPainter.setCompositionMode(QPainter::CompositionMode_Screen);
	wPainter.drawPixmap(0,0,*PixmapLayer1);
	wPainter.drawPixmap(0,0,*PixmapLayer2);
	wPainter.drawPixmap(0,0,*PixmapLayer3);
	wPainter.end();
	
	redrawMethod=RS2::RedrawNone;
    RS_DEBUG->print("QG_GraphicView::paintEvent end");
}



/**
 * Previews the given url for the file open dialog.
 */
void QG_GraphicView::previewUrl(const Q3Url &u) {
	//static RS_Graphic* gr = new RS_Graphic();
	// TODO: Decide if previewurl is needed in qt4
	/*
	RS_DEBUG->print("QG_GraphicView::previewUrl");
	
	if (container!=NULL && container->rtti()==RS2::EntityGraphic) {
		((RS_Graphic*)container)->open(u.path(), RS2::FormatUnknown);
		zoomAuto();
	}
	//setContainer(gr);
	
	RS_DEBUG->print("QG_GraphicView::previewUrl: OK");
	 */
}


