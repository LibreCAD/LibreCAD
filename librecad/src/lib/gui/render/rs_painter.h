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


#ifndef RS_PAINTER_H
#define RS_PAINTER_H

#include <QPainter>

#include "lc_coordinates_mapper.h"
#include "lc_rect.h"
#include "rs_pen.h"

class RS_Arc;
class RS_Circle;
class RS_Color;
class RS_Ellipse;
class RS_Entity;
class RS_EntityContainer;
class RS_Pen;
class RS_Polyline;
class RS_Spline;

class QPainterPath;
class QRect;
class QRectF;
class QPolygon;
class QPolygonF;
class QImage;
class QBrush;
class QString;

class LC_GraphicViewport;
class LC_GraphicViewportRenderer;

struct LC_SplinePointsData;

/**
 * This class is a common interface for a painter class. Such
 * a class will in it's implementation be responsible to paint
 * lines, arcs, ... in widgets. All angles in rad.
 *
 * Note that this is just an interface used as a slot to
 * communicate with the LibreCAD from a GUI level. This
 * does not contain any Qt or platform specific code.
 */
class RS_Painter: public QPainter, LC_CoordinatesMapper {
public:
    explicit RS_Painter(QPaintDevice* pd);
    ~RS_Painter() = default;

    enum ArcRenderHint{
        FULL_IN_VIEW,
        ARC_IN_VIEW,
        SEGMENT,
        ANY
    };

    // coordinates translations
    RS_Vector toGui(const RS_Vector& worldCoordinates) const;
    QPointF toGuiPointF(const RS_Vector& worldCoordinates) const;
    void toGui(const RS_Vector& pos, double &x, double &y) const;
    double toGuiDX(double d) const;
    double toGuiDY(double d) const;
    QTransform getToGuiTransform() const;

    bool isPrinting() const
    {
        return printinMode;
    } // fixme - temporary support, refactor further
    bool isPrintPreview() const
    {
        return printPreview;
    } // fixme - temporary support, refactor further

    LC_GraphicViewport* getViewPort() const
    {
        return viewport;
    }
    void setViewPort(LC_GraphicViewport* v);
    void setRenderer(LC_GraphicViewportRenderer *r) {renderer = r;}
    void updateDashOffset(RS_Entity* e);
    void clearDashOffset() {currenPatternOffset = 0.0;}
    double currentDashOffset() const {return currenPatternOffset;}

    void drawEntityArc(RS_Arc* arc);
    void drawEntityPolyline(const RS_Polyline *polyline);
    void drawEntityCircle(RS_Circle* circle);

    int determinePointScreenSize(double pdsize) const;

    // methods called form entities to render them. All methods with WCS suffix - obtains world coordinates!
    void drawEllipseWCS(const RS_Vector &wcsCenter, double wcsMajorRadius, double ratio, double wcsAngleDegrees);

    void drawEllipseArcWCS(
        const RS_Vector &wcsCenter, double wcsMajorRadius, double ratio, double wcsAngleDegrees,
        double angle1Degrees, double angle2Degrees, double angleLength, bool reversed);

    void drawSplinePointsWCS(const std::vector<RS_Vector> &wcsControlPoints, bool closed);
    void drawCircleWCS(const RS_Vector &wcsCenter, double wcsRadius);
    void drawPointEntityWCS(const RS_Vector &p);
    void drawRefPointEntityWCS(const RS_Vector &wcsPos, int pdMode, double pdSize);
    void drawSolidWCS(const RS_Vector &wcsP1, const RS_Vector &wcsP2, const RS_Vector &wcsP3, const RS_Vector &wcsP4);
    void drawSolidWCS(const RS_VectorSolutions& wcsVertices);
    void drawFilledPolygonWCS(const RS_Vector& wcsV1, const RS_Vector& wcsV2, const RS_Vector& wcsV3,
                             const RS_Vector& wcsV4,const RS_Vector& wcsV5);
    void drawFilledCircleWCS(const RS_Vector& wcsCenter, double radius);
    void drawPolygonWCS(const RS_Vector& wcsV1, const RS_Vector& wcsV2, const RS_Vector& wcsV3,
                        const RS_Vector& wcsV4, const RS_Vector& wcsV5);
    void drawPolygonWCS(const std::vector<RS_Vector>& wcsPoints);

