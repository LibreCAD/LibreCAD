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

#include <QDir>
#include <QFileInfo>
#include<iostream>
#include <QGuiApplication>

#include "qc_applicationwindow.h"
#include "rs_debug.h"
#include "rs_entitycontainer.h"
#include "rs_line.h"
#include "rs_math.h"
#include "rs_painter.h"
#include "rs_polyline.h"
#include "rs_settings.h"

namespace {
    // fixme - sand - files - move to utility for relative paths calculations

    // Return the file path name to use relative to the dxf file folder
    QString imageRelativePathName(QString& imageFile) {
        // fixme - sand - files - this logic is incorrect, as it relies on the currently open document.
        // relative part should be calculated via graphics...
        const auto currentDocumentFileName = QC_ApplicationWindow::getAppWindow()->getCurrentDocumentFileName();
        if (currentDocumentFileName.isEmpty() || imageFile.isEmpty()) {
            return imageFile;
        }

        const QFileInfo dxfFileInfo(currentDocumentFileName);
        QFileInfo fileInfo(imageFile);
        if (fileInfo.exists()) {
            // file exists as input file path
            const QDir dxfDir(dxfFileInfo.canonicalPath());
            imageFile = dxfDir.relativeFilePath(imageFile);
            return fileInfo.canonicalFilePath();
        }
        // test a relative file path from the dxf file folder
        fileInfo.setFile(dxfFileInfo.canonicalPath() + "/" + imageFile);
        if (fileInfo.exists()) {
            return fileInfo.canonicalFilePath();
        }
        // search the current folder of the dxf for the dxf file name
        return dxfFileInfo.canonicalPath() + "/" + fileInfo.fileName();
    }
}

RS_ImageData::RS_ImageData(const int handle, const RS_Vector& insertionPoint, const RS_Vector& uVector, const RS_Vector& vVector,
                           const RS_Vector& size, const QString& file, const int brightness, const int contrast,
                           const int fade) : handle(handle), insertionPoint(insertionPoint), uVector(uVector), vVector(vVector), size(size),
                                             file(file), brightness(brightness), contrast(contrast), fade(fade) {
}

std::ostream& operator <<(std::ostream& os, const RS_ImageData& ld) {
    os << "(" << ld.insertionPoint << ")";
    return os;
}

/**
 * Constructor.
 */
RS_Image::RS_Image(RS_EntityContainer* parent, const RS_ImageData& d)
    : RS_AtomicEntity(parent), m_data(d) {
    RS_Image::update();
    RS_Image::calculateBorders();
}

RS_Entity* RS_Image::clone() const {
    auto* i = new RS_Image(*this);
    i->setHandle(getHandle());
    i->update();
    return i;
}

void RS_Image::updateData(const RS_Vector& size, const RS_Vector& Uv, const RS_Vector& Vv) {
    m_data.size = size;
    m_data.uVector = Uv;
    m_data.vVector = Vv;
    update();
    calculateBorders();
}

