/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2015 A. Stebich (librecad@mail.lordofbikes.de)
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

#include<climits>
#include<cmath>

#include <QApplication>
#include <QDesktopWidget>
#include <QAction>
#include <QMouseEvent>
#include <QtAlgorithms>

#include "rs_graphicview.h"

#include "rs_line.h"
#include "rs_linetypepattern.h"
#include "rs_eventhandler.h"
#include "rs_graphic.h"
#include "rs_grid.h"
#include "rs_painter.h"
#include "rs_mtext.h"
#include "rs_text.h"
#include "rs_settings.h"
#include "rs_dialogfactory.h"
#include "rs_layer.h"
#include "rs_math.h"
#include "rs_debug.h"
#include "rs_color.h"

#ifdef EMU_C99
#include "emu_c99.h"
#endif

/**
 * Constructor.
 */
RS_GraphicView::RS_GraphicView(QWidget* parent, Qt::WindowFlags f)
    :QWidget(parent, f)
	,eventHandler{new RS_EventHandler{this}}
	,gridColor(Qt::gray)
	,metaGridColor{64, 64, 64}
	,grid{new RS_Grid{this}}
	,drawingMode(RS2::ModeFull)
	,savedViews(16)
    ,previousViewTime(QDateTime::currentDateTime())
    ,panning(false)
{
    RS_SETTINGS->beginGroup("Colors");
    setBackground(QColor(RS_SETTINGS->readEntry("/background", Colors::background)));
    setGridColor(QColor(RS_SETTINGS->readEntry("/grid", Colors::grid)));
    setMetaGridColor(QColor(RS_SETTINGS->readEntry("/meta_grid", Colors::meta_grid)));
    setSelectedColor(QColor(RS_SETTINGS->readEntry("/select", Colors::select)));
    setHighlightedColor(QColor(RS_SETTINGS->readEntry("/highlight", Colors::highlight)));
    setStartHandleColor(QColor(RS_SETTINGS->readEntry("/start_handle", Colors::start_handle)));
    setHandleColor(QColor(RS_SETTINGS->readEntry("/handle", Colors::handle)));
    setEndHandleColor(QColor(RS_SETTINGS->readEntry("/end_handle", Colors::end_handle)));
    RS_SETTINGS->endGroup();
}

RS_GraphicView::~RS_GraphicView()
{
    qDeleteAll(overlayEntities);
}

/**
 * Must be called by any derived class in the destructor.
 */
void RS_GraphicView::cleanUp() {
	m_bIsCleanUp=true;
}

/**
 * Sets the pointer to the graphic which contains the entities
 * which are visualized by this widget.
 */
void RS_GraphicView::setContainer(RS_EntityContainer* container) {
	this->container = container;
	//adjustOffsetControls();
}



/**
 * Sets the zoom factor in X for this visualization of the graphic.
 */
void RS_GraphicView::setFactorX(double f) {
	if (!zoomFrozen) {
		factor.x = fabs(f);
	}
}



/**
 * Sets the zoom factor in Y for this visualization of the graphic.
 */
void RS_GraphicView::setFactorY(double f) {
	if (!zoomFrozen) {
		factor.y = fabs(f);
	}
}

void RS_GraphicView::setOffset(int ox, int oy) {
	//    DEBUG_HEADER
	//    RS_DEBUG->print(/*RS_Debug::D_WARNING, */"set offset from (%d, %d) to (%d, %d)", getOffsetX(), getOffsetY(), ox, oy);
	setOffsetX(ox);
	setOffsetY(oy);
}

/**
 * @return true if the grid is switched on.
 */
bool RS_GraphicView::isGridOn() const{
	if (container) {
		RS_Graphic* g = container->getGraphic();
		if (g) {
			return g->isGridOn();
		}
	}
	return true;
}

/**
 * @return true if the grid is isometric
 *
 *@Author: Dongxu Li
 */
bool RS_GraphicView::isGridIsometric() const{
	return grid->isIsometric();
}


void RS_GraphicView::setCrosshairType(RS2::CrosshairType chType){
	grid->setCrosshairType(chType);
}

RS2::CrosshairType RS_GraphicView::getCrosshairType() const{
	return grid->getCrosshairType();
}

/**
 * Centers the drawing in x-direction.
 */
void RS_GraphicView::centerOffsetX() {
	if (container && !zoomFrozen) {
		offsetX = (int)(((getWidth()-borderLeft-borderRight)
						 - (container->getSize().x*factor.x))/2.0
						- (container->getMin().x*factor.x)) + borderLeft;
	}
}



/**
 * Centers the drawing in y-direction.
 */
void RS_GraphicView::centerOffsetY() {
	if (container && !zoomFrozen) {
		offsetY = (int)((getHeight()-borderTop-borderBottom
						 - (container->getSize().y*factor.y))/2.0
						- (container->getMin().y*factor.y)) + borderBottom;
	}
}



/**
 * Centers the given coordinate in the view in x-direction.
 */
void RS_GraphicView::centerX(double v) {
	if (!zoomFrozen) {
		offsetX = (int)((v*factor.x)
						- (double)(getWidth()-borderLeft-borderRight)/2.0);
	}
}



/**
 * Centers the given coordinate in the view in y-direction.
 */
void RS_GraphicView::centerY(double v) {
	if (!zoomFrozen) {
		offsetY = (int)((v*factor.y)
						- (double)(getHeight()-borderTop-borderBottom)/2.0);
	}
}

/**
 * @return Current action or nullptr.
 */
RS_ActionInterface* RS_GraphicView::getDefaultAction() {
	if (eventHandler) {
		return eventHandler->getDefaultAction();
	} else {
        return nullptr;
	}
}



/**
 * Sets the default action of the event handler.
 */
void RS_GraphicView::setDefaultAction(RS_ActionInterface* action) {
	if (eventHandler) {
		eventHandler->setDefaultAction(action);
	}
}



/**
 * @return Current action or nullptr.
 */
RS_ActionInterface* RS_GraphicView::getCurrentAction() {
	if (eventHandler) {
		return eventHandler->getCurrentAction();
	} else {
        return nullptr;
	}
}



/**
 * Sets the current action of the event handler.
 */
void RS_GraphicView::setCurrentAction(RS_ActionInterface* action) {
    RS_DEBUG->print("RS_GraphicView::setCurrentAction");
	if (eventHandler) {
		eventHandler->setCurrentAction(action);
	}
	RS_DEBUG->print("RS_GraphicView::setCurrentAction: OK");
}


/**
 * Kills all running selection actions. Called when a selection action
 * is launched to reduce confusion.
 */
void RS_GraphicView::killSelectActions() {
	if (eventHandler) {
		eventHandler->killSelectActions();
	}
}



/**
 * Kills all running actions.
 */
