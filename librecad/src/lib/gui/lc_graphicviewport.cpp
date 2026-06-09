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

#include "lc_defaults.h"
#include "lc_graphicviewportlistener.h"
#include "lc_linemath.h"
#include "lc_overlayentitiescontainer.h"
#include "lc_refpoint.h"
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
    m_grid{std::make_unique<RS_Grid>(this)},
    m_savedViews(16),
    m_previousViewTime{std::make_unique<QDateTime>(QDateTime::currentDateTime())},
    m_formatter{std::make_unique<LC_Formatter>(this)} {
}

LC_GraphicViewport::~LC_GraphicViewport() = default;

void LC_GraphicViewport::setDocument(RS_Document *c) {
    m_document = c;
    RS_Graphic* g = nullptr;
    if (m_document->rtti() == RS2::EntityGraphic){
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
        m_refPointMode = LC_GET_INT("RefPointType", DXF_FORMAT_PDMode_EncloseSquare(DXF_FORMAT_PDMode_CentreDot));
        const QString pdsizeStr = LC_GET_STR("RefPointSize", "2.0");

        bool ok = false;
        m_refPointSize = RS_Math::eval(pdsizeStr, &ok);
        if (!ok) {
            m_refPointSize = LC_DEFAULTS_PDSize;
        }
    }
    LC_GROUP_END();

    if (m_grid != nullptr){
        m_grid->loadSettings();
    }

    m_formatter->updateByGraphic(m_graphic);
}

void LC_GraphicViewport::setBorders(const int left, const int top, const int right, const int bottom) {
    m_borderLeft = left;
    m_borderTop = top;
    m_borderRight = right;
    m_borderBottom = bottom;
}

bool LC_GraphicViewport::areAnglesCounterClockwise() const {
    if (m_graphic != nullptr){
        return m_graphic->areAnglesCounterClockWise();
    }
    return true;
}

double LC_GraphicViewport::getAnglesBaseAngle() const {
    if (m_graphic != nullptr){
        return m_graphic->getAnglesBase();
    }
    return 0.0;
}

void LC_GraphicViewport::highlightLocation(const RS_Vector &vector) {
    const auto container = m_overlaysManager.getEntitiesContainer(RS2::OverlayGraphics::PermanentHighlights);
    container->addEntity(new LC_RefPoint(container, vector,m_refPointSize, m_refPointMode));
    notifyChanged(RS2::RedrawOverlay, true);
}

void LC_GraphicViewport::clearLocationsHighlight()  {
    const auto container = m_overlaysManager.getEntitiesContainer(RS2::OverlayGraphics::PermanentHighlights);
    container->clear();
}

/**
 * @return true if the grid is switched on.
 */
