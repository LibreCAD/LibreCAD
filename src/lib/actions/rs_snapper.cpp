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


#include "rs_snapper.h"

#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_grid.h"
#include "rs_settings.h"
#include "rs_overlayline.h"

/**
 * Constructor.
 */
RS_Snapper::RS_Snapper(RS_EntityContainer& container,
                       RS_GraphicView& graphicView) {
                       RS_DEBUG->print("RS_Snapper::RS_Snapper()");
    this->container = &container;
    this->graphicView = &graphicView;
    finished = false;
    init();
}



/**
 * Destructor.
 */
RS_Snapper::~RS_Snapper() {}



/**
 * Initialize (called by all constructors)
 */
void RS_Snapper::init() {
    snapMode = graphicView->getDefaultSnapMode();
    //snapRes = graphicView->getSnapRestriction();
    keyEntity = NULL;
    snapSpot = RS_Vector(false);
    snapCoord = RS_Vector(false);
    distance = 1.0;
    RS_SETTINGS->beginGroup("/Snap");
    snapRange = RS_SETTINGS->readNumEntry("/Range", 20);
    //middlePoints behaviors weird, add brutal force here
    //todo, clean up middlePoints
    //middlePoints= RS_SETTINGS->readNumEntry("/MiddlePoints", 1);
    //distance=RS_SETTINGS->readEntry("/Distance", QString("1")).toDouble();
    RS_SETTINGS->endGroup();
    RS_SETTINGS->beginGroup("/Appearance");
    showCrosshairs = (bool)RS_SETTINGS->readNumEntry("/ShowCrosshairs", 1);
    RS_SETTINGS->endGroup();
    if (snapRange<2) {
        snapRange = 20;
    }
}

void RS_Snapper::finish() {
    finished = true;
    deleteSnapper();
}

void RS_Snapper::setSnapMode(RS_SnapMode snapMode) {
    this->snapMode = snapMode;
    if (RS_DIALOGFACTORY==NULL) return;
    RS_DIALOGFACTORY->requestSnapDistOptions(distance, snapMode.snapDistance);
    RS_DIALOGFACTORY->requestSnapMiddleOptions(middlePoints, snapMode.snapMiddle);
//std::cout<<"RS_Snapper::setSnapMode(): middlePoints="<<middlePoints<<std::endl;
}
//get current mouse coordinates
RS_Vector RS_Snapper::snapFree(QMouseEvent* e) {
    if (e==NULL) {
                RS_DEBUG->print(RS_Debug::D_WARNING,
                        "RS_Snapper::snapFree: event is NULL");
        return RS_Vector(false);
    }
    snapSpot=graphicView->toGraph(e->x(), e->y());
    snapCoord=snapSpot;
    showCrosshairs=true;
    return snapCoord;
}

/**
 * Snap to a coordinate in the drawing using the current snap mode.
 *
 * @param e A mouse event.
 * @return The coordinates of the point or an invalid vector.
 */