void RS_GraphicView::killAllActions() {
	if (eventHandler) {
		eventHandler->killAllActions();
	}
}



/**
 * Go back in menu or current action.
 */
void RS_GraphicView::back()
{
	if (eventHandler && eventHandler->hasAction())
    {
		eventHandler->back();
    }
}

/**
 * Go forward with the current action.
 */
void RS_GraphicView::enter() {
	if (eventHandler && eventHandler->hasAction()) {
		eventHandler->enter();
	}
}

/**
 * Called by the actual GUI class which implements a command line.
 */
void RS_GraphicView::commandEvent(RS_CommandEvent* e) {
	if (eventHandler) {
		eventHandler->commandEvent(e);
	}
}

/**
 * Enables coordinate input in the command line.
 */
void RS_GraphicView::enableCoordinateInput() {
	if (eventHandler) {
		eventHandler->enableCoordinateInput();
	}
}



/**
 * Disables coordinate input in the command line.
 */
void RS_GraphicView::disableCoordinateInput() {
	if (eventHandler) {
		eventHandler->disableCoordinateInput();
	}
}



/**
 * zooms in by factor f
 */
void RS_GraphicView::zoomIn(double f, const RS_Vector& center) {

	if (f<1.0e-6) {
		RS_DEBUG->print(RS_Debug::D_WARNING,
						"RS_GraphicView::zoomIn: invalid factor");
		return;
	}

    RS_Vector c = center;
    if (!c.valid) {
        //find mouse position
        c= getMousePosition();
    }

	zoomWindow(
				toGraph(0, 0)
				.scale(c, RS_Vector(1.0/f,1.0/f)),
				toGraph(getWidth(), getHeight())
				.scale(c, RS_Vector(1.0/f,1.0/f)));

	//adjustOffsetControls();
	//adjustZoomControls();
	// updateGrid();
	redraw();
}



/**
 * zooms in by factor f in x
 */
void RS_GraphicView::zoomInX(double f) {
	factor.x*=f;
	offsetX=(int)((offsetX-getWidth()/2)*f)+getWidth()/2;
	adjustOffsetControls();
	adjustZoomControls();
	// updateGrid();
	redraw();
}



/**
 * zooms in by factor f in y
 */
void RS_GraphicView::zoomInY(double f) {
	factor.y*=f;
	offsetY=(int)((offsetY-getHeight()/2)*f)+getHeight()/2;
	adjustOffsetControls();
	adjustZoomControls();
	//    updateGrid();
	redraw();
}



/**
 * zooms out by factor f
 */
void RS_GraphicView::zoomOut(double f, const RS_Vector& center) {
	if (f<1.0e-6) {
		RS_DEBUG->print(RS_Debug::D_WARNING,
						"RS_GraphicView::zoomOut: invalid factor");
		return;
	}
	zoomIn(1/f, center);
}



/**
 * zooms out by factor f in x
 */
void RS_GraphicView::zoomOutX(double f) {
	if (f<1.0e-6) {
		RS_DEBUG->print(RS_Debug::D_WARNING,
						"RS_GraphicView::zoomOutX: invalid factor");
		return;
	}
	factor.x/=f;
	offsetX=(int)(offsetX/f);
	adjustOffsetControls();
	adjustZoomControls();
	//    updateGrid();
	redraw();
}



/**
 * zooms out by factor f y
 */
void RS_GraphicView::zoomOutY(double f) {
	if (f<1.0e-6) {
		RS_DEBUG->print(RS_Debug::D_WARNING,
						"RS_GraphicView::zoomOutY: invalid factor");
		return;
	}
	factor.y/=f;
	offsetY=(int)(offsetY/f);
	adjustOffsetControls();
	adjustZoomControls();
	//    updateGrid();
	redraw();
}

/**
 * performs autozoom
 *
 * @param axis include axis in zoom
 * @param keepAspectRatio true: keep aspect ratio 1:1
 *                        false: factors in x and y are stretched to the max
 */
#include <iostream>
void RS_GraphicView::zoomAuto(bool axis, bool keepAspectRatio) {

	RS_DEBUG->print("RS_GraphicView::zoomAuto");


	if (container) {
        container->calculateBorders();

		double sx, sy;
		if (axis) {
			auto const dV = container->getMax() - container->getMin();
			sx = std::max(dV.x, 0.);
			sy = std::max(dV.y, 0.);
		} else {
			sx = container->getSize().x;
			sy = container->getSize().y;
		}
//		    std::cout<<" RS_GraphicView::zoomAuto("<<sx<<","<<sy<<")"<<std::endl;
//			std::cout<<" RS_GraphicView::zoomAuto("<<axis<<","<<keepAspectRatio<<")"<<std::endl;

		double fx=1., fy=1.;
		unsigned short fFlags=0;

		if (sx>RS_TOLERANCE) {
			fx = (getWidth()-borderLeft-borderRight) / sx;
		} else {
			fFlags += 1; //invalid x factor
		}

		if (sy>RS_TOLERANCE) {
			fy = (getHeight()-borderTop-borderBottom) / sy;
		} else {
			fFlags += 2; //invalid y factor
		}
		//    std::cout<<"0: fx= "<<fx<<"\tfy="<<fy<<std::endl;

		RS_DEBUG->print("f: %f/%f", fx, fy);

		switch(fFlags){
		case 1:
			fx=fy;
			break;
		case 2:
			fy=fx;
			break;
		case 3:
			return; //do not do anything, invalid factors
		default:
			if (keepAspectRatio) {
				fx = fy = std::min(fx, fy);
			}
			//                break;
		}
		//    std::cout<<"1: fx= "<<fx<<"\tfy="<<fy<<std::endl;

		RS_DEBUG->print("f: %f/%f", fx, fy);
		//exclude invalid factors
		fFlags=0;
		if (fx<RS_TOLERANCE||fx>RS_MAXDOUBLE) {
			fx=1.0;
			fFlags += 1;
		}
		if (fy<RS_TOLERANCE||fy>RS_MAXDOUBLE) {
			fy=1.0;
			fFlags += 2;
		}
		if(fFlags == 3 ) return;
		saveView();
		//        std::cout<<"2: fx= "<<fx<<"\tfy="<<fy<<std::endl;
		setFactorX(fx);
		setFactorY(fy);

		RS_DEBUG->print("f: %f/%f", fx, fy);


		//        RS_DEBUG->print("adjustZoomControls");
		adjustZoomControls();
		//        RS_DEBUG->print("centerOffsetX");
		centerOffsetX();
		//        RS_DEBUG->print("centerOffsetY");
		centerOffsetY();
		//        RS_DEBUG->print("adjustOffsetControls");
		adjustOffsetControls();
		//        RS_DEBUG->print("updateGrid");
		//    updateGrid();

		redraw();
	}
	RS_DEBUG->print("RS_GraphicView::zoomAuto OK");
}