bool LC_GraphicViewport::isGridOn() const {
    if (m_document != nullptr) {
        const RS_Graphic *graphic = m_document->getGraphic();
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
    return m_grid->isIsometric();
}


void LC_GraphicViewport::setIsoViewType(const RS2::IsoGridViewType chType) const {
    m_grid->setIsoViewType(chType);
}

RS2::IsoGridViewType LC_GraphicViewport::getIsoViewType() const {
    return m_grid->getIsoViewType();
}


/**
 * Sets the zoom factor in X for this visualization of the graphic.
 */
void LC_GraphicViewport::setFactorX(const double f) {
    if (!m_zoomFrozen) {
        m_factor.x = std::abs(f);
        fireViewportChanged();
    }
}

/**
 * Sets the zoom factor in Y for this visualization of the graphic.
 */
void LC_GraphicViewport::setFactorY(const double f) {
    if (!m_zoomFrozen) {
        m_factor.y = std::abs(f);
        fireViewportChanged();
    }
}

void LC_GraphicViewport::setOffset(const int ox, const int oy) {
    m_offsetX = ox;
    m_offsetY = oy;
    fireViewportChanged();
}


void LC_GraphicViewport::setOffsetX(const int ox) {
    m_offsetX = ox;
    fireViewportChanged();
}

void LC_GraphicViewport::setOffsetY(const int oy) {
    m_offsetY = oy;
    fireViewportChanged();
}


/**
 * zooms in by factor f in y
 */
void LC_GraphicViewport::zoomInY(const double f) {
    m_factor.y *= f;
    m_offsetY = static_cast<int>((m_offsetY - getHeight() / 2) * f) + getHeight() / 2;
    fireViewportChanged();
}

/**
 * zooms out by factor f
 */
void LC_GraphicViewport::zoomOut(const double f, const RS_Vector &center) {
    if (f < 1.0e-6) {
        RS_DEBUG->print(RS_Debug::D_WARNING, "RS_GraphicView::zoomOut: invalid factor");
        return;
    }
    zoomIn(1 / f, center);
}

/**
 * zooms out by factor f in x
 */
void LC_GraphicViewport::zoomOutX(const double f) {
    if (f < 1.0e-6) {
        RS_DEBUG->print(RS_Debug::D_WARNING,"RS_GraphicView::zoomOutX: invalid factor");
        return;
    }
    m_factor.x /= f;
    m_offsetX = static_cast<int>(m_offsetX / f);
    fireViewportChanged();
}

void LC_GraphicViewport::centerOffsetXandY(const RS_Vector& containerMin, const RS_Vector& containerSize) {
    if ((m_document != nullptr) && !m_zoomFrozen) {
        m_offsetX = static_cast<int>((getWidth() - m_borderLeft - m_borderRight - containerSize.x * m_factor.x) / 2.0 - containerMin.x * m_factor.x) + m_borderLeft;
        m_offsetY = static_cast<int>((getHeight() - m_borderTop - m_borderBottom - containerSize.y * m_factor.y) / 2.0 - containerMin.y * m_factor.y) + m_borderBottom;
        fireViewportChanged();
    }
}

/**
 * Centers the drawing in x-direction.
 */
void LC_GraphicViewport::centerOffsetX(const RS_Vector& containerMin, const RS_Vector& containerSize) {
    if ((m_document != nullptr) && !m_zoomFrozen) {
        m_offsetX = static_cast<int>((getWidth() - m_borderLeft - m_borderRight - containerSize.x * m_factor.x) / 2.0 - containerMin.x * m_factor.x) + m_borderLeft;
       fireViewportChanged();
    }
}

/**
 * Centers the drawing in y-direction.
 */
void LC_GraphicViewport::centerOffsetY(const RS_Vector& containerMin, const RS_Vector& containerSize) {
    if ((m_document != nullptr) && !m_zoomFrozen) {
        m_offsetY = static_cast<int>((getHeight() - m_borderTop - m_borderBottom - containerSize.y * m_factor.y) / 2.0 - containerMin.y * m_factor.y) + m_borderBottom;
        fireViewportChanged();
    }
}

/**
 * Centers the given coordinate in the view in x-direction.
 */
void LC_GraphicViewport::centerX(const double x) {
    if (!m_zoomFrozen) {
        m_offsetX = static_cast<int>(m_factor.x * x - static_cast<double>(getWidth() - m_borderLeft - m_borderRight) / 2.0);
        fireViewportChanged();
    }
}

/**
 * Centers the given coordinate in the view in y-direction.
 */
void LC_GraphicViewport::centerY(const double y) {
    if (!m_zoomFrozen) {
        m_offsetY = static_cast<int>(y * m_factor.y - static_cast<double>(getHeight() - m_borderTop - m_borderBottom) / 2.0);
        fireViewportChanged();
    }
}

/**
 * Centers the point v1.
 */
void LC_GraphicViewport::zoomPan(const int dx, const int dy) {
    m_offsetX += dx;
    m_offsetY -= dy;
    fireViewportChanged();
}

/**
 * Zooms the area given by v1 and v2.
 *
 * @param v1
 * @param v2
 * @param keepAspectRatio true: keeps the aspect ratio 1:1
 *                        false: zooms exactly the selected range to the
 *                               current graphic view
 */
void LC_GraphicViewport::zoomWindow(RS_Vector v1, RS_Vector v2, const bool keepAspectRatio) {

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
    const int zoomBorder = 0; // fixme - what for this variable?

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
                const double zoom = static_cast<double>(getWidth() - 2 * zoomBorder) / getWidth() * zoomX;
                zoomX = zoom;
                zoomY = zoom;
            }
        } else {
            if (getHeight() != 0) {
                const double zoom = static_cast<double>(getHeight() - 2 * zoomBorder) / getHeight() * zoomY;
                zoomX = zoom;
                zoomY = zoom;
            }
        }
    }

    zoomX = std::abs(zoomX);
    zoomY = std::abs(zoomY);