RS_Vector RS_Snapper::snapPoint(QMouseEvent* e) {
        RS_DEBUG->print("RS_Snapper::snapPoint");

    snapSpot = RS_Vector(false);
    RS_Vector t(false);

    if (e==NULL) {
                RS_DEBUG->print(RS_Debug::D_WARNING,
                        "RS_Snapper::snapPoint: event is NULL");
        return snapSpot;
    }

    RS_Vector mouseCoord = graphicView->toGraph(e->x(), e->y());

    if (snapMode.snapEndpoint) {
        t = snapEndpoint(mouseCoord);

        if (mouseCoord.distanceTo(t) < mouseCoord.distanceTo(snapSpot))
            snapSpot = t;
    }
    if (snapMode.snapCenter) {
        t = snapCenter(mouseCoord);

        if (mouseCoord.distanceTo(t) < mouseCoord.distanceTo(snapSpot))
            snapSpot = t;
    }
    if (snapMode.snapMiddle) {
        //this is still brutal force
        //todo: accept value from widget QG_SnapMiddleOptions
        if(RS_DIALOGFACTORY != NULL) {
            RS_DIALOGFACTORY->requestSnapMiddleOptions(middlePoints, snapMode.snapMiddle);
        }
        t = snapMiddle(mouseCoord);

        if (mouseCoord.distanceTo(t) < mouseCoord.distanceTo(snapSpot))
            snapSpot = t;
    }
    if (snapMode.snapDistance) {
        //this is still brutal force
        //todo: accept value from widget QG_SnapDistOptions
        if(RS_DIALOGFACTORY != NULL) {
            RS_DIALOGFACTORY->requestSnapDistOptions(distance, snapMode.snapDistance);
        }
        t = snapDist(mouseCoord);

        if (mouseCoord.distanceTo(t) < mouseCoord.distanceTo(snapSpot))
            snapSpot = t;
    }
    if (snapMode.snapIntersection) {
        t = snapIntersection(mouseCoord);

        if (mouseCoord.distanceTo(t) < mouseCoord.distanceTo(snapSpot))
            snapSpot = t;
    }

    if (snapMode.snapOnEntity &&
        snapSpot.distanceTo(mouseCoord) > snapMode.distance) {
        t = snapOnEntity(mouseCoord);

        if (mouseCoord.distanceTo(t) < mouseCoord.distanceTo(snapSpot))
            snapSpot = t;
    }

    if (snapMode.snapGrid) {
        t = snapGrid(mouseCoord);

        if (mouseCoord.distanceTo(t) < mouseCoord.distanceTo(snapSpot))
            snapSpot = t;
    }

    if( ! snapSpot.valid ) {
        snapSpot=mouseCoord; //default to snapFree
    } else {
        if (mouseCoord.distanceTo(snapSpot) > snapRange ) snapSpot = mouseCoord;
    }
    //if (snapSpot.distanceTo(mouseCoord) > snapMode.distance) {
    // handle snap restrictions that can be activated in addition
    //   to the ones above:
    //apply restriction
    RS_Vector rz = graphicView->getRelativeZero();
    RS_Vector vpv(rz.x,snapSpot.y);
    RS_Vector vph(snapSpot.x,rz.y);
    switch (snapMode.restriction) {
    case RS2::RestrictOrthogonal:
        snapCoord= ( mouseCoord.distanceTo(vpv)< mouseCoord.distanceTo(vph))?
                    vpv:vph;
        break;
    case RS2::RestrictHorizontal:
        snapCoord = vph;
        break;
    case RS2::RestrictVertical:
        snapCoord = vpv;
        break;

    //case RS2::RestrictNothing:
    default:
        snapCoord = snapSpot;
        break;
    }
    //}
    //else snapCoord = snapSpot;

    drawSnapper();

    if (RS_DIALOGFACTORY!=NULL) {
        RS_DIALOGFACTORY->updateCoordinateWidget(snapCoord,
                snapCoord - graphicView->getRelativeZero());
    }

        RS_DEBUG->print("RS_Snapper::snapPoint: OK");

    return snapCoord;
}



/**
 * Snaps to a free coordinate.
 *
 * @param coord The mouse coordinate.
 * @return The coordinates of the point or an invalid vector.
 */
RS_Vector RS_Snapper::snapFree(RS_Vector coord) {
    keyEntity = NULL;
    return coord;
}



/**
 * Snaps to the closest endpoint.
 *
 * @param coord The mouse coordinate.
 * @return The coordinates of the point or an invalid vector.
 */
RS_Vector RS_Snapper::snapEndpoint(RS_Vector coord) {
    RS_Vector vec(false);

    vec = container->getNearestEndpoint(coord,
                                        NULL/*, &keyEntity*/);
    return vec;
}



/**
 * Snaps to a grid point.
 *
 * @param coord The mouse coordinate.
 * @return The coordinates of the point or an invalid vector.
 */
