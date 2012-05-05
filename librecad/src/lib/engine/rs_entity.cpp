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


#include "rs_entity.h"

#include <iostream>

#include "rs_arc.h"
#include "rs_circle.h"
#include "rs_ellipse.h"
#include "rs_graphic.h"
#include "rs_graphicview.h"
#include "rs_insert.h"
#include "rs_layer.h"
#include "rs_line.h"
#include "rs_point.h"
#include "rs_polyline.h"
#include "rs_text.h"
#include "rs_information.h"
#include <QPolygon>

/**
 * Default constructor.
 * @param parent The parent entity of this entity.
 *               E.g. a line might have a graphic entity or
 *               a polyline entity as parent.
 */
RS_Entity::RS_Entity(RS_EntityContainer* parent) {

    this->parent = parent;
    init();
}


/**
 * Copy constructor.
 */
/*RS_Entity::RS_Entity(const RS_Entity& e) : RS_Flags(e.getFlags()) {
        cout << "copy constructor called\n";
        init();
        parent = e.parent;
        layer = e.layer;
        //setFlag(e.getFlags());
    minV = e.minV;
    maxV = e.maxV;
    pen = e.pen;
}*/



/**
 * Destructor.
 */
RS_Entity::~RS_Entity() {}



/**
 * Initialisation. Called from all constructors.
 */
void RS_Entity::init() {
    resetBorders();

    setFlag(RS2::FlagVisible);
    //layer = NULL;
    //pen = RS_Pen();
        updateEnabled = true;
    setLayerToActive();
    setPenToActive();
    initId();
}

/**
 * Gives this entity a new unique id.
 */
void RS_Entity::initId() {
    static unsigned long int idCounter=0;
    id = idCounter++;
}

/**
 * Resets the borders of this element.
 */
void RS_Entity::resetBorders() {
    // TODO: Check that. windoze XP crashes with MAXDOUBLE
    double maxd = RS_MAXDOUBLE;
    double mind = RS_MINDOUBLE;

    minV.set(maxd, maxd);
    maxV.set(mind, mind);
}



/**
 * Selects or deselects this entity.
 *
 * @param select True to select, false to deselect.
 */
bool RS_Entity::setSelected(bool select) {
    // layer is locked:
    if (select && isLocked()) {
        return false;
    }

    if (select) {
        setFlag(RS2::FlagSelected);
    } else {
        delFlag(RS2::FlagSelected);
    }

    return true;
}



/**
 * Toggles select on this entity.
 */
bool RS_Entity::toggleSelected() {
    return setSelected(!isSelected());
    //toggleFlag(RS2::FlagSelected);
}



/**
 * @return True if the entity is selected. Note that an entity might
 * not be selected but one of its parents is selected. In that case
 * this function returns false.
 */
bool RS_Entity::isSelected() const {
    return getFlag(RS2::FlagSelected);
}



/**
 * @return true if a parent entity of this entity is selected.
 */
bool RS_Entity::isParentSelected() {
    RS_Entity* p = this;

    do {
        p = p->getParent();
        if (p!=NULL && p->isSelected()==true) {
            return true;
        }
    } while(p!=NULL);

    return false;
}



/**
 * Sets or resets the processed flag of this entity.
 *
 * @param on True to set, false to reset.
 */
void RS_Entity::setProcessed(bool on) {
    if (on) {
        setFlag(RS2::FlagProcessed);
    } else {
        delFlag(RS2::FlagProcessed);
    }
}



/**
 * @return True if the processed flag is set.
 */
bool RS_Entity::isProcessed() const {
    return getFlag(RS2::FlagProcessed);
}



/**
 * Called when the undo state changed.
 *
 * @param undone true: entity has become invisible.
 *               false: entity has become visible.
 */
void RS_Entity::undoStateChanged(bool /*undone*/) {
        setSelected(false);
    update();
}


/**
 * @return true if this entity or any parent entities are undone.
 */
bool RS_Entity::isUndone() const {
        if (parent==NULL) {
                return RS_Undoable::isUndone();
        }
        else {
                return RS_Undoable::isUndone() || parent->isUndone();
        }
}


/**
 * @return True if the entity is in the given range.
 */
bool RS_Entity::isInWindow(RS_Vector v1, RS_Vector v2) {
    double right, left, top, bottom;

    right = std::max(v1.x, v2.x);
    left = std::min(v1.x, v2.x);
    top = std::max(v1.y, v2.y);
    bottom = std::min(v1.y, v2.y);

    return (getMin().x>=left &&
            getMax().x<=right &&
            getMin().y>=bottom &&
            getMax().y<=top);
}