/**
 * Shows previous view.
 */
void RS_GraphicView::zoomPrevious() {

	RS_DEBUG->print("RS_GraphicView::zoomPrevious");

	if (container) {
		restoreView();
	}
}



/**
 * Saves the current view as previous view to which we can
 * switch back later with @see restoreView().
 */
void RS_GraphicView::saveView() {
	if(getGraphic()) getGraphic()->setModified(true);
	QDateTime noUpdateWindow=QDateTime::currentDateTime().addMSecs(-500);
	//do not update view within 500 milliseconds
	if(previousViewTime > noUpdateWindow) return;
	previousViewTime = QDateTime::currentDateTime();
	savedViews[savedViewIndex]=std::make_tuple(offsetX,offsetY,factor);
	savedViewIndex = (savedViewIndex+1)%savedViews.size();
	if(savedViewCount<savedViews.size()) savedViewCount++;

	if(savedViewCount==1){
		emit previous_zoom_state(true);
	}
}



/**
 * Restores the view previously saved with
 * @see saveView().
 */
void RS_GraphicView::restoreView() {
	if(savedViewCount == 0) return;
	savedViewCount --;
	if(savedViewCount==0){
		emit previous_zoom_state(false);
	}
	savedViewIndex = (savedViewIndex + savedViews.size() - 1)%savedViews.size();

	offsetX = std::get<0>(savedViews[savedViewIndex]);
	offsetY = std::get<1>(savedViews[savedViewIndex]);
	factor = std::get<2>(savedViews[savedViewIndex]);

	adjustOffsetControls();
	adjustZoomControls();
	//    updateGrid();

	redraw();
}


/*	*
 *	Function name:
 *	Description:		Performs autozoom in Y axis only.
 *	Author(s):			..., Claude Sylvain
 *	Created:				?
 *	Last modified:		23 July 2011
 *
 *	Parameters:			bool axis:
 *								Axis in zoom.
 *
 *	Returns:				void
 *	*/

void RS_GraphicView::zoomAutoY(bool axis) {
	if (container) {
		double visibleHeight = 0.0;
		double minY = RS_MAXDOUBLE;
		double maxY = RS_MINDOUBLE;
		bool noChange = false;

		for(auto e: *container){

			if (e->rtti()==RS2::EntityLine) {
				RS_Line* l = (RS_Line*)e;
				double x1, x2;
				x1 = toGuiX(l->getStartpoint().x);
				x2 = toGuiX(l->getEndpoint().x);

				if (	((x1 > 0.0) && (x1 < (double) getWidth())) ||
						((x2 > 0.0) && (x2 < (double) getWidth())))
				{
					minY = std::min(minY, l->getStartpoint().y);
					minY = std::min(minY, l->getEndpoint().y);
					maxY = std::max(maxY, l->getStartpoint().y);
					maxY = std::max(maxY, l->getEndpoint().y);
				}
			}
		}

		if (axis) {
			visibleHeight = std::max(maxY, 0.0) - std::min(minY, 0.0);
		} else {
			visibleHeight = maxY-minY;
		}

		if (visibleHeight<1.0) {
			noChange = true;
		}

		double fy = 1.0;
		if (visibleHeight>1.0e-6) {
			fy = (getHeight()-borderTop-borderBottom)
					/ visibleHeight;
			if (factor.y<0.000001) {
				noChange = true;
			}
		}

		if (noChange==false) {
			setFactorY(fy);
			//centerOffsetY();
			offsetY = (int)((getHeight()-borderTop-borderBottom
							 - (visibleHeight*factor.y))/2.0
							- (minY*factor.y)) + borderBottom;
			adjustOffsetControls();
			adjustZoomControls();
			//    updateGrid();

		}
		RS_DEBUG->print("Auto zoom y ok");
	}
}



/**
 * Zooms the area given by v1 and v2.
 *
 * @param keepAspectRatio true: keeps the aspect ratio 1:1
 *                        false: zooms exactly the selected range to the
 *                               current graphic view
 */
void RS_GraphicView::zoomWindow(RS_Vector v1, RS_Vector v2,
								bool keepAspectRatio) {



	double zoomX=480.0;    // Zoom for X-Axis
	double zoomY=640.0;    // Zoom for Y-Axis   (Set smaller one)
	int zoomBorder = 0;

	// Switch left/right and top/bottom is necessary:
	if(v1.x>v2.x) {
		std::swap(v1.x,v2.x);
	}
	if(v1.y>v2.y) {
		std::swap(v1.y,v2.y);
	}

	// Get zoom in X and zoom in Y:
	if(v2.x-v1.x>1.0e-6) {
		zoomX = getWidth() / (v2.x-v1.x);
	}
	if(v2.y-v1.y>1.0e-6) {
		zoomY = getHeight() / (v2.y-v1.y);
	}

	// Take smaller zoom:
	if (keepAspectRatio) {
		if(zoomX<zoomY) {
			if(getWidth()!=0) {
				zoomX = zoomY = ((double)(getWidth()-2*zoomBorder)) /
						(double)getWidth()*zoomX;
			}
		} else {
			if(getHeight()!=0) {
				zoomX = zoomY = ((double)(getHeight()-2*zoomBorder)) /
						(double)getHeight()*zoomY;
			}
		}
	}

	zoomX=fabs(zoomX);
	zoomY=fabs(zoomY);

	// Borders in pixel after zoom
	int pixLeft  =(int)(v1.x*zoomX);
	int pixTop   =(int)(v2.y*zoomY);
	int pixRight =(int)(v2.x*zoomX);
	int pixBottom=(int)(v1.y*zoomY);
	if(  pixLeft == INT_MIN || pixLeft== INT_MAX ||
		 pixRight == INT_MIN || pixRight== INT_MAX ||
		 pixTop == INT_MIN || pixTop== INT_MAX ||
		 pixBottom == INT_MIN || pixBottom== INT_MAX ) {
		RS_DIALOGFACTORY->commandMessage("Requested zooming factor out of range. Zooming not changed");
		return;
	}
	saveView();

	// Set new offset for zero point:
	offsetX = - pixLeft + (getWidth() -pixRight +pixLeft)/2;
	offsetY = - pixTop + (getHeight() -pixBottom +pixTop)/2;
	factor.x = zoomX;
	factor.y = zoomY;

	adjustOffsetControls();
	adjustZoomControls();
	//    updateGrid();

	redraw();
}



/**
 * Centers the point v1.
 */
void RS_GraphicView::zoomPan(int dx, int dy) {
	//offsetX+=(int)toGuiDX(v1.x);
	//offsetY+=(int)toGuiDY(v1.y);

	offsetX += dx;
	offsetY -= dy;

	adjustOffsetControls();
	//adjustZoomControls();
	//    updateGrid();

	redraw();
}



