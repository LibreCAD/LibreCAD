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

#include<cmath>
#include<memory>

#include "rs_painterqt.h"
#include "rs_math.h"
#include "rs_graphicview.h"
#include "dxf_format.h"
#include "rs_debug.h"

namespace {
/**
 * Wrapper for Qt
 * convert RS2::LineType to Qt::PenStyle
 */
Qt::PenStyle rsToQtLineType(RS2::LineType t) {
	switch (t) {
	case RS2::NoPen:
		return Qt::NoPen;
	case RS2::SolidLine:
		return Qt::SolidLine;
	case RS2::DotLine:
	case RS2::DotLineTiny:
	case RS2::DotLine2:
	case RS2::DotLineX2:
		return Qt::DotLine;
	case RS2::DashLine:
	case RS2::DashLineTiny:
	case RS2::DashLine2:
	case RS2::DashLineX2:
		return Qt::DashLine;
	case RS2::DashDotLine:
	case RS2::DashDotLineTiny:
	case RS2::DashDotLine2:
	case RS2::DashDotLineX2:
		return Qt::DashDotLine;
	case RS2::DivideLine:
	case RS2::DivideLineTiny:
	case RS2::DivideLine2:
	case RS2::DivideLineX2:
		return Qt::DashDotDotLine;
	case RS2::CenterLine:
	case RS2::CenterLineTiny:
	case RS2::CenterLine2:
	case RS2::CenterLineX2:
		return Qt::DashDotLine;
	case RS2::BorderLine:
	case RS2::BorderLineTiny:
	case RS2::BorderLine2:
	case RS2::BorderLineX2:
		return Qt::DashDotLine;
	case RS2::LineByLayer:
	case RS2::LineByBlock:
	default:
		return Qt::SolidLine;
	}
	return Qt::SolidLine;
}
}

/**
 * Constructor.
 */
// RVT_PORT changed from RS_PainterQt::RS_PainterQt( const QPaintDevice* pd)
RS_PainterQt::RS_PainterQt( QPaintDevice* pd)
        : QPainter(pd), RS_Painter() {}

void RS_PainterQt::moveTo(int x, int y) {
        //RVT_PORT changed from QPainter::moveTo(x,y);
        rememberX=x;
        rememberY=y;
}


void RS_PainterQt::lineTo(int x, int y) {
        // RVT_PORT changed from QPainter::lineTo(x, y);
        QPainterPath path;
        path.moveTo(rememberX,rememberY);
        path.lineTo(x,y);
        QPainter::drawPath(path);
        rememberX=x;
        rememberY=y;
}

/**
 * Draws a grid point at (x1, y1).
 */
void RS_PainterQt::drawGridPoint(const RS_Vector& p) {
    QPainter::drawPoint(toScreenX(p.x), toScreenY(p.y));
}



/**
 * Draws a point at (x1, y1).
 */
