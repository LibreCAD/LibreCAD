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


#include <iostream>
#include <map>
#include <utility>

#include <QPolygon>
#include <QString>

#include "rs_arc.h"
#include "rs_block.h"
#include "rs_circle.h"
#include "rs_ellipse.h"
#include "rs_entity.h"
#include "rs_graphic.h"
#include "rs_information.h"
#include "rs_insert.h"
#include "rs_layer.h"
#include "rs_line.h"
#include "rs_math.h"
#include "rs_mtext.h"
#include "rs_pen.h"
#include "rs_point.h"
#include "rs_polyline.h"
#include "rs_text.h"
#include "rs_vector.h"
#include "lc_quadratic.h"


struct RS_Entity::Impl {
    //! pen (attributes) for this entity
    RS_Pen pen{};
    std::map<QString, QString> varList;

    void fromOther(Impl* other) {
        if (other != nullptr) {
            pen = other->pen;
            varList = other->varList;
        }
    }
};

/**
 * @param parent The parent entity of this entity.
 *               E.g. a line might have a graphic entity or
 *               a polyline entity as parent.
 */
RS_Entity::RS_Entity(RS_EntityContainer *parent)
    : parent{parent}
    , m_pImpl{std::make_unique<Impl>()}
{
    init();
}

RS_Entity::RS_Entity(const RS_Entity& other):
    parent{other.parent}
    , updateEnabled {other.updateEnabled}
{
    init();
    m_pImpl->fromOther(other.m_pImpl.get());
    m_layer  = other.m_layer;
    minV = other.minV;
    maxV = other.maxV;
}

RS_Entity& RS_Entity::operator = (const RS_Entity& other)
{
    parent = other.parent;
    updateEnabled = other.updateEnabled;
    init();
    m_pImpl->fromOther(other.m_pImpl.get());
    m_layer  = other.m_layer;
    minV  = other.minV;
    maxV  = other.maxV;
    return *this;
}

RS_Entity::RS_Entity(RS_Entity&& other):
    parent{other.parent}
    , updateEnabled {other.updateEnabled}
{
    init();
    m_pImpl = std::move(other.m_pImpl);
    m_layer  = other.m_layer;
    minV  = other.minV;
    maxV  = other.maxV;
}

RS_Entity& RS_Entity::operator = (RS_Entity&& other)
{
    parent = other.parent;
    updateEnabled = other.updateEnabled;
    init();
    m_pImpl = std::move(other.m_pImpl);
    m_layer  = other.m_layer;
    minV  = other.minV;
    maxV  = other.maxV;
    return *this;
}

RS_Entity::~RS_Entity() = default;

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
 * Initialisation. Called from all constructors.
 */
void RS_Entity::init() {
    if (m_pImpl == nullptr) {
        m_pImpl = std::make_unique<Impl>();
    }
    resetBorders();
    setFlag(RS2::FlagVisible);
    updateEnabled = true;
    setLayerToActive();
    setPenToActive();
    initId();
}

/**
 * Gives this entity a new unique m_id.
 */
void RS_Entity::initId() {
    static unsigned long long idCounter=0;
    m_id = ++idCounter;
}

