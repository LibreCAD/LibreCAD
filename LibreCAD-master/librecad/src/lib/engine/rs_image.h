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
#ifndef RS_IMAGE_H
#define RS_IMAGE_H

#include <memory>
#include "rs_atomicentity.h"

class QImage;

/**
 * Holds the data that defines a line.
 */
struct RS_ImageData {
    /**
     * Default constructor. Leaves the data object uninitialized.
     */
	RS_ImageData() = default;

    RS_ImageData(int handle,
                                const RS_Vector& insertionPoint,
                const RS_Vector& uVector,
                                const RS_Vector& vVector,
                                const RS_Vector& size,
                                const QString& file,
                                int brightness,
                                int contrast,
								int fade);

	/** Handle of image definition. */
	int handle;
	/** Insertion point. */
	RS_Vector insertionPoint;
	/** u vector. Points along visual bottom of image. */
	RS_Vector uVector;
	/** v vector. Points along visual left of image. */
	RS_Vector vVector;
	/** Image size in pixel. */
	RS_Vector size;
	/** Path to image file. */
	QString file;
	/** Brightness (0..100, default: 50). */
	int brightness;
	/** Contrast (0..100, default: 50). */
	int contrast;
	/** Fade (0..100, default: 0). */
	int fade;
};



/**
 * Class for a line entity.
 *
 * @author Andrew Mustun
 */
class RS_Image : public RS_AtomicEntity {
public:
    RS_Image(RS_EntityContainer* parent,
            const RS_ImageData& d);
	RS_Image(const RS_Image& _image);
	RS_Image(RS_Image&& _image);
	RS_Image& operator = (const RS_Image& _image);
	RS_Image& operator = (RS_Image&& _image);

	RS_Entity* clone() const override;

    /**	@return RS2::EntityImage */
	RS2::EntityType rtti() const override{
        return RS2::EntityImage;
    }

		void update() override;

    /** @return Copy of data that defines the image. */
    RS_ImageData getData() const {
        return data;
    }

    /** @return Insertion point of the entity */
	RS_Vector getInsertionPoint() const {
        return data.insertionPoint;
    }
    /** Sets the insertion point for the image. */
    void setInsertionPoint(RS_Vector ip) {
        data.insertionPoint = ip;
        calculateBorders();
    }

    /** Update image data ONLY for plugins. */
    void updateData(RS_Vector size, RS_Vector Uv, RS_Vector Vv);

        /** @return File name of the image. */
        QString getFile() const {
                return data.file;
        }

        /** Sets the file name of the image.  */
        void setFile(const QString& file) {
                data.file = file;
        }

        /** @return u Vector. Points along bottom, 1 pixel long. */
        RS_Vector getUVector() const {
                return data.uVector;
        }
        /** @return v Vector. Points along left, 1 pixel long. */
        RS_Vector getVVector() const {
                return data.vVector;
        }
        /** @return Width of image in pixels. */
        int getWidth() const {
                return (int)data.size.x;
        }
        /** @return Height of image in pixels. */
        int getHeight() const {
                return (int)data.size.y;
        }
        /** @return Brightness. */
        int getBrightness() const {
                return data.brightness;
        }
        /** @return Contrast. */
        int getContrast() const {
                return data.contrast;
        }
        /** @return Fade. */
        int getFade() const {
                return data.fade;
        }
        /** @return Image definition handle. */
        int getHandle() const {
                return data.handle;
        }
        /** Sets the image definition handle. */
        void setHandle(int h) {
                data.handle = h;
        }


        /** @return The four corners. **/
        RS_VectorSolutions getCorners() const;

        /**
         * @return image with in graphic units.
         */
        double getImageWidth() {
                return data.size.x * data.uVector.magnitude();
        }

        /**
         * @return image height in graphic units.
         */
        double getImageHeight() {
                return data.size.y * data.vVector.magnitude();
        }


	RS_Vector getNearestEndpoint(const RS_Vector& coord,
										 double* dist = NULL)const override;
	RS_Vector getNearestPointOnEntity(const RS_Vector& coord,
			bool onEntity=true, double* dist = NULL, RS_Entity** entity=NULL)const override;
	RS_Vector getNearestCenter(const RS_Vector& coord,
									   double* dist = NULL)const override;
	RS_Vector getNearestMiddle(const RS_Vector& coord,
                                       double* dist = NULL,
									   int middlePoints=1)const override;
	RS_Vector getNearestDist(double distance,
                                     const RS_Vector& coord,
									 double* dist = NULL)const override;
	double getDistanceToPoint(const RS_Vector& coord,
                                      RS_Entity** entity=NULL,
                                      RS2::ResolveLevel level=RS2::ResolveNone,
							  double solidDist = RS_MAXDOUBLE) const override;

//        double getLength() const {
//                return -1.0;
//        }

	void move(const RS_Vector& offset) override;
	void rotate(const RS_Vector& center, const double& angle) override;
	void rotate(const RS_Vector& center, const RS_Vector& angleVector) override;
	void scale(const RS_Vector& center, const RS_Vector& factor) override;
	void mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) override;
	/*void stretch(RS_Vector firstCorner,
                         RS_Vector secondCorner,
                         RS_Vector offset);*/

	void draw(RS_Painter* painter, RS_GraphicView* view, double& patternOffset) override;

    friend std::ostream& operator << (std::ostream& os, const RS_Image& l);

	void calculateBorders() override;


protected:
	// whether the point is within image
	bool containsPoint(const RS_Vector& coord) const;
	RS_ImageData data;
	std::unique_ptr<QImage> img;
        //QImage** img;
        //int nx;
        //int ny;
};

#endif
