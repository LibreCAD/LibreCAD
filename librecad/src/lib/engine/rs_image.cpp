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


#include "rs_image.h"

#include "rs_constructionline.h"
#include "rs_debug.h"
#include "rs_graphicview.h"
#include "rs_painter.h"
#include "rs_painterqt.h"


/**
 * Constructor.
 */
RS_Image::RS_Image(RS_EntityContainer* parent,
                   const RS_ImageData& d)
        :RS_AtomicEntity(parent), data(d) {

    update();
    calculateBorders();
}



/**
 * Destructor.
 */
RS_Image::~RS_Image() {
    /*if (img!=NULL) {
        delete[] img;
    }*/
}




RS_Entity* RS_Image::clone() {
    RS_Image* i = new RS_Image(*this);
        i->setHandle(getHandle());
    i->initId();
    i->update();
    return i;
}



void RS_Image::update() {

    RS_DEBUG->print("RS_Image::update");

    // the whole image:
    //QImage image = QImage(data.file);
    img = QImage(data.file);
    if (!img.isNull()) {
        data.size = RS_Vector(img.width(), img.height());
    }

    RS_DEBUG->print("RS_Image::update: OK");

    /*
    // number of small images:
    nx = image.width()/100;
    ny = image.height()/100;

    // create small images:
    img = new QImage*[nx];
    QPixmap pm;
    int w,h;
    for (int x = 0; x<nx; ++x) {
        img[x] = new QImage[ny];
        for (int y = 0; y<ny; ++y) {
                if (x<nx-1) {
                        w = 100;
                }
                else {
                        w = image.width()%100;
                }

                if (y<ny-1) {
                        h = 100;
                }
                else {
                        h = image.height()%100;
                }

                pm = QPixmap(w, h);
                RS_PainterQt painter(&pm);
                painter.drawImage(-x*100, -y*100, image);
                img[x][y] = pm.convertToImage();
        }
    }
    */
}



void RS_Image::calculateBorders() {

    RS_VectorSolutions sol = getCorners();
        minV =  RS_Vector::minimum(
                RS_Vector::minimum(sol.get(0), sol.get(1)),
                RS_Vector::minimum(sol.get(2), sol.get(3))
                    );
        maxV =  RS_Vector::maximum(
                RS_Vector::maximum(sol.get(0), sol.get(1)),
                RS_Vector::maximum(sol.get(2), sol.get(3))
                    );
}

RS_VectorSolutions RS_Image::getCorners() const {
        RS_VectorSolutions sol(4);

        sol.set(0, data.insertionPoint);
        sol.set(1,
                data.insertionPoint + data.uVector*RS_Math::round(data.size.x));
        sol.set(3,
                data.insertionPoint + data.vVector*RS_Math::round(data.size.y));
        sol.set(2, sol.get(3) + data.uVector*RS_Math::round(data.size.x));

        return sol;
}

/**
  * whether a given point is within image region
  *@ coord, a point
  *@ returns true, if the point is within borders of image
  */
bool RS_Image::containsPoint(const RS_Vector& coord) const{
    QPolygonF paf;
    RS_VectorSolutions corners =getCorners();
    for(int i=0;i<corners.getNumber();i++){
        paf.push_back(QPointF(corners.get(i).x,corners.get(i).y));
    }
    paf.push_back(paf.at(0));
    return paf.containsPoint(QPointF(coord.x,coord.y),Qt::OddEvenFill);
}

RS_Vector RS_Image::getNearestEndpoint(const RS_Vector& coord,
                                       double* dist) const {
    RS_VectorSolutions corners =getCorners();
    return corners.getClosest(coord, dist);
}



RS_Vector RS_Image::getNearestPointOnEntity(const RS_Vector& coord,
        bool onEntity, double* dist, RS_Entity** entity) const{

    if (entity!=NULL) {
        *entity = const_cast<RS_Image*>(this);
    }

    RS_VectorSolutions corners =getCorners();
    //allow selecting image by clicking within images, bug#3464626
    if(containsPoint(coord)){
        //if coord is within image
        if(dist!=NULL) *dist=0.;
        return coord;
    }
    RS_VectorSolutions points(4);

    RS_Line l[] =
        {
            RS_Line(NULL, RS_LineData(corners.get(0), corners.get(1))),
            RS_Line(NULL, RS_LineData(corners.get(1), corners.get(2))),
            RS_Line(NULL, RS_LineData(corners.get(2), corners.get(3))),
            RS_Line(NULL, RS_LineData(corners.get(3), corners.get(0)))
        };

    for (int i=0; i<4; ++i) {
        points.set(i, l[i].getNearestPointOnEntity(coord, onEntity));
    }

    return points.getClosest(coord, dist);
}



RS_Vector RS_Image::getNearestCenter(const RS_Vector& coord,
                                     double* dist) {

    RS_VectorSolutions points(4);
    RS_VectorSolutions corners = getCorners();
    if(containsPoint(coord)){
        //if coord is within image
        if(dist!=NULL) *dist=0.;
        return coord;
    }

    points.set(0, (corners.get(0) + corners.get(1))/2.0);
    points.set(1, (corners.get(1) + corners.get(2))/2.0);
    points.set(2, (corners.get(2) + corners.get(3))/2.0);
    points.set(3, (corners.get(3) + corners.get(0))/2.0);

    return points.getClosest(coord, dist);
}


/*
 * ToDo, implement middlePoints
 */
RS_Vector RS_Image::getNearestMiddle(const RS_Vector& coord,
                                     double* dist,
                                     const int /*middlePoints*/) const{
    return const_cast<RS_Image*>(this)->getNearestCenter(coord, dist);
}



