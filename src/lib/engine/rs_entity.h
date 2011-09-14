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


#ifndef RS_ENTITY_H
#define RS_ENTITY_H

#include <QMultiHash>

#include "rs_math.h"
#include "rs_pen.h"
#include "rs_undoable.h"
#include "rs_vector.h"

class RS_Arc;
class RS_Block;
class RS_Circle;
class RS_Document;
class RS_EntityContainer;
class RS_Graphic;
class RS_GraphicView;
class RS_Insert;
class RS_Line;
class RS_Painter;
class RS_Point;
class RS_Polyline;
class RS_Text;
class RS_Layer;



/**
 * Base class for an entity (line, arc, circle, ...)
 *
 * @author Andrew Mustun
 */
class RS_Entity : public RS_Undoable {
public:


    RS_Entity(RS_EntityContainer* parent=NULL);
    virtual ~RS_Entity();

    void init();
    virtual void initId();

    virtual RS_Entity* clone() = 0;

    virtual void reparent(RS_EntityContainer* parent) {
        this->parent = parent;
    }

    void resetBorders();

    /**
     * Must be overwritten to return the rtti of this entity
     * (e.g. RS2::EntityArc).
     */
    virtual RS2::EntityType rtti() const {
        return RS2::EntityUnknown;
    }

    /**
     * Identify all entities as undoable entities.
     * @return RS2::UndoableEntity
     */
    virtual RS2::UndoableType undoRtti() {
        return RS2::UndoableEntity;
    }

    /**
     * @return Unique Id of this entity.
     */
    unsigned long int getId() const {
        return id;
    }
	
    /**
     * This method must be overwritten in subclasses and return the 
     * number of <b>atomic</b> entities in this entity.
     */
    virtual unsigned long int count() = 0;
	
    /**
     * This method must be overwritten in subclasses and return the 
     * number of <b>atomic</b> entities in this entity including sub containers.
     */
    virtual unsigned long int countDeep() = 0;


	/**
	 * Implementations must return the total length of the entity
	 * or a negative number if the entity has no length (e.g. a text or hatch).
	 */
	virtual double getLength() {
		return -1.0;
	}

    /**
     * @return Parent of this entity or NULL if this is a root entity.
     */
    RS_EntityContainer* getParent() const {
        return parent;
    }

    /**
     * Reparents this entity.
     */
    void setParent(RS_EntityContainer* p) {
        parent = p;
    }

    RS_Graphic* getGraphic();
    RS_Block* getBlock();
    RS_Insert* getInsert();
    RS_Entity* getBlockOrInsert();
    RS_Document* getDocument();

    void setLayer(const QString& name);
    void setLayer(RS_Layer* l);
    void setLayerToActive();
    RS_Layer* getLayer(bool resolve = true) const;

    /**
     * Sets the explicit pen for this entity or a pen with special
     * attributes such as BY_LAYER, ..
     */
    void setPen(const RS_Pen& pen) {
        this->pen = pen;
    }
	
	
    void setPenToActive();
    RS_Pen getPen(bool resolve = true) const;

    /**
     * Must be overwritten to return true if an entity type 
     * is a container for other entities (e.g. polyline, group, ...).
     */
    virtual bool isContainer() const = 0;

    /**
     * Must be overwritten to return true if an entity type 
     * is an atomic entity.
     */
    virtual bool isAtomic() const = 0;

    /**
     * Must be overwritten to return true if an entity type 
     * is a potential edge entity of a contour. By default
    * this returns false.
     */
    virtual bool isEdge() const {
        return false;
    }

    /**
     * @return true for all document entities (e.g. Graphics or Blocks).
     * false otherwise.
     */
    virtual bool isDocument() const {
        return false;
    }

    virtual bool setSelected(bool select);
    virtual bool toggleSelected();
    virtual bool isSelected() const;
	virtual bool isParentSelected();
    virtual bool isProcessed() const;
    virtual void setProcessed(bool on);
    virtual bool isInWindow(RS_Vector v1, RS_Vector v2);
    virtual bool hasEndpointsWithinWindow(RS_Vector /*v1*/, RS_Vector /*v2*/) {
        return false;
    }
    virtual bool isVisible();
    virtual void setVisible(bool v) {
        if (v) {
            setFlag(RS2::FlagVisible);
        } else {
            delFlag(RS2::FlagVisible);
        }
    }
	virtual void setHighlighted(bool on);
	virtual bool isHighlighted();
	
    virtual bool isLocked();

    virtual void undoStateChanged(bool undone);
    virtual bool isUndone() const;

    /**
     * Can be implemented by child classes to update the entities
     * temporary subentities. update() is called if the entity's
     * paramters or undo state changed.
     */
    virtual void update() {}
	
