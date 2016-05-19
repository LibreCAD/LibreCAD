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


#ifndef RS_HATCH_H
#define RS_HATCH_H

#include "rs_entity.h"
#include "rs_entitycontainer.h"

/**
 * Holds the data that defines a hatch entity.
 */
struct RS_HatchData {
    /**
     * Default constructor. Leaves the data object uninitialized.
     */
	RS_HatchData() = default;
	~RS_HatchData() = default;

        /**
         * @param solid true: solid fill, false: pattern.
         * @param scale Pattern scale or spacing.
         * @param pattern Pattern name.
         */
	RS_HatchData(bool solid,
				 double scale,
				 double angle,
				 const QString& pattern);


	bool solid;
	double scale;
	double angle;
	QString pattern;
};

std::ostream& operator << (std::ostream& os, const RS_HatchData& td);



/**
 * Class for a hatch entity.
 *
 * @author Andrew Mustun
 */
class RS_Hatch : public RS_EntityContainer {
public:
    enum RS_HatchError { HATCH_UNDEFINED = -1,
                         HATCH_OK,
                         HATCH_INVALID_CONTOUR,
                         HATCH_PATTERN_NOT_FOUND,
                         HATCH_TOO_SMALL,
                         HATCH_AREA_TOO_BIG };

	RS_Hatch() = default;

    RS_Hatch(RS_EntityContainer* parent,
            const RS_HatchData& d);

	RS_Entity* clone() const override;

    /**	@return RS2::EntityHatch */
	RS2::EntityType rtti() const override{
        return RS2::EntityHatch;
    }

    /**
     * @return true: if this is a hatch with lines (hatch pattern),
     *         false: if this is filled with a solid color.
     */
	bool isContainer() const override;

    /** @return Copy of data that defines the hatch. */
    RS_HatchData getData() const {
        return data;
    }

        bool validate();

		int countLoops() const;

        /** @return true if this is a solid fill. false if it is a pattern hatch. */
        bool isSolid() const {
                return data.solid;
        }
        void setSolid(bool solid) {
                data.solid = solid;
        }

        QString getPattern() {
                return data.pattern;
        }
        void setPattern(const QString& pattern) {
                data.pattern = pattern;
        }

        double getScale() {
                return data.scale;
        }
        void setScale(double scale) {
                data.scale = scale;
        }

        double getAngle() {
                return data.angle;
        }
        void setAngle(double angle) {
                data.angle = angle;
        }
		double getTotalArea();

		void calculateBorders() override;
		void update() override;
        int getUpdateError() {
                return updateError;
        }
        void activateContour(bool on);

		void draw(RS_Painter* painter, RS_GraphicView* view,
						  double& patternOffset) override;

		//	double getLength() {
        //		return -1.0;
        //	}

		double getDistanceToPoint(const RS_Vector& coord,
                                          RS_Entity** entity = NULL,
                                          RS2::ResolveLevel level = RS2::ResolveNone,
										  double solidDist = RS_MAXDOUBLE) const override;


		void move(const RS_Vector& offset) override;
		void rotate(const RS_Vector& center, const double& angle) override;
		void rotate(const RS_Vector& center, const RS_Vector& angleVector) override;
		void scale(const RS_Vector& center, const RS_Vector& factor) override;
		void mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) override;
		void stretch(const RS_Vector& firstCorner,
                             const RS_Vector& secondCorner,
							 const RS_Vector& offset) override;

        friend std::ostream& operator << (std::ostream& os, const RS_Hatch& p);

protected:
        RS_HatchData data;
        RS_EntityContainer* hatch;
        bool updateRunning;
        bool needOptimization;
        int  updateError;
};

#endif