/** whether the entity's bounding box intersects with visible portion of graphic view */
bool RS_Entity::isVisibleInWindow(RS_GraphicView* view) const
{
    RS_Vector vpMin(view->toGraph(0,view->getHeight()));
    RS_Vector vpMax(view->toGraph(view->getWidth(),0));
    if( getStartpoint().isInWindowOrdered(vpMin, vpMax) ) return true;
    if( getEndpoint().isInWindowOrdered(vpMin, vpMax) ) return true;
    QPolygonF boundingBox(QRectF(vpMin.x,vpMin.y,vpMax.x-vpMin.x, vpMax.y-vpMin.y));
    QVector<RS_Vector> vps;
    for(unsigned short i=0;i<4;i++){
        auto& vp(boundingBox.at(i));
        vps<<RS_Vector(vp.x(),vp.y());
    }
    for(unsigned short i=0;i<4;i++){
        RS_Line line(NULL,RS_LineData(vps.at(i),vps.at((i+1)%4)));
        if( RS_Information::getIntersection(const_cast<RS_Entity*>(this), &line, true).size()>0) return true;
    }
    return false;
}

/**
 * @param tolerance Tolerance.
 *
 * @retval true if the given point is on this entity.
 * @retval false otherwise
 */
bool RS_Entity::isPointOnEntity(const RS_Vector& coord,
                                double tolerance) const {
    double dist = getDistanceToPoint(coord, NULL, RS2::ResolveNone);
    return (dist<=fabs(tolerance));
}


/**
 * Is this entity visible?
 *
 * @return true Only if the entity and the layer it is on are visible.
 * The Layer might also be NULL. In that case the layer visiblity
* is ignored.
 */
bool RS_Entity::isVisible() {

    if (!getFlag(RS2::FlagVisible)) {
        return false;
    }

    if (isUndone()) {
        return false;
    }

        /*RS_EntityCotnainer* parent = getParent();
        if (parent!=NULL && parent->isUndone()) {
                return false;
        }*/

    if (getLayer()==NULL) {
        return true;
    }

    // inserts are usually visible - the entities in them have their own
    //   layers which might be frozen
    // upd: i'm not sure if that is the best behaviour
    //if (rtti()==RS2::EntityInsert) {
    //	return true;
    //}
    // blocks are visible in editting window, issue#253
    if( isDocument() && (rtti()==RS2::EntityBlock || rtti()==RS2::EntityInsert)) {
        return true;
    }

    if (layer!=NULL /*&& layer->getName()!="ByBlock"*/) {

        if (!layer->isFrozen()) {
            return true;
        } else {
            return false;
        }
    }

    if (layer==NULL /*&& getLayer()->getName()!="ByBlock"*/) {
        if (getLayer()==NULL) {
            return true;
        } else {
            if (!getLayer()->isFrozen()) {
                return true;
            } else {
                return false;
            }
        }
    }

    if (getBlockOrInsert()==NULL) {
        return true;
    }

    if (getBlockOrInsert()->rtti()==RS2::EntityBlock) {
        if (getLayer(false)==NULL || !getLayer(false)->isFrozen()) {
            return true;
        } else {
            return false;
        }
    }


    if (getBlockOrInsert()->getLayer()==NULL) {
        return true;
    }

    if(!getBlockOrInsert()->getLayer()->isFrozen()) {
        return true;
    }

    return false;
}



/**
 * Sets the highlight status of the entity. Highlighted entities
 * usually indicate a feedback to a user action.
 */
void RS_Entity::setHighlighted(bool on) {
    if (on) {
        setFlag(RS2::FlagHighlighted);
    } else {
        delFlag(RS2::FlagHighlighted);
    }
}



/**
 * @return true if the entity is highlighted.
 */
bool RS_Entity::isHighlighted() {
    return getFlag(RS2::FlagHighlighted);
}



/**
 * @return true if the layer this entity is on is locked.
 */
bool RS_Entity::isLocked() {
    if (getLayer(true)!=NULL && getLayer()->isLocked()) {
        return true;
    } else {
        return false;
    }
}


/**
 * @return The parent graphic in which this entity is stored
 * or the parent's parent graphic or NULL if none of the parents
 * are stored in a graphic.
 */
RS_Graphic* RS_Entity::getGraphic() {
    if (rtti()==RS2::EntityGraphic) {
        return (RS_Graphic*)this;
    } else if (parent==NULL) {
        return NULL;
    } else {
        return parent->getGraphic();
    }
}



/**
 * @return The parent block in which this entity is stored
 * or the parent's parent block or NULL if none of the parents
 * are stored in a block.
 */