void RS_Image::update() {
    RS_DEBUG->print("RS_Image::update");

    // fixme - sand - merge - this should be guarded by ifdef. Otherwise, it's not needed runtime overhead!

    // Headless guard: constructing a QImage pulls in the GUI image plugins,
    // which abort without a running GUI application (e.g. a QCoreApplication
    // test harness round-tripping a drawing that contains an IMAGE/WIPEOUT).
    // Geometry/borders are unaffected; skip only the raster load when no GUI
    // app is present. Behaviour is unchanged once a real QApplication exists.
    if (QGuiApplication::instance() == nullptr) {
        RS_DEBUG->print("RS_Image::update: no GUI application, skipping raster load");
        return;
    }

    // the whole image:
    QString filePathName = imageRelativePathName(m_data.file);

    //QImage image = QImage(data.file);
    m_img = std::make_shared<QImage>(filePathName);
    if (!m_img->isNull()) {
        m_data.size = RS_Vector(m_img->width(), m_img->height());
        RS_Image::calculateBorders(); // image update need this.
    }
    else {
        LC_LOG(RS_Debug::D_ERROR) << "RS_Image::" << __func__ << "(): image file not found: " << m_data.file << "(" << filePathName << ")";
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
    const RS_VectorSolutions sol = getCorners();
    m_minV = RS_Vector::minimum(RS_Vector::minimum(sol.get(0), sol.get(1)), RS_Vector::minimum(sol.get(2), sol.get(3)));
    m_maxV = RS_Vector::maximum(RS_Vector::maximum(sol.get(0), sol.get(1)), RS_Vector::maximum(sol.get(2), sol.get(3)));
}

void RS_Image::updateRectRegion() {
    // x/y-size
    const RS_Vector dx = m_data.uVector * RS_Math::round(m_data.size.x);
    const RS_Vector dy = m_data.vVector * RS_Math::round(m_data.size.y);

    // image corners without the insertion point
    RS_VectorSolutions sol{RS_Vector{0., 0.}, dx, dy, dx + dy};

    sol.move(m_data.insertionPoint);

    m_rectRegion.setCorners(sol.get(0), sol.get(2), sol.get(3), sol.get(1));
}

RS_VectorSolutions RS_Image::getCorners() const {
    return m_rectRegion.getCorners();
}

/**
  * whether a given point is within image region
  *@ coord, a point
  *@ returns true, if the point is within borders of image
  */
bool RS_Image::containsPoint(const RS_Vector& coord) const {
    QPolygonF paf;
    RS_VectorSolutions corners = getCorners();
    for (const RS_Vector& vp : corners) {
        paf.push_back(QPointF(vp.x, vp.y));
    }
    paf.push_back(paf.at(0));
    return paf.containsPoint(QPointF(coord.x, coord.y), Qt::OddEvenFill);
}

RS_Vector RS_Image::doGetNearestEndpoint(const RS_Vector& coord, double* dist, RS_Entity** entity) const {
    const RS_VectorSolutions corners = getCorners();
    if (entity != nullptr) {
        *entity = const_cast<RS_Image*>(this);
    }
    return corners.getClosest(coord, dist);
}

RS_Vector RS_Image::doGetNearestPointOnEntity(const RS_Vector& coord, const bool onEntity, double* dist, RS_Entity** entity) const {
    if (entity != nullptr) {
        *entity = const_cast<RS_Image*>(this);
    }

    const RS_VectorSolutions& corners = getCorners();
    //allow selecting image by clicking within images, bug#3464626
    if (containsPoint(coord)) {
        //if coord is within image
        if (dist != nullptr) {
            *dist = 0.;
        }
        return coord;
    }
    RS_VectorSolutions points;
    for (size_t i = 0; i < corners.size(); ++i) {
        const size_t j = (i + 1) % corners.size();
        const RS_Line l{corners.at(i), corners.at(j)};
        const RS_Vector vp = l.getNearestPointOnEntity(coord, onEntity);
        points.push_back(vp);
    }

    return points.getClosest(coord, dist);
}

RS_Vector RS_Image::doGetNearestCenter(const RS_Vector& coord, double* dist, RS_Entity** centerEntity) const {
    const RS_VectorSolutions& corners{getCorners()};
    //bug#485, there's no clear reason to ignore snapping to center within an image
    //    if(containsPoint(coord)){
    //        //if coord is within image
    //        if(dist) *dist=0.;
    //        return coord;
    //    }

    RS_VectorSolutions points;
    for (size_t i = 0; i < corners.size(); ++i) {
        const size_t j = (i + 1) % corners.size();
        points.push_back((corners.get(i) + corners.get(j)) * 0.5);
    }
    points.push_back((corners.get(0) + corners.get(2)) * 0.5);

    if (centerEntity != nullptr) {
        *centerEntity = const_cast<RS_Image*>(this);
    }
    return points.getClosest(coord, dist);
}

/*
 * ToDo, implement middlePoints
 */
RS_Vector RS_Image::doGetNearestMiddle(const RS_Vector& coord, double* dist, [[maybe_unused]] const int middlePoints) const {
    return getNearestCenter(coord, dist);
}

RS_Vector RS_Image::doGetNearestDist(const double distance, const RS_Vector& coord, double* dist) const {
    const RS_VectorSolutions& corners = getCorners();
    RS_VectorSolutions points;

    for (size_t i = 0; i < corners.size(); ++i) {
        const size_t j = (i + 1) % corners.size();
        const RS_Line l{corners.get(i), corners.get(j)};
        const RS_Vector& vp = l.getNearestDist(distance, coord, dist);
        points.push_back(vp);
    }

    return points.getClosest(coord, dist);
}

double RS_Image::doGetDistanceToPoint(const RS_Vector& coord, RS_Entity** entity, [[maybe_unused]] RS2::ResolveLevel level,
                                      [[maybe_unused]] double solidDist) const {
    if (entity != nullptr) {
        *entity = const_cast<RS_Image*>(this);
    }

    const RS_VectorSolutions corners = getCorners();

    //allow selecting image by clicking within images, bug#3464626
    if (containsPoint(coord)) {
        //if coord is on image
        // fixme - sand - review why it's picked from settings and not from graphic view
        const bool draftMode = LC_GET_ONE_BOOL("Appearance", "DraftMode");
        if (!draftMode) {
            return 0.0;
        }
    }
    //continue to allow selecting by image edges
    double minDist = RS_MAXDOUBLE;

    for (size_t i = 0; i < corners.size(); ++i) {
        const size_t j = (i + 1) % corners.size();
        const RS_Line l{corners.get(i), corners.get(j)};
        const double dist = l.getDistanceToPoint(coord, nullptr);
        minDist = std::min(minDist, dist);
    }

    return minDist;
}

void RS_Image::move(const RS_Vector& offset) {
    m_data.insertionPoint.move(offset);
    calculateBorders();
}

void RS_Image::rotate(const RS_Vector& center, const double angle) {
    const RS_Vector angleVector(angle);
    m_data.insertionPoint.rotate(center, angleVector);
    m_data.uVector.rotate(angleVector);
    m_data.vVector.rotate(angleVector);
    calculateBorders();
}

void RS_Image::rotate(const RS_Vector& center, const RS_Vector& angleVector) {
    m_data.insertionPoint.rotate(center, angleVector);
    m_data.uVector.rotate(angleVector);
    m_data.vVector.rotate(angleVector);
    calculateBorders();
}

void RS_Image::scale(const RS_Vector& center, const RS_Vector& factor) {
    m_data.insertionPoint.scale(center, factor);
    m_data.uVector.scale(factor.x);
    m_data.vVector.scale(factor.y);
    calculateBorders();
}

void RS_Image::mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) {
    m_data.insertionPoint.mirror(axisPoint1, axisPoint2);
    const RS_Vector vp0(0., 0.);
    const RS_Vector vp1(axisPoint2 - axisPoint1);
    m_data.uVector.mirror(vp0, vp1);
    m_data.vVector.mirror(vp0, vp1);
    calculateBorders();
}