    virtual void setUpdateEnabled(bool on) {
		updateEnabled = on;
	}

    /**
     * This method doesn't do any calculations. 
     * @return minimum coordinate of the entity.
     * @see calculateBorders()
     */
    RS_Vector getMin() const {
        return minV;
    }

    /**
     * This method doesn't do any calculations. 
     * @return minimum coordinate of the entity.
     * @see calculateBorders()
     */
    RS_Vector getMax() const {
        return maxV;
    }

    /**
     * This method returns the difference of max and min returned 
     * by the above functions. 
     * @return size of the entity.
     * @see calculateBorders()
     * @see getMin()
     * @see getMax()
     */
    RS_Vector getSize() const {
        return maxV-minV;
    }

    void addGraphicVariable(const QString& key, double val, int code);
    void addGraphicVariable(const QString& key, int val, int code);
    void addGraphicVariable(const QString& key, const QString& val, int code);
	
    double getGraphicVariableDouble(const QString& key, double def);
    int getGraphicVariableInt(const QString& key, int def);
    QString getGraphicVariableString(const QString& key,
                                       const QString& def);

	RS2::Unit getGraphicUnit();

    /**
     * Must be overwritten to get all reference points of the entity. 
     */
    virtual RS_VectorSolutions getRefPoints() {
		RS_VectorSolutions ret;
		return ret;
	}


    /**
     * Must be overwritten to get the closest endpoint to the 
     * given coordinate for this entity. 
     *
     * @param coord Coordinate (typically a mouse coordinate)
     * @param dist Pointer to a value which will contain the measured
     * distance between 'coord' and the closest endpoint. The passed
     * pointer can also be NULL in which case the distance will be
     * lost.
     *
     * @return The closest endpoint.
     */
    virtual RS_Vector getNearestEndpoint(const RS_Vector& coord,
                                         double* dist = NULL) = 0;

    /**
     * Must be overwritten to get the closest coordinate to the 
    * given coordinate which is on this entity.
     *
     * @param coord Coordinate (typically a mouse coordinate)
     * @param dist Pointer to a value which will contain the measured
     * distance between \p coord and the point. The passed pointer can 
     * also be \p NULL in which case the distance will be lost.
     *
     * @return The closest coordinate.
     */
    virtual RS_Vector getNearestPointOnEntity(const RS_Vector& /*coord*/,
            bool onEntity = true, double* dist = NULL,
            RS_Entity** entity = NULL) = 0;

    /**
     * Must be overwritten to get the (nearest) center point to the 
     * given coordinate for this entity. 
     *
     * @param coord Coordinate (typically a mouse coordinate)
     * @param dist Pointer to a value which will contain the measured
     * distance between 'coord' and the closest center point. The passed
     * pointer can also be NULL in which case the distance will be
     * lost.
     *
     * @return The closest center point.
     */
    virtual RS_Vector getNearestCenter(const RS_Vector& coord,
                                       double* dist = NULL) = 0;

    /**
     * Must be overwritten to get the (nearest) middle point to the 
     * given coordinate for this entity. 
     *
     * @param coord Coordinate (typically a mouse coordinate)
     * @param dist Pointer to a value which will contain the measured
     * distance between 'coord' and the closest middle point. The passed
     * pointer can also be NULL in which case the distance will be
     * lost.
     *
     * @return The closest middle point.
     */
    virtual RS_Vector getMiddlePoint(void){
            return RS_Vector(false);
    }
    virtual RS_Vector getNearestMiddle(const RS_Vector& coord,
                                       double* dist = NULL,
                                       int middlePoints = 1
                                       ) = 0;

    /**
     * Must be overwritten to get the nearest point with a given
     * distance to the endpoint to the given coordinate for this entity. 
     *
     * @param distance Distance to endpoint.
     * @param coord Coordinate (typically a mouse coordinate)
     * @param dist Pointer to a value which will contain the measured
     * distance between 'coord' and the closest point. The passed
     * pointer can also be NULL in which case the distance will be
     * lost.
     *
     * @return The closest point with the given distance to the endpoint.
     */
    virtual RS_Vector getNearestDist(double distance,
                                     const RS_Vector& coord,
                                     double* dist = NULL) = 0;
									 
    /**
     * Must be overwritten to get the point with a given
     * distance to the start- or endpoint to the given coordinate for this entity. 
     *
     * @param distance Distance to endpoint.
     * @param startp true = measured from Startpoint, false = measured from Endpoint
     *
     * @return The point with the given distance to the start- or endpoint.
     */
    virtual RS_Vector getNearestDist(double /*distance*/,
                                     bool /*startp*/) {
		return RS_Vector(false);
	}
									 