/**
 * Scrolls in the given direction.
 */
void RS_GraphicView::zoomScroll(RS2::Direction direction) {
	switch (direction) {
	case RS2::Up:
		offsetY-=50;
		break;
	case RS2::Down:
		offsetY+=50;
		break;
	case RS2::Right:
		offsetX+=50;
		break;
	case RS2::Left:
		offsetX-=50;
		break;
	}
	adjustOffsetControls();
	adjustZoomControls();
	//    updateGrid();

	redraw();
}



/**
 * Zooms to page extends.
 */
void RS_GraphicView::zoomPage() {

	RS_DEBUG->print("RS_GraphicView::zoomPage");
	if (!container) {
		return;
	}

	RS_Graphic* graphic = container->getGraphic();
	if (!graphic) {
		return;
	}

	RS_Vector s = graphic->getPrintAreaSize()/graphic->getPaperScale();

	double fx, fy;

	if (s.x>RS_TOLERANCE) {
		fx = (getWidth()-borderLeft-borderRight) / s.x;
	} else {
		fx = 1.0;
	}

	if (s.y>RS_TOLERANCE) {
		fy = (getHeight()-borderTop-borderBottom) / s.y;
	} else {
		fy = 1.0;
	}

	RS_DEBUG->print("f: %f/%f", fx, fy);

	fx = fy = std::min(fx, fy);

	RS_DEBUG->print("f: %f/%f", fx, fy);

	if (fx<RS_TOLERANCE) {
		fx=fy=1.0;
	}

	setFactorX(fx);
	setFactorY(fy);

	RS_DEBUG->print("f: %f/%f", fx, fy);

	centerOffsetX();
	centerOffsetY();
	adjustOffsetControls();
	adjustZoomControls();
	//    updateGrid();

	redraw();
}



/**
 * Draws the entities within the given range.
 */
void RS_GraphicView::drawWindow_DEPRECATED(RS_Vector v1, RS_Vector v2) {
	RS_DEBUG->print("RS_GraphicView::drawWindow() begin");
	if (container) {
		for(auto se: *container){
			if (se->isInWindow(v1, v2)) {
                drawEntity(nullptr, se);
			}
		}
	}
	RS_DEBUG->print("RS_GraphicView::drawWindow() end");
}

/**
 * Draws the entities.
 * This function can only be called from within the paint event
 *
 */
void RS_GraphicView::drawLayer1(RS_Painter *painter) {

	// drawing paper border:
	if (isPrintPreview()) {
		drawPaper(painter);
	}

	// drawing meta grid:
	if (!isPrintPreview()) {

		//increase grid point size on for DPI>96
		int dpiX = qApp->desktop()->logicalDpiX();
		//        DEBUG_HEADER
		//        RS_DEBUG->print(RS_Debug::D_ERROR, "dpiX=%d\n",dpiX);
		const RS_Pen penSaved=painter->getPen();
		if(dpiX>96) {
			RS_Pen pen=penSaved;
			pen.setWidth(RS2::Width01);
			painter->setPen(pen);
		}

		//only drawGrid updates the grid layout (updatePointArray())
		drawMetaGrid(painter);
		//draw grid after metaGrid to avoid overwriting grid points by metaGrid lines
		//bug# 3430258
		drawGrid(painter);

		if(dpiX>96) painter->setPen(penSaved);

	}

}


/*	*
 *	Function name:
 *	Description: 		Do the drawing, step 2/3.
 *	Author(s):			..., Claude Sylvain
 *	Created:				?
 *	Last modified:		23 July 2011
 *
 *	Parameters:			RS_Painter *painter:
 *								...
 *
 *	Returns:				void
 *	*/

void RS_GraphicView::drawLayer2(RS_Painter *painter)
{
	drawEntity(painter, container);	//	Draw all entities.

	//	If not in print preview, draw the absolute zero reference.
	//	----------------------------------------------------------
	if (!isPrintPreview())
		drawAbsoluteZero(painter);
}


void RS_GraphicView::drawLayer3(RS_Painter *painter) {
	// drawing zero points:
	if (!isPrintPreview()) {
		drawRelativeZero(painter);
		drawOverlay(painter);
	}
}


/*	*
 *	Function name:
 *
 *	Description:	- Sets the pen of the painter object to the suitable pen
 *						  for the given entity.
 *
 *	Author(s):		..., Claude Sylvain
 *	Created:			?
 *	Last modified:	17 November 2011
 *
 *	Parameters:		RS_Painter *painter:
 *							...
 *
 *						RS_Entity *e:
 *							...
 *
 *	Returns:			void
 */

void RS_GraphicView::setPenForEntity(RS_Painter *painter,RS_Entity *e)
{
	if (draftMode) {
		painter->setPen(RS_Pen(foreground,
							   RS2::Width00, RS2::SolidLine));
	}

	// Getting pen from entity (or layer)
	RS_Pen pen = e->getPen(true);

	int w = pen.getWidth();
	if (w<0) {
		w = 0;
	}

	// - Scale pen width.
	// - By default pen width is not scaled on print and print preview.
	//   This is the standard (AutoCAD like) behaviour.
	// bug# 3437941
	// ------------------------------------------------------------
	if (!draftMode)
	{
		double	uf = 1.0;	// Unit factor.
		double	wf = 1.0;	// Width factor.

		RS_Graphic* graphic = container->getGraphic();

        if (graphic)
		{
			uf = RS_Units::convert(1.0, RS2::Millimeter, graphic->getUnit());

			if ((isPrinting() || isPrintPreview()) &&
					graphic->getPaperScale() > RS_TOLERANCE )
			{
				if (scaleLineWidth)
				{
					wf = graphic->getVariableDouble("$DIMSCALE", 1.0);
				}
				else
				{
					wf = 1.0 / graphic->getPaperScale();
				}

			}
		}

		pen.setScreenWidth(toGuiDX(w / 100.0 * uf * wf));
	}
	else
	{
		//		pen.setWidth(RS2::Width00);
		pen.setScreenWidth(0);
	}

	// prevent drawing with 1-width which is slow:
	if (RS_Math::round(pen.getScreenWidth())==1) {
		pen.setScreenWidth(0.0);
	}

    // prevent background color on background drawing
    // and enhance visibility of black lines on dark backgrounds
    RS_Color    penColor {pen.getColor().stripFlags()};
    if ( penColor == background.stripFlags()
         || (penColor.toIntColor() == RS_Color::Black
             && penColor.colorDistance( background) < RS_Color::MinColorDistance)) {
        pen.setColor( foreground);
    }

	if (!isPrinting() && !isPrintPreview())
	{
		// this entity is selected:
		if (e->isSelected()) {
			pen.setLineType(RS2::DotLine);
			pen.setColor(selectedColor);
		}

		// this entity is highlighted:
		if (e->isHighlighted()) {
			pen.setColor(highlightedColor);
		}
	}

	// deleting not drawing:
	if (getDeleteMode()) {
		pen.setColor(background);
	}

	painter->setPen(pen);
}