RS_Entity *RS_Entity::cloneProxy() const {
    return clone();
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


void RS_Entity::moveBorders(const RS_Vector& offset){
    minV.move(offset);
    maxV.move(offset);
}

void RS_Entity::scaleBorders(const RS_Vector& center, const RS_Vector& factor){
    minV.scale(center,factor);
    maxV.scale(center,factor);
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
	//bug 557, Selected entities in invisible layers are deleted
	return getFlag(RS2::FlagSelected) && isVisible(); // fixme - renderperf - isVisible is costly
}

/**
 * @return true if a parent entity of this entity is selected.
 */
bool RS_Entity::isParentSelected() const{
    RS_Entity const* p = this;
    while(p) {
        p = p->getParent();
        if (p && p->isSelected()==true) {
            return true;
        }
    }
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
void RS_Entity::undoStateChanged([[maybe_unused]] bool undone){
    setSelected(false);
    update();
}

/**
 * @return true if this entity or any parent entities are undone.
 */
bool RS_Entity::isUndone() const {
    if (!parent) {
        return RS_Undoable::isUndone();
    }
    else {
        return RS_Undoable::isUndone() || parent->isUndone();
    }
}

/**
 * @return True if the entity is in the given range.
 */
bool RS_Entity::isInWindow(RS_Vector v1, RS_Vector v2) const{
    return
        RS_Math::inBetween(getMin().x, v1.x, v2.x)
        && RS_Math::inBetween(getMax().x, v1.x, v2.x)
        && RS_Math::inBetween(getMin().y, v1.y, v2.y)
        && RS_Math::inBetween(getMax().y, v1.y, v2.y);
}

double RS_Entity::areaLineIntegral() const{
    return 0.;
}

bool RS_Entity::isArc() const{
    switch (rtti()) {
        case RS2::EntityArc:
        case RS2::EntityCircle:
//ellipse implements its own test
        case RS2::EntityEllipse:
            return true;
        default:
            return false;
    }
}

bool RS_Entity::isArcCircleLine() const{
    switch (rtti()) {
        case RS2::EntityArc:
        case RS2::EntityCircle:
        case RS2::EntityLine:
        case RS2::EntityPoint:
            return true;
        default:
            return false;
    }
}

/** whether the entity's bounding box intersects with visible portion of graphic view */
/*bool RS_Entity::isVisibleInWindow(RS_GraphicView* view) const{
    RS_Vector vpMin(view->toGraph(0,view->getHeight()));
    RS_Vector vpMax(view->toGraph(view->getWidth(),0));
    if( getStartpoint().isInWindowOrdered(vpMin, vpMax) ) return true;
    if( getEndpoint().isInWindowOrdered(vpMin, vpMax) ) return true;
    QPolygonF visualBox(QRectF(vpMin.x,vpMin.y,vpMax.x-vpMin.x, vpMax.y-vpMin.y));
    std::vector<RS_Vector> vps;
    for(unsigned short i=0;i<4;i++){
        const QPointF& vp(visualBox.at(i));
        vps.emplace_back(vp.x(),vp.y());
    }
    for(unsigned short i=0;i<4;i++){
        RS_Line const line{vps.at(i),vps.at((i+1)%4)};
        if( RS_Information::getIntersection(this, &line, true).size()>0) return true;
    }
    if( minV.isInWindowOrdered(vpMin,vpMax)||maxV.isInWindowOrdered(vpMin,vpMax)) return true;
    return false;
}*/

/**
 * @param tolerance Tolerance.
 *
 * @retval true if the given point is on this entity.
 * @retval false otherwise
 */
bool RS_Entity::isPointOnEntity(const RS_Vector& coord,
                                double tolerance) const {
    double dist = getDistanceToPoint(coord, nullptr, RS2::ResolveNone);
    return dist <= std::abs(tolerance);
}

double RS_Entity::getDistanceToPoint(const RS_Vector& coord,
                                     RS_Entity** entity,
                                     RS2::ResolveLevel /*level*/,
                                     double /*solidDist*/) const{
    if (entity) {
        *entity=const_cast<RS_Entity*>(this);
    }
    double dToEntity = RS_MAXDOUBLE;
    (void) getNearestPointOnEntity(coord, true, &dToEntity, entity);

    // RVT 6 Jan 2011 : Add selection by center point
    if(getCenter().valid){
        double dToCenter=getCenter().distanceTo(coord);
        return std::min(dToEntity,dToCenter);
    }else
        return dToEntity;
}

/**
 * Is this entity visible?
 *
 * @return true Only if the entity and the layer it is on are visible.
 * The Layer might also be nullptr. In that case the layer visibility
* is ignored.
 */
bool RS_Entity::isVisible() const {
    if (!getFlag(RS2::FlagVisible)) {
        return false;
    }

    if (isUndone()) {
        return false;
    }

    // inserts are usually visible - the entities in them have their own
    //   layers which might be frozen
    // upd: i'm not sure if that is the best behaviour
    //if (rtti()==RS2::EntityInsert) {
    //	return true;
    //}
    // blocks are visible in editing window, issue#253
    if (isDocument() && (rtti() == RS2::EntityBlock || rtti() == RS2::EntityInsert)) {
        return true;
    }
    if (m_layer != nullptr) {
        return !m_layer->isFrozen();
    } else {
        /*RS_EntityContainer* parent = getParent();
if (parent && parent->isUndone()) {
        return false;
}*/
        RS_Layer* resolvedLayer = getLayerResolved();
        if (resolvedLayer == nullptr) {
            return true;
        }
        else {
            return !resolvedLayer ->isFrozen();
        }
    }
}

void RS_Entity::setVisible(bool v) {
    if (v) {
        setFlag(RS2::FlagVisible);
    } else {
        delFlag(RS2::FlagVisible);
    }
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

bool RS_Entity::isTransparent() const{
    return getFlag(RS2::FlagTransparent);
}

void RS_Entity::setTransparent(bool on) {
    if (on) {
        setFlag(RS2::FlagTransparent);
    } else {
        delFlag(RS2::FlagTransparent);
    }
}

RS_Vector RS_Entity::getStartpoint() const {
    return RS_Vector{false};
}

RS_Vector RS_Entity::getEndpoint() const {
	return {};
}

RS_VectorSolutions RS_Entity::getTangentPoint(const RS_Vector& /*point*/) const {
	return {};
}

RS_Vector RS_Entity::getTangentDirection(const RS_Vector& /*point*/)const{
	return {};
}
/**
 * @return true if the entity is highlighted.
 */
bool RS_Entity::isHighlighted() const{
    return getFlag(RS2::FlagHighlighted);
}

RS_Vector RS_Entity::getSize() const {
    return maxV-minV;
}

/**
 * @return true if the layer this entity is on is locked.
 */
bool RS_Entity::isLocked() const{
    return getLayer(true) && getLayer()->isLocked();
}

RS_Vector RS_Entity::getCenter() const {
    return {};
}

double RS_Entity::getRadius() const {
	return RS_MAXDOUBLE;
}

void RS_Entity::setRadius([[maybe_unused]] double r){}

/**
 * @return The parent graphic in which this entity is stored
 * or the parent's parent graphic or nullptr if none of the parents
 * are stored in a graphic.
 */
RS_Graphic* RS_Entity::getGraphic() const{
    if (rtti()==RS2::EntityGraphic) {
        auto const* ret=static_cast<RS_Graphic const*>(this);
        return const_cast<RS_Graphic*>(ret);
    } else if (!parent) {
        return nullptr;
    }
    return parent->getGraphic();
}

/**
 * @return The parent block in which this entity is stored
 * or the parent's parent block or nullptr if none of the parents
 * are stored in a block.
 */
RS_Block* RS_Entity::getBlock() const{
    if (rtti()==RS2::EntityBlock) {
        RS_Block const* ret=static_cast<RS_Block const*>(this);
        return const_cast<RS_Block*>(ret);
    } else if (!parent) {
        return nullptr;
    }
    return parent->getBlock();
}

/** return the equation of the entity
for quadratic,

return a vector contains:
m0 x^2 + m1 xy + m2 y^2 + m3 x + m4 y + m5 =0

for linear:
m0 x + m1 y + m2 =0
**/
LC_Quadratic RS_Entity::getQuadratic() const{
		return LC_Quadratic{};
}

/**
 * @return The parent insert in which this entity is stored
 * or the parent's parent block or nullptr if none of the parents
 * are stored in a block.
 */
RS_Insert* RS_Entity::getInsert() const{
    if (rtti()==RS2::EntityInsert) {
        RS_Insert const* ret=static_cast<RS_Insert const*>(this);
        return const_cast<RS_Insert*>(ret);
    } else if (!parent) {
        return nullptr;
    } else {
        return parent->getInsert();
    }
}

/**
 * @return The parent block or insert in which this entity is stored
 * or the parent's parent block or insert or nullptr if none of the parents
 * are stored in a block or insert.
 */
RS_Entity* RS_Entity::getBlockOrInsert() const{
    RS_Entity* ret{nullptr};
    switch(rtti()){
        case RS2::EntityBlock:
        case RS2::EntityInsert:
            ret=const_cast<RS_Entity*>(this);
            break;
        default:
            if(parent) {
                return parent->getBlockOrInsert();
            }
    }
    return ret;
}

/**
 * @return The parent document in which this entity is stored
 * or the parent's parent document or nullptr if none of the parents
 * are stored in a document. Note that a document is usually
 * either a Graphic or a Block.
 */
RS_Document* RS_Entity::getDocument() const{
    if (isDocument()) {
        RS_Document const* ret=static_cast<RS_Document const*>(this);
        return const_cast<RS_Document*>(ret);
    } else if (!parent) {
        return nullptr;
    }
    return parent->getDocument();
}

/**
 * Sets a variable value for the parent graphic object.
 *
 * @param key Variable name (e.g. "$DIMASZ")
 * @param val Default value
 */
void RS_Entity::addGraphicVariable(const QString& key, double val, int code) {
    RS_Graphic* graphic = getGraphic();
    if (graphic) {
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
    if (graphic) {
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
    if (graphic) {
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
    if (graphic) {
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
int RS_Entity::getGraphicVariableInt(const QString& key, int def) const{
    RS_Graphic* graphic = getGraphic();
    int ret=def;
    if (graphic) {
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
                                            const QString&  def) const
{
    RS_Graphic* graphic = getGraphic();
    QString ret=def;
    if (graphic) {
        ret = graphic->getVariableString(key, def);
    }
    return ret;
}

/**
 * @return The unit the parent graphic works on or None if there's no
 * parent graphic.
 */
RS2::Unit RS_Entity::getGraphicUnit() const
{
    RS_Graphic* graphic = getGraphic();
    RS2::Unit ret = RS2::None;
	if (graphic) {
        ret = graphic->getUnit();
    }
    return ret;
}

RS_Layer* RS_Entity::getLayerResolved() const {
    // we have no layer but a parent that might have one.
    // return parent's layer instead:
    if (m_layer == nullptr /*|| layer->getName()=="ByBlock"*/) {
        if (parent != nullptr) {
            return parent->getLayerResolved();
        } else {
            return nullptr;
        }
    }
    else{
        return m_layer;
    }
}

/**
 * Returns a pointer to the layer this entity is on or nullptr.
 *
 * @para resolve true: if the layer is ByBlock, the layer of the
 *               block this entity is in is returned.
 *               false: the layer of the entity is returned.
 *
 * @return pointer to the layer this entity is on. If the layer
 * is set to nullptr the layer of the next parent that is not on
 * layer nullptr is returned. If all parents are on layer nullptr, nullptr
 * is returned.
 */
RS_Layer* RS_Entity::getLayer(bool resolve) const {
    if (resolve) {
        // we have no layer but a parent that might have one.
        // return parent's layer instead:
        if (m_layer == nullptr /*|| layer->getName()=="ByBlock"*/) {
            if (parent != nullptr) {
                return parent->getLayer(true);
            } else {
                return nullptr;
            }
        }
    }

// return our layer. might still be nullptr:
    return m_layer;
}

/**
 * Sets the layer of this entity to the layer with the given name
 */
void RS_Entity::setLayer(const QString& name) {
    RS_Graphic* graphic = getGraphic();
    if (graphic) {
        m_layer = graphic->findLayer(name);
    } else {
        m_layer = nullptr;
    }
}

/**
 * Sets the layer of this entity to the layer given.
 */
void RS_Entity::setLayer(RS_Layer* l) {
    m_layer = l;
}

/**
 * Sets the layer of this entity to the current layer of
 * the graphic this entity is in. If this entity (and none
 * of its parents) are in a graphic the layer is set to nullptr.
 */
void RS_Entity::setLayerToActive() {
    RS_Graphic* graphic = getGraphic();

    if (graphic) {
        m_layer = graphic->getActiveLayer();
    } else {
        m_layer = nullptr;
    }
}

RS_Pen RS_Entity::getPenResolved() const {
    RS_Pen p = m_pImpl->pen;
    // use parental attributes (e.g. vertex of a polyline, block
    // entities when they are drawn in block documents):
    if (parent != nullptr && parent->rtti() != RS2::EntityGraphic) {
        //if pen is invalid gets all from parent
        if (!p.isValid()) {
            p = parent->getPen(false);
        }
        //pen is valid, verify byBlock parts
        RS_EntityContainer *ep = parent;
        //If parent is byblock check parent.parent (nested blocks)
        while (p.isColorByBlock()) {
            if (ep) {
                p.setColorFromPen(parent->getPen(false)); // fixme - check whether resolved pen is actually needed there...
                ep = ep->parent;
            } else
                break;
        }
        ep = parent;
        while (p.isWidthByBlock()) {
            if (ep) {
                p.setWidthFromPen(parent->getPen(false)); // fixme - check whether resolved pen is actually needed there...
                ep = ep->parent;
            } else
                break;
        }
        ep = parent;
        while (p.isLineTypeByBlock()) {
            if (ep) {
                p.setLineTypeFromPen(parent->getPen(false)); // fixme - check whether resolved pen is actually needed there...
                ep = ep->parent;
            } else
                break;
        }
    }

    // use layer's color:
    bool colorByLayer = p.isColorByLayer();
    bool widthByLayer = p.isWidthByLayer();
    bool lineByLayer = p.isLineTypeByLayer();
    if (colorByLayer || widthByLayer || lineByLayer) {
        RS_Layer *l = getLayerResolved();
        // check byLayer attributes:
        if (l != nullptr) {
            const RS_Pen &layerPen = l->getPen();
            if (colorByLayer) {
                p.setColorFromPen(layerPen);
            }

            // use layer's width:
            if (widthByLayer) {
                p.setWidthFromPen(layerPen);
            }

            // use layer's linetype:
            if (lineByLayer) {
                p.setLineTypeFromPen(layerPen);
            }
        }
    }
    return p;
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
    return resolve ? getPenResolved() : m_pImpl->pen;
}

void RS_Entity::setPen(const RS_Pen& pen) {
    m_pImpl->pen = pen;
}

/**
 * Sets the pen of this entity to the current pen of
 * the graphic this entity is in. If this entity (and none
 * of its parents) are in a graphic the pen is not changed.
 */
void RS_Entity::setPenToActive() {
    RS_Document* doc = getDocument();
    if (doc != nullptr) {
        m_pImpl->pen = doc->getActivePen();
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
// fixme - sand - it seems  this method is   not used
/**
 * @return Factor for scaling the line styles considering the current
 * paper scaling and the fact that styles are stored in Millimeter.
 */
/*double RS_Entity::getStyleFactor(RS_GraphicView* view) {
    double styleFactor = 1.0;
    if (!view) return styleFactor;


    if (view->isPrinting()==false && view->isDraftMode()) {
        styleFactor = 1.0/view->getFactor().x;
    } else {
//styleFactor = getStyleFactor();
// the factor caused by the unit:
        RS2::Unit unit = RS2::None;
        RS_Graphic* g = getGraphic();
        if (g) {
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
        if (graphic && graphic->getPaperScale()>1.0e-6) {
            styleFactor /= graphic->getPaperScale();
        }
    }

//RS_DEBUG->print("stylefactor: %f", styleFactor);
//RS_DEBUG->print("viewfactor: %f", view->getFactor().x);

    if (styleFactor*view->getFactor().x<0.2) {
        styleFactor = -1.0;
    }

    return styleFactor;
}
*/

/**
 * @return User defined variable connected to this entity or nullptr if not found.
 */
QString RS_Entity::getUserDefVar(const QString& key) const {
    auto it=m_pImpl->varList.find(key);
    return (it == m_pImpl->varList.end()) ? QString{} : it->second;
}

/*
 * @coord
 * @normal
 * @bool
 * return a line tangent to entity and orthogonal to the line (*normal)
 */
RS_Vector RS_Entity::getNearestOrthTan(const RS_Vector& /*coord*/,
                    const RS_Line& /*normal*/,
					bool /*onEntity = false*/) const{
        return RS_Vector(false);
}


/**
 * Add a user defined variable to this entity.
 */
void RS_Entity::setUserDefVar(QString key, QString val) {
    m_pImpl->varList.emplace(key, val);
}

/**
 * Deletes the given user defined variable.
 */
void RS_Entity::delUserDefVar(QString key) {
    m_pImpl->varList.erase(key);
}

/**
 * @return A list of all keys connected to this entity.
 */
std::vector<QString> RS_Entity::getAllKeys() const{
    std::vector<QString> ret;
    for(auto const& [key, val]: m_pImpl->varList){
        ret.push_back(key);
    }
    return ret;
}

//! constructionLayer contains entities of infinite length, constructionLayer doesn't show up in print
bool RS_Entity::isConstruction(bool typeCheck) const{
    if(typeCheck &&  getParent() &&  rtti() != RS2::EntityLine){
        // do not expand entities on construction layers, except lines
        return false;
    }

    // Issue #1773, hatch filling curves are not shown as infinite on construction layers
    if (getFlag(RS2::FlagHatchChild)){
        return false;
    }
    /*if (isHatchMember(this))
        return false;*/

    return (m_layer != nullptr) && m_layer->isConstruction();
}

//! whether printing is enabled or disabled for the entity's layer
bool RS_Entity::isPrint(void) const{
    return nullptr == m_layer || m_layer->isPrint();
}

bool RS_Entity::trimmable() const{
    switch(rtti()){
    case RS2::EntityArc:
    case RS2::EntityCircle: // fixme - check whether prepareTrim() is supported there?
    case RS2::EntityEllipse:
    case RS2::EntityLine:
    case RS2::EntityParabola:
    case RS2::EntitySplinePoints: // fixme - check whether prepareTrim() is supported there?
        return true;
    default:
        return false;
    }
}

RS_VectorSolutions RS_Entity::getRefPoints() const{
	return RS_VectorSolutions();
}

RS_Vector RS_Entity::getNearestRef(const RS_Vector& coord,double* dist) const{
	RS_VectorSolutions const&& s = getRefPoints();
	return s.getClosest(coord, dist);
}

RS_Vector RS_Entity::getNearestSelectedRef(const RS_Vector& coord,
										   double* dist) const{
    if (isSelected()) {
        return getNearestRef(coord, dist);
    }
    else {
        return RS_Vector(false);
    }
}

/**
 * Dumps the elements data to stdout.
 */
std::ostream& operator << (std::ostream& os, RS_Entity& e) {
    //os << "Warning: Virtual entity!\n";
    //return os;

    os << " {Entity id: " << e.m_id;
    if (e.parent) {
        os << " | parent id: " << e.parent->getId() << "\n";
    } else {
        os << " | no parent\n";
    }

    os << " flags: " << (e.getFlag(RS2::FlagVisible) ? "RS2::FlagVisible" : "");
    os << (e.getFlag(RS2::FlagUndone) ? " RS2::FlagUndone" : "");
    os << (e.getFlag(RS2::FlagSelected) ? " RS2::FlagSelected" : "");
    os << "\n";

    if (!e.m_layer) {
        os << " layer: nullptr ";
    } else {
        os << " layer: " << e.m_layer->getName().toLatin1().data() << " ";
        os << " layer address: " << e.m_layer << " ";
    }

    os << e.m_pImpl->pen << "\n";

    os << "variable list:\n";
    for(auto const& v: e.m_pImpl->varList){
        os << v.first.toLatin1().data()<< ": "
           << v.second.toLatin1().data()
           << ", ";
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

        case RS2::EntityMText:
            os << (RS_MText&)e;
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

bool RS_Entity::isParentIgnoredOnModifications() const {
     return parent != nullptr && parent->ignoredOnModification();
}


unsigned long long RS_Entity::getId() const
{
    return m_pImpl != nullptr ? m_id : 0ULL;
}