    void drawArcWCS(const RS_Vector &wcsCenter, double wcsRadius, double wcsStartAngleDegrees, double angularLength);
    void drawSplineWCS(const RS_Spline &spline);
    void drawLineWCS(const RS_Vector &wcsP1, const RS_Vector &wcP2);
    void drawLineUIScaled(QPointF from, QPointF to, double lineWidthFactor);
    void drawLineWCSScaled(const RS_Vector& wcsP1, const RS_Vector& wcsP2, double lineWidthFactor);
    void drawPolylineWCS(const RS_Polyline *polyline);
    void drawHandleWCS(const RS_Vector &wcsPosition, const RS_Color &c, int size = -1);
    void drawImgWCS(QImage &img, const RS_Vector &wcsInsertionPoint, const RS_Vector &uVector, const RS_Vector &vVector);

    // drawing in screen coordinates
    void drawCircleUI(const RS_Vector& uiCenter, double uiRadius);
    // just draws circle without trying to use any interpolations, used by overlays etc...
    void drawCircleUIDirect(const RS_Vector& uiPos, double uiRadius);
    void drawCircleUI(double uiCenterX, double uiCenterY, double uiRadius);
    void drawLineUISimple(double x1, double y1, double x2, double y2);
    void drawLineUISimple(const RS_Vector &p1, const RS_Vector &p2);
    void drawText(const QRect &uiRect, int flags, const QString &text, QRect *uiBoundingBox);
    void drawText(const QRect &rect, const QString &text, QRect *boundingBox);
    void drawRectUI(double uiX1, double uiY1, double uiX2, double uiY2);
    void drawPointEntityUI(const RS_Vector& uiPos, int pdmode, int pdsize);

    // methods invoked from entity containers and printing
    void drawEntity(RS_Entity* entity);
    void drawAsChild(RS_Entity* entity);
    void drawInfiniteWCS(RS_Vector start, RS_Vector end);

    /**
     * Sets the drawing mode.
     */
    void setDrawingMode(RS2::DrawingMode m) {drawingMode = m;}

    // When set to true, only entities that are selected will be drawn
    void setDrawSelectedOnly(bool dso) {drawSelectedEntities=dso;}

    // When true, only selected items will be draw
    bool shouldDrawSelected() const {return drawSelectedEntities;}
    /**
     * @return Current drawing mode.
     */
    RS2::DrawingMode getDrawingMode() {return drawingMode;}
    void setPointsMode(int pdMode){pointsMode = pdMode;}

    void drawGridPoint(const RS_Vector& p);
    void drawGridPoint(double x, double y);


    void fillRect(int x1, int y1, int w, int h, const RS_Color& col);
    void fillRect ( const QRectF & rectangle, const RS_Color & color );
    void fillRect ( const QRectF & rectangle, const QBrush & brush );

    void fillPolygonUI(const QPolygonF& polygon);
    void fillTriangleUI(const RS_Vector& uiP1,const RS_Vector& uiP2,const RS_Vector& uiP3);
    void fillTriangleUI(double uiX1, double uiY1, double uiX2, double uiY2, double uiX3, double uiY3);
    void fillEllipseUI(QPointF point_f, double radiusX, double radiusY);

    void drawPath ( const QPainterPath & path);
    void fillPath ( const QPainterPath & path, const QBrush& brush);
//    void drawHandle(const RS_Vector& p, const RS_Color& c, int size=-1);