RS_Block* RS_Entity::getBlock() {
    if (rtti()==RS2::EntityBlock) {
        return (RS_Block*)this;
    } else if (parent==NULL) {
        return NULL;
    } else {
        return parent->getBlock();
    }
}



/**
 * @return The parent insert in which this entity is stored
 * or the parent's parent block or NULL if none of the parents
 * are stored in a block.
 */
RS_Insert* RS_Entity::getInsert() {
    if (rtti()==RS2::EntityInsert) {
        return (RS_Insert*)this;
    } else if (parent==NULL) {
        return NULL;
    } else {
        return parent->getInsert();
    }
}



/**
 * @return The parent block or insert in which this entity is stored
 * or the parent's parent block or insert or NULL if none of the parents
 * are stored in a block or insert.
 */
RS_Entity* RS_Entity::getBlockOrInsert() {
    if (rtti()==RS2::EntityBlock || rtti()==RS2::EntityInsert) {
        return this;
    } else if (parent==NULL) {
        return NULL;
    } else {
        return parent->getBlockOrInsert();
    }
}



/**
 * @return The parent document in which this entity is stored
 * or the parent's parent document or NULL if none of the parents
 * are stored in a document. Note that a document is usually
 * either a Graphic or a Block.
 */
RS_Document* RS_Entity::getDocument() {
    if (isDocument()==true) {
        return (RS_Document*)this;
    } else if (parent==NULL) {
        return NULL;
    } else {
        return parent->getDocument();
    }
}



/**
 * Sets a variable value for the parent graphic object.
 *
 * @param key Variable name (e.g. "$DIMASZ")
 * @param val Default value
 */
void RS_Entity::addGraphicVariable(const QString& key, double val, int code) {
    RS_Graphic* graphic = getGraphic();
    if (graphic!=NULL) {
        graphic->addVariable(key, val, code);
    }
}



/**
 * Sets a variable value for the parent graphic object.
 *
 * @param key Variable name (e.g. "$DIMASZ")
 * @param val Default value
 */
void RS_Entity::addGraphicVariable(const QString& key, int val, int code) {
    RS_Graphic* graphic = getGraphic();
    if (graphic!=NULL) {
        graphic->addVariable(key, val, code);
    }
}



/**
 * Sets a variable value for the parent graphic object.
 *
 * @param key Variable name (e.g. "$DIMASZ")
 * @param val Default value
 */
void RS_Entity::addGraphicVariable(const QString& key,
                                   const QString& val, int code) {
    RS_Graphic* graphic = getGraphic();
    if (graphic!=NULL) {
        graphic->addVariable(key, val, code);
    }
}



/**
 * A safe member function to return the given variable.
 *
 * @param key Variable name (e.g. "$DIMASZ")
 * @param def Default value
 *
 * @return value of variable or default value if the given variable
 *    doesn't exist.
 */
double RS_Entity::getGraphicVariableDouble(const QString& key, double def) {
    RS_Graphic* graphic = getGraphic();
    double ret=def;
    if (graphic!=NULL) {
        ret = graphic->getVariableDouble(key, def);
    }
    return ret;
}



/**
 * A safe member function to return the given variable.
 *
 * @param key Variable name (e.g. "$DIMASZ")
 * @param def Default value
 *
 * @return value of variable or default value if the given variable
 *    doesn't exist.
 */
int RS_Entity::getGraphicVariableInt(const QString& key, int def) {
    RS_Graphic* graphic = getGraphic();
    int ret=def;
    if (graphic!=NULL) {
        ret = graphic->getVariableInt(key, def);
    }
    return ret;
}



/**
 * A safe member function to return the given variable.
 *
 * @param key Variable name (e.g. "$DIMASZ")
 * @param def Default value
 *
 * @return value of variable or default value if the given variable
 *    doesn't exist.
 */
QString RS_Entity::getGraphicVariableString(const QString& key,
        const QString&  def) {
    RS_Graphic* graphic = getGraphic();
    QString ret=def;
    if (graphic!=NULL) {
        ret = graphic->getVariableString(key, def);
    }
    return ret;
}



/**
 * @return The unit the parent graphic works on or None if there's no
 * parent graphic.
 */
RS2::Unit RS_Entity::getGraphicUnit() {
    RS_Graphic* graphic = getGraphic();
    RS2::Unit ret = RS2::None;
    if (graphic!=NULL) {
        ret = graphic->getUnit();
    }
    return ret;
}



