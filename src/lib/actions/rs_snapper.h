/****************************************************************************
**
** This file is part of the CADuntu project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (caduntu@rvt.dds.nl)
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


#ifndef RS_SNAPPER_H
#define RS_SNAPPER_H

#include "rs_entitycontainer.h"

#include "rs.h"
#include "rs_mouseevent.h"
#include "rs_coordinateevent.h"

class RS_Entity;
class RS_GraphicView;
class RS_Vector;
class RS_Preview;

/**
 * This class is used for snapping functions in a graphic view. 
 * Actions are usually derrived from this base class if they need
 * to catch entities or snap to coordinates. Use the methods to
 * retrieve a graphic coordinate from a mouse coordinate.
 *
 * Possible snapping functions are described in RS_SnapMode.
 *
 * @author Andrew Mustun
 */
class RS_Snapper {
public:
    RS_Snapper(RS_EntityContainer& container, RS_GraphicView& graphicView);
    virtual ~RS_Snapper();

    void init();
	void finish();

    /**
     * @return Pointer to the entity which was the key entity for the
     * last successful snapping action. If the snap mode is "end point"
     * the key entity is the entity whos end point was caught.
     * If the snap mode didn't require an entity (e.g. free, grid) this
     * method will return NULL.
     */
    RS_Entity* getKeyEntity() {
        return keyEntity;
    }

    /** Sets a new snap mode. */
    void setSnapMode(RS2::SnapMode snapMode) {
        this->snapMode = snapMode;
    }
    /** Sets a new snap restriction. */
    void setSnapRestriction(RS2::SnapRestriction snapRes) {
        this->snapRes = snapRes;
    }

	/** 
	 * Sets the snap range in pixels for catchEntity().
	 *
	 * @see catchEntity()
	 */
	void setSnapRange(int r) {
		snapRange = r;
	}

    RS_Vector snapPoint(RS_MouseEvent* e);

    RS_Vector snapFree(RS_Vector coord);
    RS_Vector snapEndpoint(RS_Vector coord);
    RS_Vector snapGrid(RS_Vector coord);
    RS_Vector snapOnEntity(RS_Vector coord);
    RS_Vector snapCenter(RS_Vector coord);
    RS_Vector snapMiddle(RS_Vector coord);
    RS_Vector snapDist(RS_Vector coord);
    RS_Vector snapIntersection(RS_Vector coord);
    //RS_Vector snapDirect(RS_Vector coord, bool abs);
	
    RS_Vector restrictOrthogonal(RS_Vector coord);
    RS_Vector restrictHorizontal(RS_Vector coord);
    RS_Vector restrictVertical(RS_Vector coord);

    //RS_Entity* catchLeafEntity(const RS_Vector& pos);
    //RS_Entity* catchLeafEntity(RS_MouseEvent* e);
    RS_Entity* catchEntity(const RS_Vector& pos,
                           RS2::ResolveLevel level=RS2::ResolveNone);
    RS_Entity* catchEntity(RS_MouseEvent* e,
                           RS2::ResolveLevel level=RS2::ResolveNone);

    /**
     * Suspends this snapper while another action takes place.
     */
    virtual void suspend() {
		// RVT Don't delete the snapper here!
        // RVT_PORT (can be deleted)();
        snapSpot = snapCoord = RS_Vector(false);
    }

    /**
     * Resumes this snapper after it has been suspended.
     */
    virtual void resume() {
        drawSnapper();
    }

    virtual void hideOptions();
    virtual void showOptions();

    void drawSnapper();

protected:
    void deleteSnapper();
    RS_EntityContainer* container;
    RS_GraphicView* graphicView;
    RS_Entity* keyEntity;
    RS_Vector snapCoord;
    RS_Vector snapSpot;
    RS2::SnapMode snapMode;
    RS2::SnapRestriction snapRes;
    /**
     * Snap distance for snaping to points with a 
     * given distance from endpoints.
     */
    double distance;
	/**
	 * Snap range for catching entities.
	 */
	int snapRange;
	/**
	 * Show large cross hairs.
	 */
	bool showCrosshairs;
	bool finished;
};

#endif