    RS_Pen getPen() const;
    void setPen(const RS_Pen& pen);
    void setPen(const RS_Color& color);
    void setPen(int r, int g, int b);
    void disablePen();
    void setBrushColor(const RS_Color& color);
    void erase();
    int getWidth() const;  // todo - sand - ucs - check usage!!! Probably it's different width expected (from viewport, rather than from device)
    int getHeight() const; // todo - sand - ucs - check usage!!! Probably it's different width expected (from viewport, rather than from device)
    double getDpmm() const;
    void setClipRect(int x, int y, int w, int h);
    void resetClipping();
    /**
     * @brief createSolidFillPath create QPainterPath for solid fill. The contour is expected to contain
     *          RS_EntityContainer only, with each container contains a closed edges, ordered by contour order:
     *          i.e., each edge's end point is coincident with the start point of its next neighbor. The end point
     *          of the last edge is coincident of the start point of the first edge.
     *          For closed edges (circles/ellipses), each loop container contains a single edge.
     *          TODO: self-intersection support.
     * @param entities
     * @return
     */
    QPainterPath createSolidFillPath(const RS_EntityContainer& contour);
    void noCapStyle();
    RS_Pen& getRsPen();
    void setPenJoinStyle(Qt::PenJoinStyle penJoinStyle);
    void setPenCapStyle(Qt::PenCapStyle penCapStyle);
    void setMinCircleDrawingRadius(double minCircleDrawingRadius);
    void setMinArcDrawingRadius(double minArcDrawingRadius);
    void setMinEllipseMajorRadius(double minEllipseMajorRadius);
    void setMinEllipseMinorRadius(double minEllipseMinorRadius);
    void setMinLineDrawingLen(double minLineDrawingLen);
    void setMinRenderableTextHeightInPx(int i);
    void setDefaultWidthFactor(double factor){ defaultWidthFactor = factor;}
    void updatePointsScreenSize(double pdSize);

    bool isTextLineNotRenderable(double d) const;

    void setRenderArcsInterpolate(bool value){ arcRenderInterpolate = value;}
    void setRenderArcsInterpolationAngleFixed(bool value){arcRenderInterpolationAngleFixed = value;}
    void setRenderArcsInterpolationAngleValue(double val) {arcRenderInterpolationAngleValue = val;}
    void setRenderArcsInterpolationMaxSagitta(double val) {arcRenderInterpolationMaxSagitta = val;}
    void setRenderCirclesSameAsArcs(bool val) {circleRenderSameAsArcs = val;}

    void disableUCS();

    void setWorldBoundingRect(LC_Rect &worldBoundingRect) {wcsBoundingRect = worldBoundingRect;}
    bool isFullyWithinBoundingRect(RS_Entity* e);
    bool isFullyWithinBoundingRect(const LC_Rect &rect);

    const LC_Rect &getWcsBoundingRect() const;
    /**
     * @brief getMaximumArcSplineError - the maximum rendering error due to QPainter arc rendering by cubic spline approximation,
     *                                   for an arc of raidus 1, the maximum rendering error from approximating the and arc of 0
     *                                   to 90 degrees by a cubic spline with control points:
     *                                   (1, 0), (1, 4/3 (\sqrt 2 - 1)), (4/3 (\sqrt 2 - 1), 1), (0, 1)
     * @return - the QPainter implementation has the maximum error at 3e-4 for r=1
     */
    static constexpr double getMaximumArcSplineError() {
        // Issue #2035 : arc render precision
        // QPainter::arcTo() approximates an arc or radius=1, with angle from 0 to 90 degrees by a cubic spline with
        // 4 control points: (1, 0), (1, 4/3 (\sqrt 2 - 1)), (4/3 (\sqrt 2 - 1), 1), (0, 1)
        // The maximum approximation error is 3e-4
        return 3e-4;
    }

    static constexpr int getMaximumArcNonErrorRadius() {
        // fixme - sand - move to the setting??
        return 3000;
    }

    void drawEllipseBySplinePointsUI(const RS_Ellipse& ellipse, QPainterPath &path);