/**
 * Draws an entity. Might be recursively called e.g. for polylines.
 * If the class wide painter is nullptr a new painter will be created
 * and destroyed afterwards.
 *
 * @param patternOffset Offset of line pattern (used for connected
 *        lines e.g. in splines).
 * @param db Double buffering on (recommended) / off
 */
void RS_GraphicView::drawEntity(RS_Entity* /*e*/, double& /*patternOffset*/) {
	RS_DEBUG->print("RS_GraphicView::drawEntity(RS_Entity*,patternOffset) not supported anymore");
	// RVT_PORT this needs to be optimized
	// One way to do is to send a RS2::RedrawSelected, then the draw routine will only draw all selected entities
	// Dis-advantage is that we still need to iterate over all entities, but
	// this might be very fast
	// For now we just redraw the drawing until we are going to optimize drawing
	redraw(RS2::RedrawDrawing);
}
void RS_GraphicView::drawEntity(RS_Entity* /*e*/ /*patternOffset*/) {
	RS_DEBUG->print("RS_GraphicView::drawEntity(RS_Entity*,patternOffset) not supported anymore");
	// RVT_PORT this needs to be optimized
	// One way to do is to send a RS2::RedrawSelected, then the draw routine will only draw all selected entities
	// Dis-advantage is that we still need to iterate over all entities, but
	// this might be very fast
	// For now we just redraw the drawing until we are going to optimize drawing
	redraw(RS2::RedrawDrawing);
}
void RS_GraphicView::drawEntity(RS_Painter *painter, RS_Entity* e) {
	double offset(0.);
	drawEntity(painter,e,offset);
}
void RS_GraphicView::drawEntity(RS_Painter *painter, RS_Entity* e, double& patternOffset) {

	// update is disabled:
    // given entity is nullptr:
	if (!e) {
		return;
	}

	// entity is not visible:
	if (!e->isVisible()) {
		return;
	}
	if( isPrintPreview() || isPrinting() ) {
		// do not draw construction layer on print preview or print
		if( ! e->isPrint()
				||  e->isConstruction())
        return;
	}

    // test if the entity is in the viewport
    if (!isPrinting() &&
        e->rtti() != RS2::EntityGraphic &&
        e->rtti() != RS2::EntityLine &&
       (toGuiX(e->getMax().x)<0 || toGuiX(e->getMin().x)>getWidth() ||
        toGuiY(e->getMin().y)<0 || toGuiY(e->getMax().y)>getHeight())) {
        return;
    }

	// set pen (color):
	setPenForEntity(painter, e );

	//RS_DEBUG->print("draw plain");
	if (isDraftMode()) {
        switch(e->rtti()){
        case RS2::EntityMText:
        case RS2::EntityText:
            painter->drawRect(toGui(e->getMin()), toGui(e->getMax()));
            break;
        case RS2::EntityImage:
            // all images as rectangles:
            painter->drawRect(toGui(e->getMin()), toGui(e->getMax()));
            break;
        case RS2::EntityHatch:
            //skip hatches
            break;
        default:
            drawEntityPlain(painter, e, patternOffset);
        }
	} else {
		drawEntityPlain(painter, e, patternOffset);
	}

	// draw reference points:
	if (e->isSelected() && !(isPrinting() || isPrintPreview())) {
		if (!e->isParentSelected()) {
			RS_VectorSolutions const& s = e->getRefPoints();

			for (size_t i=0; i<s.getNumber(); ++i) {
				int sz = -1;
				RS_Color col = handleColor;
				if (i == 0) {
					col = startHandleColor;
				}
				else if (i == s.getNumber() - 1) {
					col = endHandleColor;
				}
				if (getDeleteMode()) {
					painter->drawHandle(toGui(s.get(i)), background, sz);
				} else {
					painter->drawHandle(toGui(s.get(i)), col, sz);
				}
			}
		}
	}

	//RS_DEBUG->print("draw plain OK");


	//RS_DEBUG->print("RS_GraphicView::drawEntity() end");
}


/**
 * Draws an entity.
 * The painter must be initialized and all the attributes (pen) must be set.
 */
void RS_GraphicView::drawEntityPlain(RS_Painter *painter, RS_Entity* e, double& patternOffset) {
	if (!e) {
		return;
	}

	if (!e->isContainer() && (e->isSelected()!=painter->shouldDrawSelected())) {
		return;
	}

	e->draw(painter, this, patternOffset);

}
void RS_GraphicView::drawEntityPlain(RS_Painter *painter, RS_Entity* e) {
	if (!e) {
		return;
	}

	if (!e->isContainer() && (e->isSelected()!=painter->shouldDrawSelected())) {
		return;
	}
	double patternOffset(0.);
	e->draw(painter, this, patternOffset);
}
/**
 * Deletes an entity with the background color.
 * Might be recursively called e.g. for polylines.
 */
void RS_GraphicView::deleteEntity(RS_Entity* e) {

	// RVT_PORT When we delete a single entity, we can do this but we need to remove this then also from containerEntities
	RS_DEBUG->print("RS_GraphicView::deleteEntity will for now redraw the whole screen instead of just deleting the entity");
	setDeleteMode(true);
	drawEntity(e);
	setDeleteMode(false);
	redraw(RS2::RedrawDrawing);
}




/**
 * @return Pointer to the static pattern struct that belongs to the
 * given pattern type or nullptr.
 */
