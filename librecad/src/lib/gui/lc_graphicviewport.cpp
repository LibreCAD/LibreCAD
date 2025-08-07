/*******************************************************************************
*
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2025 LibreCAD.org
 Copyright (C) 2025 sand1024

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 ******************************************************************************/
#include "lc_graphicviewport.h"

#include <QDateTime>

#include "lc_graphicviewportlistener.h"
#include "lc_linemath.h"
#include "lc_overlayentitiescontainer.h"
#include "lc_undoablerelzero.h"
#include "rs_debug.h"
#include "rs_dialogfactory.h"
#include "rs_dialogfactoryinterface.h"
#include "rs_graphic.h"
#include "rs_grid.h"
#include "rs_line.h"
#include "rs_settings.h"
#include "rs_units.h"

LC_GraphicViewport::LC_GraphicViewport():
    grid{std::make_unique<RS_Grid>(this)},
    savedViews(16),
    previousViewTime{std::make_unique<QDateTime>(QDateTime::currentDateTime())} {
}

LC_GraphicViewport::~LC_GraphicViewport() = default;

void LC_GraphicViewport::setContainer(RS_EntityContainer *c) {
    container = c;
    RS_Graphic* g = nullptr;
    if (container->rtti() == RS2::EntityGraphic){
        g = static_cast<RS_Graphic*>(c);
    }
    else{
        g = c->getGraphic();
    }
    setGraphic(g);
}

void LC_GraphicViewport::loadSettings() {
    LC_GROUP("Appearance");
    {
        m_ucsApplyingPolicy = LC_GET_INT("UCSApplyPolicy",0);
        m_modifyOnZoom = LC_GET_BOOL("ModifyOnViewChange", true);
    }
    LC_GROUP_END();

    if (grid != nullptr){
        grid->loadSettings();
    }
}

void LC_GraphicViewport::setBorders(int left, int top, int right, int bottom) {
    borderLeft = left;
    borderTop = top;
    borderRight = right;
    borderBottom = bottom;
}

bool LC_GraphicViewport::areAnglesCounterClockwise(){
    if (graphic != nullptr){
        return graphic->areAnglesCounterClockWise();
    }
    return true;
}

double LC_GraphicViewport::getAnglesBaseAngle(){
    if (graphic != nullptr){
        return graphic->getAnglesBase();
    }
    return 0.0;
}

/**
 * @return true if the grid is switched on.
 */
bool LC_GraphicViewport::isGridOn() const {
    if (container != nullptr) {
        RS_Graphic *graphic = container->getGraphic();
        if (graphic != nullptr) {
            return graphic->isGridOn();
        }
    }
    return false;
}

/**
 * @return true if the grid is isometric
 *
 *@Author: Dongxu Li
 */
bool LC_GraphicViewport::isGridIsometric() const {
    return grid->isIsometric();
}


void LC_GraphicViewport::setIsoViewType(RS2::IsoGridViewType chType) {
    grid->setIsoViewType(chType);
}

RS2::IsoGridViewType LC_GraphicViewport::getIsoViewType() const {
    return grid->getIsoViewType();
}


/**
 * Sets the zoom factor in X for this visualization of the graphic.
 */
void LC_GraphicViewport::setFactorX(double f) {
    if (!zoomFrozen) {
        factor.x = std::abs(f);
        fireViewportChanged();
    }
}

/**
 * Sets the zoom factor in Y for this visualization of the graphic.
 */
void LC_GraphicViewport::setFactorY(double f) {
    if (!zoomFrozen) {
        factor.y = std::abs(f);
        fireViewportChanged();
    }
}

void LC_GraphicViewport::setOffset(int ox, int oy) {
    offsetX = ox;
    offsetY = oy;
    fireViewportChanged();
}


void LC_GraphicViewport::setOffsetX(int ox) {
    offsetX = ox;
    fireViewportChanged();
}

void LC_GraphicViewport::setOffsetY(int oy) {
    offsetY = oy;
    fireViewportChanged();
}


/**
 * zooms in by factor f in y
 */
void LC_GraphicViewport::zoomInY(double f) {
    factor.y *= f;
    offsetY = (int) ((offsetY - getHeight() / 2) * f) + getHeight() / 2;
    fireViewportChanged();
}

/**
 * zooms out by factor f
 */
void LC_GraphicViewport::zoomOut(double f, const RS_Vector &center) {
    if (f < 1.0e-6) {
        RS_DEBUG->print(RS_Debug::D_WARNING, "RS_GraphicView::zoomOut: invalid factor");
        return;
    }
    zoomIn(1 / f, center);
}

/**
 * zooms out by factor f in x
 */
void LC_GraphicViewport::zoomOutX(double f) {
    if (f < 1.0e-6) {
        RS_DEBUG->print(RS_Debug::D_WARNING,"RS_GraphicView::zoomOutX: invalid factor");
        return;
    }
    factor.x /= f;
    offsetX = (int) (offsetX / f);
    fireViewportChanged();
}