    /**
     * @brief Generates a clipped QPainterPath for a parametric entity (arc, circle, ellipse, etc.)
     *        by finding viewport border intersections and approximating visible segments.
     *      * This is a convenience wrapper that:
     *  - computes intersection parameters with the current viewport rectangle borders,
     *  - adds start/end parameters,
     *  - sorts and deduplicates them,
     *  - then delegates segment approximation to createPathForParametricCurve().
     *      * It is intended to be called from entity classes' `createPainterPath()` methods
     * to keep viewport clipping + tangent filtering logic centralized and reusable.
     *      * @param path              [out] The QPainterPath to append the generated segments to.
     *                          May already contain content (e.g. moveTo from previous subpath).
     *      * @param entity            The entity being rendered. Used only for:
     *                          - intersection tests with viewport lines
     *                          - tangent direction queries via entity->getTangentDirection()
     *                          Must implement RS_Entity interface (not null).
     *      * @param baseParam         Starting parameter value (e.g. angle1 for arc/ellipse, 0 for circle).
     *                          All relative parameters are computed with respect to this base.
     *      * @param fullParamLength   Total parameter range covered by the entity
     *                          (e.g. 2*M_PI for full circle/ellipse, getAngleLength() for arcs).
     *      * @param getParamFromPoint Function that converts a world point → parameter value on the curve.
     *                          Example for arc/circle:    [](const RS_Vector& p){ return (p - center).angle(); }
     *                          Example for ellipse:       [](const RS_Vector& p){ return getEllipseAngle(p); }
     *      * @param getPointFromParam Function that converts a parameter value → world point on the curve.
     *                          Example for arc/circle:    [](double t){ return center + RS_Vector::polar(radius, t); }
     *                          Example for ellipse:       [](double t){ return getEllipsePoint(t); }
     *      * @param approxRadius      Reference radius used to control approximation quality / step size.
     *                          • circle → radius
     *                          • ellipse → major radius (conservative choice)
     *                          Larger values produce fewer segments (coarser approximation).
     *      * @note
     *   - The generated path is **not** automatically closed.
     *     If building a closed contour for solid fill, call path.closeSubpath() after all segments.
     *   - Assumes the entity is visible and has valid geometry (caller should check beforehand).
     *   - Does **not** include fast-path full-shape generation (addEllipse / addEllipseArcWCS).
     *     Entities should handle full/visible fast paths themselves before calling this method.
     *   - Intersection points are filtered by tangent direction difference to avoid adding
     *     tangent/grazing points that do not actually split the visible portion.
     *      * Typical usage in an entity's createPainterPath():
     * @code
     * void RS_Arc::createPainterPath(RS_Painter* painter, QPainterPath& path) const
     * {
     *     if (!painter) return;
     *      *     // fast reject / fast full-circle path here if applicable...
     *      *     double base   = isReversed() ? data.angle2 : data.angle1;
     *     double length = getAngleLength();
     *      *     painter->createPathForEntity(
     *         path,
     *         this,
     *         base,
     *         length,
     *         [this](const RS_Vector& p){ return getArcAngle(p); },
     *         [this](double t){ return getPointAtParameter(t); },
     *         getRadius()
     *     );
     * }
     * @endcode
     *      * @see createPathForParametricCurve() — the lower-level method that actually builds segments
     */
    void pathForEntity(
        QPainterPath& path,
        const RS_Entity* entity,
        double baseParam,
        double fullParamLength,
        const std::function<double(const RS_Vector&)>& getParamFromPoint,
        const std::function<RS_Vector(double)>& getPointFromParam,
        double approxRadius
        ) const;

