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

#include <QDir>
#include <QFileInfo>

#include "qc_applicationwindow.h"
#include "rs_debug.h"
#include "rs_entitycontainer.h"
#include "rs_image.h"
#include "rs_line.h"
#include "rs_math.h"
#include "rs_painter.h"
#include "rs_polyline.h"
#include "rs_settings.h"

namespace
{
// fixme - sand - files - move to utility for relative paths calculations


// Return the file path name to use relative to the dxf file folder
    QString imageRelativePathName(QString& imageFile){
        // fixme - sand - files - this logic is incorrect, as it relies on the currently open document.
        // relative part should be calculated via graphics...
        auto currentDocumentFileName = QC_ApplicationWindow::getAppWindow()->getCurrentDocumentFileName();
        if (currentDocumentFileName.isEmpty() || imageFile.isEmpty()) {
            return imageFile;
        }

        QFileInfo dxfFileInfo(currentDocumentFileName);
        QFileInfo fileInfo(imageFile);
        if (fileInfo.exists()) {  // file exists as input file path
            QDir dxfDir(dxfFileInfo.canonicalPath());
            imageFile = dxfDir.relativeFilePath(imageFile);
            return fileInfo.canonicalFilePath();
        }
        else { // test a relative file path from the dxf file folder
            fileInfo.setFile(dxfFileInfo.canonicalPath() + "/" + imageFile);
            if (fileInfo.exists()) {
                return fileInfo.canonicalFilePath();
            }
            else { // search the current folder of the dxf for the dxf file name
                return dxfFileInfo.canonicalPath() + "/" + fileInfo.fileName();
            }
        }
        return {};
    }
}

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
    , fade(_fade){
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
    RS_Image::update();
    RS_Image::calculateBorders();
}

RS_Entity* RS_Image::clone() const {
    auto* i = new RS_Image(*this);
    i->setHandle(getHandle());
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
    QString filePathName = imageRelativePathName(data.file);

    //QImage image = QImage(data.file);
    img = std::make_shared<QImage>(filePathName);
    if (!img->isNull()) {
        data.size = RS_Vector(img->width(), img->height());
        RS_Image::calculateBorders(); // image update need this.
    } else {
        LC_LOG(RS_Debug::D_ERROR)<<"RS_Image::"<<__func__<<"(): image file not found: "<<data.file<<"("<<filePathName<<")";
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
    updateRectRegion();
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

void RS_Image::updateRectRegion()  {
    // x/y-size
    const RS_Vector dx = data.uVector*RS_Math::round(data.size.x);
    const RS_Vector dy = data.vVector*RS_Math::round(data.size.y);

    // image corners without the insertion point
    RS_VectorSolutions sol{
        RS_Vector{0., 0.},
        dx,
        dy,
        dx + dy
    };

    sol.move(data.insertionPoint);

    rectRegion.setCorners(sol.get(0),sol.get(2),sol.get(3), sol.get(1));

}
RS_VectorSolutions RS_Image::getCorners() const {
    return rectRegion.getCorners();
}

/**
  * whether a given point is within image region
  *@ coord, a point
  *@ returns true, if the point is within borders of image
  */
bool RS_Image::containsPoint(const RS_Vector& coord) const{
    QPolygonF paf;
    RS_VectorSolutions corners = getCorners();
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
        // fixme - sand - review why it's picked from settings and not from graphic view
        bool draftMode = LC_GET_ONE_BOOL("Appearance", "DraftMode");
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
    calculateBorders();
}

void RS_Image::rotate(const RS_Vector& center, double angle) {
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
    calculateBorders();
}

void RS_Image::mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) {
    data.insertionPoint.mirror(axisPoint1, axisPoint2);
    RS_Vector vp0(0.,0.);
    RS_Vector vp1( axisPoint2-axisPoint1 );
    data.uVector.mirror(vp0,vp1);
    data.vVector.mirror(vp0,vp1);
    calculateBorders();
}

void RS_Image::draw(RS_Painter* painter) {
    if (!img.get() || img->isNull()) {
        return;
    }
    painter->drawImgWCS(*img, data.insertionPoint, data.uVector, data.vVector);

    if (isSelected() && !(painter->isPrinting() || painter->isPrintPreview())) {
        RS_VectorSolutions sol = getCorners();

        RS_Vector c0 = sol.get(0);
        RS_Vector c1 = sol.get(1);
        RS_Vector c2 = sol.get(2);
        RS_Vector c3 = sol.get(3);

        painter->drawLineWCS(c0, c1);
        painter->drawLineWCS(c1, c2);
        painter->drawLineWCS(c2, c3);
        painter->drawLineWCS(c0, c3);
    }
}

void RS_Image::drawDraft([[maybe_unused]]RS_Painter *painter) {

    RS_VectorSolutions sol = getCorners();

    RS_Vector c0 = sol.get(0);
    RS_Vector c1 = sol.get(1);
    RS_Vector c2 = sol.get(2);
    RS_Vector c3 = sol.get(3);

    painter->drawLineWCS(c0, c1);
    painter->drawLineWCS(c1, c2);
    painter->drawLineWCS(c2, c3);
    painter->drawLineWCS(c3, c0);
    painter->drawLineWCS(c0, c2);
    painter->drawLineWCS(c1, c3);
}

RS_VectorSolutions RS_Image::getRefPoints() const {
    return rectRegion.getAllPoints();
}

void RS_Image::moveRef([[maybe_unused]]const RS_Vector &vector, const RS_Vector &offset) {
    move(offset);
}

void RS_Image::moveSelectedRef([[maybe_unused]]const RS_Vector &ref, const RS_Vector &offset) {
    /* todo - sand - restore later and support resizing of image via ref points
     * double uAngle = data.uVector.angle();

    RS_Vector newRef = ref+offset; // new position of ref

    // rotate rect and points if needed to ensure that rect now is parallel to axises
    RS_Vector normalizedRef = rectRegion.getRotatedPoint(ref, uAngle);
    RS_Vector normalizedNewRef = rectRegion.getRotatedPoint(newRef, uAngle);
    LC_RectRegion* rotated = rectRegion.getRotated(uAngle);

    LC_TransformData rectTransformData = rotated->determineTransformData(normalizedRef, normalizedNewRef);
    */
    move(offset);
}

/**
 * Dumps the point's data to stdout.
 */
std::ostream& operator << (std::ostream& os, const RS_Image& i) {
    os << " Image: " << i.getData() << "\n";
    return os;
}

RS_Entity *RS_Image::cloneProxy() const {
    auto result = new RS_EntityContainer(nullptr, true);
    auto pl = new RS_Polyline(result);
    // draw a rectangle for images as preview
    // Image corners: from insertion point, (0,0), dx, dy, dx + dy
    const RS_VectorSolutions corners = getCorners();
    for (const RS_Vector& corner: corners) {
        pl->addVertex(corner);
    }
    pl->addVertex(corners.at(0));
    pl->addVertex(corners.at(2));

    result->addEntity(pl);

    auto* diag = new RS_Line(result, {corners.at(1), corners.at(3)});
    result->addEntity(diag);
    return result;
}
