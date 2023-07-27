/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2015 Dongxu Li (dongxuli2011@gmail.com)
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

#ifndef RS_SNAPPER_H
#define RS_SNAPPER_H

#include <memory>
#include <QList>
#include "rs.h"

class RS_Entity;
class RS_GraphicView;
class RS_Vector;
class RS_Preview;
class QMouseEvent;
class RS_EntityContainer;

/**
  * This class holds information on how to snap the mouse.
  *
  * @author Kevin Cox
  */
struct RS_SnapMode {
    /* SnapModes for RS_SnapMode to Int conversion and vice versa
     *
     * The conversion is only used for save/restore of active snap modes in application settings.
     * Don't change existing mode order, because this will mess up settings on upgrades
     *
     * When adding new values, take care for correct implementation in \p toInt() and \p fromInt()
     */
    enum SnapModes {
        SnapIntersection    = 1 << 0,
        SnapOnEntity        = 1 << 1,
        SnapCenter          = 1 << 2,
        SnapDistance        = 1 << 3,
        SnapMiddle          = 1 << 4,
        SnapEndpoint        = 1 << 5,
        SnapGrid            = 1 << 6,
        SnapFree            = 1 << 7,
        RestrictHorizontal  = 1 << 8,
        RestrictVertical    = 1 << 9,
        RestrictOrthogonal  = RestrictHorizontal | RestrictVertical,
        SnapAngle           = 1 << 10
    };

    bool snapIntersection   {false}; //< Whether to snap to intersections or not.
    bool snapOnEntity       {false}; //< Whether to snap to entities or not.
    bool snapCenter         {false}; //< Whether to snap to centers or not.
    bool snapDistance       {false}; //< Whether to snap to distance from endpoints or not.
    bool snapMiddle         {false}; //< Whether to snap to midpoints or not.
    bool snapEndpoint       {false}; //< Whether to snap to endpoints or not.
    bool snapGrid           {false}; //< Whether to snap to grid or not.
    bool snapFree           {false}; //< Whether to snap freely
    bool snapAngle          {false}; //< Whether to snap along line under certain angle

    RS2::SnapRestriction restriction {RS2::RestrictNothing}; /// The restriction on the free snap.

    double distance {5.0}; //< The distance to snap before defaulting to free snapping.

    /**
      * Disable all snapping.
      *
      * This effectively puts the object into free snap mode.
      *
      * @returns A reference to itself.
      */
    RS_SnapMode const & clear(void);
    bool operator == (RS_SnapMode const& rhs) const;

    static unsigned toInt(const RS_SnapMode& s);    //< convert to int, to save settings
    static RS_SnapMode fromInt(unsigned int);   //< convert from int, to restore settings
};

using EntityTypeList = QList<RS2::EntityType>;

/**
 * This class is used for snapping functions in a graphic view.
 * Actions are usually derived from this base class if they need
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
	//!
	//! \brief finish stop using snapper
	//!
    void finish();

    /**
     * @return Pointer to the entity which was the key entity for the
     * last successful snapping action. If the snap mode is "end point"
     * the key entity is the entity whose end point was caught.
     * If the snap mode didn't require an entity (e.g. free, grid) this
     * method will return NULL.
     */
	RS_Entity* getKeyEntity() const {
        return keyEntity;
    }

    /** Sets a new snap mode. */
    void setSnapMode(const RS_SnapMode& snapMode);

	RS_SnapMode const* getSnapMode() const;
	RS_SnapMode* getSnapMode();

    /** Sets a new snap restriction. */
    void setSnapRestriction(RS2::SnapRestriction /*snapRes*/) {
        //this->snapRes = snapRes;
    }

        /**
         * Sets the snap range in pixels for catchEntity().
         *
         * @see catchEntity()
         */
        void setSnapRange(int r) {
                catchEntityGuiRange = r;
        }

        /**manually set snapPoint*/
    RS_Vector snapPoint(const RS_Vector& coord, bool setSpot = false);
    RS_Vector snapPoint(QMouseEvent* e);
    RS_Vector snapFree(QMouseEvent* e);

    RS_Vector snapFree(const RS_Vector& coord);
    RS_Vector snapGrid(const RS_Vector& coord);
    RS_Vector snapEndpoint(const RS_Vector& coord);
    RS_Vector snapOnEntity(const RS_Vector& coord);
    RS_Vector snapCenter(const RS_Vector& coord);
    RS_Vector snapMiddle(const RS_Vector& coord);
    RS_Vector snapDist(const RS_Vector& coord);
    RS_Vector snapIntersection(const RS_Vector& coord);
    //RS_Vector snapDirect(RS_Vector coord, bool abs);
    RS_Vector snapToAngle(const RS_Vector &coord, const RS_Vector &ref_coord, const double ang_res);

    RS_Vector restrictOrthogonal(const RS_Vector& coord);
    RS_Vector restrictHorizontal(const RS_Vector& coord);
    RS_Vector restrictVertical(const RS_Vector& coord);


    //RS_Entity* catchLeafEntity(const RS_Vector& pos);
    //RS_Entity* catchLeafEntity(QMouseEvent* e);
    RS_Entity* catchEntity(const RS_Vector& pos,
                           RS2::ResolveLevel level=RS2::ResolveNone);
    RS_Entity* catchEntity(QMouseEvent* e,
                           RS2::ResolveLevel level=RS2::ResolveNone);
    // catch Entity closest to pos and of the given entity type of enType, only search for a particular entity type
    RS_Entity* catchEntity(const RS_Vector& pos, RS2::EntityType enType,
                           RS2::ResolveLevel level=RS2::ResolveNone);
    RS_Entity* catchEntity(QMouseEvent* e, RS2::EntityType enType,
                           RS2::ResolveLevel level=RS2::ResolveNone);
	RS_Entity* catchEntity(QMouseEvent* e, const EntityTypeList& enTypeList,
                           RS2::ResolveLevel level=RS2::ResolveNone);

    /**
     * Suspends this snapper while another action takes place.
     */
	virtual void suspend();

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
    double getSnapRange() const;
    RS_EntityContainer *container = nullptr;
    RS_GraphicView *graphicView = nullptr;
    RS_Entity *keyEntity = nullptr;
    RS_SnapMode snapMode{};
    //RS2::SnapRestriction snapRes;
    /**
     * Snap distance for snapping to points with a
     * given distance from endpoints.
     */
    double m_SnapDistance = 1.;
    /**
     * Snap to equidistant middle points
     * default to 1, i.e., equidistant to start/end points
     */
    int middlePoints = 1;
    /**
     * Snap range for catching entities. In GUI units
     */
    int catchEntityGuiRange = 32;
    bool finished{false};

private:
	struct ImpData;
	std::unique_ptr<ImpData> pImpData;

    struct Indicator;
    std::unique_ptr<Indicator> snap_indicator;
};

#endif
//EOF