    /**
     * Must be overwritten to get the nearest reference point for this entity. 
     *
     * @param coord Coordinate (typically a mouse coordinate)
     * @param dist Pointer to a value which will contain the measured
     * distance between 'coord' and the closest point. The passed
     * pointer can also be NULL in which case the distance will be
     * lost.
     *
     * @return The closest point with the given distance to the endpoint.
     */
    virtual RS_Vector getNearestRef(const RS_Vector& coord,
                                     double* dist = NULL) { 
		RS_VectorSolutions s = getRefPoints();
		
		return s.getClosest(coord, dist); 
	}
	
    /**
     * Gets the nearest reference point of this entity if it is selected. 
	 * Containers re-implement this method to return the nearest reference
	 * point of a selected sub entity.
     *
     * @param coord Coordinate (typically a mouse coordinate)
     * @param dist Pointer to a value which will contain the measured
     * distance between 'coord' and the closest point. The passed
     * pointer can also be NULL in which case the distance will be
     * lost.
     *
     * @return The closest point with the given distance to the endpoint.
     */
    virtual RS_Vector getNearestSelectedRef(const RS_Vector& coord,
                                     double* dist = NULL) { 
		if (isSelected()) {
			return getNearestRef(coord, dist);
		}
		else {
			return RS_Vector(false); 
		}
	}

    /**
     * Must be overwritten to get the shortest distance between this 
     * entity and a coordinate.
     *
     * @param coord Coordinate (typically a mouse coordinate)
     * @param entity Pointer which will contain the (sub-)entity which is 
     *               closest to the given point or NULL if the caller is not 
     *               interested in this information.
     * @param level The resolve level. 
     *
     * @sa RS2::ResolveLevel
     *
     * @return The measured distance between \p coord and the entity.
     */
    virtual RS_Vector getNearestOrthTan(const RS_Vector& /*coord*/,
                    const RS_Line& /*normal*/,
                    bool onEntity = false);
    virtual double getDistanceToPoint(const RS_Vector& coord,
                                      RS_Entity** entity = NULL,
                                      RS2::ResolveLevel level = RS2::ResolveNone,
									  double solidDist = RS_MAXDOUBLE) = 0;

    virtual bool isPointOnEntity(const RS_Vector& coord,
                                 double tolerance=RS_TOLERANCE);

    /**
     * Implementations must move the entity by the given vector.
     */
    virtual void move(RS_Vector offset) = 0;

    /**
     * Implementations must rotate the entity by the given angle around
     * the given center.
     */
    virtual void rotate(RS_Vector center, double angle) = 0;

    /**
     * Implementations must scale the entity by the given factors.
     */
    virtual void scale(RS_Vector center, RS_Vector factor) = 0;

    /**
     * Acts like scale(RS_Vector) but with equal factors.
     * Equal to scale(center, RS_Vector(factor, factor)).
     */
    virtual void scale(RS_Vector center, double factor) {
        scale(center, RS_Vector(factor, factor));
    }

    /**
     * Implementations must mirror the entity by the given axis.
     */
    virtual void mirror(RS_Vector axisPoint1, RS_Vector axisPoint2) = 0;

    virtual void stretch(RS_Vector firstCorner,
                         RS_Vector secondCorner,
                         RS_Vector offset);

	/**
	 * Implementations must drag the reference point(s) of all
	 * (sub-)entities that are very close to ref by offset.
	 */
	virtual void moveRef(const RS_Vector& /*ref*/, 
		const RS_Vector& /*offset*/) {
		return;
	}
	
	/**
	 * Implementations must drag the reference point(s) of selected 
	 * (sub-)entities that are very close to ref by offset.
	 */
	virtual void moveSelectedRef(const RS_Vector& /*ref*/, 
		const RS_Vector& /*offset*/) {
		return;
	}

    /**
     * Implementations must draw the entity on the given device.
     */
    virtual void draw(RS_Painter* painter, RS_GraphicView* view, 
		double patternOffset = 0.0) = 0;

	double getStyleFactor(RS_GraphicView* view);
	
        QString getUserDefVar(QString key);
        QList<QString> getAllKeys();
        void setUserDefVar(QString key, QString val);
        void delUserDefVar(QString key);

    friend std::ostream& operator << (std::ostream& os, RS_Entity& e);

    /** Recalculates the borders of this entity. */
    virtual void calculateBorders() = 0;

protected:
    //! Entity's parent entity or NULL is this entity has no parent.
    RS_EntityContainer* parent;
    //! minimum coordinates
    RS_Vector minV;
    //! maximum coordinates
    RS_Vector maxV;

    //! Pointer to layer
    RS_Layer* layer;

    //! Entity id
    unsigned long int id;

    //! pen (attributes) for this entity
    RS_Pen pen;
	
	//! auto updating enabled?
	bool updateEnabled;

private:
        QMultiHash<QString, QString> varList;
};

#endif