void LC_GraphicViewport::centerOffsetXandY(const RS_Vector& containerMin, const RS_Vector& containerSize) {
    if (container && !zoomFrozen) {
        offsetX = (int) (((getWidth() - borderLeft - borderRight) - (containerSize.x * factor.x)) / 2.0 - (containerMin.x * factor.x)) + borderLeft;
        offsetY = (int) ((getHeight() - borderTop - borderBottom - (containerSize.y * factor.y)) / 2.0  - (containerMin.y * factor.y)) + borderBottom;
        fireViewportChanged();
    }
}

/**
 * Centers the drawing in x-direction.
 */
void LC_GraphicViewport::centerOffsetX(const RS_Vector& containerMin, const RS_Vector& containerSize) {
    if (container && !zoomFrozen) {
        offsetX = (int) (((getWidth() - borderLeft - borderRight) - (containerSize.x * factor.x)) / 2.0 - (containerMin.x * factor.x)) + borderLeft;
       fireViewportChanged();
    }
}

/**
 * Centers the drawing in y-direction.
 */
void LC_GraphicViewport::centerOffsetY(const RS_Vector& containerMin, const RS_Vector& containerSize) {
    if (container && !zoomFrozen) {
        offsetY = (int) ((getHeight() - borderTop - borderBottom - (containerSize.y * factor.y)) / 2.0  - (containerMin.y * factor.y)) + borderBottom;
        fireViewportChanged();
    }
}

/**
 * Centers the given coordinate in the view in x-direction.
 */
void LC_GraphicViewport::centerX(double v) {
    if (!zoomFrozen) {
        offsetX = (int) ((v * factor.x) - (double) (getWidth() - borderLeft - borderRight) / 2.0);
        fireViewportChanged();
    }
}

/**
 * Centers the given coordinate in the view in y-direction.
 */
void LC_GraphicViewport::centerY(double v) {
    if (!zoomFrozen) {
        offsetY = (int) ((v * factor.y) - (double) (getHeight() - borderTop - borderBottom) / 2.0);
        fireViewportChanged();
    }
}

/**
 * Centers the point v1.
 */
void LC_GraphicViewport::zoomPan(int dx, int dy) {
    offsetX += dx;
    offsetY -= dy;
    fireViewportChanged();
}

/**
 * Zooms the area given by v1 and v2.
 *
 * @param keepAspectRatio true: keeps the aspect ratio 1:1
 *                        false: zooms exactly the selected range to the
 *                               current graphic view
 */
void LC_GraphicViewport::zoomWindow(RS_Vector v1, RS_Vector v2,bool keepAspectRatio) {

    // Switch left/right and top/bottom is necessary:
    /*  if (v1.x > v2.x) {
          std::swap(v1.x, v2.x);
      }
      if (v1.y > v2.y) {
          std::swap(v1.y, v2.y);
      }

      LC_ERR << "Zoom: Original Diagonal " << v1.distanceTo(v2);
      RS_Vector worldCorner1 = v1;
      RS_Vector worldCorner3 = v2;

      RS_Vector ucsMin;
      RS_Vector ucsMax;

      ucsBoundingBox(worldCorner1, worldCorner3, ucsMin, ucsMax);

      v1 = ucsMin;
      v2 = ucsMax;

      LC_ERR << "Zoom: UCS Diagonal " << v1.distanceTo(v2);*/

    double zoomX = 480.0;    // Zoom for X-Axis
    double zoomY = 640.0;    // Zoom for Y-Axis   (Set smaller one)
    int zoomBorder = 0; // fixme - what for this variable?

// Switch left/right and top/bottom is necessary:
    if (v1.x > v2.x) {
        std::swap(v1.x, v2.x);
    }
    if (v1.y > v2.y) {
        std::swap(v1.y, v2.y);
    }

// Get zoom in X and zoom in Y:
    if (v2.x - v1.x > 1.0e-6) {
        zoomX = getWidth() / (v2.x - v1.x);
    }
    if (v2.y - v1.y > 1.0e-6) {
        zoomY = getHeight() / (v2.y - v1.y);
    }

// Take smaller zoom:
    if (keepAspectRatio) {
        if (zoomX < zoomY) {
            if (getWidth() != 0) {
                zoomX = zoomY = double(getWidth() - 2 * zoomBorder) /
                                getWidth() * zoomX;
            }
        } else {
            if (getHeight() != 0) {
                zoomX = zoomY = double(getHeight() - 2 * zoomBorder) /
                                getHeight() * zoomY;
            }
        }
    }

    zoomX = std::abs(zoomX);
    zoomY = std::abs(zoomY);

// Borders in pixel after zoom
    int pixLeft = int(v1.x * zoomX);
    int pixTop = int(v2.y * zoomY);
    int pixRight = int(v2.x * zoomX);
    int pixBottom = int(v1.y * zoomY);
    if (pixLeft == INT_MIN || pixLeft == INT_MAX ||
        pixRight == INT_MIN || pixRight == INT_MAX ||
        pixTop == INT_MIN || pixTop == INT_MAX ||
        pixBottom == INT_MIN || pixBottom == INT_MAX) {
        RS_DIALOGFACTORY->commandMessage("Requested zooming factor out of range. Zooming not changed");
        return;
    }
    saveView();

// Set new offset for zero point:
    offsetX = (getWidth() - (v1.x + v2.x) * zoomX) / 2;
    offsetY = (getHeight() - (v1.y + v2.y) * zoomY) / 2;
    factor.x = zoomX;
    factor.y = zoomY;

    fireViewportChanged();
}


