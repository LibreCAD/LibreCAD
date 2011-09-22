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


#include "rs_graphicview.h"

#include "rs_linetypepattern.h"
#include "rs_eventhandler.h"
#include "rs_graphic.h"
#include "rs_grid.h"
#include "rs_painter.h"
#include "rs_text.h"
#include "rs_settings.h"
#include "rs_dialogfactory.h"



/**
 * Constructor.
 */
RS_GraphicView::RS_GraphicView()
        : background(), foreground() {
    drawingMode = RS2::ModeFull;
	printing = false;
    deleteMode = false;
    factor = RS_Vector(1.0,1.0);
    offsetX = 0;
    offsetY = 0;
    previousFactor = RS_Vector(1.0,1.0);
    previousOffsetX = 0;
    previousOffsetY = 0;
    container = NULL;
    eventHandler = new RS_EventHandler(this);
    gridColor = Qt::gray;
    metaGridColor = RS_Color(64,64,64);
    grid = new RS_Grid(this);
    zoomFrozen = false;
    //gridVisible = true;
    draftMode = false;

    borderLeft = 0;
    borderTop = 0;
    borderRight = 0;
    borderBottom = 0;

    relativeZero = RS_Vector(false);
    relativeZeroLocked=false;

    mx = my = 0;

    //@load default snap mode from prefrences.
    RS_SETTINGS->beginGroup("/Snap");
    unsigned int snapFlags(RS_SETTINGS->readNumEntry("/SnapMode",0));
    RS_SETTINGS->endGroup();
    defaultSnapMode=RS_Snapper::intToSnapMode(snapFlags);
    defaultSnapRes=defaultSnapMode.restriction;


    RS_SETTINGS->beginGroup("/Appearance");
    setBackground(QColor(RS_SETTINGS->readEntry("/BackgroundColor", "#000000")));
    setGridColor(QColor(RS_SETTINGS->readEntry("/GridColor", "#7F7F7F")));
    setMetaGridColor(QColor(RS_SETTINGS->readEntry("/MetaGridColor", "#3F3F3F")));
    setSelectedColor(QColor(RS_SETTINGS->readEntry("/SelectedColor", "#A54747")));
    setHighlightedColor(QColor(RS_SETTINGS->readEntry("/HighlightedColor",
                               "#739373")));
    RS_SETTINGS->endGroup();

    printPreview = false;


    //currentInsert = NULL;
}



/**
 * Destructor.
 */
RS_GraphicView::~RS_GraphicView() {
    //@write default snap mode from prefrences.
    RS_SETTINGS->beginGroup("/Snap");
    unsigned int snapFlags=RS_Snapper::snapModeToInt(defaultSnapMode);
    RS_SETTINGS->writeEntry("/SnapMode",QString::number(snapFlags));
    RS_SETTINGS->endGroup();
    //delete grid;
    delete grid;
}



/**
 * Must be called by any derrived class in the destructor.
 */