RS_Vector RS_Image::getNearestDist(double distance,
                                   const RS_Vector& coord,
                                   double* dist) {

    RS_VectorSolutions corners = getCorners();
    RS_VectorSolutions points(4);

    RS_Line l[] =
        {
            RS_Line(NULL, RS_LineData(corners.get(0), corners.get(1))),
            RS_Line(NULL, RS_LineData(corners.get(1), corners.get(2))),
            RS_Line(NULL, RS_LineData(corners.get(2), corners.get(3))),
            RS_Line(NULL, RS_LineData(corners.get(3), corners.get(0)))
        };

    for (int i=0; i<4; ++i) {
        points.set(i, l[i].getNearestDist(distance, coord, dist));
    }

    return points.getClosest(coord, dist);
}



double RS_Image::getDistanceToPoint(const RS_Vector& coord,
                                    RS_Entity** entity,
                                    RS2::ResolveLevel /*level*/,
                                                                        double /*solidDist*/) const{
    if (entity!=NULL) {
        *entity = const_cast<RS_Image*>(this);
    }

    RS_VectorSolutions corners = getCorners();

    //allow selecting image by clicking within images, bug#3464626
    if(containsPoint(coord)){
        //if coord is on image
        return double(0.);
    }
    //continue to allow selecting by image edges
    double dist;
    double minDist = RS_MAXDOUBLE;

    RS_Line l[] =
        {
            RS_Line(NULL, RS_LineData(corners.get(0), corners.get(1))),
            RS_Line(NULL, RS_LineData(corners.get(1), corners.get(2))),
            RS_Line(NULL, RS_LineData(corners.get(2), corners.get(3))),
            RS_Line(NULL, RS_LineData(corners.get(3), corners.get(0)))
        };

    for (int i=0; i<4; ++i) {
        dist = l[i].getDistanceToPoint(coord, NULL);
        if (dist<minDist) {
            minDist = dist;
        }
    }

    return minDist;
}



void RS_Image::move(const RS_Vector& offset) {
    data.insertionPoint.move(offset);
    moveBorders(offset);
}



void RS_Image::rotate(const RS_Vector& center, const double& angle) {
    RS_Vector angleVector(angle);
    data.insertionPoint.rotate(center, angleVector);
    data.uVector.rotate(angleVector);
    data.vVector.rotate(angleVector);
    calculateBorders();
}

void RS_Image::rotate(const RS_Vector& center, const RS_Vector& angleVector) {
    data.insertionPoint.rotate(center, angleVector);
    data.uVector.rotate(angleVector);
    data.vVector.rotate(angleVector);
    calculateBorders();
}


void RS_Image::scale(const RS_Vector& center, const RS_Vector& factor) {
    data.insertionPoint.scale(center, factor);
    data.uVector.scale(factor);
    data.vVector.scale(factor);
    scaleBorders(center,factor);
}



void RS_Image::mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) {
    data.insertionPoint.mirror(axisPoint1, axisPoint2);
    RS_Vector vp0(0.,0.);
    RS_Vector vp1( axisPoint2-axisPoint1 );
    data.uVector.mirror(vp0,vp1);
    data.vVector.mirror(vp0,vp1);
    calculateBorders();
}



void RS_Image::draw(RS_Painter* painter, RS_GraphicView* view, double& /*patternOffset*/) {
    if (painter==NULL || view==NULL || img.isNull()) {
        return;
    }

    // erase image:
    //if (painter->getPen().getColor()==view->getBackground()) {
    //	RS_VectorSolutions sol = getCorners();
    //
    //}

    int ox = 0;
    int oy = 0;
    int width = view->getWidth();
    int height = view->getHeight();

    RS_Vector scale = RS_Vector(view->toGuiDX(data.uVector.magnitude()),
                                view->toGuiDY(data.vVector.magnitude()));
    double angle = data.uVector.angle();

    int startX, stopX, startY, stopY;

    if (data.uVector.x>1.0e-6 && data.vVector.y>1.0e-6) {
        startX = (int)((view->toGraphX(ox)-data.insertionPoint.x) /
                       data.uVector.x) - 1;
        if (startX<0) {
            startX = 0;
        }
        stopX = (int)((view->toGraphX(width)-data.insertionPoint.x) /
                      data.uVector.x) + 1;
        if (stopX>(int)data.size.x) {
            stopX = (int)data.size.x;
        }
        startY = -(int)((view->toGraphY(oy) -
                         (data.insertionPoint.y+getImageHeight())) /
                        data.vVector.y) - 1;
        if (startY<0) {
            startY = 0;
        }
        stopY = -(int)((view->toGraphY(height) -
                        (data.insertionPoint.y+getImageHeight())) /
                       data.vVector.y) + 1;
        if (stopY>(int)data.size.y) {
            stopY = (int)data.size.y;
        }
    }
        else {
                startX = 0;
                startY = 0;
                stopX = 0;
                stopY = 0;
        }

    painter->drawImg(img,
                     view->toGui(data.insertionPoint),
                     angle, scale,
                     startX, startY, stopX-startX, stopY-startY);

    if (isSelected()) {
        RS_VectorSolutions sol = getCorners();

        painter->drawLine(view->toGui(sol.get(0)), view->toGui(sol.get(1)));
        painter->drawLine(view->toGui(sol.get(1)), view->toGui(sol.get(2)));
        painter->drawLine(view->toGui(sol.get(2)), view->toGui(sol.get(3)));
        painter->drawLine(view->toGui(sol.get(3)), view->toGui(sol.get(0)));
    }
}



/**
 * Dumps the point's data to stdout.
 */
std::ostream& operator << (std::ostream& os, const RS_Image& i) {
    os << " Image: " << i.getData() << "\n";
    return os;
}

