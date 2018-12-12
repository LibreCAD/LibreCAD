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
#include<iostream>
#include <QImage>
#include "rs_image.h"
#include "rs_line.h"
#include "rs_settings.h"

#include "rs_constructionline.h"
#include "rs_debug.h"
#include "rs_graphicview.h"
#include "rs_painterqt.h"
#include "rs_math.h"

RS_ImageData::RS_ImageData(int _handle,
						   const RS_Vector& _insertionPoint,
						   const RS_Vector& _uVector,
						   const RS_Vector& _vVector,
						   const RS_Vector& _size,
						   const QString& _file,
						   int _brightness,
						   int _contrast,
						   int _fade):
	handle(_handle)
  , insertionPoint(_insertionPoint)
  , uVector(_uVector)
  , vVector(_vVector)
  , size(_size)
  , file(_file)
  , brightness(_brightness)
  , contrast(_contrast)
  , fade(_fade)
{
}

std::ostream& operator << (std::ostream& os, const RS_ImageData& ld) {
	os << "(" << ld.insertionPoint << ")";
	return os;
}

/**
 * Constructor.
 */
RS_Image::RS_Image(RS_EntityContainer* parent,
                   const RS_ImageData& d)
        :RS_AtomicEntity(parent), data(d) {

    update();
    calculateBorders();
}

RS_Image::RS_Image(const RS_Image& _image):
	RS_AtomicEntity(_image.getParent())
  ,data(_image.data)
  ,img(_image.img.get()?new QImage(*_image.img):nullptr)
{
}

RS_Image& RS_Image::operator = (const RS_Image& _image)
{
	data=_image.data;
	if(_image.img.get()){
		img.reset(new QImage(*_image.img));
	}else{
		img.reset();
	}
	return *this;
}

RS_Image::RS_Image(RS_Image&& _image):
	RS_AtomicEntity(_image.getParent())
  ,data(std::move(_image.data))
  ,img(std::move(_image.img))
{
}

RS_Image& RS_Image::operator = (RS_Image&& _image)
{
	data=_image.data;
	img = std::move(_image.img);
	return *this;
}


RS_Entity* RS_Image::clone() const {
    RS_Image* i = new RS_Image(*this);
        i->setHandle(getHandle());
    i->initId();
    i->update();
    return i;
}


void RS_Image::updateData(RS_Vector size, RS_Vector Uv, RS_Vector Vv) {
    data.size = size;
    data.uVector = Uv;
    data.vVector = Vv;
    update();
    calculateBorders();
}


void RS_Image::update() {

    RS_DEBUG->print("RS_Image::update");

    // the whole image:
    //QImage image = QImage(data.file);
	img.reset(new QImage(data.file));
	if (!img->isNull()) {
		data.size = RS_Vector(img->width(), img->height());
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
	for(const RS_Vector& vp: corners){
		paf.push_back(QPointF(vp.x, vp.y));
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

    if (entity) {
        *entity = const_cast<RS_Image*>(this);
    }

	RS_VectorSolutions const& corners =getCorners();
    //allow selecting image by clicking within images, bug#3464626
    if(containsPoint(coord)){
        //if coord is within image
        if(dist) *dist=0.;
        return coord;
    }
	RS_VectorSolutions points;
	for (size_t i=0; i < corners.size(); ++i){
		size_t const j = (i+1)%corners.size();
		RS_Line const l{corners.at(i), corners.at(j)};
		RS_Vector const vp = l.getNearestPointOnEntity(coord, onEntity);
		points.push_back(vp);
	}

	return points.getClosest(coord, dist);
}



RS_Vector RS_Image::getNearestCenter(const RS_Vector& coord,
									 double* dist) const{

	RS_VectorSolutions const& corners{getCorners()};
    //bug#485, there's no clear reason to ignore snapping to center within an image
//    if(containsPoint(coord)){
//        //if coord is within image
//        if(dist) *dist=0.;
//        return coord;
//    }

	RS_VectorSolutions points;
	for (size_t i=0; i < corners.size(); ++i) {
		size_t const j = (i+1)%corners.size();
		points.push_back((corners.get(i) + corners.get(j))*0.5);
	}
	points.push_back((corners.get(0) + corners.get(2))*0.5);

    return points.getClosest(coord, dist);
}


/*
 * ToDo, implement middlePoints
 */
RS_Vector RS_Image::getNearestMiddle(const RS_Vector& coord,
                                     double* dist,
                                     const int /*middlePoints*/) const{
	return getNearestCenter(coord, dist);
}



RS_Vector RS_Image::getNearestDist(double distance,
                                   const RS_Vector& coord,
								   double* dist) const{

	RS_VectorSolutions const& corners = getCorners();
	RS_VectorSolutions points;

	for (size_t i = 0; i < corners.size(); ++i){
		size_t const j = (i+1)%corners.size();
		RS_Line const l{corners.get(i), corners.get(j)};
		RS_Vector const& vp = l.getNearestDist(distance, coord, dist);
		points.push_back(vp);
	}

    return points.getClosest(coord, dist);
}



double RS_Image::getDistanceToPoint(const RS_Vector& coord,
                                    RS_Entity** entity,
                                    RS2::ResolveLevel /*level*/,
                                                                        double /*solidDist*/) const{
    if (entity) {
        *entity = const_cast<RS_Image*>(this);
    }

    RS_VectorSolutions corners = getCorners();

    //allow selecting image by clicking within images, bug#3464626
	if(containsPoint(coord)){
		//if coord is on image

		RS_SETTINGS->beginGroup("/Appearance");
		bool draftMode = (bool)RS_SETTINGS->readNumEntry("/DraftMode", 0);
		RS_SETTINGS->endGroup();
		if(!draftMode) return double(0.);
	}
    //continue to allow selecting by image edges
    double minDist = RS_MAXDOUBLE;

	for (size_t i = 0; i < corners.size(); ++i){
		size_t const j = (i+1)%corners.size();
		RS_Line const l{corners.get(i), corners.get(j)};
		double const dist = l.getDistanceToPoint(coord, nullptr);
		minDist = std::min(minDist, dist);
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
	if (!(painter && view) || !img.get() || img->isNull())
		return;

    // erase image:
    //if (painter->getPen().getColor()==view->getBackground()) {
    //	RS_VectorSolutions sol = getCorners();
    //
    //}

	RS_Vector scale{view->toGuiDX(data.uVector.magnitude()),
								view->toGuiDY(data.vVector.magnitude())};
    double angle = data.uVector.angle();

	painter->drawImg(*img,
                     view->toGui(data.insertionPoint),
                     angle, scale);

    if (isSelected()) {
        RS_VectorSolutions sol = getCorners();
		for (size_t i = 0; i < sol.size(); ++i){
			size_t const j = (i+1)%sol.size();
			painter->drawLine(view->toGui(sol.get(i)), view->toGui(sol.get(j)));
		}
    }
}



/**
 * Dumps the point's data to stdout.
 */
std::ostream& operator << (std::ostream& os, const RS_Image& i) {
    os << " Image: " << i.getData() << "\n";
    return os;
}