RS_Vector RS_Snapper::snapGrid(const RS_Vector& coord) {

//    RS_DEBUG->print("RS_Snapper::snapGrid begin");

//    RS_Vector vec(false);
//    double dist=0.0;

//    RS_Grid* grid = graphicView->getGrid();
    return  graphicView->getGrid()->snapGrid(coord);


//    if (grid!=NULL) {
//        RS_Vector* pts = grid->getPoints();
//        int closest = -1;
//        dist = 32000.00;
//        for (int i=0; i<grid->count(); ++i) {
//            double d = pts[i].distanceTo(coord);
//            if (d<dist) {
//                closest = i;
//                dist = d;
//            }
//        }
//        if (closest>=0) {
//            vec = pts[closest];
//        }
//    }else {
//        RS_DEBUG->print("RS_Snapper:: getGrid() returns NULL\n");
//    }
//    keyEntity = NULL;

//    RS_DEBUG->print("RS_Snapper::snapGrid end");

//    return vec;
}



/**
 * Snaps to a point on an entity.
 *
 * @param coord The mouse coordinate.
 * @return The coordinates of the point or an invalid vector.
 */
RS_Vector RS_Snapper::snapOnEntity(RS_Vector coord) {

    RS_Vector vec(false);
    vec = container->getNearestPointOnEntity(coord, true, NULL, &keyEntity);
    return vec;
}



/**
 * Snaps to the closest center.
 *
 * @param coord The mouse coordinate.
 * @return The coordinates of the point or an invalid vector.
 */
RS_Vector RS_Snapper::snapCenter(RS_Vector coord) {
    RS_Vector vec(false);

    vec = container->getNearestCenter(coord, NULL);
    return vec;
}



/**
 * Snaps to the closest middle.
 *
 * @param coord The mouse coordinate.
 * @return The coordinates of the point or an invalid vector.
 */
RS_Vector RS_Snapper::snapMiddle(RS_Vector coord) {
//std::cout<<"RS_Snapper::snapMiddle(): middlePoints="<<middlePoints<<std::endl;
    return container->getNearestMiddle(coord,static_cast<double *>(NULL),middlePoints);
}



/**
 * Snaps to the closest point with a given distance to the endpoint.
 *
 * @param coord The mouse coordinate.
 * @return The coordinates of the point or an invalid vector.
 */
RS_Vector RS_Snapper::snapDist(RS_Vector coord) {
    RS_Vector vec;

//std::cout<<" RS_Snapper::snapDist(RS_Vector coord): distance="<<distance<<std::endl;
    vec = container->getNearestDist(distance,
                                    coord,
                                    NULL);
    return vec;
}



/**
 * Snaps to the closest intersection point.
 *
 * @param coord The mouse coordinate.
 * @return The coordinates of the point or an invalid vector.
 */
RS_Vector RS_Snapper::snapIntersection(RS_Vector coord) {
    RS_Vector vec(false);

    vec = container->getNearestIntersection(coord,
                                            NULL);
    return vec;
}



/**
 * 'Corrects' the given coordinates to 0, 90, 180, 270 degrees relative to
 * the current relative zero point.
 *
 * @param coord The uncorrected coordinates.
 * @return The corrected coordinates.
 */
RS_Vector RS_Snapper::restrictOrthogonal(RS_Vector coord) {
    RS_Vector rz = graphicView->getRelativeZero();
    RS_Vector ret(coord);

    RS_Vector retx = RS_Vector(rz.x, ret.y);
    RS_Vector rety = RS_Vector(ret.x, rz.y);

    if (retx.distanceTo(ret) > rety.distanceTo(ret)) {
        ret = rety;
    } else {
        ret = retx;
    }

    return ret;
}

/**
 * 'Corrects' the given coordinates to 0, 180 degrees relative to
 * the current relative zero point.
 *
 * @param coord The uncorrected coordinates.
 * @return The corrected coordinates.
 */
RS_Vector RS_Snapper::restrictHorizontal(RS_Vector coord) {
    RS_Vector rz = graphicView->getRelativeZero();
    RS_Vector ret = RS_Vector(coord.x, rz.y);
    return ret;
}


