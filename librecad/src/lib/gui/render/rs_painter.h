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

#include <QPen>
#include <QPainter>
#include "rs.h"
#include "qnamespace.h"
#include "rs_vector.h"
#include "rs_entity.h"
#include "lc_graphicviewportrenderer.h"
#include "lc_coordinates_mapper.h"

class RS_Color;
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
    void toGui(const RS_Vector& pos, double &x, double &y);
    double toGuiDX(double d) const;
    double toGuiDY(double d) const;

    bool isPrinting() {return printinMode;}; // fixme - temporary support, refactor further
    bool isPrintPreview() {return printPreview;}; // fixme - temporary support, refactor further

    LC_GraphicViewport* getViewPort() {return viewport;}
    void setViewPort(LC_GraphicViewport* v);
    void setRenderer(LC_GraphicViewportRenderer *r) {renderer = r;}
    void updateDashOffset(RS_Entity* e);
    void clearDashOffset() {currenPatternOffset = 0.0;};
    double currentDashOffset() const {return currenPatternOffset;}

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
    void drawEntityArc(RS_Arc* arc);

    void drawSplineWCS(const RS_Spline &spline);
    void drawLineWCS(const RS_Vector &wcsP1, const RS_Vector &wcP2);
    void drawEntityPolyline(const RS_Polyline *polyline);
    void drawHandleWCS(const RS_Vector &wcsPosition, const RS_Color &c, int size = -1);
    void drawImgWCS(QImage &img, const RS_Vector &wcsInsertionPoint, const RS_Vector &uVector, const RS_Vector &vVector);

    // drawing in screen coordinates
    void drawCircleUI(double uiCenterX, double uiCenterY, double uiRadius);
    void drawLineUISimple(const double &x1, const double &y1, const double &x2, const double &y2);
    void drawLineUISimple(const RS_Vector &p1, const RS_Vector &p2);
    void drawText(const QRect &uiRect, int flags, const QString &text, QRect *uiBoundingBox);
    void drawText(const QRect &rect, const QString &text, QRect *boundingBox);
    void drawRectUI(const double uiX1, const double uiY1, const double uiX2, const double uiY2);
    void drawPointEntityUI(double uiX, double uiY, int pdmode, int pdsize);

    // methods invoked from entity containers and printing
    void drawEntity(RS_Entity* entity) {renderer->renderEntity(this, entity);}
    void drawAsChild(RS_Entity* entity){renderer->renderEntityAsChild(this, entity);};
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
    void drawGridPoint(const double& x, const double& y);


    void fillRect(int x1, int y1, int w, int h, const RS_Color& col);
    void fillRect ( const QRectF & rectangle, const RS_Color & color );
    void fillRect ( const QRectF & rectangle, const QBrush & brush );

    void fillTriangleUI(const RS_Vector& uiP1,const RS_Vector& uiP2,const RS_Vector& uiP3);
    void fillTriangleUI(double uiX1, double uiY1, double uiX2, double uiY2, double uiX3, double uiY3);

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
    void createSolidFillPath(QPainterPath &path,QList<RS_Entity *> entities);
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
    void setDefaultWidthFactor(double factor){ defaultWidthFactor = factor;};
    void updatePointsScreenSize(double pdSize);

    bool isTextLineNotRenderable(double d);

    void setRenderArcsInterpolate(bool value){ arcRenderInterpolate = value;}
    void setRenderArcsInterpolationAngleFixed(bool value){arcRenderInterpolationAngleFixed = value;}
    void setRenderArcsInterpolationAngleValue(double val) {arcRenderInterpolationAngleValue = val;}
    void setRenderArcsInterpolationMaxSagitta(double val) {arcRenderInterpolationMaxSagitta = val;}
    void setRenderCirclesSameAsArcs(bool val) {circleRenderSameAsArcs = val;}

    void disableUCS();

    void setWorldBoundingRect(LC_Rect &worldBoundingRect) {wcsBoundingRect = worldBoundingRect;};
    bool isFullyWithinBoundingRect(RS_Entity* e);
    bool isFullyWithinBoundingRect(const LC_Rect &rect);

    const LC_Rect &getWcsBoundingRect() const;
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
    double cachedDpmm;
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
    double viewPortFactorX = 1.0;
    double viewPortFactorY = 1.0;
    int viewPortOffsetX = 0;
    int viewPortOffsetY = 0;
    double viewPortHeight = 0.0;

    LC_Rect wcsBoundingRect;

    LC_GraphicViewportRenderer* renderer = nullptr;
    LC_GraphicViewport* viewport = nullptr;

//    void drawPolygonF(const QPolygonF &a, Qt::FillRule rule);
    void debugOutPath(const QPainterPath &tmpPath) const;
    double getDpmmCached() const {return cachedDpmm;};

    void drawArcEntity(RS_Arc* arc, QPainterPath &path);

    // painting in UI coordinates
    void drawEllipseUI(double uiCenterX, double uiCenterY, double uiRadiusMajor, double uiRadiusMinor, double uiAngleDegrees);
    void drawEllipseArcUI(double uiCenterX, double uiCenterY, double uiMajorRadius, double uiMinorRadius, double uiMajorAngleDegrees,
                           double angle1Degrees, double angle2Degrees, double angleLength, bool reversed);
    void drawSplinePointsUI(const std::vector<RS_Vector> &uiControlPoints, bool closed);
    void drawArcSplinePointsUI(const std::vector<RS_Vector> &uiControlPoints, QPainterPath &path);

    void drawLineUI(const double &x1, const double &y1, const double &x2, const double &y2);
    void drawImgUI(QImage& img, double uiInsertX, double uiInsertY, const RS_Vector& uVector, const RS_Vector& vVector, const RS_Vector& factor);

    void drawRectUI(const RS_Vector& p1, const RS_Vector& p2);


    void drawTextH(int x1, int y1, int x2, int y2,
                   const QString& text);
    void drawTextV(int x1, int y1, int x2, int y2,
                   const QString& text);

// fixme - sand, ucs - temporary, remove
    bool printinMode = false;
    bool printPreview = false;

    void drawArcInterpolatedByLines(double uiCenterX, double uiCenterY, double uiRadiusX, double uiStartAngleDegrees,
                                    double angularLength, QPainterPath &path) const;

    void drawArcQT(double uiCenterX, double uiCenterY, double uiRadiusX, double uiRadiusY, double uiStartAngleDegrees,
                   double angularLength, QPainterPath &path);

    void drawArcSegmentBySplinePointsUI(double uiCenterX, double uiCenterY, double uiRadiusX, double uiStartAngleDegrees,
                                        double angularLength, QPainterPath &path);
};

#endif