void RS_Image::draw(RS_Painter* painter) {
    if ((m_img.get() == nullptr) || m_img->isNull()) {
        return;
    }
    painter->drawImgWCS(*m_img, m_data.insertionPoint, m_data.uVector, m_data.vVector);

    if (isSelected() && !(painter->isPrinting() || painter->isPrintPreview())) {
        const RS_VectorSolutions sol = getCorners();

        const RS_Vector c0 = sol.get(0);
        const RS_Vector c1 = sol.get(1);
        const RS_Vector c2 = sol.get(2);
        const RS_Vector c3 = sol.get(3);

        painter->drawLineWCS(c0, c1);
        painter->drawLineWCS(c1, c2);
        painter->drawLineWCS(c2, c3);
        painter->drawLineWCS(c0, c3);
    }
}

void RS_Image::drawDraft([[maybe_unused]] RS_Painter* painter) {
    const RS_VectorSolutions sol = getCorners();

    const RS_Vector c0 = sol.get(0);
    const RS_Vector c1 = sol.get(1);
    const RS_Vector c2 = sol.get(2);
    const RS_Vector c3 = sol.get(3);

    painter->drawLineWCS(c0, c1);
    painter->drawLineWCS(c1, c2);
    painter->drawLineWCS(c2, c3);
    painter->drawLineWCS(c3, c0);
    painter->drawLineWCS(c0, c2);
    painter->drawLineWCS(c1, c3);
}

RS_VectorSolutions RS_Image::getRefPoints() const {
    return m_rectRegion.getAllPoints();
}

void RS_Image::moveRef([[maybe_unused]] const RS_Vector& vector, const RS_Vector& offset) {
    move(offset);
}

void RS_Image::moveSelectedRef([[maybe_unused]] const RS_Vector& ref, const RS_Vector& offset) {
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
std::ostream& operator <<(std::ostream& os, const RS_Image& img) {
    os << " Image: " << img.getData() << "\n";
    return os;
}

RS_Entity* RS_Image::cloneProxy() const {
    // fixme - sand - rework this, create special object for proxy - it may represent entity container PLUS support of refPoints for highlight.
    // fixme - sand - proxy will be used just in limited cases (for image - it's important on highlight!), so such a specialized object may be handy
    const auto result = new RS_EntityContainer(nullptr, true);
    const auto pl = new RS_Polyline(result);
    // draw a rectangle for images as preview
    // Image corners: from insertion point, (0,0), dx, dy, dx + dy
    const RS_VectorSolutions corners = getCorners();
    for (const RS_Vector& corner : corners) {
        pl->addVertex(corner);
    }
    pl->addVertex(corners.at(0));
    pl->addVertex(corners.at(2));

    result->addEntity(pl);

    const auto* diag = new RS_Line(result, {corners.at(1), corners.at(3)});
    result->addEntity(diag);
    result->setSelectionFlag(true);
    return result;
}