void RS_PainterQt::drawPoint(const RS_Vector& p, int pdmode, int pdsize) {
	int screenX = toScreenX(p.x);
	int screenY = toScreenY(p.y);
	int halfPDSize = pdsize/2;

	/*	PDMODE values =>
		bits 0-3 = 0, centre dot
		         = 1, centre blank
		         = 2, centre +
		         = 3, centre X
		         = 4, centre vertical tick
		bit 5 = 1 => added surrounding circle
		bit 6 = 1 => added surrounding square
	*/
	switch (DXF_FORMAT_PDMode_getCentre(pdmode)) {
	case DXF_FORMAT_PDMode_CentreDot:
	default:
		/*	Centre dot - use a tiny + to make it visible  */
		QPainter::drawLine(screenX-1, screenY, screenX+1, screenY);
		QPainter::drawLine(screenX, screenY-1, screenX, screenY+1);
		break ;

	case DXF_FORMAT_PDMode_CentreBlank:
		/*	Centre is blank  */
		break ;

	case DXF_FORMAT_PDMode_CentrePlus:
		/*	Centre +  */
		QPainter::drawLine(screenX-pdsize, screenY, screenX+pdsize, screenY);
		QPainter::drawLine(screenX, screenY-pdsize, screenX, screenY+pdsize);
		break ;

	case DXF_FORMAT_PDMode_CentreCross:
		/*	Centre X  */
		QPainter::drawLine(screenX-pdsize, screenY-pdsize, screenX+pdsize, screenY+pdsize);
		QPainter::drawLine(screenX+pdsize, screenY-pdsize, screenX-pdsize, screenY+pdsize);
		break ;

	case DXF_FORMAT_PDMode_CentreTick:
		/*	Centre vertical tick  */
		QPainter::drawLine(screenX, screenY-halfPDSize, screenX, screenY);
		break ;
	}

	/*	Surrounding circle if required  */
	if (DXF_FORMAT_PDMode_hasEncloseCircle(pdmode)) {
		/*	Approximate circle by an octagon  */
		int xMin = screenX-halfPDSize;
		int xMax = screenX+halfPDSize;
		int yMin = screenY-halfPDSize;
		int yMax = screenY+halfPDSize;
		int octOffset = halfPDSize * 0.71;
		int xOctMin = screenX - octOffset;
		int xOctMax = screenX + octOffset;
		int yOctMin = screenY - octOffset;
		int yOctMax = screenY + octOffset;

		QPainter::drawLine(screenX, yMin, xOctMax, yOctMin);
		QPainter::drawLine(screenX, yMin, xOctMin, yOctMin);
		QPainter::drawLine(screenX, yMax, xOctMax, yOctMax);
		QPainter::drawLine(screenX, yMax, xOctMin, yOctMax);

		QPainter::drawLine(xMin, screenY, xOctMin, yOctMin);
		QPainter::drawLine(xMin, screenY, xOctMin, yOctMax);
		QPainter::drawLine(xMax, screenY, xOctMax, yOctMin);
		QPainter::drawLine(xMax, screenY, xOctMax, yOctMax);
	}

	/*	Surrounding square if required  */
	if (DXF_FORMAT_PDMode_hasEncloseSquare(pdmode)) {
		int xMin = screenX-halfPDSize;
		int xMax = screenX+halfPDSize;
		int yMin = screenY-halfPDSize;
		int yMax = screenY+halfPDSize;

		QPainter::drawLine(xMin, yMin, xMax, yMin);
		QPainter::drawLine(xMin, yMax, xMax, yMax);
		QPainter::drawLine(xMin, yMin, xMin, yMax);
		QPainter::drawLine(xMax, yMin, xMax, yMax);
	}
}



/**
 * Draws a line from (x1, y1) to (x2, y2).
 */
void RS_PainterQt::drawLine(const RS_Vector& p1, const RS_Vector& p2)
{
    QPainter::drawLine(toScreenX(p1.x), toScreenY(p1.y),
                       toScreenX(p2.x), toScreenY(p2.y));
}






/**
 * Draws a rectangle with corners p1, p2.
 */
/*void RS_PainterQt::drawRect(const RS_Vector& p1, const RS_Vector& p2) {
        / *QPainter::drawRect(toScreenX(p1.x), toScreenY(p1.y),
                                           abs(toScreenX(p2.x) - toScreenX(p1.x)),
                                           abs(toScreenY(p2.y) - toScreenY(p1.y)));* /
        QPainter::drawLine(toScreenX(p1.x), toScreenY(p1.y),
                       toScreenX(p2.x), toScreenY(p1.y));
    QPainter::drawLine(toScreenX(p2.x), toScreenY(p1.y),
                       toScreenX(p2.x), toScreenY(p2.y));
    QPainter::drawLine(toScreenX(p2.x), toScreenY(p2.y),
                       toScreenX(p1.x), toScreenY(p2.y));
    QPainter::drawLine(toScreenX(p1.x), toScreenY(p2.y),
                       toScreenX(p1.x), toScreenY(p1.y));
}*/