/**
 * Returns a pointer to the layer this entity is on or NULL.
 *
 * @para resolve true: if the layer is ByBlock, the layer of the
 *               block this entity is in is returned.
 *               false: the layer of the entity is returned.
 *
 * @return pointer to the layer this entity is on. If the layer
 * is set to NULL the layer of the next parent that is not on
 * layer NULL is returned. If all parents are on layer NULL, NULL
 * is returned.
 */
RS_Layer* RS_Entity::getLayer(bool resolve) const {
    if (resolve) {
        // we have no layer but a parent that might have one.
        // return parent's layer instead:
        if (layer==NULL /*|| layer->getName()=="ByBlock"*/) {
            if (parent!=NULL) {
                return parent->getLayer(true);
            } else {
                return NULL;
            }
        }
    }

    // return our layer. might still be NULL:
    return layer;
}



/**
 * Sets the layer of this entity to the layer with the given name
 */
void RS_Entity::setLayer(const QString& name) {
    RS_Graphic* graphic = getGraphic();
    if (graphic!=NULL) {
        layer = graphic->findLayer(name);
    } else {
        layer = NULL;
    }
}



/**
 * Sets the layer of this entity to the layer given.
 */
void RS_Entity::setLayer(RS_Layer* l) {
    layer = l;
}



/**
 * Sets the layer of this entity to the current layer of
 * the graphic this entity is in. If this entity (and none
 * of its parents) are in a graphic the layer is set to NULL.
 */
void RS_Entity::setLayerToActive() {
    RS_Graphic* graphic = getGraphic();

    if (graphic!=NULL) {
        layer = graphic->getActiveLayer();
    } else {
        layer = NULL;
    }
}



/**
 * Gets the pen needed to draw this entity.
 * The attributes can also come from the layer this entity is on
 * if the flags are set accordingly.
 *
 * @param resolve true: Resolve the pen to a drawable pen (e.g. the pen
 *         from the layer or parent..)
 *         false: Don't resolve and return a pen or ByLayer, ByBlock, ...
 *
 * @return Pen for this entity.
 */
RS_Pen RS_Entity::getPen(bool resolve) const {

    if (!resolve) {
        return pen;
    } else {

        RS_Pen p = pen;
        RS_Layer* l = getLayer(true);

        // use parental attributes (e.g. vertex of a polyline, block
        // entities when they are drawn in block documents):
        if (!p.isValid() || p.getColor().isByBlock()) {
            if (parent!=NULL) {
                p = parent->getPen();
            }
        }
        // use layer attributes:
        else if (l!=NULL) {

            // layer is "ByBlock":
            /*if (layer->getName()=="ByBlock" && getBlockOrInsert()!=NULL) {
                p = getBlockOrInsert()->getPen();
        } else {*/

            // use layer's color:
            if (p.getColor().isByLayer()) {
                p.setColor(l->getPen().getColor());
            }

            // use layer's width:
            if (p.getWidth()==RS2::WidthByLayer) {
                p.setWidth(l->getPen().getWidth());
            }

            // use layer's linetype:
            if (p.getLineType()==RS2::LineByLayer) {
                p.setLineType(l->getPen().getLineType());
            }
            //}
        }

        return p;
    }
}



/**
 * Sets the pen of this entity to the current pen of
 * the graphic this entity is in. If this entity (and none
 * of its parents) are in a graphic the pen is not changed.
 */
void RS_Entity::setPenToActive() {
    RS_Document* doc = getDocument();
    if (doc!=NULL) {
        pen = doc->getActivePen();
    } else {
        //RS_DEBUG->print(RS_Debug::D_WARNING, "RS_Entity::setPenToActive(): "
        //                "No document / active pen linked to this entity.");
    }
    //else {
    //   pen = RS_Pen();
    //}
}



/**
 * Implementations must stretch the given range of the entity
 * by the given offset. This default implementation moves the
 * whole entity if it is completely inside the given range.
 */
void RS_Entity::stretch(const RS_Vector& firstCorner,
                        const RS_Vector& secondCorner,
                        const RS_Vector& offset) {

    //e->calculateBorders();
    if (getMin().isInWindow(firstCorner, secondCorner) &&
            getMax().isInWindow(firstCorner, secondCorner)) {

        move(offset);
    }
}



/**
 * @return Factor for scaling the line styles considering the current
 * paper scaling and the fact that styles are stored in Millimeter.
 */