// Borders in pixel after zoom
    const int pixLeft = static_cast<int>(v1.x * zoomX);
    const int pixTop = static_cast<int>(v2.y * zoomY);
    const int pixRight = static_cast<int>(v2.x * zoomX);
    const int pixBottom = static_cast<int>(v1.y * zoomY);
    if (pixLeft == INT_MIN || pixLeft == INT_MAX ||
        pixRight == INT_MIN || pixRight == INT_MAX ||
        pixTop == INT_MIN || pixTop == INT_MAX ||
        pixBottom == INT_MIN || pixBottom == INT_MAX) {
        RS_DIALOGFACTORY->commandMessage("Requested zooming factor out of range. Zooming not changed");
        return;
    }
    saveView();

// Set new offset for zero point:
    m_offsetX = (getWidth() - (v1.x + v2.x) * zoomX) / 2;
    m_offsetY = (getHeight() - (v1.y + v2.y) * zoomY) / 2;
    m_factor.x = zoomX;
    m_factor.y = zoomY;

    fireViewportChanged();
}


/**
 * zooms in by factor f
 */
void LC_GraphicViewport::zoomIn(const double f, const RS_Vector &center) {
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

    const auto scaleVector = RS_Vector(1.0 / f, 1.0 / f);
    zoomWindow(zeroCorner.scale(c, scaleVector), rightTopCorner.scale(c, scaleVector));
}

/**
 * zooms in by factor f in x
 */
void LC_GraphicViewport::zoomInX(const double f) {
    m_factor.x *= f;
    m_offsetX = static_cast<int>((m_offsetX - getWidth() / 2) * f) + getWidth() / 2;
    fireViewportChanged();
}

void LC_GraphicViewport::addViewportListener(LC_GraphicViewPortListener *listener) {
    m_viewportListeners.append(listener);
}

void LC_GraphicViewport::removeViewportListener(LC_GraphicViewPortListener *listener) {
    m_viewportListeners.removeOne(listener);
}


void LC_GraphicViewport::fireViewportChanged() const {
    invalidateGrid();
    for (const auto l : m_viewportListeners) {
        l->onViewportChanged();
    }
}

void LC_GraphicViewport::invalidateGrid() const {
    m_grid->invalidate(isGridOn());
}

void LC_GraphicViewport::fireRedrawNeeded(const RS2::RedrawMethod method, bool immediately) const {
    for (const auto l : m_viewportListeners) {
        l->onViewportRedrawNeeded(method, immediately);
    }
}

void LC_GraphicViewport::fireUcsChanged(LC_UCS *ucs) const {
    invalidateGrid();
    for (const auto l : m_viewportListeners) {
        l->onUCSChanged(ucs);
    }
}

void LC_GraphicViewport::firePreviousZoomChanged([[maybe_unused]]bool value) {
// fixme - ucs - complete - restore!!!
//     emit previousZoomAvailable(true);
}

void LC_GraphicViewport::fireRelativeZeroChanged(const RS_Vector &pos) const {
    for (const auto l : m_viewportListeners) {
        l->onRelativeZeroChanged(pos);
    }
}

/**
 * zooms out by factor f y
 */
void LC_GraphicViewport::zoomOutY(const double f) {
    if (f < 1.0e-6) {
        RS_DEBUG->print(RS_Debug::D_WARNING, "RS_GraphicView::zoomOutY: invalid factor");
        return;
    }
    m_factor.y /= f;
    m_offsetY = static_cast<int>(m_offsetY / f);
    fireViewportChanged();
}

/**
 * Scrolls in the given direction.
 */