/**
 * Draws an arc which starts / ends exactly at the given coordinates.
 *
 * @param cx center in x
 * @param cy center in y
 * @param radius Radius
 * @param a1 Angle 1 in rad
 * @param a2 Angle 2 in rad
 * @param x1 startpoint x
 * @param y1 startpoint y
 * @param x2 endpoint x
 * @param y2 endpoint y
 * @param reversed true: clockwise, false: counterclockwise
 */
void RS_PainterQt::drawArc(const RS_Vector& cp, double radius,
                           double a1, double a2,
                           const RS_Vector& p1, const RS_Vector& p2,
                           bool reversed) {
    /*
    QPainter::drawArc(cx-radius, cy-radius,
                      2*radius, 2*radius,
                      a1*16, (a2-a1)*16);
    */

    if(radius<=0.5) {
        drawGridPoint(cp);
    } else {
        int   cix;            // Next point on circle
        int   ciy;            //
        double aStep;         // Angle Step (rad)
        double a;             // Current Angle (rad)
        double linStep;       // linear step (pixels)

        if (drawingMode==RS2::ModePreview) {
            linStep = 20.0;
        } else {
            linStep = 6.0;
        }

        if (std::abs(linStep/radius)<=1.0) {
            aStep=std::asin(linStep/radius);
        } else {
            aStep=1.0;
        }

        if (aStep<0.05) {
            aStep = 0.05;
        }

        if(!reversed) {
            // Arc Counterclockwise:
            if(a1>a2-1.0e-10) {
                a2+=2*M_PI;
            }
            //moveTo(toScreenX(p1.x), toScreenY(p1.y));
            QPolygon pa;
            int i=0;
            pa.resize(i+1);
            pa.setPoint(i++, toScreenX(p1.x), toScreenY(p1.y));
            for(a=a1+aStep; a<=a2; a+=aStep) {
                cix = toScreenX(cp.x+std::cos(a)*radius);
                ciy = toScreenY(cp.y-std::sin(a)*radius);
                //lineTo(cix, ciy);
                pa.resize(i+1);
                pa.setPoint(i++, cix, ciy);
            }
            //lineTo(toScreenX(p2.x), toScreenY(p2.y));
            pa.resize(i+1);
            pa.setPoint(i++, toScreenX(p2.x), toScreenY(p2.y));
            drawPolyline(pa);
        } else {
            // Arc Clockwise:
            if(a1<a2+1.0e-10) {
                a2-=2*M_PI;
            }
            QPolygon pa;
            int i=0;
            pa.resize(i+1);
            pa.setPoint(i++, toScreenX(p1.x), toScreenY(p1.y));
            //moveTo(toScreenX(p1.x), toScreenY(p1.y));
            for(a=a1-aStep; a>=a2; a-=aStep) {
                cix = toScreenX(cp.x+std::cos(a)*radius);
                ciy = toScreenY(cp.y-std::sin(a)*radius);
                //lineTo(cix, ciy);
                pa.resize(i+1);
                pa.setPoint(i++, cix, ciy);
            }
            //lineTo(toScreenX(p2.x), toScreenY(p2.y));
            pa.resize(i+1);
            pa.setPoint(i++, toScreenX(p2.x), toScreenY(p2.y));
            drawPolyline(pa);
        }
    }
}



/**
 * Draws an arc.
 *
 * @param cx center in x
 * @param cy center in y
 * @param radius Radius
 * @param a1 Angle 1 in rad
 * @param a2 Angle 2 in rad
 * @param reversed true: clockwise, false: counterclockwise
 */