/**
 * 'Corrects' the given coordinates to 90, 270 degrees relative to
 * the current relative zero point.
 *
 * @param coord The uncorrected coordinates.
 * @return The corrected coordinates.
 */
RS_Vector RS_Snapper::restrictVertical(RS_Vector coord) {
    RS_Vector rz = graphicView->getRelativeZero();
    RS_Vector ret = RS_Vector(rz.x, coord.y);
    return ret;
}


/**
 * Catches an entity which is close to the given position 'pos'.
 *
 * @param pos A graphic coordinate.
 * @param level The level of resolving for iterating through the entity
 *        container
 * @return Pointer to the entity or NULL.
 */
RS_Entity* RS_Snapper::catchEntity(const RS_Vector& pos,
                                   RS2::ResolveLevel level) {

    RS_DEBUG->print("RS_Snapper::catchEntity");

        // set default distance for points inside solids
    double dist = graphicView->toGraphDX(snapRange)*0.9;

    RS_Entity* entity = container->getNearestEntity(pos, &dist, level);

        int idx = -1;
        if (entity!=NULL && entity->getParent()!=NULL) {
                idx = entity->getParent()->findEntity(entity);
        }

    if (entity!=NULL && dist<=graphicView->toGraphDX(snapRange)) {
        // highlight:
        RS_DEBUG->print("RS_Snapper::catchEntity: found: %d", idx);
        return entity;
    } else {
        RS_DEBUG->print("RS_Snapper::catchEntity: not found");
        return NULL;
    }
    RS_DEBUG->print("RS_Snapper::catchEntity: OK");
}



/**
 * Catches an entity which is close to the mouse cursor.
 *
 * @param e A mouse event.
 * @param level The level of resolving for iterating through the entity
 *        container
 * @return Pointer to the entity or NULL.
 */
RS_Entity* RS_Snapper::catchEntity(QMouseEvent* e,
                                   RS2::ResolveLevel level) {

    return catchEntity(
               RS_Vector(graphicView->toGraphX(e->x()),
                         graphicView->toGraphY(e->y())),
               level);
}



/**
 * Hides the snapper options. Default implementation does nothing.
 */
void RS_Snapper::hideOptions() {
    //not used any more, will be removed
}

/**
 * Shows the snapper options. Default implementation does nothing.
 */
void RS_Snapper::showOptions() {
    //not used any more, will be removed
}


/**
 * Deletes the snapper from the screen.
 */
void RS_Snapper::deleteSnapper() {// RVT_PORT (can be deleted??)
        RS_DEBUG->print("RS_Snapper::Delete Snapper");

        graphicView->getOverlayContainer(RS2::Snapper)->clear();
        graphicView->redraw(RS2::RedrawOverlay); // redraw will happen in the mouse movement event
}



/**
 * We could properly speed this up by calling the draw function of this snapper within the paint event
 * this will avoid creating/deletion of the lines
 */