    /**
     * @brief Generates a QPainterPath by approximating a parametric curve with quadratic segments.
     *      * This is the core method used by RS_Arc, RS_Circle, RS_Ellipse (and potentially splines)
     * to draw visible portions of curves while respecting the current viewport clipping rectangle.
     *      * It splits the parameter range into visible segments (based on intersections with viewport borders)
     * and approximates each segment using quadratic Bezier curves (via LC_SplinePoints) with sagitta-based
     * adaptive sampling — this gives good visual quality at reasonable performance.
     *      * The function is deliberately designed to be reusable both for:
     *  - normal entity drawing (stroke only)
     *  - building closed contours for solid filling / hatching (fillPath)
     *      * @param path              [out] The QPainterPath to append segments to.
     *                          Usually starts empty, but can be appended to existing paths.
     *      * @param paramPoints       Sorted list of parameter values (in curve parameter space)
     *                          that define split points — typically includes:
     *                          • 0.0 (start)
     *                          • fullLength (end)
     *                          • all valid intersection parameters with viewport borders
     *                          The list must be sorted ascending and contain no duplicates
     *                          closer than RS_TOLERANCE_ANGLE.
     *      * @param getPointAtParam   Functor/lambda that maps a parameter value → world coordinate (RS_Vector)
     *                          Example for circle:    [this](double t){ return center + polar(radius, t); }
     *                          Example for ellipse:   [this](double t){ return getEllipsePoint(t); }
     *      * @param approxRadius      Reference radius used to estimate approximation error / step size.
     *                          • For circles → radius
     *                          • For ellipses → major radius (conservative choice)
     *                          Larger values → coarser approximation (fewer segments)
     *                          Smaller values → finer approximation (more segments, better quality)
     *      * Typical usage pattern in entity classes:
     * @code
     * std::vector<double> splits = {0.0, fullLength};
     * // add viewport intersection parameters ...
     * std::sort(splits); std::unique(...);
     *      * auto pointGetter = [this](double t){ return getPointAt(t); };
     * painter->pathForParametricCurve(path, splits, pointGetter, getMajorRadius());
     * @endcode
     *      * @note
     *   - The function assumes paramPoints are already sorted and cleaned.
     *   - It does NOT close the path — caller must call closeSubpath() if needed (usually for solid fill).
     *   - Approximation quality scales with screen pixels — very small curves get minimal points.
     *   - For full-circle / full-ellipse fast paths, use addEllipse() directly instead of this method.
     *      * @see addClippedParametricSegments() — higher-level wrapper that also computes split points
     */
    void pathForParametricCurve(
        QPainterPath& path,
        const std::vector<double>& paramPoints,
        const std::function<RS_Vector(double)>& getPointAtParam,
        double approxRadius
        ) const;

protected:
    /**
     * Current drawing mode.
     */
    RS2::DrawingMode drawingMode = RS2::ModeFull;
    // When set to true, only selected entities should be drawn
    bool drawSelectedEntities = false;

    RS_Pen lpen;
    long rememberX = 0; // Used for the moment because QPainter doesn't support moveTo anymore, thus we need to remember ourselves the moveTo positions
    long rememberY = 0;

    Qt::PenJoinStyle penJoinStyle = Qt::RoundJoin;
    Qt::PenCapStyle penCapStyle = Qt::RoundCap;
    QPen lastUsedPen;
    double cachedDpmm = 0.;
    double minCircleDrawingRadius = 2.0;
    double minArcDrawingRadius = 0.8;
    double minEllipseMajorRadius = 2.;
    double minEllipseMinorRadius = 1.;
    double minLineDrawingLen = 2;
    bool arcRenderInterpolate = false;
    bool arcRenderInterpolationAngleFixed = false;
    double arcRenderInterpolationAngleValue = M_PI/36;
    double arcRenderInterpolationMaxSagitta = 0.9;
    bool circleRenderSameAsArcs = false;

    double minRenderableTextHeightInPx = 1;
    double defaultWidthFactor = 1.0;

    double currenPatternOffset = 0.0;

    int screenPointsSize = 0;
    int pointsMode = 0;