void RS_PainterQt::drawArc( const RS_Vector& cp, 
                            double radius,
                            double a1, 
                            double a2,
                            [[maybe_unused]] bool reversed)
{
    if (radius <= 0.5)
    {
        drawGridPoint(cp);
    }
    else
    {
        if (reversed)
            std::swap(a1, a2);

        // shift a1 to the range of 0 to 2 pi, by a difference of multiplier of 2 pi
        a1 = M_PI + std::remainder(a1 - M_PI, 2. * M_PI);
        // shift a2 - a1 to the range of 0 to 2 pi
        a2 = a1+ M_PI + std::remainder(a2 - a1 - M_PI, 2. * M_PI);

        QPainter::drawArc( toScreenX(cp.x - radius), 
                           toScreenY(cp.y - radius), 
                           2.0 * radius, 
                           2.0 * radius, 
                           a1  * 16.0 * 180.0 / M_PI, 
                       (a2-a1) * 16.0 * 180.0 / M_PI);
    }
}


/**
 * Draws an arc on apple.
 *
 * @param cx center in x
 * @param cy center in y
 * @param radius Radius
 * @param a1 Angle 1 in rad
 * @param a2 Angle 2 in rad
 * @param reversed true: clockwise, false: counterclockwise
 */
void RS_PainterQt::drawArcMac(const RS_Vector& cp, double radius,
                           double a1, double a2,
                           bool reversed) {
        RS_DEBUG->print("RS_PainterQt::drawArcMac");
    if(radius<=0.5) {
        drawGridPoint(cp);
    } else {
        //QPointArray pa;
        //createArc(pa, cp, radius, a1, a2, reversed);

              double cix;            // Next point on circle
              double ciy;            //
              double aStep;         // Angle Step (rad)
              double a;             // Current Angle (rad)
                          double ox;
                          double oy;

              if(2.0/radius<=1.0) {
                aStep=std::asin(2.0/radius);
              } else {
                  aStep=1.0;
              }

              if (aStep<0.05) {
                  aStep = 0.05;
              }

              //QPointArray pa;
              //int i=0;
              //pa.resize(i+1);
              //pa.setPoint(i++, toScreenX(cp.x+cos(a1)*radius),
              //            toScreenY(cp.y-sin(a1)*radius));
              //moveTo(toScreenX(cp.x+cos(a1)*radius),
              //       toScreenY(cp.y-sin(a1)*radius));
              ox = cp.x+std::cos(a1)*radius;
              oy = cp.y-std::sin(a1)*radius;
              if(!reversed) {
                  // Arc Counterclockwise:
                  if(a1>a2-1.0e-10) {
                      a2+=2*M_PI;
                  }
                  for(a=a1+aStep; a<=a2; a+=aStep) {
                      cix = cp.x+std::cos(a)*radius;
                      ciy = cp.y-std::sin(a)*radius;
                      //lineTo(cix, ciy);
                                          drawLine(RS_Vector(ox, oy), RS_Vector(cix, ciy));
                                          ox = cix;
                                          oy = ciy;
                      //pa.resize(i+1);
                      //pa.setPoint(i++, cix, ciy);
                  }
              } else {
                  // Arc Clockwise:
                  if(a1<a2+1.0e-10) {
                      a2-=2*M_PI;
                  }
                  for(a=a1-aStep; a>=a2; a-=aStep) {
                      cix = cp.x+std::cos(a)*radius;
                      ciy = cp.y-std::sin(a)*radius;
                      drawLine(RS_Vector(ox, oy), RS_Vector(cix, ciy));
                                          ox = cix;
                                          oy = ciy;
                                          //lineTo(cix, ciy);
                      //pa.resize(i+1);
                      //pa.setPoint(i++, cix, ciy);
                  }
              }
              drawLine(RS_Vector(ox, oy),
                       RS_Vector(cp.x+std::cos(a2)*radius,
                                 cp.y-std::sin(a2)*radius));
              //lineTo(toScreenX(cp.x+cos(a2)*radius),
              //       toScreenY(cp.y-sin(a2)*radius));
              //pa.resize(i+1);
              //pa.setPoint(i++,
              //            toScreenX(cp.x+cos(a2)*radius),
              //            toScreenY(cp.y-sin(a2)*radius));
        //drawPolyline(pa);
    }
}