/**
 * zooms in by factor f
 */
void LC_GraphicViewport::zoomIn(double f, const RS_Vector &center) {
    if (f < 1.0e-6) {
        RS_DEBUG->print(RS_Debug::D_WARNING, "RS_GraphicView::zoomIn: invalid factor");
        return;
    }

    RS_Vector zeroCorner = getUCSViewLeftBottom();
    RS_Vector rightTopCorner = getUCSViewRightTop();

    RS_Vector c = center;
    if (!c.valid) {
        c = (zeroCorner + rightTopCorner) * 0.5;
    }

    const RS_Vector scaleVector = RS_Vector(1.0 / f, 1.0 / f);
    zoomWindow(zeroCorner.scale(c, scaleVector), rightTopCorner.scale(c, scaleVector));
}

/**
 * zooms in by factor f in x
 */
void LC_GraphicViewport::zoomInX(double f) {
    factor.x *= f;
    offsetX = (int) ((offsetX - getWidth() / 2) * f) + getWidth() / 2;
    fireViewportChanged();
}

void LC_GraphicViewport::addViewportListener(LC_GraphicViewPortListener *listener) {
    viewportListeners.append(listener);
}

void LC_GraphicViewport::removeViewportListener(LC_GraphicViewPortListener *listener) {
    viewportListeners.removeOne(listener);
}


void LC_GraphicViewport::fireViewportChanged(){
    invalidateGrid();
    for (int i=0; i<viewportListeners.size(); ++i) {
        LC_GraphicViewPortListener* l = viewportListeners.at(i);
        l->onViewportChanged();
    }
}

void LC_GraphicViewport::invalidateGrid() {
    grid->invalidate(isGridOn());
}

void LC_GraphicViewport::fireRedrawNeeded(){
    for (int i=0; i<viewportListeners.size(); ++i) {
        LC_GraphicViewPortListener* l = viewportListeners.at(i);
        l->onViewportRedrawNeeded();
    }
}

void LC_GraphicViewport::fireUcsChanged(LC_UCS *ucs) {
    invalidateGrid();
    for (int i=0; i<viewportListeners.size(); ++i) {
        LC_GraphicViewPortListener* l = viewportListeners.at(i);
        l->onUCSChanged(ucs);
    }
}

void LC_GraphicViewport::firePreviousZoomChanged([[maybe_unused]]bool value) {
// fixme - ucs - complete - restore!!!
//     emit previous_zoom_state(true);
}

void LC_GraphicViewport::fireRelativeZeroChanged(const RS_Vector &pos){
    for (int i=0; i<viewportListeners.size(); ++i) {
        LC_GraphicViewPortListener* l = viewportListeners.at(i);
        l->onRelativeZeroChanged(pos);
    }
}

/**
 * zooms out by factor f y
 */
void LC_GraphicViewport::zoomOutY(double f) {
    if (f < 1.0e-6) {
        RS_DEBUG->print(RS_Debug::D_WARNING, "RS_GraphicView::zoomOutY: invalid factor");
        return;
    }
    factor.y /= f;
    offsetY = (int) (offsetY / f);
    fireViewportChanged();
}

/**
 * Scrolls in the given direction.
 */