    // cached factor and offset from viewport - for efficiency of coordinates translations.
    RS_Vector m_viewPortFactor{1., 1.};
    double& viewPortFactorX = m_viewPortFactor.x;
    double& viewPortFactorY = m_viewPortFactor.y;
    int viewPortOffsetX = 0;
    int viewPortOffsetY = 0;
    RS_Vector m_viewPortOffset;
    double viewPortHeight = 0.0;

    LC_Rect wcsBoundingRect;

    LC_GraphicViewportRenderer* renderer = nullptr;
    LC_GraphicViewport* viewport = nullptr;

//    void drawPolygonF(const QPolygonF &a, Qt::FillRule rule);
    void debugOutPath(const QPainterPath &tmpPath) const;
    double getDpmmCached() const {return cachedDpmm;}

    void drawArcEntity(RS_Arc* arc, QPainterPath &path);

    // painting in UI coordinates
    void drawEllipseUI(double uiCenterX, double uiCenterY, double uiRadiusMajor, double uiRadiusMinor, double uiAngleDegrees);
    void drawEllipseArcUI(double uiCenterX, double uiCenterY, double uiMajorRadius, double uiMinorRadius, double uiMajorAngleDegrees,
                          double angle1Degrees, double angle2Degrees, double angleLength, bool reversed);
    void drawEllipseUI(const RS_Vector& uiCenter, const RS_Vector& uiRadii, double uiAngleDegrees);
    void drawEllipseArcUI(const RS_Vector& uiCenter, const RS_Vector& uiRadii, double uiMajorAngleDegrees,
                           double angle1Degrees, double angle2Degrees, double angleLength, bool reversed);
    void drawSplinePointsUI(const std::vector<RS_Vector> &uiControlPoints, bool closed);
    void drawArcSplinePointsUI(const std::vector<RS_Vector> &uiControlPoints, QPainterPath &path);

    void drawArcEntityUI( double uiCenterX,double uiCenterY,double uiRadiusX,double uiRadiusY,double uiStartAngleDegrees,double angularLength);
    void drawArc(double uiCenterX, double uiCenterY, double uiRadiusX, double uiRadiusY,
                 double uiStartAngleDegrees, double angularLength, QPainterPath &path) const;
    void drawLineUI(double x1, double y1, double x2, double y2);
    void drawLineUI(const QPointF& startPoint, const QPointF& endPoint);
    void drawImgUI(QImage& img, const RS_Vector& uiInsert, const RS_Vector& uVector, const RS_Vector& vVector, const RS_Vector& factor);

    void drawRectUI(const RS_Vector& p1, const RS_Vector& p2);

    void drawTextH(int x1, int y1, int x2, int y2,
                   const QString& text);
    void drawTextV(int x1, int y1, int x2, int y2,
                   const QString& text);

// fixme - sand, ucs - temporary, remove
    bool printinMode = false;
    bool printPreview = false;

    void drawArcInterpolatedByLines(const RS_Vector& uiCenter, double uiRadiusX, double uiStartAngleDegrees,
                                    double angularLength, QPainterPath &path) const;

    void drawArcQT(const RS_Vector& uiCenter, const RS_Vector& uiRadii, double uiStartAngleDegrees,
                   double angularLength, QPainterPath &path);

    void drawArcSegmentBySplinePointsUI(const RS_Vector& center, double uiRadiusX, double uiStartAngleDegrees,
                                        double angularLength, QPainterPath &path);
private:
    void addEllipseArcToPath(QPainterPath& localPath, const RS_Vector& uiRadii, double startAngleDeg, double angularLengthDeg, bool useSpline);
    // helper method: approximate a centered ellipse with lc_splinepoints
    void drawEllipseSegmentBySplinePointsUI(const RS_Vector& uiRadii, double startRad, double lenRad, QPainterPath &path, bool closed);
    void addSplinePointsToPath(const std::vector<RS_Vector> &uiControlPoints, bool closed, QPainterPath &path) const;
};

#endif
