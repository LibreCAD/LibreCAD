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

class RS_Color;
class RS_GraphicView;
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
class RS_Painter: public QPainter {
public:
    RS_Painter( QPaintDevice* pd);
    ~RS_Painter() = default;

    /**
     * Sets the drawing mode.
     */
    void setDrawingMode(RS2::DrawingMode m) {
        drawingMode = m;
    }

    // When set to true, only entities that are selected will be drawn
    void setDrawSelectedOnly(bool dso) {
        drawSelectedEntities=dso;
    }

    // When true, only selected items will be draw
    bool shouldDrawSelected() {
        return drawSelectedEntities;
    }

    /**
     * @return Current drawing mode.
     */
    RS2::DrawingMode getDrawingMode() {
        return drawingMode;
    }

    void drawGridPoint(const RS_Vector& p);
    void drawGridPoint(const double& x, const double& y);
    void drawPoint(const RS_Vector& p, int pdmode, int pdsize);
    void drawLine(const RS_Vector& p1, const RS_Vector& p2);
    void drawLine(const double &x1, const double &y1, const double &x2, const double &y2);
    void drawLineSimple(const double &x1, const double &y1, const double &x2, const double &y2);
    void drawRect(const RS_Vector& p1, const RS_Vector& p2);
    void drawArcEntity(const RS_Vector &cp, double radiusX, double radiusY, double startAngleDegrees, double angularLength);

    void drawCircle(const RS_Vector& cp, double radius);
    void drawEllipse(double centerX, double centerY,
                             double radius1, double radius2,
                             double angle);
    void drawEllipseArc(double centerX, double centerY,
                             double radius1, double radius2,
                             double angle,
                             double angle1, double angle2, double angleLength,
                             bool reversed);
    void drawPolyline(const RS_Polyline& polyline, const RS_GraphicView& view);
    void drawSplinePoints(const 	std::vector<RS_Vector> &controlPoints, bool closed);
    void drawSpline(const RS_Spline& spline, const RS_GraphicView& view);
    void drawImg(QImage& img, const RS_Vector& pos,
                               const RS_Vector& u, const RS_Vector& v, const RS_Vector& factor);
    void drawTextH(int x1, int y1, int x2, int y2,
                           const QString& text);
    void drawTextV(int x1, int y1, int x2, int y2,
                           const QString& text);
    void drawText(const QRect& rect, const QString& text, QRect* boundingBox);
    void drawText(const QRect& rect, int flags, const QString& text, QRect* boundingBox);

    void fillRect(int x1, int y1, int w, int h, const RS_Color& col);
    void fillRect ( const QRectF & rectangle, const RS_Color & color );
    void fillRect ( const QRectF & rectangle, const QBrush & brush );

    void fillTriangle(const RS_Vector& p1,
                              const RS_Vector& p2,
                              const RS_Vector& p3);

    void drawPath ( const QPainterPath & path);
    void fillPath ( const QPainterPath & path, const QBrush& brush);
    void drawHandle(const RS_Vector& p, const RS_Color& c, int size=-1);

    RS_Pen getPen() const;
    void setPen(const RS_Pen& pen);
    void setPen(const RS_Color& color);
    void setPen(int r, int g, int b);
    void disablePen();
    const QBrush& brush() const;
    void setBrush(const RS_Color& color);
    void setBrush(const QBrush& color);
    void drawPolygon(const QPolygon& a, Qt::FillRule rule=Qt::WindingFill);
    void erase();
    int getWidth() const;
    int getHeight() const;
    double getDpmm() const;
    void setClipRect(int x, int y, int w, int h);
    void resetClipping();
    void createSolidFillPath(QPainterPath &path, const RS_GraphicView *view, QList<RS_Entity *> entities);
    void noCapStyle();
    RS_Pen& getRsPen();
    void setPenJoinStyle(Qt::PenJoinStyle penJoinStyle);
    void setPenCapStyle(Qt::PenCapStyle penCapStyle);
    void setMinCircleDrawingRadius(double minCircleDrawingRadius);
    void setMinArcDrawingRadius(double minArcDrawingRadius);
    void setMinEllipseMajorRadius(double minEllipseMajorRadius);
    void setMinEllipseMinorRadius(double minEllipseMinorRadius);
    void setMinLineDrawingLen(double minLineDrawingLen);
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

    void drawPolygonF(const QPolygonF &a, Qt::FillRule rule);
    void debugOutPath(const QPainterPath &tmpPath) const;
    double getDpmmCached() const {return cachedDpmm;};
};

#endif