void RS_GraphicView::cleanUp() {
    //delete eventHandler;
    delete eventHandler;
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



/**
 * @return true if the grid is switched on.
 */
bool RS_GraphicView::isGridOn() {
    if (container!=NULL) {
        RS_Graphic* g = container->getGraphic();
        if (g!=NULL) {
            return g->isGridOn();
        }
    }
    return true;
}




/**
 * Centers the drawing in x-direction.
 */
void RS_GraphicView::centerOffsetX() {
    if (container!=NULL && !zoomFrozen) {
        offsetX = (int)(((getWidth()-borderLeft-borderRight)
                         - (container->getSize().x*factor.x))/2.0
                        - (container->getMin().x*factor.x)) + borderLeft;
    }
}



/**
 * Centers the drawing in y-direction.
 */
void RS_GraphicView::centerOffsetY() {
    if (container!=NULL && !zoomFrozen) {
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
 * @return Current action or NULL.
 */
RS_ActionInterface* RS_GraphicView::getDefaultAction() {
    if (eventHandler!=NULL) {
        return eventHandler->getDefaultAction();
    } else {
        return NULL;
    }
}



/**
 * Sets the default action of the event handler.
 */
void RS_GraphicView::setDefaultAction(RS_ActionInterface* action) {
    if (eventHandler!=NULL) {
        eventHandler->setDefaultAction(action);
    }
}



/**
 * @return Current action or NULL.
 */
RS_ActionInterface* RS_GraphicView::getCurrentAction() {
    if (eventHandler!=NULL) {
        return eventHandler->getCurrentAction();
    } else {
        return NULL;
    }
}



/**
 * Sets the current action of the event handler.
 */
void RS_GraphicView::setCurrentAction(RS_ActionInterface* action) {
	RS_DEBUG->print("RS_GraphicView::setCurrentAction");
    if (eventHandler!=NULL) {
        eventHandler->setCurrentAction(action);
    }
	RS_DEBUG->print("RS_GraphicView::setCurrentAction: OK");
}


/**
 * Kills all running selection actions. Called when a selection action
 * is launched to reduce confusion.
 */
void RS_GraphicView::killSelectActions() {
    if (eventHandler!=NULL) {
        eventHandler->killSelectActions();
    }
}



/**
 * Kills all running actions.
 */
void RS_GraphicView::killAllActions() {
    if (eventHandler!=NULL) {
        eventHandler->killAllActions();
    }
}



/**
 * Go back in menu or current action.
 */
void RS_GraphicView::back() {
    if (eventHandler!=NULL && eventHandler->hasAction()) {
        eventHandler->back();
    } else {
        if (RS_DIALOGFACTORY!=NULL) {
            RS_DIALOGFACTORY->requestPreviousMenu();
        }
    }
}



/**
 * Go forward with the current action.
 */
void RS_GraphicView::enter() {
    if (eventHandler!=NULL && eventHandler->hasAction()) {
        eventHandler->enter();
    }
}



/**
 * Called by the actual GUI class which implements the RS_GraphicView
 * interface to notify LibreCAD about mouse events.
 */
void RS_GraphicView::mousePressEvent(QMouseEvent* e) {
    if (eventHandler!=NULL) {
        eventHandler->mousePressEvent(e);
    }
}



/**
 * Called by the actual GUI class which implements the RS_GraphicView
 * interface to notify LibreCAD about mouse events.
 */
void RS_GraphicView::mouseReleaseEvent(QMouseEvent* e) {
	RS_DEBUG->print("RS_GraphicView::mouseReleaseEvent");
    if (eventHandler!=NULL) {
        if (e->button()!=Qt::RightButton ||
			eventHandler->hasAction()) {

            eventHandler->mouseReleaseEvent(e);
            //e->accept();
        }
        else {
            back();

            e->accept();
        }
    }
	RS_DEBUG->print("RS_GraphicView::mouseReleaseEvent: OK");
}


/*	*
 *	Function name:
 *
 *	Description:		- Called by the actual GUI class which implements the
 *							  RS_GraphicView interface to notify LibreCAD about
 *							  mouse events.
 *
 *	Author(s):			..., Claude Sylvain
 *	Created:				?
 *	Last modified:		23 July 2011
 *
 *	Parameters:			QMouseEvent* e:
 *								...
 *
 *	Returns:				void
 *	*/

void RS_GraphicView::mouseMoveEvent(QMouseEvent* e) {
    RS_DEBUG->print("RS_GraphicView::mouseMoveEvent begin");

    RS_Graphic* graphic = NULL;

    if (container->rtti()==RS2::EntityGraphic) {
        graphic = (RS_Graphic*)container;
    }

    RS_DEBUG->print("RS_GraphicView::mouseMoveEvent 001");

    if (e!=NULL) {
        mx = e->x();
        my = e->y();
    }

    RS_DEBUG->print("RS_GraphicView::mouseMoveEvent 002");

    if (eventHandler!=NULL) {
        eventHandler->mouseMoveEvent(e);
    }

    RS_DEBUG->print("RS_GraphicView::mouseMoveEvent 003");

    if (	((eventHandler == NULL) || !eventHandler->hasAction()) &&
			(graphic != NULL))
	 {
        RS_Vector	mouse		= toGraph(RS_Vector(mx, my));
        RS_Vector	relMouse	= mouse - getRelativeZero();

        if (RS_DIALOGFACTORY!=NULL)
            RS_DIALOGFACTORY->updateCoordinateWidget(mouse, relMouse);
    }

    RS_DEBUG->print("RS_GraphicView::mouseMoveEvent end");
}



/**
 * Called by the actual GUI class which implements the RS_GraphicView
 * interface to notify LibreCAD about mouse events.
 */
void RS_GraphicView::mouseLeaveEvent() {
    if (eventHandler!=NULL) {
        eventHandler->mouseLeaveEvent();
    }
}



/**
 * Called by the actual GUI class which implements the RS_GraphicView
 * interface to notify LibreCAD about mouse events.
 */
void RS_GraphicView::mouseEnterEvent() {
    if (eventHandler!=NULL) {
        eventHandler->mouseEnterEvent();
    }
}



/**
 * Called by the actual GUI class which implements the RS_GraphicView
 * interface to notify LibreCAD about key events.
 */
void RS_GraphicView::keyPressEvent(QKeyEvent* e) {
    if (eventHandler!=NULL) {
        eventHandler->keyPressEvent(e);
    }
}



/**
 * Called by the actual GUI class which implements the RS_GraphicView
 * interface to notify LibreCAD about key events.
 */
void RS_GraphicView::keyReleaseEvent(QKeyEvent* e) {
    if (eventHandler!=NULL) {
        eventHandler->keyReleaseEvent(e);
    }
}



/**
 * Called by the actual GUI class which implements a command line.
 */
void RS_GraphicView::commandEvent(RS_CommandEvent* e) {
    if (eventHandler!=NULL) {
        eventHandler->commandEvent(e);
    }
}



/**
 * Enables coordinate input in the command line.
 */
void RS_GraphicView::enableCoordinateInput() {
    if (eventHandler!=NULL) {
        eventHandler->enableCoordinateInput();
    }
}



/**
 * Disables coordinate input in the command line.
 */
void RS_GraphicView::disableCoordinateInput() {
    if (eventHandler!=NULL) {
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
    if (c.valid==false) {
        c = toGraph(RS_Vector(getWidth()/2, getHeight()/2));
    }

    zoomWindow(
        toGraph(RS_Vector(0,0))
        .scale(c, RS_Vector(1.0/f,1.0/f)),
        toGraph(RS_Vector(getWidth(),getHeight()))
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
void RS_GraphicView::zoomAuto(bool axis, bool keepAspectRatio) {

    RS_DEBUG->print("RS_GraphicView::zoomAuto");

	saveView();

    if (container!=NULL) {
        container->calculateBorders();

        double sx, sy;
        if (axis) {
            sx = std::max(container->getMax().x, 0.0)
                 - std::min(container->getMin().x, 0.0);
            sy = std::max(container->getMax().y, 0.0)
                 - std::min(container->getMin().y, 0.0);
        } else {
            sx = container->getSize().x;
            sy = container->getSize().y;
        }

        double fx, fy;

        if (sx>RS_TOLERANCE) {
            fx = (getWidth()-borderLeft-borderRight) / sx;
        } else {
            fx = 1.0;
        }

        if (sy>RS_TOLERANCE) {
            fy = (getHeight()-borderTop-borderBottom) / sy;
        } else {
            fy = 1.0;
        }

        RS_DEBUG->print("f: %f/%f", fx, fy);

        if (keepAspectRatio) {
            fx = fy = std::min(fx, fy);
        }

        RS_DEBUG->print("f: %f/%f", fx, fy);

        if (fx<RS_TOLERANCE) {
            fx=fy=1.0;
        }
        setFactorX(fx);
        setFactorY(fy);

        RS_DEBUG->print("f: %f/%f", fx, fy);


        RS_DEBUG->print("adjustOffsetControls");
        adjustOffsetControls();
        RS_DEBUG->print("adjustZoomControls");
        adjustZoomControls();
        RS_DEBUG->print("centerOffsetX");
        centerOffsetX();
        RS_DEBUG->print("centerOffsetY");
        centerOffsetY();
        RS_DEBUG->print("updateGrid");
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

    if (container!=NULL) {
		restoreView();
    }
}



/**
 * Saves the current view as previous view to which we can
 * switch back later with @see restoreView().
 */
void RS_GraphicView::saveView() {
	previousOffsetX = offsetX;
	previousOffsetY = offsetY;
	previousFactor = factor;
}



/**
 * Restores the view previously saved with
 * @see saveView().
 */
void RS_GraphicView::restoreView() {
	int pox = previousOffsetX;
	int poy = previousOffsetY;
	RS_Vector pf = previousFactor;

	saveView();

	offsetX = pox;
	offsetY = poy;
	factor = pf;

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
    if (container!=NULL) {
        double visibleHeight = 0.0;
        double minY = RS_MAXDOUBLE;
        double maxY = RS_MINDOUBLE;
        bool noChange = false;

        for (RS_Entity* e=container->firstEntity(RS2::ResolveNone);
                e!=NULL;
                e = container->nextEntity(RS2::ResolveNone)) {

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


	saveView();

    double zoomX=480.0;    // Zoom for X-Axis
    double zoomY=640.0;    // Zoom for Y-Axis   (Set smaller one)
    double dum;            // Dummy for switching values
    int zoomBorder = 0;

    // Switch left/right and top/bottom is necessary:
    if(v1.x>v2.x) {
        dum=v1.x;
        v1.x=v2.x;
        v2.x=dum;
    }
    if(v1.y>v2.y) {
        dum=v1.y;
        v1.y=v2.y;
        v2.y=dum;
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

    if(zoomX<0.0) {
        zoomX*=-1;
    }
    if(zoomY<0.0) {
        zoomY*=-1;
    }

    // Borders in pixel after zoom
    int pixLeft  =(int)(v1.x*zoomX);
    int pixTop   =(int)(v2.y*zoomY);
    int pixRight =(int)(v2.x*zoomX);
    int pixBottom=(int)(v1.y*zoomY);

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
    if (container==NULL) {
        return;
    }

    RS_Graphic* graphic = container->getGraphic();
    if (graphic==NULL) {
        return;
    }

    RS_Vector s = graphic->getPaperSize();
    RS_Vector pinsbase = graphic->getPaperInsertionBase();

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
    if (container!=NULL) {
        for (RS_Entity* se=container->firstEntity(RS2::ResolveNone);
                se!=NULL;
                se = container->nextEntity(RS2::ResolveNone)) {
            if (se->isInWindow(v1, v2)) {
                drawEntity(NULL, se);
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

        //only drawGrid updates the grid layout (updatePointArray())
        drawGrid(painter);
        drawMetaGrid(painter);

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
 *	Last modified:	24 July 2011
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

		// Scale pen width.
		//	----------------
		if (!draftMode)
		{
			double uf = 1.0;	//	Unit factor.

			RS_Graphic* graphic = container->getGraphic();

			if (graphic != NULL)
				uf = RS_Units::convert(1.0, RS2::Millimeter, graphic->getUnit());

				pen.setScreenWidth(toGuiDX(w / 100.0 * uf));
		}
		else
			pen.setScreenWidth(0);

        // prevent drawing with 1-width which is slow:
        if (RS_Math::round(pen.getScreenWidth())==1) {
            pen.setScreenWidth(0.0);
        }

        // prevent background color on background drawing:
        if (pen.getColor().stripFlags()==background.stripFlags()) {
            pen.setColor(foreground);
        }

        // this entity is selected:
        if (e->isSelected()) {
            pen.setLineType(RS2::DotLine);
            pen.setColor(selectedColor);
        }

        // this entity is highlighted:
        if (e->isHighlighted()) {
            pen.setColor(highlightedColor);
        }

        // deleting not drawing:
        if (getDeleteMode()) {
            pen.setColor(background);
        }

        painter->setPen(pen);
}


/**
 * Draws an entity. Might be recusively called e.g. for polylines.
 * If the class wide painter is NULL a new painter will be created
 * and destroyed afterwards.
 *
 * @param patternOffset Offset of line pattern (used for connected
 *        lines e.g. in splines).
 * @param db Double buffering on (recommended) / off
 */
void RS_GraphicView::drawEntity(RS_Entity* e, double patternOffset) {
	RS_DEBUG->print("RS_GraphicView::drawEntity(RS_Entity*,patternOffset) not supported anymore");
	// RVT_PORT this needs to be optimized
	// ONe way to do is to send a RS2::RedrawSelected, then teh draw routine will onyl draw all selected entities
	// Dis-advantage is that we still need to iterate over all entities, but
	// this might be very fast
	// For now we just redraw the drawing untill we are going to optmize drawing
	redraw(RS2::RedrawDrawing);
}
void RS_GraphicView::drawEntity(RS_Painter *painter, RS_Entity* e, double patternOffset) {

    // update is diabled:
    // given entity is NULL:
    if (e==NULL) {
        return;
    }

    // entity is not visible:
    if (!e->isVisible()) {
        return;
    }

    // test if the entity is in the viewport
	/* temporary disabled so rs_overlaylien can be drawn
    if (!e->isContainer() && !isPrinting() &&
            (painter==NULL || !painter->isPreviewMode()) &&
            (toGuiX(e->getMax().x)<0 || toGuiX(e->getMin().x)>getWidth() ||
             toGuiY(e->getMin().y)<0 || toGuiY(e->getMax().y)>getHeight())) {
        return;
    } */

    // set pen (color):
    setPenForEntity(painter, e );

    //RS_DEBUG->print("draw plain");
    if (isDraftMode()) {
		// large texts as rectangles:
        if (e->rtti()==RS2::EntityText) {
            if (toGuiDX(((RS_Text*)e)->getHeight())<4 || e->countDeep()>100) {
                painter->drawRect(toGui(e->getMin()), toGui(e->getMax()));
            } else {
                drawEntityPlain(painter, e, patternOffset);
            }
		}

		// all images as rectangles:
		else if (e->rtti()==RS2::EntityImage) {
            painter->drawRect(toGui(e->getMin()), toGui(e->getMax()));
        }

		// hide hatches:
		else if (e->rtti()==RS2::EntityHatch) {
            // nothing
        }

        else {
            drawEntityPlain(painter, e, patternOffset);
        }
    } else {
        drawEntityPlain(painter, e, patternOffset);
    }

    // draw reference points:
    if (e->isSelected()) {
        if (!e->isParentSelected()) {
            RS_VectorSolutions s = e->getRefPoints();

            for (int i=0; i<s.getNumber(); ++i) {
				int sz = -1;
				RS_Color col = RS_Color(0,0,255);
				if (e->rtti()==RS2::EntityPolyline) {
					if (i==0 || i==s.getNumber()-1) {
						if (i==0) {
							sz = 4;
							col = QColor(0,64,255);
						}
						else {
							sz = 3;
							col = QColor(0,0,128);
						}
					}
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
void RS_GraphicView::drawEntityPlain(RS_Painter *painter, RS_Entity* e, double patternOffset) {
    if (e==NULL) {
        return;
    }

    if (!e->isContainer() && (e->isSelected()!=painter->shouldDrawSelected())) {
        return;
    }

    e->draw(painter, this, patternOffset);

}

/**
 * Deletes an entity with the background color.
 * Might be recusively called e.g. for polylines.
 */
void RS_GraphicView::deleteEntity(RS_Entity* e) {

	// RVT_PORT When we delete a single entoty, we can do this but we need to remove this then also from containerEntities
	RS_DEBUG->print("RS_GraphicView::deleteEntity will for now redraw the whole screen instead of just deleting the entity");
	setDeleteMode(true);
    drawEntity(e);
	setDeleteMode(false);
	redraw(RS2::RedrawDrawing);
}




/**
 * @return Pointer to the static pattern struct that belongs to the
 * given pattern type or NULL.
 */
RS_LineTypePattern* RS_GraphicView::getPattern(RS2::LineType t) {
    switch (t) {
    case RS2::SolidLine:
        return &patternSolidLine;
        break;

    case RS2::DotLine:
        return &patternDotLine;
        break;
    case RS2::DotLine2:
        return &patternDotLine2;
        break;
    case RS2::DotLineX2:
        return &patternDotLineX2;
        break;

    case RS2::DashLine:
        return &patternDashLine;
        break;
    case RS2::DashLine2:
        return &patternDashLine2;
        break;
    case RS2::DashLineX2:
        return &patternDashLineX2;
        break;

    case RS2::DashDotLine:
        return &patternDashDotLine;
        break;
    case RS2::DashDotLine2:
        return &patternDashDotLine2;
        break;
    case RS2::DashDotLineX2:
        return &patternDashDotLineX2;
        break;

    case RS2::DivideLine:
        return &patternDivideLine;
        break;
    case RS2::DivideLine2:
        return &patternDivideLine2;
        break;
    case RS2::DivideLineX2:
        return &patternDivideLineX2;
        break;

    case RS2::CenterLine:
        return &patternCenterLine;
        break;
    case RS2::CenterLine2:
        return &patternCenterLine2;
        break;
    case RS2::CenterLineX2:
        return &patternCenterLineX2;
        break;

    case RS2::BorderLine:
        return &patternBorderLine;
        break;
    case RS2::BorderLine2:
        return &patternBorderLine2;
        break;
    case RS2::BorderLineX2:
        return &patternBorderLineX2;
        break;

    case RS2::LineByLayer:
        return &patternBlockLine;
        break;
    case RS2::LineByBlock:
        return &patternBlockLine;
        break;
    default:
        break;
    }
    return NULL;
}



/**
 * This virtual method can be overwritten to draw the absolute
 * zero. It's called from within drawIt(). The default implemetation
 * draws a simple red cross on the zero of thge sheet
 * THis function can ONLY be called from within a paintEvent because it will
 * use the painter
 *
 * @see drawIt()
 */
void RS_GraphicView::drawAbsoluteZero(RS_Painter *painter) {

    int zr = 20;

	RS_Pen p(QColor(255,0,0), RS2::Width00, RS2::SolidLine);
	p.setScreenWidth(0);
    painter->setPen(p);
    //painter->setBrush(Qt::NoBrush);

    painter->drawLine(RS_Vector(toGuiX(0.0)-zr,
                                toGuiY(0.0)),
                      RS_Vector(toGuiX(0.0)+zr,
                                toGuiY(0.0)));

    painter->drawLine(RS_Vector(toGuiX(0.0),
                                toGuiY(0.0)-zr),
                      RS_Vector(toGuiX(0.0),
                                toGuiY(0.0)+zr));

}



/**
 * This virtual method can be overwritten to draw the relative
 * zero point. It's called from within drawIt(). The default implemetation
 * draws a simple red round zero point. This is the point that was last created by the user, end of a line for example
 * THis function can ONLY be called from within a paintEvent because it will
 * use the painter
 *
 * @see drawIt()
 */
void RS_GraphicView::drawRelativeZero(RS_Painter *painter) {

    if (relativeZero.valid==false) {
        return;
    }

	RS_Pen p(RS_Color(255, 0, 0), RS2::Width00, RS2::SolidLine);
	p.setScreenWidth(0);
    painter->setPen(p);

    int zr=5;

    painter->drawLine(RS_Vector(toGuiX(relativeZero.x)-zr,
                                toGuiY(relativeZero.y)),
                      RS_Vector(toGuiX(relativeZero.x)+zr,
                                toGuiY(relativeZero.y)));

    painter->drawLine(RS_Vector(toGuiX(relativeZero.x),
                                toGuiY(relativeZero.y)-zr),
                      RS_Vector(toGuiX(relativeZero.x),
                                toGuiY(relativeZero.y)+zr));

    painter->drawCircle(toGui(relativeZero), 5);
}



/**
 * Draws the paper border (for print previews).
 * This function can ONLY be called from within a paintEvent because it will
 * use the painter
 *
 * @see drawIt()
 */
void RS_GraphicView::drawPaper(RS_Painter *painter) {

    if (container==NULL) {
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
    RS_Vector size = graphic->getPaperSize();
    double scale = graphic->getPaperScale();

    RS_Vector v1 = toGui((RS_Vector(0,0)-pinsbase)/scale);
    RS_Vector v2 = toGui((size-pinsbase)/scale);

    // gray background:
    painter->fillRect(0,0, getWidth(), getHeight(),
                      RS_Color(200,200,200));

    // shadow
    painter->fillRect(
        (int)(v1.x)+6, (int)(v1.y)+6,
        (int)((v2.x-v1.x)), (int)((v2.y-v1.y)),
        RS_Color(64,64,64));

    // border:
    painter->fillRect(
        (int)(v1.x), (int)(v1.y),
        (int)((v2.x-v1.x)), (int)((v2.y-v1.y)),
        RS_Color(64,64,64));

    // paper
    painter->fillRect(
        (int)(v1.x)+1, (int)(v1.y)-1,
        (int)((v2.x-v1.x))-2, (int)((v2.y-v1.y))+2,
        RS_Color(255,255,255));

}


/**
 * Draws the grid.
 *
 * @see drawIt()
 */
void RS_GraphicView::drawGrid(RS_Painter *painter) {

    if (grid==NULL || isGridOn()==false) {
        return;
    }


    // draw grid:
    //painter->setPen(Qt::gray);
    painter->setPen(gridColor);

	grid->updatePointArray();
    RS_Vector* pts = grid->getPoints();
    if (pts!=NULL) {
        for (int i=0; i<grid->count(); ++i) {
            painter->drawGridPoint(toGui(pts[i]));
        }
    }

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

    if (grid==NULL || isGridOn()==false /*|| grid->getMetaSpacing()<0.0*/) {
        return;
    }

    RS_Pen pen(metaGridColor,
               RS2::Width00,
               RS2::DotLine);
    painter->setPen(pen);

    // draw meta grid:
    double* mx = grid->getMetaX();
    if (mx!=NULL) {
        for (int i=0; i<grid->countMetaX(); ++i) {
            painter->drawLine(RS_Vector(toGuiX(mx[i]), 0),
                              RS_Vector(toGuiX(mx[i]), getHeight()));
        }
    }
    double* my = grid->getMetaY();
    if (my!=NULL) {
        for (int i=0; i<grid->countMetaY(); ++i) {
            painter->drawLine(RS_Vector(0, toGuiY(my[i])),
                              RS_Vector(getWidth(), toGuiY(my[i])));
        }
    }


}

void RS_GraphicView::drawOverlay(RS_Painter *painter) {
	QList<int> keys=overlayEntities.keys();
	for (int i = 0; i < keys.size(); ++i) {
		if (overlayEntities[i] != NULL) {
			setPenForEntity(painter, overlayEntities[i] );
			drawEntityPlain(painter, overlayEntities[i], 0.0);
		}
	}
}

/**
 * Sets the default snap mode used by newly created actions.
 */
void RS_GraphicView::setDefaultSnapMode(RS_SnapMode sm) {
    defaultSnapMode = sm;
    if (eventHandler!=NULL) {
        eventHandler->setSnapMode(sm);
    }
}



/**
 * Sets a snap restriction (e.g. orthogonal).
 */
void RS_GraphicView::setSnapRestriction(RS2::SnapRestriction sr) {
    defaultSnapRes = sr;

    if (eventHandler!=NULL) {
        eventHandler->setSnapRestriction(sr);
    }
}



/**
 * Translates a vector in real coordinates to a vector in screen coordinates.
 */
RS_Vector RS_GraphicView::toGui(RS_Vector v) {
    return RS_Vector(toGuiX(v.x), toGuiY(v.y), 0.0);
}



/**
 * Translates a real coordinate in X to a screen coordinate X.
 * @param visible Pointer to a boolean which will contain true
 * after the call if the coordinate is within the visible range.
 */
double RS_GraphicView::toGuiX(double x) {
    return x*factor.x + offsetX;
}



/**
 * Translates a real coordinate in Y to a screen coordinate Y.
 */
double RS_GraphicView::toGuiY(double y) {
    return -y*factor.y + getHeight() - offsetY;
}



/**
 * Translates a real coordinate distance to a screen coordinate distance.
 */
double RS_GraphicView::toGuiDX(double d) {
    return d*factor.x;
}



/**
 * Translates a real coordinate distance to a screen coordinate distance.
 */
double RS_GraphicView::toGuiDY(double d) {
    return d*factor.y;
}



/**
 * Translates a vector in screen coordinates to a vector in real coordinates.
 */
RS_Vector RS_GraphicView::toGraph(RS_Vector v) {
    return RS_Vector(toGraphX(RS_Math::round(v.x)),
                     toGraphY(RS_Math::round(v.y)), 0.0);
}



/**
 * Translates two screen coordinates to a vector in real coordinates.
 */
RS_Vector RS_GraphicView::toGraph(int x, int y) {
    return RS_Vector(toGraphX(x), toGraphY(y), 0.0);
}


/**
 * Translates a screen coordinate in X to a real coordinate X.
 */
double RS_GraphicView::toGraphX(int x) {
    return (x - offsetX)/factor.x;
}



/**
 * Translates a screen coordinate in Y to a real coordinate Y.
 */
double RS_GraphicView::toGraphY(int y) {
    return -(y - getHeight() + offsetY)/factor.y;
}



/**
 * Translates a screen coordinate distance to a real coordinate distance.
 */
double RS_GraphicView::toGraphDX(int d) {
    return d/factor.x;
}



/**
 * Translates a screen coordinate distance to a real coordinate distance.
 */
double RS_GraphicView::toGraphDY(int d) {
    return d/factor.y;
}



/**
 * Sets the relative zero coordinate (if not locked)
 * without deleting / drawing the point.
 */
void RS_GraphicView::setRelativeZero(const RS_Vector& pos) {
    if (relativeZeroLocked==false) {
        relativeZero = pos;
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
 * Remove all overlay entities
 */
RS_EntityContainer* RS_GraphicView::getOverlayContainer(RS2::OverlayGraphics position)
{
	if (overlayEntities[position]!=NULL) {
		return overlayEntities[position];
	}
	overlayEntities[position]=new RS_EntityContainer(NULL);

	return overlayEntities[position];

}