void RS_Snapper::drawSnapper() {
        graphicView->getOverlayContainer(RS2::Snapper)->clear();
    if (!finished && snapSpot.valid) {
                RS_EntityContainer *container=graphicView->getOverlayContainer(RS2::Snapper);
                RS_Pen crossHairPen(RS_Color(255,194,0), RS2::Width00, RS2::DashLine2);

        if (snapCoord.valid) {
                        RS_DEBUG->print("RS_Snapper::Snapped draw start");
                        // Pen for snapper
                        RS_Pen pen(RS_Color(255,194,0), RS2::Width00, RS2::SolidLine);
                        pen.setScreenWidth(1);

                        // Circle to show snap area
                        RS_Circle *circle=new RS_Circle(NULL, RS_CircleData(snapCoord, 4/graphicView->getFactor().x));
                        circle->setPen(pen);

                        container->addEntity(circle);

            // crosshairs:
            if (showCrosshairs==true) {


                                RS_OverlayLine *line=new RS_OverlayLine(NULL, RS_LineData(RS_Vector(0, graphicView->toGuiY(snapCoord.y)),
                                                                                                                RS_Vector(graphicView->getWidth(), graphicView->toGuiY(snapCoord.y))));
                                line->setPen(crossHairPen);
                                container->addEntity(line);

                                line=new RS_OverlayLine(NULL, RS_LineData(RS_Vector(graphicView->toGuiX(snapCoord.x),0),
                                                                                                                RS_Vector(graphicView->toGuiX(snapCoord.x), graphicView->getHeight())));
                                line->setPen(crossHairPen);
                                container->addEntity(line);

            }
                        graphicView->redraw(RS2::RedrawOverlay); // redraw will happen in the mouse movement event
                        RS_DEBUG->print("RS_Snapper::Snapped draw end");
        }
        if (snapCoord.valid && snapCoord!=snapSpot) {

                        RS_OverlayLine *line=new RS_OverlayLine(NULL, RS_LineData(graphicView->toGui(snapSpot)+RS_Vector(-5,0),
                                                                                                                                          graphicView->toGui(snapSpot)+RS_Vector(-1,4)));
                        line->setPen(crossHairPen);
                        container->addEntity(line);
                        line=new RS_OverlayLine(NULL, RS_LineData(graphicView->toGui(snapSpot)+RS_Vector(0,5),
                                                                                                                                          graphicView->toGui(snapSpot)+RS_Vector(4,1)));
                        line->setPen(crossHairPen);
                        container->addEntity(line);
                        line=new RS_OverlayLine(NULL, RS_LineData(graphicView->toGui(snapSpot)+RS_Vector(5,0),
                                                                                                                                          graphicView->toGui(snapSpot)+RS_Vector(1,-4)));
                        line->setPen(crossHairPen);
                        container->addEntity(line);
                        line=new RS_OverlayLine(NULL, RS_LineData(graphicView->toGui(snapSpot)+RS_Vector(0,-5),
                                                                                                                                          graphicView->toGui(snapSpot)+RS_Vector(-4,-1)));
                        line->setPen(crossHairPen);
                        container->addEntity(line);

                        graphicView->redraw(RS2::RedrawOverlay); // redraw will happen in the mouse movement event
        }
    }

}

/**
  * snap mode to a flag integer
  */
unsigned int RS_Snapper::snapModeToInt(RS_SnapMode s)
{
    unsigned int ret; //initial
    switch (s.restriction) {
    case RS2::RestrictHorizontal:
        ret=1;
        break;
    case RS2::RestrictVertical:
        ret=2;
        break;
    case RS2::RestrictOrthogonal:
        ret=3;
        break;
    default:
        ret=0;
    }
    ret <<=1;ret |= s.snapGrid;
    ret <<=1;ret |= s.snapEndpoint;
    ret <<=1;ret |= s.snapMiddle;
    ret <<=1;ret |= s.snapDistance;
    ret <<=1;ret |= s.snapCenter;
    ret <<=1;ret |= s.snapOnEntity;
    ret <<=1;ret |= s.snapIntersection;
   return ret;
}
/**
  * integer flag to snapMode
  */
RS_SnapMode RS_Snapper::intToSnapMode(unsigned int ret)
{
    RS_SnapMode s; //initial
    unsigned int binaryOne(0x1);
    s.snapIntersection =ret & binaryOne;
    ret >>= 1;
    s.snapOnEntity =ret & binaryOne;
    ret >>= 1;
    s.snapCenter =ret & binaryOne;
    ret >>= 1;
    s.snapDistance =ret & binaryOne;
    ret >>= 1;
    s.snapMiddle =ret & binaryOne;
    ret >>= 1;
    s.snapEndpoint =ret & binaryOne;
    ret >>= 1;
    s.snapGrid =ret & binaryOne;
    ret >>= 1;
    switch (ret) {
    case 1:
            s.restriction=RS2::RestrictHorizontal;
        break;
    case 2:
            s.restriction=RS2::RestrictVertical;
        break;
    case 3:
            s.restriction=RS2::RestrictOrthogonal;
        break;
    default:
            s.restriction=RS2::RestrictNothing;
    }
   return s;
}