const RS_LineTypePattern* RS_GraphicView::getPattern(RS2::LineType t) {
	switch (t) {
	case RS2::SolidLine:
		return &RS_LineTypePattern::patternSolidLine;
		break;

	case RS2::DotLine:
		return &RS_LineTypePattern::patternDotLine;
		break;
	case RS2::DotLineTiny:
		return &RS_LineTypePattern::patternDotLineTiny;
		break;
	case RS2::DotLine2:
		return &RS_LineTypePattern::patternDotLine2;
		break;
	case RS2::DotLineX2:
		return &RS_LineTypePattern::patternDotLineX2;
		break;

	case RS2::DashLine:
		return &RS_LineTypePattern::patternDashLine;
		break;
	case RS2::DashLineTiny:
		return &RS_LineTypePattern::patternDashLineTiny;
		break;
	case RS2::DashLine2:
		return &RS_LineTypePattern::patternDashLine2;
		break;
	case RS2::DashLineX2:
		return &RS_LineTypePattern::patternDashLineX2;
		break;

	case RS2::DashDotLine:
		return &RS_LineTypePattern::patternDashDotLine;
		break;
	case RS2::DashDotLineTiny:
		return &RS_LineTypePattern::patternDashDotLineTiny;
		break;
	case RS2::DashDotLine2:
		return &RS_LineTypePattern::patternDashDotLine2;
		break;
	case RS2::DashDotLineX2:
		return &RS_LineTypePattern::patternDashDotLineX2;
		break;

	case RS2::DivideLine:
		return &RS_LineTypePattern::patternDivideLine;
		break;
	case RS2::DivideLineTiny:
		return &RS_LineTypePattern::patternDivideLineTiny;
		break;
	case RS2::DivideLine2:
		return &RS_LineTypePattern::patternDivideLine2;
		break;
	case RS2::DivideLineX2:
		return &RS_LineTypePattern::patternDivideLineX2;
		break;

	case RS2::CenterLine:
		return &RS_LineTypePattern::patternCenterLine;
		break;
	case RS2::CenterLineTiny:
		return &RS_LineTypePattern::patternCenterLineTiny;
		break;
	case RS2::CenterLine2:
		return &RS_LineTypePattern::patternCenterLine2;
		break;
	case RS2::CenterLineX2:
		return &RS_LineTypePattern::patternCenterLineX2;
		break;

	case RS2::BorderLine:
		return &RS_LineTypePattern::patternBorderLine;
		break;
	case RS2::BorderLineTiny:
		return &RS_LineTypePattern::patternBorderLineTiny;
		break;
	case RS2::BorderLine2:
		return &RS_LineTypePattern::patternBorderLine2;
		break;
	case RS2::BorderLineX2:
		return &RS_LineTypePattern::patternBorderLineX2;
		break;

	case RS2::LineByLayer:
		return &RS_LineTypePattern::patternBlockLine;
		break;
	case RS2::LineByBlock:
		return &RS_LineTypePattern::patternBlockLine;
		break;
	default:
		break;
	}
    return nullptr;
}



/**
 * This virtual method can be overwritten to draw the absolute
 * zero. It's called from within drawIt(). The default implementation
 * draws a simple red cross on the zero of the sheet
 * This function can ONLY be called from within a paintEvent because it will
 * use the painter
 *
 * @see drawIt()
 */
void RS_GraphicView::drawAbsoluteZero(RS_Painter *painter)
{
    int const zr = 20;

    RS_Pen pen_xAxis (RS_Color(255,0,0), RS2::Width00, RS2::SolidLine);
	pen_xAxis.setScreenWidth(0);

    RS_Pen pen_yAxis (RS_Color(0,255,0), RS2::Width00, RS2::SolidLine);
	pen_yAxis.setScreenWidth(0);

    auto originPoint = toGui(RS_Vector(0,0));

    if (((originPoint.x + zr) < 0) || ((originPoint.x - zr) > getWidth()))  return;
    if (((originPoint.y + zr) < 0) || ((originPoint.y - zr) > getHeight())) return;

    RS_SETTINGS->beginGroup("/Appearance");
    const bool extendAxisLines { (bool) RS_SETTINGS->readNumEntry("/ExtendAxisLines", 0) };
    RS_SETTINGS->endGroup();

    double xAxisPoints [2];
    double yAxisPoints [2];

    if (extendAxisLines)
    {
        xAxisPoints [0] = 0.0;
        xAxisPoints [1] = getWidth();

        yAxisPoints [0] = 0.0;
        yAxisPoints [1] = getHeight();
    }
    else
    {
        xAxisPoints [0] = originPoint.x - zr;
        xAxisPoints [1] = originPoint.x + zr;

        yAxisPoints [0] = originPoint.y - zr;
        yAxisPoints [1] = originPoint.y + zr;
    }

    painter->setPen(pen_xAxis);
    painter->drawLine(RS_Vector(xAxisPoints[0], originPoint.y), RS_Vector(xAxisPoints[1], originPoint.y));

    painter->setPen(pen_yAxis);
    painter->drawLine(RS_Vector(originPoint.x, yAxisPoints[0]), RS_Vector(originPoint.x, yAxisPoints[1]));
}

/**
 * This virtual method can be overwritten to draw the relative
 * zero point. It's called from within drawIt(). The default implementation
 * draws a simple red round zero point. This is the point that was last created by the user, end of a line for example
 * This function can ONLY be called from within a paintEvent because it will
 * use the painter
 *
 * @see drawIt()
 */
void RS_GraphicView::drawRelativeZero(RS_Painter *painter) {

    if (!relativeZero.valid) {
		return;
	}

    RS_Pen p(RS_Color(255,0,0), RS2::Width00, RS2::SolidLine);
	p.setScreenWidth(0);
	painter->setPen(p);

    int const zr=5;
    auto vp=toGui(relativeZero);
    if( vp.x +zr<0 || vp.x-zr>getWidth()) return;
    if( vp.y +zr<0 || vp.y-zr>getHeight()) return;

    painter->drawLine(RS_Vector(vp.x-zr, vp.y),
                      RS_Vector(vp.x+zr, vp.y)
                      );
    painter->drawLine(RS_Vector(vp.x, vp.y-zr),
                      RS_Vector(vp.x, vp.y+zr)
                      );

    painter->drawCircle(vp, 5);
}



/**
 * Draws the paper border (for print previews).
 * This function can ONLY be called from within a paintEvent because it will
 * use the painter
 *
 * @see drawIt()
 */
void RS_GraphicView::drawPaper(RS_Painter *painter) {

	if (!container) {
		return;
	}

	RS_Graphic* graphic = container->getGraphic();
	if (graphic->getPaperScale()<1.0e-6) {
		return;
	}

	// draw paper:
	// RVT_PORT rewritten from     painter->setPen(Qt::gray);
	painter->setPen(QColor(Qt::gray));

	RS_Vector pinsbase = graphic->getPaperInsertionBase();
	RS_Vector printAreaSize = graphic->getPrintAreaSize();
	double scale = graphic->getPaperScale();

	RS_Vector v1 = toGui((RS_Vector(0,0)-pinsbase)/scale);
	RS_Vector v2 = toGui((printAreaSize-pinsbase)/scale);

	int marginLeft = (int)(graphic->getMarginLeftInUnits() * factor.x / scale);
	int marginTop = (int)(graphic->getMarginTopInUnits() * factor.y / scale);
	int marginRight = (int)(graphic->getMarginRightInUnits() * factor.x / scale);
	int marginBottom = (int)(graphic->getMarginBottomInUnits() * factor.y / scale);

	int printAreaW = (int)(v2.x-v1.x);
	int printAreaH = (int)(v2.y-v1.y);

	int paperX1 = (int)v1.x;
	int paperY1 = (int)v1.y;
	// Don't show margins between neighbor pages.
	int paperW = printAreaW + marginLeft + marginRight;
	int paperH = printAreaH - marginTop - marginBottom;

	int numX = graphic->getPagesNumHoriz();
	int numY = graphic->getPagesNumVert();

	// gray background:
	painter->fillRect(0,0, getWidth(), getHeight(),
					  RS_Color(200,200,200));

	// shadow:
	painter->fillRect(paperX1+6, paperY1+6, paperW, paperH,
					  RS_Color(64,64,64));

	// border:
	painter->fillRect(paperX1, paperY1, paperW, paperH,
					  RS_Color(64,64,64));

	// paper:
	painter->fillRect(paperX1+1, paperY1-1, paperW-2, paperH+2,
					  RS_Color(180,180,180));

	// print area:
	painter->fillRect(paperX1+1+marginLeft, paperY1-1-marginBottom,
					  printAreaW-2, printAreaH+2,
					  RS_Color(255,255,255));

	// don't paint boundaries if zoom is to small
	if (qMin(fabs(printAreaW/numX), fabs(printAreaH/numY)) > 2) {
		// boundaries between pages:
		for (int pX = 1; pX < numX; pX++) {
			double offset = ((double)printAreaW*pX)/numX;
			painter->fillRect(paperX1+marginLeft+offset, paperY1,
							  1, paperH,
							  RS_Color(64,64,64));
		}
		for (int pY = 1; pY < numY; pY++) {
			double offset = ((double)printAreaH*pY)/numY;
			painter->fillRect(paperX1, paperY1-marginBottom+offset,
							  paperW, 1,
							  RS_Color(64,64,64));
		}
	}
}