/**
 * Draws a circle.
 * @param cp Center point
 * @param radius Radius
 */
void RS_PainterQt::drawCircle(const RS_Vector& cp, double radius)
{
    QPainter::drawEllipse(QPointF(cp.x, cp.y), radius, radius);
}



/**
 * Draws a rotated ellipse arc.
 */
void RS_PainterQt::drawEllipse(const RS_Vector& cp,
                               double radius1, double radius2,
                               double angle,
                               double a1, double a2,
                               bool reversed) {
    QPolygon pa;
    createEllipse(pa, cp, radius1, radius2, angle, a1, a2, reversed);
    drawPolyline(pa);
}


void RS_PainterQt::drawImg(QImage& img, const RS_Vector& pos,
                           const RS_Vector& uVector, const RS_Vector& vVector, const RS_Vector& factor) {
    save();

    // Render smooth only at close zooms
    if (factor.x < 1 || factor.y < 1) {
       RS_PainterQt::setRenderHint(SmoothPixmapTransform , true);
    }
    else {
      RS_PainterQt::setRenderHint(SmoothPixmapTransform);
    }

    RS_Vector un = uVector/uVector.magnitude();
    RS_Vector vn = vVector/vVector.magnitude();

    // Image mirroring is switching the handedness of u-v vectors pair which can be detected by
    // looking at the sign of the z component of their cross product. If z is negative image is mirrored.
    std::unique_ptr<QTransform> wm;
    if(RS_Vector::crossP(uVector, vVector).z < 0) {
        wm.reset(new QTransform(un.x, -vn.x, -un.y, vn.y, pos.x, pos.y));
    } else {
        wm.reset( new QTransform(un.x, vn.x, un.y, vn.y, pos.x, pos.y));
    }

    wm->scale(factor.x, factor.y);
    setWorldTransform(*wm);

    drawImage(0,-img.height(), img);

    restore();
}


void RS_PainterQt::drawTextH(int x1, int y1,
                             int x2, int y2,
                             const QString& text) {
    drawText(x1, y1, x2, y2,
             Qt::AlignRight|Qt::AlignVCenter,
             text);
}



void RS_PainterQt::drawTextV(int x1, int y1,
                             int x2, int y2,
                             const QString& text) {
    save();
    QTransform wm = worldTransform();
    wm.rotate(-90.0);
    setWorldTransform(wm);

    drawText(x1, y1, x2, y2,
             Qt::AlignRight|Qt::AlignVCenter,
             text);

    restore();
}

void RS_PainterQt::fillRect(int x1, int y1, int w, int h,
                            const RS_Color& col) {
    QPainter::fillRect(x1, y1, w, h, col);
}


void RS_PainterQt::fillTriangle(const RS_Vector& p1,
                                const RS_Vector& p2,
                                const RS_Vector& p3) {

    QPolygon arr(3);
    QBrush brushSaved=brush();
    arr.putPoints(0, 3,
                  toScreenX(p1.x),toScreenY(p1.y),
                  toScreenX(p2.x),toScreenY(p2.y),
                  toScreenX(p3.x),toScreenY(p3.y));
    setBrush(RS_Color(pen().color()));
    drawPolygon(arr);
    setBrush(brushSaved);
}


void RS_PainterQt::erase() {
    QPainter::eraseRect(0,0,getWidth(),getHeight());
}


int RS_PainterQt::getWidth() const{
    return device()->width();
}

/** get Density per millimeter on screen/print device
  *@return density per millimeter in pixel/mm
  */
double RS_PainterQt::getDpmm() const{
    int mm(device()->widthMM());
    if(mm==0) mm=400;
    return double(device()->width())/mm;
}