void LC_GraphicViewport::zoomScroll(RS2::Direction direction) {
    switch (direction) {
        case RS2::Up:
            offsetY -= 50;
            break;
        case RS2::Down:
            offsetY += 50;
            break;
        case RS2::Right:
            offsetX += 50;
            break;
        case RS2::Left:
            offsetX -= 50;
            break;
    }
    fireViewportChanged();
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

void LC_GraphicViewport::zoomAutoY(bool axis) {
    if (container) {
        double visibleHeight = 0.0;
        double minY = RS_MAXDOUBLE;
        double maxY = RS_MINDOUBLE;
        bool noChange = false;

        // fixme - sand -- ?? WHY ?? What if there are entities outside lines? This is not reliable at all
        for (auto e: *container) {
            if (e->rtti() == RS2::EntityLine) {
                auto *l = (RS_Line *) e;
                double x1, x2;
                x1 = toGuiX(l->getStartpoint().x);
                x2 = toGuiX(l->getEndpoint().x);

                if (((x1 > 0.0) && (x1 < (double) getWidth())) ||
                    ((x2 > 0.0) && (x2 < (double) getWidth()))) {
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
            visibleHeight = maxY - minY;
        }

        if (visibleHeight < 1.0) {
            noChange = true;
        }

        double fy = 1.0;
        if (visibleHeight > 1.0e-6) {
            fy = (getHeight() - borderTop - borderBottom)
                 / visibleHeight;
            if (factor.y < 0.000001) {
                noChange = true;
            }
        }

        if (noChange == false) {
            if (!zoomFrozen) {
                factor.y = std::abs(fy);
            }
            offsetY = (int) ((getHeight() - borderTop - borderBottom
                              - (visibleHeight * factor.y)) / 2.0
                             - (minY * factor.y)) + borderBottom;
            fireViewportChanged();

        }
        RS_DEBUG->print("Auto zoom y ok");
    }
}

void LC_GraphicViewport::zoomAutoEnsurePointsIncluded(const RS_Vector &wcsP1, const RS_Vector &wcsP2, const RS_Vector &wcsP3) {
    if (container) {
        container->calculateBorders();
        RS_Vector min = container->getMin();
        RS_Vector max = container->getMax();

        min = RS_Vector::minimum(min, wcsP1);
        min = RS_Vector::minimum(min, wcsP2);
        min = RS_Vector::minimum(min, wcsP3);

        max = RS_Vector::maximum(max, wcsP1);
        max = RS_Vector::maximum(max, wcsP2);
        max = RS_Vector::maximum(max, wcsP3);
        doZoomAuto(min,max, true, true);
    }
}

/**
 * performs autozoom
 *
 * @param axis include axis in zoom
 * @param keepAspectRatio true: keep aspect ratio 1:1
 *                        false: factors in x and y are stretched to the max
 */
void LC_GraphicViewport::zoomAuto(bool axis, bool keepAspectRatio) {
    RS_DEBUG->print("RS_GraphicView::zoomAuto");
    if (container) {
        container->calculateBorders();
        RS_Vector min = container->getMin();
        RS_Vector max = container->getMax();
        doZoomAuto(min,max, axis, keepAspectRatio);
    }
    RS_DEBUG->print("RS_GraphicView::zoomAuto OK");
}

void LC_GraphicViewport::doZoomAuto(const RS_Vector& min, const RS_Vector& max, bool axis, bool keepAspectRatio) {
    double sx = 0., sy = 0.;
    RS_Vector containerSize;
    RS_Vector containerMin;

    if (hasUCS()){
        RS_Vector ucsMin;
        RS_Vector ucsMax;

        ucsBoundingBox(min, max, ucsMin, ucsMax);

        RS_Vector ucsSize = ucsMax - ucsMin;

        sx = ucsSize.x;
        sy = ucsSize.y;

        containerSize = ucsSize;
        containerMin = ucsMin;
    }
    else {
        auto const dV = max - min;
        if (axis) {
            sx = std::max(dV.x, 0.);
            sy = std::max(dV.y, 0.);
        } else {
            sx = dV.x;
            sy = dV.y;
        }
        containerSize = dV;
        containerMin = min;
    }

    double fx = 1., fy = 1.;
    unsigned short fFlags = 0;

    if (sx > RS_TOLERANCE) {
        fx = (getWidth() - borderLeft - borderRight) / sx;
    } else {
        fFlags += 1; //invalid x factor
    }

    if (sy > RS_TOLERANCE) {
        fy = (getHeight() - borderTop - borderBottom) / sy;
    } else {
        fFlags += 2; //invalid y factor
    }
    switch (fFlags) {
        case 1:
            fx = fy;
            break;
        case 2:
            fy = fx;
            break;
        case 3:
            return; //do not do anything, invalid factors
        default:
            if (keepAspectRatio) {
                fx = fy = std::min(fx, fy);
            }
    }
    //exclude invalid factors
    fFlags = 0;
    if (fx < RS_TOLERANCE || fx > RS_MAXDOUBLE) {
        fx = 1.0;
        fFlags += 1;
    }
    if (fy < RS_TOLERANCE || fy > RS_MAXDOUBLE) {
        fy = 1.0;
        fFlags += 2;
    }
    if (fFlags == 3) return;
    saveView();

    if (!zoomFrozen) {
        factor.x = std::abs(fx);
        factor.y = std::abs(fy);
    }
    centerOffsetXandY(containerMin, containerSize);
}

RS_Grid *LC_GraphicViewport::getGrid() const {
    return grid.get();
}


/**
 * Shows previous view.
 */
void LC_GraphicViewport::zoomPrevious() {

    RS_DEBUG->print("RS_GraphicView::zoomPrevious");

    if (container) {
        restoreView();
    }
}

/**
 * Saves the current view as previous view to which we can
 * switch back later with @see restoreView().
 */
void LC_GraphicViewport::saveView() {
    if (graphic != nullptr) {
        if (m_modifyOnZoom) {
            getGraphic()->setModified(true);
        }
    }
    QDateTime noUpdateWindow = QDateTime::currentDateTime().addMSecs(-500);
//do not update view within 500 milliseconds
    if (*previousViewTime > noUpdateWindow) {
        return;
    }
    *previousViewTime = QDateTime::currentDateTime();
    savedViews[savedViewIndex] = std::make_tuple(offsetX, offsetY, factor);
    savedViewIndex = (savedViewIndex + 1) % savedViews.size();
    if (savedViewCount < savedViews.size()) savedViewCount++;

    if (savedViewCount == 1) {
        firePreviousZoomChanged(true);
    }
}


/**
 * Restores the view previously saved with
 * @see saveView().
 */
void LC_GraphicViewport::restoreView() {
    if (savedViewCount == 0) return;
    savedViewCount--;
    if (savedViewCount == 0) {
//        emit previous_zoom_state(false);
        firePreviousZoomChanged(false);
    }
    savedViewIndex = (savedViewIndex + savedViews.size() - 1) % savedViews.size();

    offsetX = std::get<0>(savedViews[savedViewIndex]);
    offsetY = std::get<1>(savedViews[savedViewIndex]);
    factor = std::get<2>(savedViews[savedViewIndex]);

    fireViewportChanged();
}


void LC_GraphicViewport::setFactor(double f) {
    if (!zoomFrozen) {
        double absF = std::abs(f);
        factor.x = absF;
        factor.y = absF;
        fireViewportChanged();
    }
}

void LC_GraphicViewport::setOffsetAndFactor(int ox, int oy, double f){
    justSetOffsetAndFactor(ox, oy, f);
    fireViewportChanged();
}

void LC_GraphicViewport::justSetOffsetAndFactor(int ox, int oy, double f){
    offsetX = ox;
    offsetY = oy;
    factor.x = std::abs(f);
    factor.y = std::abs(f);
    // fixme - ucs - this method is called on initial reading, is it necessary to update listeners there (as force redraw may be invoked later???)
    //  fireViewportChanged();
}

/*RS_Vector LC_GraphicViewport::toUCSFromGui(const QPointF& pos) const{
    return RS_Vector(toUcsX(pos.x()), toUcsY(pos.y()));
}*/

RS_Vector LC_GraphicViewport::getUCSViewLeftBottom() const{
    return toUCSFromGui(0,0);
}

RS_Vector LC_GraphicViewport::getUCSViewRightTop() const{
    return toUCSFromGui(getWidth(),getHeight());
}

double LC_GraphicViewport::toAbsUCSAngle(double ucsRelAngle) {
    return toUCSAbsAngle(ucsRelAngle, getAnglesBaseAngle(), areAnglesCounterClockwise());
}

double LC_GraphicViewport::toBasisUCSAngle(double ucsAbsAngle) {
    return toUCSBasisAngle(ucsAbsAngle, getAnglesBaseAngle(), areAnglesCounterClockwise());
}

void LC_GraphicViewport::toUI(RS_Vector wcsCoordinate, double &uiX, double &uiY) const{
    if (hasUCS()){
        doWCS2UCS(wcsCoordinate.x, wcsCoordinate.y, uiX, uiY);
        uiX = toGuiX(uiX);
        uiY = toGuiY(uiY);
    }
    else{
        uiX = toGuiX(wcsCoordinate.x);
        uiY = toGuiY(wcsCoordinate.y);
    }
};

void LC_GraphicViewport::loadGridSettings() {
    if (grid != nullptr){
        grid->loadSettings();
    }
    fireRedrawNeeded();
}

void LC_GraphicViewport::setUCS(const RS_Vector &origin, double angle, bool isometric, RS2::IsoGridViewType isoType) {
    RS_Vector ucsOrigin = doSetUCS(origin, angle, isometric, isoType);
    switch (m_ucsApplyingPolicy){
        case UCSApplyingPolicy::ZoomAuto: {
            zoomAuto();
            break;
        }
        case UCSApplyingPolicy::PanOriginCenter: {
            int offX = (int) ((ucsOrigin.x * factor.x) + (double) (getWidth() - borderLeft - borderRight) / 2.0);
            int offY = (int) ((ucsOrigin.y * factor.y) + (double) (getHeight() - borderTop - borderBottom) / 2.0);
            setOffset(offX, offY);
            break;
        }
        case UCSApplyingPolicy::PanOriginLowerLeft:{
            setOffset(ucsOrigin.x* factor.x+(borderLeft + borderRight)/2, ucsOrigin.y* factor.y + (borderBottom + borderRight)/2);
            break;
        }
        default:{
            zoomAuto();
        }
    }
}

void LC_GraphicViewport::applyUCS(LC_UCS *ucsToSet) {
    if (ucsToSet == nullptr){
        return;
    }
    RS_Vector originToSet = ucsToSet->getOrigin();
    double angleToSet = ucsToSet->getXAxisDirection();
    bool hasIso = ucsToSet->isIsometric();
    RS2::IsoGridViewType isoType = ucsToSet->getIsoGridViewType();
    setUCS(originToSet, angleToSet, hasIso, isoType);
    if (graphic != nullptr){
        graphic->setCurrentUCS(ucsToSet);
    }
    fireUcsChanged(ucsToSet);
}

void LC_GraphicViewport::extractUCS(){
    if (hasUCS()){
        if (graphic != nullptr) {
            LC_UCSList *ucsList = graphic->getUCSList();
            LC_UCS *candidate = createUCSEntity(getUcsOrigin(), -getXAxisAngle(), isGridIsometric(), getIsoViewType());
            LC_UCS *createdUCS = ucsList->tryAddUCS(candidate);
            if (createdUCS != nullptr) {
                applyUCS(createdUCS);
            }
        }
    }
}


RS_Vector LC_GraphicViewport::doSetUCS(const RS_Vector &origin, double angle, bool isometric, RS2::IsoGridViewType &isoType) {
    bool customUCS = LC_LineMath::isMeaningfulAngle(angle) || LC_LineMath::isMeaningfulDistance(origin, RS_Vector(0, 0, 0));
    RS_Vector ucsOrigin = customUCS ? toUCS(origin) : RS_Vector{0., 0.};
    if (customUCS)
        update(origin, -angle);
    useUCS(customUCS);
    auto g = getGraphic();
    if (g != nullptr){
        bool oldIsometricGrid = g->isIsometricGrid();
        RS2::IsoGridViewType oldIsoViewType = g->getIsoView();
        if (oldIsometricGrid != isometric || oldIsoViewType != isoType) {
            g->setIsometricGrid(isometric);
            if (isometric) {
                g->setIsoView(isoType);
            }
            loadGridSettings();
        }
    }
    return ucsOrigin;
}


void LC_GraphicViewport::createUCS(const RS_Vector &origin, double angle) {
    bool customUCS = LC_LineMath::isMeaningfulAngle(angle) || LC_LineMath::isMeaningfulDistance(origin, RS_Vector(0, 0, 0));
    if (customUCS){
        auto g = getGraphic();
        if (g != nullptr) {
            auto ucsList = g->getUCSList();
            auto candidate = createUCSEntity(origin, angle, isGridIsometric(), getIsoViewType());
            auto createdUCS = ucsList->tryAddUCS(candidate);
            if (createdUCS != nullptr){
                setUCS(origin, angle, isGridIsometric(), getIsoViewType());
                g->setCurrentUCS(createdUCS);
                fireUcsChanged(createdUCS);
            }
        }
    }
}

LC_UCS *LC_GraphicViewport::createUCSEntity(const RS_Vector &origin, double angle, bool isometric, RS2::IsoGridViewType isoType) const{
    auto* result = new LC_UCS("");
    result->setOrigin(origin);

    RS_Vector xAxis = RS_Vector(1.0, 0, 0);
    RS_Vector yAxis = RS_Vector(0,1.0, 0);

    xAxis.rotate(angle);
    yAxis.rotate(angle);

    result->setXAxis(xAxis);
    result->setYAxis(yAxis);

    result->setElevation(0.0);
    result->setTemporary(true);

    result->setOrthoOrigin(RS_Vector(0,0,0));

    int orthoType = LC_UCS::NON_ORTHO;
    if (isometric){
        switch (isoType){
            case (RS2::IsoRight):{
                orthoType = LC_UCS::RIGHT;
                break;
            }
            case (RS2::IsoLeft):{
                orthoType = LC_UCS::LEFT;
                break;
            }
            case (RS2::IsoTop):{
                orthoType = LC_UCS::TOP;
                break;
            }
            default:
                orthoType = LC_UCS::NON_ORTHO;
        }
    }
    result->setOrthoType(orthoType);
    return result;
}

void LC_GraphicViewport::applyUCSAfterLoad(){
    LC_UCS* ucsCurrent = getGraphic()->getCurrentUCS();
    if (ucsCurrent != nullptr) {
        RS_Vector originToSet = ucsCurrent->getOrigin();
        double angleToSet = ucsCurrent->getXAxisDirection();
        bool isometric = ucsCurrent->isIsometric();
        RS2::IsoGridViewType isoType = ucsCurrent->getIsoGridViewType();
        doSetUCS(originToSet, angleToSet, isometric, isoType);
        fireUcsChanged(ucsCurrent);
        delete ucsCurrent;
    }
}

void LC_GraphicViewport::fillCurrentUCSInfo(RS_Vector& origin, double& xAsixDirection) const {
    if (hasUCS()) {
        origin = getUcsOrigin();
        xAsixDirection = -getXAxisAngle();
    }
    else {
        origin = RS_Vector(0, 0, 0);
        xAsixDirection = 0;
    }
}

LC_UCS* LC_GraphicViewport::getCurrentUCS() const{
    LC_UCS* result = nullptr;
    if (hasUCS()){
        result = createUCSEntity(getUcsOrigin(), -getXAxisAngle(),isGridIsometric(), getIsoViewType());
    }
    return result;
}

RS_Vector LC_GraphicViewport::snapGrid(const RS_Vector &coord) const {
    if (hasUCS()) {
        // basically, wcs coordinate still should be returned there.
        // however, it will be rotated according to the grid (which is not rotated in ucs).
        RS_Vector snap = getGrid()->snapGrid(toUCS(coord));
        snap = toWorld(snap);
        return snap;
    }
    else{
        RS_Vector snap = getGrid()->snapGrid(coord);
        return snap;
    }
}

void LC_GraphicViewport::restoreView(LC_View *view) {
    if (view == nullptr){
        return;
    }

    RS_Vector center = view->getCenter();
    RS_Vector size = view->getSize();

    const RS_Vector halfSize = size / 2;
    RS_Vector v1 = center - halfSize;
    RS_Vector v2 = center + halfSize;

    RS_Vector origin = RS_Vector(0,0,0);
    double angle = 0;
    bool isometric = false;
    RS2::IsoGridViewType isoType = RS2::IsoTop;

    auto* ucs = view->getUCS();
    if (ucs != nullptr){
        origin = ucs->getOrigin();
        angle = ucs->getXAxisDirection();
        isometric = ucs->isIsometric();
        isoType = ucs->getIsoGridViewType();
    }
    else{
        ucs = &LC_WCS::instance;
    };

    auto g = getGraphic();
    if (g != nullptr){
        g->setCurrentUCS(ucs);
    }
    doSetUCS(origin, angle, isometric, isoType);
    fireUcsChanged(ucs);
    zoomWindow(v1, v2, true);
}

void LC_GraphicViewport::initAfterDocumentOpen() {
    applyUCSAfterLoad();
}

LC_View* LC_GraphicViewport::createNamedView(QString name) const{
    auto* viewToCreate = new LC_View(name);
    doUpdateViewByGraphicView(viewToCreate);
    return viewToCreate;
}

void LC_GraphicViewport::updateNamedView(LC_View* view) const{
    doUpdateViewByGraphicView(view);
}

void LC_GraphicViewport::doUpdateViewByGraphicView(LC_View *view) const {
    view->setForPaperView(isPrintPreview());

    int width = getWidth();
    int height = getHeight();

    double x = toUcsX(width);
    double y = toUcsY(height);

    double x0 = toUcsX(0);
    double y0 = toUcsY(0);

    view->setCenter({(x + x0) / 2.0, (y + y0) / 2.0, 0});
    view->setSize({(x - x0), (y - y0), 0});

    view->setTargetPoint({0, 0, 0});

    LC_UCS* viewUCS = getCurrentUCS();
    if (viewUCS != nullptr) {
        view->setUCS(viewUCS);
        if (graphic != nullptr) {
            LC_UCSList *ucsList = graphic->getUCSList();

            LC_UCS *existingListUCS = ucsList->findExisting(viewUCS);
            if (existingListUCS != nullptr) {
                QString ucsName = existingListUCS->getName();
                viewUCS->setName(ucsName);
            }
        }
    }
    else{
        // this is WCS
        view->setUCS(new LC_WCS());
    }
}

/**
 * Sets the relative zero coordinate (if not locked)
 * without deleting / drawing the point.
 */
void LC_GraphicViewport::setRelativeZero(const RS_Vector &pos) {
    if (!relativeZeroLocked) {
        markedRelativeZero = relativeZero;
        relativeZero = pos;
        fireRelativeZeroChanged(pos);
    }
}

/**
 * Sets the relative zero coordinate, deletes the old position
 * on the screen and draws the new one.
 */
void LC_GraphicViewport::moveRelativeZero(const RS_Vector &pos) {
    setRelativeZero(pos);
}

RS_Undoable *LC_GraphicViewport::getRelativeZeroUndoable() {
    RS_Undoable* result = nullptr;
    if (LC_LineMath::isMeaningfulDistance(markedRelativeZero, relativeZero)){
        result = new LC_UndoableRelZero(this, markedRelativeZero, relativeZero);
        markRelativeZero();
    }
    return result;
}

void LC_GraphicViewport::setGraphic(RS_Graphic *g) {
     graphic = g;
     overlaysManager.setGraphic(g);
}

/**
 * Zooms to page extends.
 */
 // fixme - ucs, potentially, these methods should live in some other place...
void LC_GraphicViewport::zoomPage() {
    RS_DEBUG->print("RS_GraphicView::zoomPage");
    if (!container) {
        return;
    }

    RS_Graphic *graphic = container->getGraphic();
    if (!graphic) {
        return;
    }

    RS_Vector s = graphic->getPrintAreaSize() / graphic->getPaperScale();

    double fx = 0., fy = 0.;

    if (s.x > RS_TOLERANCE) {
        fx = (getWidth() - borderLeft - borderRight) / s.x;
    } else {
        fx = 1.0;
    }

    if (s.y > RS_TOLERANCE) {
        fy = (getHeight() - borderTop - borderBottom) / s.y;
    } else {
        fy = 1.0;
    }

    RS_DEBUG->print("f: %f/%f", fx, fy);

    fx = fy = std::min(fx, fy);

    RS_DEBUG->print("f: %f/%f", fx, fy);

    if (fx < RS_TOLERANCE) {
        fx = fy = 1.0;
    }

     if (!zoomFrozen) {
         factor.x = std::abs(fx);
         factor.y = std::abs(fy);
     }


    RS_DEBUG->print("f: %f/%f", fx, fy);

    RS_Vector containerMin = container->getMin();
    RS_Vector containerSize = container->getSize();

     centerOffsetXandY(containerMin, containerSize);
    // fixme - remove debug code
//    LC_ERR << "Normal Zoom " << offsetX << " , " << offsetY << " Factor: " << fx;;
//    adjustOffsetControls();
//    adjustZoomControls();
//    updateGrid();

//    redraw();
//    fireUpdateNeeded();
}

void LC_GraphicViewport::zoomPageEx() {
    RS_DEBUG->print("RS_GraphicView::zoomPage");
    if (!container) {
        return;
    }

    RS_Graphic *graphic = container->getGraphic();
    if (!graphic) {
        return;
    }

    RS2::Unit dest = graphic->getUnit();
    double marginsWidth = RS_Units::convert(graphic->getMarginLeft() + graphic->getMarginRight(), RS2::Millimeter, dest);
    double marginsHeight = RS_Units::convert(graphic->getMarginTop() +graphic->getMarginBottom(), RS2::Millimeter, dest);

    const RS_Vector &printAreaSize = graphic->getPrintAreaSize(true);
    double paperScale = graphic->getPaperScale();
    RS_Vector printAreaSizeInViewCoordinates = (printAreaSize + RS_Vector(marginsWidth, marginsHeight)) / paperScale;

    LC_ERR<<"margin: "<<marginsWidth<<", "<<marginsHeight;
    LC_ERR<<__LINE__<<" printAreaSizeInViewCoordinates "<< printAreaSizeInViewCoordinates.x<<", "<<printAreaSizeInViewCoordinates.y;
    double fx=1., fy=1.;

    int widthToFit = getWidth() - borderLeft - borderRight;

    if (printAreaSizeInViewCoordinates.x > RS_TOLERANCE) {
        fx = widthToFit / printAreaSizeInViewCoordinates.x;
    } else {
        fx = 1.0;
    }

    int heightToFit = getHeight() - borderTop - borderBottom;
    if (printAreaSizeInViewCoordinates.y > RS_TOLERANCE) {
        fy = heightToFit / printAreaSizeInViewCoordinates.y;
    } else {
        fy = 1.0;
    }

    RS_DEBUG->print("f: %f/%f", fx, fy);

    fx = fy = std::min(fx, fy);

    RS_DEBUG->print("f: %f/%f", fx, fy);

    if (fx < RS_TOLERANCE) {
        fx = fy = 1.0;
    }

    if (!zoomFrozen) {
        factor.x = std::abs(fx);
        factor.y = std::abs(fy);
    }

    RS_DEBUG->print("f: %f/%f", fx, fy);

    const RS_Vector &paperInsertionBase = graphic->getPaperInsertionBase();

    offsetX = (int) ((getWidth() - borderLeft - borderRight - (printAreaSizeInViewCoordinates.x) * factor.x) / 2.0 +
                     (paperInsertionBase.x * factor.x / paperScale)) + borderLeft;

    fy = factor.y;

    offsetY = (int) ((getHeight() - borderTop - borderBottom - (printAreaSizeInViewCoordinates.y) * fy) / 2.0 +
                     paperInsertionBase.y * fy / paperScale) + borderBottom;

    LC_LOG<<"LC_GraphicViewport::"<<__func__<<"(): end normally";
    fireViewportChanged();
}

void LC_GraphicViewport::clearOverlayEntitiesContainer(RS2::OverlayGraphics overlayType) {
    auto overlayEntities = overlaysManager.entitiesAt(overlayType);
    if (overlayEntities != nullptr) {
        overlayEntities->clear();
    }
}

void LC_GraphicViewport::clearOverlayDrawablesContainer(RS2::OverlayGraphics overlayType) {
    auto overlayDrawables = overlaysManager.drawablesAt(overlayType);
    if (overlayDrawables != nullptr) {
        overlayDrawables->clear();
    }
}