/**
 * Draws the grid.
 *
 * @see drawIt()
 */
void RS_GraphicView::drawGrid(RS_Painter *painter)
{
	if (!(grid && isGridOn())) return;

	// draw grid:
	//painter->setPen(Qt::gray);
	//painter->setPen(gridColor); #1de207

    painter->setPen(RS_Pen(gridColor, RS2::Width00, RS2::SolidLine));

    RS_SETTINGS->beginGroup("/Appearance");

    if (RS_SETTINGS->readNumEntry("/GridType", 0) == 0)
    {
	    //grid->updatePointArray();

	    auto const& pts = grid->getPoints();
	    for (auto const& v: pts) painter->drawGridPoint(toGui(v));
    }
    else
    {
        const RS_Vector cellSize = grid->getCellVector();

        auto const& mx = grid->getMetaX();
        for (auto const& x: mx)
        {
            for (int i = 1; i < 10; i++)
            {
                const double subX { x - (i * cellSize.x) };
                painter->drawLine(RS_Vector(toGuiX(subX), 0), RS_Vector(toGuiX(subX), getHeight()));
            }
        }

        auto const& my = grid->getMetaY();
        for (auto const& y: my)
        {
            for (int j = 1; j < 10; j++)
            {
                const double subY { y - (j * cellSize.y) };
                painter->drawLine(RS_Vector(0, toGuiY(subY)), RS_Vector(getWidth(), toGuiY(subY)));
            }
        }
    }
    RS_SETTINGS->endGroup();

	// draw grid info:
	//painter->setPen(Qt::white);
	QString info = grid->getInfo();
	//info = QString("%1 / %2")
	//       .arg(grid->getSpacing())
	//       .arg(grid->getMetaSpacing());

	updateGridStatusWidget(info);
}



/**
 * Draws the meta grid.
 *
 * @see drawIt()
 */
void RS_GraphicView::drawMetaGrid(RS_Painter *painter) {

	if (!(grid && isGridOn()) /*|| grid->getMetaSpacing()<0.0*/) {
		return;
	}

	//draw grid after metaGrid to avoid overwriting grid points by metaGrid lines
	//bug# 3430258
	grid->updatePointArray();

    RS_Pen pen;
    RS2::LineType penLineType;

    RS_SETTINGS->beginGroup("/Appearance");
    if (RS_SETTINGS->readNumEntry("/GridType", 0) == 0)
    {
        penLineType = RS2::DotLine;
    }
    else
    {
        penLineType = RS2::SolidLine;
    }
    RS_SETTINGS->endGroup();

    painter->setPen(RS_Pen (metaGridColor, RS2::Width00, penLineType));

	RS_Vector dv=grid->getMetaGridWidth().scale(factor);
	double dx=fabs(dv.x);
	double dy=fabs(dv.y); //potential bug, need to recover metaGrid.width
	// draw meta grid:
	auto mx = grid->getMetaX();
	for(auto const& x: mx){
		painter->drawLine(RS_Vector(toGuiX(x), 0),
						  RS_Vector(toGuiX(x), getHeight()));
		if(grid->isIsometric()){
			painter->drawLine(RS_Vector(toGuiX(x)+0.5*dx, 0),
							  RS_Vector(toGuiX(x)+0.5*dx, getHeight()));
		}
	}
	auto my = grid->getMetaY();
	if(grid->isIsometric()){//isometric metaGrid
		dx=fabs(dx);
		dy=fabs(dy);
		if(!my.size()|| dx<1||dy<1) return;
		RS_Vector baseMeta(toGui(RS_Vector(mx[0],my[0])));
		// x-x0=k*dx, x-remainder(x-x0,dx)
		RS_Vector vp0(-remainder(-baseMeta.x,dx)-dx,getHeight()-remainder(getHeight()-baseMeta.y,dy)+dy);
		RS_Vector vp1(vp0);
		RS_Vector vp2(getWidth()-remainder(getWidth()-baseMeta.x,dx)+dx,vp0.y);
		RS_Vector vp3(vp2);
		int cmx = round((vp2.x - vp0.x)/dx);
		int cmy = round((vp0.y +remainder(-baseMeta.y,dy)+dy)/dy);
		for(int i=cmx+cmy+2;i>=0;i--){
			if ( i <= cmx ) {
				vp0.x += dx;
				vp2.y -= dy;
			}else{
				vp0.y -= dy;
				vp2.x -= dx;
			}
			if ( i <= cmy ) {
				vp1.y -= dy;
				vp3.x -= dx;
			}else{
				vp1.x += dx;
				vp3.y -= dy;
			}
			painter->drawLine(vp0,vp1);
			painter->drawLine(vp2,vp3);
		}

	}else{//orthogonal

		for(auto const& y: my){
			painter->drawLine(RS_Vector(0, toGuiY(y)),
							  RS_Vector(getWidth(), toGuiY(y)));
		}
	}


}

void RS_GraphicView::drawOverlay(RS_Painter *painter)
{
    double patternOffset(0.);

    foreach (auto ec, overlayEntities)
    {
        foreach (auto e, ec->getEntityList())
        {
            setPenForEntity(painter, e);
            e->draw(painter, this, patternOffset);
        }
    }
}

RS2::SnapRestriction RS_GraphicView::getSnapRestriction() const
{
	return defaultSnapRes;
}