void LC_GraphicViewport::zoomScroll(const RS2::Direction direction) {
    switch (direction) {
        case RS2::Up:
            m_offsetY -= 50;
            break;
        case RS2::Down:
            m_offsetY += 50;
            break;
        case RS2::Right:
            m_offsetX += 50;
            break;
        case RS2::Left:
            m_offsetX -= 50;
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

void LC_GraphicViewport::zoomAutoY(const bool axis) {
    if (m_document != nullptr) {
        double visibleHeight = 0.0;
        double minY = RS_MAXDOUBLE;
        double maxY = RS_MINDOUBLE;
        bool noChange = false;

        // fixme - sand -- ?? WHY ?? What if there are entities outside lines? This is not reliable at all
        for (const auto e: *m_document) {
            if (e->rtti() == RS2::EntityLine) {
                const auto *l = static_cast<RS_Line*>(e);
                const auto& startpoint = l->getStartpoint();
                const double x1 = toGuiX(startpoint.x);
                const auto& endpoint = l->getEndpoint();
                const double x2 = toGuiX(endpoint.x);

                if (((x1 > 0.0) && (x1 < static_cast<double>(getWidth()))) ||
                    ((x2 > 0.0) && (x2 < static_cast<double>(getWidth())))) {
                    minY = std::min(minY, startpoint.y);
                    minY = std::min(minY, endpoint.y);
                    maxY = std::max(maxY, startpoint.y);
                    maxY = std::max(maxY, endpoint.y);
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
            fy = (getHeight() - m_borderTop - m_borderBottom)
                 / visibleHeight;
            if (m_factor.y < 0.000001) {
                noChange = true;
            }
        }

        if (noChange == false) {
            if (!m_zoomFrozen) {
                m_factor.y = std::abs(fy);
            }
            m_offsetY = static_cast<int>((getHeight() - m_borderTop - m_borderBottom - visibleHeight * m_factor.y) / 2.0 - (minY * m_factor.y)) + m_borderBottom;
            fireViewportChanged();

        }
        RS_DEBUG->print("Auto zoom y ok");
    }
}

void LC_GraphicViewport::zoomAutoEnsurePointsIncluded(const RS_Vector &wcsP1, const RS_Vector &wcsP2, const RS_Vector &wcsP3) {
    if (m_document != nullptr) {
        m_document->calculateBorders();
        RS_Vector min = m_document->getMin();
        RS_Vector max = m_document->getMax();

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
void LC_GraphicViewport::zoomAuto(const bool axis, const bool keepAspectRatio) {
    RS_DEBUG->print("RS_GraphicView::zoomAuto");
    if (m_document != nullptr) {
        m_document->calculateBorders();
        const RS_Vector min = m_document->getMin();
        const RS_Vector max = m_document->getMax();
        doZoomAuto(min,max, axis, keepAspectRatio);
    }
    RS_DEBUG->print("RS_GraphicView::zoomAuto OK");
}

void LC_GraphicViewport::doZoomAuto(const RS_Vector& min, const RS_Vector& max, const bool axis, const bool keepAspectRatio) {
    double sx = 0., sy = 0.;
    RS_Vector containerSize;
    RS_Vector containerMin;

    if (hasUCS()){
        RS_Vector ucsMin;
        RS_Vector ucsMax;

        ucsBoundingBox(min, max, ucsMin, ucsMax);

        const RS_Vector ucsSize = ucsMax - ucsMin;

        sx = ucsSize.x;
        sy = ucsSize.y;

        containerSize = ucsSize;
        containerMin = ucsMin;
    }
    else {
        const auto dV = max - min;
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
        fx = (getWidth() - m_borderLeft - m_borderRight) / sx;
    } else {
        fFlags += 1; //invalid x factor
    }

    if (sy > RS_TOLERANCE) {
        fy = (getHeight() - m_borderTop - m_borderBottom) / sy;
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
                const double minValue = std::min(fx, fy);
                fx = minValue;
                fy = minValue;
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
    if (fFlags == 3) {
        return;
    }
    saveView();

    if (!m_zoomFrozen) {
        m_factor.x = std::abs(fx);
        m_factor.y = std::abs(fy);
    }
    centerOffsetXandY(containerMin, containerSize);
}

RS_Grid *LC_GraphicViewport::getGrid() const {
    return m_grid.get();
}

/**
 * Shows previous view.
 */
void LC_GraphicViewport::zoomPrevious() {

    RS_DEBUG->print("RS_GraphicView::zoomPrevious");

    if (m_document != nullptr) {
        restoreView();
    }
}

/**
 * Saves the current view as previous view to which we can
 * switch back later with @see restoreView().
 */
void LC_GraphicViewport::saveView() {
    if (m_graphic != nullptr) {
        if (m_modifyOnZoom) {
            getGraphic()->setModified(true);
        }
    }
    const QDateTime noUpdateWindow = QDateTime::currentDateTime().addMSecs(-500);
//do not update view within 500 milliseconds
    if (*m_previousViewTime > noUpdateWindow) {
        return;
    }
    *m_previousViewTime = QDateTime::currentDateTime();
    m_savedViews[m_savedViewIndex] = std::make_tuple(m_offsetX, m_offsetY, m_factor);
    m_savedViewIndex = (m_savedViewIndex + 1) % m_savedViews.size();
    if (m_savedViewCount < m_savedViews.size()) {
        m_savedViewCount++;
    }

    if (m_savedViewCount == 1) {
        firePreviousZoomChanged(true);
    }
}

/**
 * Restores the view previously saved with
 * @see saveView().
 */
void LC_GraphicViewport::restoreView() {
    if (m_savedViewCount == 0) {
        return;
    }
    m_savedViewCount--;
    if (m_savedViewCount == 0) {
        firePreviousZoomChanged(false);
    }
    m_savedViewIndex = (m_savedViewIndex + m_savedViews.size() - 1) % m_savedViews.size();

    m_offsetX = std::get<0>(m_savedViews[m_savedViewIndex]);
    m_offsetY = std::get<1>(m_savedViews[m_savedViewIndex]);
    m_factor = std::get<2>(m_savedViews[m_savedViewIndex]);

    fireViewportChanged();
}

void LC_GraphicViewport::setFactor(const double f) {
    if (!m_zoomFrozen) {
        const double absF = std::abs(f);
        m_factor.x = absF;
        m_factor.y = absF;
        fireViewportChanged();
    }
}

void LC_GraphicViewport::setOffsetAndFactor(const int ox, const int oy, const double f){
    justSetOffsetAndFactor(ox, oy, f);
    fireViewportChanged();
}

void LC_GraphicViewport::justSetOffsetAndFactor(const int ox, const int oy, const double f){
    m_offsetX = ox;
    m_offsetY = oy;
    m_factor.x = std::abs(f);
    m_factor.y = std::abs(f);
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

double LC_GraphicViewport::toAbsUCSAngle(const double ucsBasisAngle) {
    return toUCSAbsAngle(ucsBasisAngle, getAnglesBaseAngle(), areAnglesCounterClockwise());
}

double LC_GraphicViewport::toBasisUCSAngle(const double ucsAbsAngle) {
    return toUCSBasisAngle(ucsAbsAngle, getAnglesBaseAngle(), areAnglesCounterClockwise());
}

void LC_GraphicViewport::toUI(const RS_Vector& wcsCoordinate, double &uiX, double &uiY) const{
    if (hasUCS()){
        doWCS2UCS(wcsCoordinate.x, wcsCoordinate.y, uiX, uiY);
        uiX = toGuiX(uiX);
        uiY = toGuiY(uiY);
    }
    else{
        uiX = toGuiX(wcsCoordinate.x);
        uiY = toGuiY(wcsCoordinate.y);
    }
}

void LC_GraphicViewport::loadGridSettings() const {
    if (m_grid != nullptr){
        m_grid->loadSettings();
    }
    fireRedrawNeeded(RS2::RedrawGrid, false);
}

void LC_GraphicViewport::setUCS(const RS_Vector &origin, const double angle, const bool isometric, const RS2::IsoGridViewType isoType) {
    const RS_Vector ucsOrigin = doSetUCS(origin, angle, isometric, isoType);
    switch (m_ucsApplyingPolicy){
        case UCSApplyingPolicy::ZoomAuto: {
            zoomAuto();
            break;
        }
        case UCSApplyingPolicy::PanOriginCenter: {
            const int offX = static_cast<int>(ucsOrigin.x * m_factor.x + (getWidth() - m_borderLeft - m_borderRight) * 0.5);
            const int offY = static_cast<int>(ucsOrigin.y * m_factor.y + (getHeight() - m_borderTop - m_borderBottom) * 0.5);
            setOffset(offX, offY);
            break;
        }
        case UCSApplyingPolicy::PanOriginLowerLeft: {
            setOffset(ucsOrigin.x * m_factor.x + (m_borderLeft + m_borderRight) / 2,
                      ucsOrigin.y * m_factor.y + (m_borderBottom + m_borderRight) / 2);
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
    const RS_Vector originToSet = ucsToSet->getOrigin();
    const double angleToSet = ucsToSet->getXAxisDirection();
    const bool hasIso = ucsToSet->isIsometric();
    const RS2::IsoGridViewType isoType = ucsToSet->getIsoGridViewType();
    setUCS(originToSet, angleToSet, hasIso, isoType);
    if (m_graphic != nullptr){
        m_graphic->setCurrentUCS(ucsToSet);
    }
    fireUcsChanged(ucsToSet);
}

void LC_GraphicViewport::extractUCS(){
    if (hasUCS()){
        if (m_graphic != nullptr) {
            LC_UCSList *ucsList = m_graphic->getUCSList();
            LC_UCS *candidate = createUCSEntity(getUcsOrigin(), -getXAxisAngle(), isGridIsometric(), getIsoViewType());
            LC_UCS *createdUCS = ucsList->tryAddUCS(candidate);
            if (createdUCS != nullptr) {
                applyUCS(createdUCS);
            }
        }
    }
}

RS_Vector LC_GraphicViewport::doSetUCS(const RS_Vector &origin, const double angle, const bool isometric, const RS2::IsoGridViewType isoType) {
    const bool customUCS = LC_LineMath::isMeaningfulAngle(angle) || LC_LineMath::isMeaningfulDistance(origin, RS_Vector(0, 0, 0));
    const RS_Vector ucsOrigin = customUCS ? toUCS(origin) : RS_Vector{0., 0.};
    if (customUCS) {
        update(origin, -angle);
    }
    useUCS(customUCS);
    const auto g = getGraphic();
    if (g != nullptr){
        const bool oldIsometricGrid = g->isIsometricGrid();
        const RS2::IsoGridViewType oldIsoViewType = g->getIsoView();
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

void LC_GraphicViewport::createUCS(const RS_Vector &origin, const double angle) {
    const bool customUCS = LC_LineMath::isMeaningfulAngle(angle) || LC_LineMath::isMeaningfulDistance(origin, RS_Vector(0, 0, 0));
    if (customUCS){
        const auto g = getGraphic();
        if (g != nullptr) {
            const auto ucsList = g->getUCSList();
            const auto candidate = createUCSEntity(origin, angle, isGridIsometric(), getIsoViewType());
            const auto createdUCS = ucsList->tryAddUCS(candidate);
            if (createdUCS != nullptr){
                setUCS(origin, angle, isGridIsometric(), getIsoViewType());
                g->setCurrentUCS(createdUCS);
                fireUcsChanged(createdUCS);
            }
        }
    }
}

LC_UCS *LC_GraphicViewport::createUCSEntity(const RS_Vector &origin, const double angle, const bool isometric, const RS2::IsoGridViewType isoType) const{
    auto* result = new LC_UCS("");
    result->setOrigin(origin);

    auto xAxis = RS_Vector(1.0, 0, 0);
    auto yAxis = RS_Vector(0,1.0, 0);

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
            case RS2::IsoRight:{
                orthoType = LC_UCS::RIGHT;
                break;
            }
            case RS2::IsoLeft:{
                orthoType = LC_UCS::LEFT;
                break;
            }
            case RS2::IsoTop:{
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
        const RS_Vector originToSet = ucsCurrent->getOrigin();
        const double angleToSet = ucsCurrent->getXAxisDirection();
        const bool isometric = ucsCurrent->isIsometric();
        const RS2::IsoGridViewType isoType = ucsCurrent->getIsoGridViewType();
        doSetUCS(originToSet, angleToSet, isometric, isoType);
        fireUcsChanged(ucsCurrent);
        delete ucsCurrent;
    }
}

void LC_GraphicViewport::fillCurrentUCSInfo(RS_Vector& origin, double& xAxisDirection) const {
    if (hasUCS()) {
        origin = getUcsOrigin();
        xAxisDirection = -getXAxisAngle();
    }
    else {
        origin = RS_Vector(0, 0, 0);
        xAxisDirection = 0;
    }
}

LC_UCS* LC_GraphicViewport::getCurrentUCS() const{
    LC_UCS* result = nullptr;
    if (hasUCS()){
        result = createUCSEntity(getUcsOrigin(), -getXAxisAngle(),isGridIsometric(), getIsoViewType());
    }
    return result;
}

RS_Vector LC_GraphicViewport::snapGrid([[maybe_unused]] const RS_Vector& coord, [[maybe_unused]] RS_Entity* entity) const {
    // fixme - reserved for the future
    Q_ASSERT_X(false, "snapGrid", "Not implemented");
    return RS_Vector(false);
}

RS_Vector LC_GraphicViewport::snapGrid(const RS_Vector& coord, const RS_Vector& rayStart, const RS_Vector& rayEnd) const {
    if (hasUCS()) {
        // basically, wcs coordinate still should be returned there.
        // however, it will be rotated according to the grid (which is not rotated in ucs).
        RS_Vector snap = getGrid()->snapGrid(toUCS(coord), toUCS(rayStart), toUCS(rayEnd));
        snap = toWorld(snap);
        return snap;
    }
    const RS_Vector snap = getGrid()->snapGrid(coord, rayStart, rayEnd);
    return snap;
}

RS_Vector LC_GraphicViewport::snapGrid(const RS_Vector &coord) const {
    if (hasUCS()) {
        // basically, wcs coordinate still should be returned there.
        // however, it will be rotated according to the grid (which is not rotated in ucs).
        RS_Vector snap = getGrid()->snapGrid(toUCS(coord));
        snap = toWorld(snap);
        return snap;
    }
    const RS_Vector snap = getGrid()->snapGrid(coord);
    return snap;
}

void LC_GraphicViewport::restoreView(const LC_View *view) {
    if (view == nullptr){
        return;
    }

    const RS_Vector center = view->getCenter();
    const RS_Vector size = view->getSize();

    const RS_Vector halfSize = size / 2;
    const RS_Vector v1 = center - halfSize;
    const RS_Vector v2 = center + halfSize;

    auto origin = RS_Vector(0,0,0);
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
    }

    const auto g = getGraphic();
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

LC_View* LC_GraphicViewport::createNamedView(const QString& name) const{
    auto* viewToCreate = new LC_View(name);
    doUpdateViewByGraphicView(viewToCreate);
    return viewToCreate;
}

void LC_GraphicViewport::updateNamedView(LC_View* view) const{
    doUpdateViewByGraphicView(view);
}

void LC_GraphicViewport::doUpdateViewByGraphicView(LC_View *view) const {
    view->setForPaperView(isPrintPreview());

    const int width = getWidth();
    const int height = getHeight();

    const double x = toUcsX(width);
    const double y = toUcsY(height);

    const double x0 = toUcsX(0);
    const double y0 = toUcsY(0);

    view->setCenter({(x + x0) / 2.0, (y + y0) / 2.0, 0});
    view->setSize({(x - x0), (y - y0), 0});

    view->setTargetPoint({0, 0, 0});

    LC_UCS* viewUCS = getCurrentUCS();
    if (viewUCS != nullptr) {
        view->setUCS(viewUCS);
        if (m_graphic != nullptr) {
            LC_UCSList *ucsList = m_graphic->getUCSList();

            const LC_UCS *existingListUCS = ucsList->findExisting(viewUCS);
            if (existingListUCS != nullptr) {
                const QString ucsName = existingListUCS->getName();
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
    if (!m_relativeZeroLocked) {
        m_relativeZero = pos;
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
    if (LC_LineMath::isMeaningfulDistance(m_markedRelativeZero, m_relativeZero)){
        result = new LC_UndoableRelZero(this, m_markedRelativeZero, m_relativeZero);
        markRelativeZero();
    }
    return result;
}

void LC_GraphicViewport::setGraphic(RS_Graphic *g) {
    m_graphic = g;
    m_overlaysManager.setGraphic(g);
    m_formatter->updateByGraphic(g);
}

/**
 * Zooms to page extends.
 */
 // fixme - ucs, potentially, these methods should live in some other place...
void LC_GraphicViewport::zoomPage() {
    RS_DEBUG->print("RS_GraphicView::zoomPage");
    if (m_document == nullptr) {
        return;
    }

    const RS_Graphic *graphic = m_document->getGraphic();
    if (graphic == nullptr) {
        return;
    }

    const LC_PlotSettings* ps = graphic->getPlotSettings();

    const RS_Vector s = ps->getPrintAreaSize() / ps->getPaperScale();

    double fx = 0., fy = 0.;

    if (s.x > RS_TOLERANCE) {
        fx = (getWidth() - m_borderLeft - m_borderRight) / s.x;
    } else {
        fx = 1.0;
    }

    if (s.y > RS_TOLERANCE) {
        fy = (getHeight() - m_borderTop - m_borderBottom) / s.y;
    } else {
        fy = 1.0;
    }

    RS_DEBUG->print("f: %f/%f", fx, fy);

    const double minValue = std::min(fx, fy);
    fx = minValue;
    fy = minValue;

    RS_DEBUG->print("f: %f/%f", fx, fy);

    if (fx < RS_TOLERANCE) {
        fx = 1.0;
        fy = 1.0;
    }

     if (!m_zoomFrozen) {
         m_factor.x = std::abs(fx);
         m_factor.y = std::abs(fy);
     }


    RS_DEBUG->print("f: %f/%f", fx, fy);

    const RS_Vector containerMin = m_document->getMin();
    const RS_Vector containerSize = m_document->getSize();

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
    if (m_document == nullptr) {
        return;
    }

    const RS_Graphic *graphic = m_document->getGraphic();
    if (graphic == nullptr) {
        return;
    }

    const RS2::Unit dest = graphic->getUnit();
    const LC_PlotSettings* ps = m_graphic->getPlotSettings();
    const double marginsWidth = RS_Units::convert(ps->getMarginLeftMm() + ps->getMarginRightMm(), RS2::Millimeter, dest);
    const double marginsHeight = RS_Units::convert(ps->getMarginTopMm() + ps->getMarginBottomMm(), RS2::Millimeter, dest);

    const RS_Vector &printAreaSize = ps->getPrintAreaSize(true);
    const double paperScale = ps->getPaperScale();
    const RS_Vector printAreaSizeInViewCoordinates = (printAreaSize + RS_Vector(marginsWidth, marginsHeight)) / paperScale;

    LC_ERR<<"margin: "<<marginsWidth<<", "<<marginsHeight;
    LC_ERR<<__LINE__<<" printAreaSizeInViewCoordinates "<< printAreaSizeInViewCoordinates.x<<", "<<printAreaSizeInViewCoordinates.y;
    double fx;
    double fy;

    if (printAreaSizeInViewCoordinates.x > RS_TOLERANCE) {
        const int widthToFit = getWidth() - m_borderLeft - m_borderRight;
        fx = widthToFit / printAreaSizeInViewCoordinates.x;
    } else {
        fx = 1.0;
    }

    if (printAreaSizeInViewCoordinates.y > RS_TOLERANCE) {
        const int heightToFit = getHeight() - m_borderTop - m_borderBottom;
        fy = heightToFit / printAreaSizeInViewCoordinates.y;
    } else {
        fy = 1.0;
    }

    RS_DEBUG->print("f: %f/%f", fx, fy);

    const double minValue = std::min(fx, fy);
    fx = minValue;
    fy = minValue;

    RS_DEBUG->print("f: %f/%f", fx, fy);

    if (fx < RS_TOLERANCE) {
        fx = 1.0;
        fy = 1.0;
    }

    if (!m_zoomFrozen) {
        m_factor.x = std::abs(fx);
        m_factor.y = std::abs(fy);
    }

    RS_DEBUG->print("f: %f/%f", fx, fy);

    const RS_Vector &paperInsertionBase = graphic->getPaperInsertionBase();

    m_offsetX = static_cast<int>((getWidth() - m_borderLeft - m_borderRight - printAreaSizeInViewCoordinates.x * m_factor.x) / 2.0 + (
        paperInsertionBase.x * m_factor.x / paperScale)) + m_borderLeft;

    fy = m_factor.y;

    m_offsetY = static_cast<int>((getHeight() - m_borderTop - m_borderBottom - printAreaSizeInViewCoordinates.y * fy) / 2.0 +
        paperInsertionBase.y * fy / paperScale) + m_borderBottom;

    LC_LOG<<"LC_GraphicViewport::"<<__func__<<"(): end normally";
    fireViewportChanged();
}

void LC_GraphicViewport::clearOverlayEntitiesContainer(const RS2::OverlayGraphics overlayType) const {
    const auto overlayEntities = m_overlaysManager.entitiesAt(overlayType);
    if (overlayEntities != nullptr) {
        overlayEntities->clear();
    }
}

void LC_GraphicViewport::clearOverlayDrawablesContainer(const RS2::OverlayGraphics overlayType) const {
    const auto overlayDrawables = m_overlaysManager.drawablesAt(overlayType);
    if (overlayDrawables != nullptr) {
        overlayDrawables->clear();
    }
}
