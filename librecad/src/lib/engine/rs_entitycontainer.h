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


#ifndef RS_ENTITYCONTAINER_H
#define RS_ENTITYCONTAINER_H

#include "rs_arc.h"
#include "rs_circle.h"
#include "rs_ellipse.h"
#include "rs_entity.h"
#include "rs_line.h"
#include "rs_point.h"

/**
 * Class representing a tree of entities.
 * Typical entity containers are graphics, polylines, groups, texts, ...)
 *
 * @author Andrew Mustun
 */
class RS_EntityContainer : public RS_Entity {

public:

    RS_EntityContainer(RS_EntityContainer* parent=NULL, bool owner=true);
    //RS_EntityContainer(const RS_EntityContainer& ec);
    virtual ~RS_EntityContainer();

    virtual RS_Entity* clone();
    virtual void detach();

    /** @return RS2::EntityContainer */
    virtual RS2::EntityType rtti() const {
        return RS2::EntityContainer;
    }

    void reparent(RS_EntityContainer* parent);

    /**
     * @return true: because entities made from this class
     *         and subclasses are containers for other entities.
     */
    virtual bool isContainer() const {
        return true;
    }

    /**
     * @return false: because entities made from this class
     *         and subclasses are containers for other entities.
     */
    virtual bool isAtomic() const {
                return false;
        }

        virtual double getLength() const;

    virtual void undoStateChanged(bool undone);
    virtual void setVisible(bool v);

    virtual bool setSelected(bool select=true);
    virtual bool toggleSelected();

    virtual void selectWindow(RS_Vector v1, RS_Vector v2,
                bool select=true, bool cross=false);

    virtual void addEntity(RS_Entity* entity);
    virtual void appendEntity(RS_Entity* entity);
    virtual void prependEntity(RS_Entity* entity);
    virtual void moveEntity(int index, QList<RS_Entity *> entList);
    virtual void insertEntity(int index, RS_Entity* entity);
//RLZ unused    virtual void replaceEntity(int index, RS_Entity* entity);
    virtual bool removeEntity(RS_Entity* entity);
    virtual RS_Entity* firstEntity(RS2::ResolveLevel level=RS2::ResolveNone);
    virtual RS_Entity* lastEntity(RS2::ResolveLevel level=RS2::ResolveNone);
    virtual RS_Entity* nextEntity(RS2::ResolveLevel level=RS2::ResolveNone);
    virtual RS_Entity* prevEntity(RS2::ResolveLevel level=RS2::ResolveNone);
    virtual RS_Entity* entityAt(int index);
    virtual void setEntityAt(int index,RS_Entity* en){
        if(autoDelete && entities.at(index) != NULL) {
            delete entities.at(index);
        }
        entities[index] = en;
    }
//RLZ unused	virtual int entityAt();
        virtual int findEntity(RS_Entity* entity);
    virtual void clear();

    QListIterator<RS_Entity*> createIterator();

    //virtual unsigned long int count() {
        //	return count(false);
        //}
    virtual bool isEmpty() {
        return count()==0;
    }
    virtual unsigned int count();
    virtual unsigned int count() const;
    virtual unsigned int countDeep();
    //virtual unsigned long int countLayerEntities(RS_Layer* layer);
    virtual unsigned int countSelected();
    virtual double totalSelectedLength();

    /**
     * Enables / disables automatic update of borders on entity removals
     * and additions. By default this is turned on.
     */
    virtual void setAutoUpdateBorders(bool enable) {
        autoUpdateBorders = enable;
    }
    virtual void adjustBorders(RS_Entity* entity);
    virtual void calculateBorders();
    virtual void forcedCalculateBorders();
    virtual void updateDimensions( bool autoText=true);
    virtual void updateInserts();
    virtual void updateSplines();
    virtual void update();
        virtual void renameInserts(const QString& oldName,
                const QString& newName);


    virtual RS_Vector getNearestEndpoint(const RS_Vector& coord,
                                         double* dist = NULL)const;
    virtual RS_Vector getNearestEndpoint(const RS_Vector& coord,
                                         double* dist, RS_Entity** pEntity )const;

    RS_Entity* getNearestEntity(const RS_Vector& point,
                                double* dist = NULL,
                                RS2::ResolveLevel level=RS2::ResolveAll);

    virtual RS_Vector getNearestPointOnEntity(const RS_Vector& coord,
            bool onEntity = true,
                        double* dist = NULL,
            RS_Entity** entity=NULL)const;

    virtual RS_Vector getNearestCenter(const RS_Vector& coord,
                                       double* dist = NULL);
    virtual RS_Vector getNearestMiddle(const RS_Vector& coord,
                                       double* dist = NULL,
                                       int middlePoints = 1
                                       )const;
    virtual RS_Vector getNearestDist(double distance,
                                     const RS_Vector& coord,
                                     double* dist = NULL);
    virtual RS_Vector getNearestIntersection(const RS_Vector& coord,
            double* dist = NULL);
    virtual RS_Vector getNearestRef(const RS_Vector& coord,
                                     double* dist = NULL);
    virtual RS_Vector getNearestSelectedRef(const RS_Vector& coord,
                                     double* dist = NULL);

    virtual double getDistanceToPoint(const RS_Vector& coord,
                                      RS_Entity** entity,
                                      RS2::ResolveLevel level=RS2::ResolveNone,
                                      double solidDist = RS_MAXDOUBLE) const;

    virtual bool optimizeContours();

    virtual bool hasEndpointsWithinWindow(const RS_Vector& v1, const RS_Vector& v2);

    virtual void move(const RS_Vector& offset);
    virtual void rotate(const RS_Vector& center, const double& angle);
    virtual void rotate(const RS_Vector& center, const RS_Vector& angleVector);
    virtual void scale(const RS_Vector& center, const RS_Vector& factor);
    virtual void mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2);

    virtual void stretch(const RS_Vector& firstCorner,
                         const RS_Vector& secondCorner,
                         const RS_Vector& offset);
        virtual void moveRef(const RS_Vector& ref, const RS_Vector& offset);
        virtual void moveSelectedRef(const RS_Vector& ref, const RS_Vector& offset);

    virtual void draw(RS_Painter* painter, RS_GraphicView* view, double& patternOffset);

    friend std::ostream& operator << (std::ostream& os, RS_EntityContainer& ec);

    bool isOwner() {return autoDelete;}
    void setOwner(bool owner) {autoDelete=owner;}
protected:

    /** entities in the container */
    QList<RS_Entity *> entities;

    /** sub container used only temporarly for iteration. */
    RS_EntityContainer* subContainer;

    /**
     * Automatically update the borders of the container when entities
     * are added or removed.
     */
    static bool autoUpdateBorders;

private:
    int entIdx;
    bool autoDelete;
};

#endif