RS_SnapMode RS_GraphicView::getDefaultSnapMode() const
{
	return defaultSnapMode;
}

/**
 * Sets the default snap mode used by newly created actions.
 */
void RS_GraphicView::setDefaultSnapMode(RS_SnapMode sm) {
	defaultSnapMode = sm;
	if (eventHandler) {
		eventHandler->setSnapMode(sm);
	}
}

/**
 * Sets a snap restriction (e.g. orthogonal).
 */
void RS_GraphicView::setSnapRestriction(RS2::SnapRestriction sr) {
	defaultSnapRes = sr;

	if (eventHandler) {
		eventHandler->setSnapRestriction(sr);
	}
}

/**
 * Translates a vector in real coordinates to a vector in screen coordinates.
 */
RS_Vector RS_GraphicView::toGui(RS_Vector v) const{
	return RS_Vector(toGuiX(v.x), toGuiY(v.y));
}

/**
 * Translates a real coordinate in X to a screen coordinate X.
 * @param visible Pointer to a boolean which will contain true
 * after the call if the coordinate is within the visible range.
 */
double RS_GraphicView::toGuiX(double x) const{
	return x*factor.x + offsetX;
}



/**
 * Translates a real coordinate in Y to a screen coordinate Y.
 */
double RS_GraphicView::toGuiY(double y) const{
	return -y*factor.y + getHeight() - offsetY;
}



/**
 * Translates a real coordinate distance to a screen coordinate distance.
 */
double RS_GraphicView::toGuiDX(double d) const{
	return d*factor.x;
}



/**
 * Translates a real coordinate distance to a screen coordinate distance.
 */
double RS_GraphicView::toGuiDY(double d) const{
	return d*factor.y;
}



/**
 * Translates a vector in screen coordinates to a vector in real coordinates.
 */
RS_Vector RS_GraphicView::toGraph(RS_Vector v) const{
	return RS_Vector(toGraphX(RS_Math::round(v.x)),
					 toGraphY(RS_Math::round(v.y)));
}



/**
 * Translates two screen coordinates to a vector in real coordinates.
 */
RS_Vector RS_GraphicView::toGraph(int x, int y) const{
	return RS_Vector(toGraphX(x), toGraphY(y));
}


/**
 * Translates a screen coordinate in X to a real coordinate X.
 */
double RS_GraphicView::toGraphX(int x) const{
	return (x - offsetX)/factor.x;
}



/**
 * Translates a screen coordinate in Y to a real coordinate Y.
 */
double RS_GraphicView::toGraphY(int y) const{
	return -(y - getHeight() + offsetY)/factor.y;
}



/**
 * Translates a screen coordinate distance to a real coordinate distance.
 */
double RS_GraphicView::toGraphDX(int d) const{
	return d/factor.x;
}



/**
 * Translates a screen coordinate distance to a real coordinate distance.
 */
double RS_GraphicView::toGraphDY(int d) const{
	return d/factor.y;
}


/**
 * Sets the relative zero coordinate (if not locked)
 * without deleting / drawing the point.
 */
void RS_GraphicView::setRelativeZero(const RS_Vector& pos) {
	if (relativeZeroLocked==false) {
		relativeZero = pos;
        emit relative_zero_changed(pos);
	}
}



/**
 * Sets the relative zero coordinate, deletes the old position
 * on the screen and draws the new one.
 */
void RS_GraphicView::moveRelativeZero(const RS_Vector& pos) {
	setRelativeZero(pos);
	redraw(RS2::RedrawGrid);
}


/**
 * Gets the specified overlay container.
 */
RS_EntityContainer* RS_GraphicView::getOverlayContainer(RS2::OverlayGraphics position)
{
	if (overlayEntities[position]) {
		return overlayEntities[position];
	}
    overlayEntities[position]=new RS_EntityContainer(nullptr);

	return overlayEntities[position];

}

RS_Grid* RS_GraphicView::getGrid() const{
	return grid.get();
}

RS_EventHandler* RS_GraphicView::getEventHandler() const{
    return eventHandler;
}

void RS_GraphicView::setBackground(const RS_Color& bg) {
	background = bg;

    RS_Color black(0,0,0);
    if (black.colorDistance( bg) >= RS_Color::MinColorDistance) {
        foreground = black;
    }
    else {
        foreground = RS_Color(255,255,255);
    }
}

void RS_GraphicView::setBorders(int left, int top, int right, int bottom) {
	borderLeft = left;
	borderTop = top;
	borderRight = right;
	borderBottom = bottom;
}

RS_Graphic* RS_GraphicView::getGraphic() const{
	if (container && container->rtti()==RS2::EntityGraphic) {
		return static_cast<RS_Graphic *>(container);
	} else {
		return nullptr;
	}
}

RS_EntityContainer* RS_GraphicView::getContainer() const{
	return container;
}

void RS_GraphicView::setFactor(double f) {
	setFactorX(f);
	setFactorY(f);
}

RS_Vector RS_GraphicView::getFactor() const{
	return factor;
}
int RS_GraphicView::getBorderLeft() const{
	return borderLeft;
}
int RS_GraphicView::getBorderTop() const{
	return borderTop;
}
int RS_GraphicView::getBorderRight() const{
	return borderRight;
}
int RS_GraphicView::getBorderBottom() const{
	return borderBottom;
}

void RS_GraphicView::freezeZoom(bool freeze) {
	zoomFrozen=freeze;
}
bool RS_GraphicView::isZoomFrozen() const{
	return zoomFrozen;
}
void RS_GraphicView::setOffsetX(int ox) {
	offsetX = ox;
}
void RS_GraphicView::setOffsetY(int oy) {
	offsetY = oy;
}
int RS_GraphicView::getOffsetX() const{
	return offsetX;
}
int RS_GraphicView::getOffsetY() const{
	return offsetY;
}
void RS_GraphicView::lockRelativeZero(bool lock) {
	relativeZeroLocked=lock;
}

bool RS_GraphicView::isRelativeZeroLocked() const{
	return relativeZeroLocked;
}

RS_Vector const& RS_GraphicView::getRelativeZero() const{
	return relativeZero;
}

void RS_GraphicView::setPrintPreview(bool pv) {
	printPreview = pv;
}

bool RS_GraphicView::isPrintPreview() const{
	return printPreview;
}

void RS_GraphicView::setPrinting(bool p) {
	printing = p;
}

bool RS_GraphicView::isPrinting() const{
	return printing;
}

bool RS_GraphicView::isDraftMode() const{
	return draftMode;
}

void RS_GraphicView::setDraftMode(bool dm) {
	draftMode=dm;
}

bool RS_GraphicView::isCleanUp(void) const
{
	return m_bIsCleanUp;
}

bool RS_GraphicView::isPanning() const {
    return panning;
}

void RS_GraphicView::setPanning(bool state) {
    panning = state;
}