int RS_PainterQt::getHeight() const{
    return device()->height();
}

const QBrush& RS_PainterQt::brush() const
{
    return QPainter::brush();
}

RS_Pen RS_PainterQt::getPen() const{
    return lpen;
    //RS_Pen p(pen().color(),
    //         RS2::qw(pen().width()),
    //         RS2::qw(pen().style()));
    //return QPainter::pen();
    //return p;
}

void RS_PainterQt::setPen(const RS_Pen& pen) {
    lpen = pen;
    switch (drawingMode) {
    case RS2::ModeBW:
        lpen.setColor( RS_Color( Qt::black));
        break;

    case RS2::ModeWB:
        lpen.setColor( RS_Color( Qt::white));
        break;

    default:
        break;
    }

    QColor pColor { lpen.getColor() };

    pColor.setAlphaF(pen.getAlpha());

    QPen p(pColor, RS_Math::round(lpen.getScreenWidth()),
		   rsToQtLineType(lpen.getLineType()));
    p.setJoinStyle(Qt::RoundJoin);
    p.setCapStyle(Qt::RoundCap);
    QPainter::setPen(p);
}

void RS_PainterQt::setPen(const RS_Color& color) {
    switch (drawingMode) {
    case RS2::ModeBW:
        lpen.setColor( RS_Color( Qt::black));
        QPainter::setPen( RS_Color( Qt::black));
        break;

    case RS2::ModeWB:
        lpen.setColor( RS_Color( Qt::white));
        QPainter::setPen( RS_Color( Qt::white));
        break;

    default:
        lpen.setColor( color);
        QPainter::setPen( color);
        break;
    }
}

void RS_PainterQt::setPen(int r, int g, int b) {
    switch (drawingMode) {
    case RS2::ModeBW:
        setPen( QColor( Qt::black));
        break;

    case RS2::ModeWB:
        setPen( QColor( Qt::white));
        break;

    default:
        setPen( QColor( r, g, b));
        break;
    }
}

void RS_PainterQt::disablePen() {
    lpen = RS_Pen(RS2::FlagInvalid);
    QPainter::setPen(Qt::NoPen);
}

void RS_PainterQt::setBrush(const RS_Color& color) {
    switch (drawingMode) {
    case RS2::ModeBW:
        QPainter::setBrush( QColor( Qt::black));
        break;

    case RS2::ModeWB:
        QPainter::setBrush( QColor( Qt::white));
        break;

    default:
        QPainter::setBrush( color);
        break;
    }
}

void RS_PainterQt::setBrush(const QBrush& color) {
    QPainter::setBrush(color);
}

void RS_PainterQt::drawPolygon(const QPolygon& a, Qt::FillRule rule) {
    QPainter::drawPolygon(a,rule);
}

void RS_PainterQt::drawPath ( const QPainterPath & path ) {
    QPainter::drawPath(path);
}


void RS_PainterQt::setClipRect(int x, int y, int w, int h) {
    QPainter::setClipRect(x, y, w, h);
    setClipping(true);
}

void RS_PainterQt::resetClipping() {
    setClipping(false);
}

void RS_PainterQt::fillRect ( const QRectF & rectangle, const RS_Color & color ) {

        double x1=rectangle.left();
        double x2=rectangle.right();
        double y1=rectangle.top();
        double y2=rectangle.bottom();

        QPainter::fillRect(toScreenX(x1),toScreenY(y1),toScreenX(x2)-toScreenX(x1),toScreenY(y2)-toScreenX(y1), color);
}
void RS_PainterQt::fillRect ( const QRectF & rectangle, const QBrush & brush ) {
        double x1=rectangle.left();
        double x2=rectangle.right();
        double y1=rectangle.top();
        double y2=rectangle.bottom();
        QPainter::fillRect(toScreenX(x1),toScreenY(y1),toScreenX(x2),toScreenY(y2), brush);
}