double RS_Entity::getStyleFactor(RS_GraphicView* view) {
    double styleFactor = 1.0;

    if (view!=NULL) {
        if (view->isPrinting()==false && view->isDraftMode()) {
            styleFactor = 1.0/view->getFactor().x;
        } else {
            //styleFactor = getStyleFactor();
            // the factor caused by the unit:
            RS2::Unit unit = RS2::None;
            RS_Graphic* g = getGraphic();
            if (g!=NULL) {
                unit = g->getUnit();
                //double scale = g->getPaperScale();
                styleFactor = RS_Units::convert(1.0, RS2::Millimeter, unit);
                // / scale;
            }

            // the factor caused by the line width:
            if (((int)getPen(true).getWidth())>0) {
                styleFactor *= ((double)getPen(true).getWidth()/100.0);
            } else if (((int)getPen(true).getWidth())==0) {
                styleFactor *= 0.01;
            }
        }

        if (view->isPrinting() || view->isPrintPreview() || view->isDraftMode()==false) {
            RS_Graphic* graphic = getGraphic();
            if (graphic!=NULL && graphic->getPaperScale()>1.0e-6) {
                styleFactor /= graphic->getPaperScale();
            }
        }
    }

        //RS_DEBUG->print("stylefactor: %f", styleFactor);
        //RS_DEBUG->print("viewfactor: %f", view->getFactor().x);

        if (styleFactor*view->getFactor().x<0.2) {
                styleFactor = -1.0;
        }

    return styleFactor;
}


/**
 * @return User defined variable connected to this entity or NULL if not found.
 */
QString RS_Entity::getUserDefVar(const QString& key) const {
    return varList.value(key, NULL);
}
/*
 * @coord
 * @normal
 * @bool
 * return a line tangent to entity and orthogonal to the line (*normal)
 */
RS_Vector RS_Entity::getNearestOrthTan(const RS_Vector& /*coord*/,
                    const RS_Line& /*normal*/,
                    bool /*onEntity = false*/){
        return RS_Vector(false);
}


/**
 * Add a user defined variable to this entity.
 */
void RS_Entity::setUserDefVar(QString key, QString val) {
    varList.insert(key, val);
}



/**
 * Deletes the given user defined variable.
 */
void RS_Entity::delUserDefVar(QString key) {
    varList.remove(key);
}



/**
 * @return A list of all keys connected to this entity.
 */
QList<QString> RS_Entity::getAllKeys() {
    return varList.keys();
}

//! helpLayer contains entities of infinite length, helpLayer doesn't show up in print
bool RS_Entity::isHelpLayer(bool typeCheck)  {
    if(typeCheck && getParent() != NULL){
        //ignore
        switch(rtti()){
        case RS2::EntitySpline:
        case RS2::EntityPolyline:
            return false;
            default:
            break;
        }
    }
    if(layer != NULL) return layer->isHelpLayer();
    return false;
}

/**
 * Dumps the elements data to stdout.
 */
std::ostream& operator << (std::ostream& os, RS_Entity& e) {
    //os << "Warning: Virtual entity!\n";
    //return os;

    os << " {Entity id: " << e.id;
    if (e.parent!=NULL) {
        os << " | parent id: " << e.parent->getId() << "\n";
    } else {
        os << " | no parent\n";
    }

    os << " flags: " << (e.getFlag(RS2::FlagVisible) ? "RS2::FlagVisible" : "");
    os << (e.getFlag(RS2::FlagUndone) ? " RS2::FlagUndone" : "");
    os << (e.getFlag(RS2::FlagSelected) ? " RS2::FlagSelected" : "");
    os << "\n";

    if (e.layer==NULL) {
        os << " layer: NULL ";
    } else {
        os << " layer: " << e.layer->getName().toLatin1().data() << " ";
        os << " layer address: " << e.layer << " ";
    }

    os << e.pen << "\n";

        os << "variable list:\n";
    QHash<QString, QString>::const_iterator it = e.varList.constBegin();
    while (it != e.varList.constEnd() ) {
        os << it.key().toLatin1().data() << ": " << it.value().toLatin1().data() << ", ";
        ++it;
    }

    // There should be a better way then this...
    switch(e.rtti()) {
    case RS2::EntityPoint:
        os << (RS_Point&)e;
        break;

    case RS2::EntityLine:
        os << (RS_Line&)e;
        break;

    case RS2::EntityPolyline:
        os << (RS_Polyline&)e;
        break;

    case RS2::EntityArc:
        os << (RS_Arc&)e;
        break;

    case RS2::EntityCircle:
        os << (RS_Circle&)e;
        break;

    case RS2::EntityEllipse:
        os << (RS_Ellipse&)e;
        break;

    case RS2::EntityInsert:
        os << (RS_Insert&)e;
        break;

    case RS2::EntityText:
        os << (RS_Text&)e;
        break;

    default:
        os << "Unknown Entity";
        break;
    }
    os << "}\n\n";

    return os;
}

