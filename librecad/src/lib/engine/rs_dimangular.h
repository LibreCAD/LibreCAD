/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2018 A. Stebich (librecad@mail.lordofbikes.de)
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

#ifndef RS_DIMANGULAR_H
#define RS_DIMANGULAR_H

#include "rs_dimension.h"
#include "rs_constructionline.h"

/**
 * Holds the data that defines a angular dimension entity.
 */
struct RS_DimAngularData
{
    RS_DimAngularData();
    RS_DimAngularData(const RS_DimAngularData& ed);

    /**
     * Constructor with initialisation.
     *
     * @param definitionPoint Definition point of the angular dimension.
     * @param leader Leader length.
     */
    RS_DimAngularData(const RS_Vector& definitionPoint1,
                      const RS_Vector& definitionPoint2,
                      const RS_Vector& definitionPoint3,
                      const RS_Vector& definitionPoint4);

    RS_Vector definitionPoint1; ///< 1st line start point, DXF codes 13,23,33
    RS_Vector definitionPoint2; ///< 1st line end point, DXF codes 14,24,34
    RS_Vector definitionPoint3; ///< 2nd line start point, DXF codes 15,25,35
                                ///< 2nd line end point is in common dim data, DXF codes 10,20,30
    RS_Vector definitionPoint4; ///< dim arc radius point, DXF codes 16,26,36
};

std::ostream& operator << (std::ostream& os, const RS_DimAngularData& dd);

/**
 * Holds the DXF variables that defines a angular dimension entity.
 */
struct LC_DimAngularVars
{
    explicit LC_DimAngularVars(const double _dimscale,
                               const double _dimexo,
                               const double _dimexe,
                               const double _dimtxt,
                               const double _dimgap,
                               const double _arrowSize);
    explicit LC_DimAngularVars(const LC_DimAngularVars& av);

    double scale(void) const {
        return dimscale;
    }
    double exo(void) const {
        return dimexo;
    }
    double exe(void) const {
        return dimexe;
    }
    double txt(void) const {
        return dimtxt;
    }
    double gap(void) const {
        return dimgap;
    }
    double arrow(void) const {
        return arrowSize;
    }

private:
    double dimscale     {1.0};  ///< general scale (DIMSCALE)
    double dimexo       {0.0};  ///< distance from entities (DIMEXO)
    double dimexe       {0.0};  ///< extension line extension (DIMEXE)
    double dimtxt       {0.0};  ///< text height (DIMTXT)
    double dimgap       {0.0};  ///< text distance to line (DIMGAP)
    double arrowSize    {0.0};  ///< arrow length
};

std::ostream& operator << (std::ostream& os, const LC_DimAngularVars& dd);

/**
 * Class for angular dimension entities.
 *
 * @author Andrew Mustun
 */
class RS_DimAngular : public RS_Dimension
{
    friend std::ostream& operator << (std::ostream& os, const RS_DimAngular& d);

public:
    RS_DimAngular(RS_EntityContainer* parent,
                  const RS_DimensionData& d,
                  const RS_DimAngularData& ed);

    RS_Entity* clone() const override;

    /** @return RS2::EntityDimAngular */
    RS2::EntityType rtti() const override {
        return RS2::EntityDimAngular;
    }

    /**
     * @return Copy of data that defines the angular dimension.
     * @see getData()
     */
    RS_DimAngularData getEData() const {
        return edata;
    }

    QString getMeasuredLabel() override;
    RS_Vector getCenter() const override;

    void updateDim(bool autoText = false) override;

    RS_Vector getDefinitionPoint1() {
        return edata.definitionPoint1;
    }
    RS_Vector getDefinitionPoint2() {
        return edata.definitionPoint2;
    }
    RS_Vector getDefinitionPoint3() {
        return edata.definitionPoint3;
    }
    RS_Vector getDefinitionPoint4() {
        return edata.definitionPoint4;
    }

    void update() override;
    void move(const RS_Vector& offset) override;
    void rotate(const RS_Vector& center, const double& angle) override;
    void rotate(const RS_Vector& center, const RS_Vector& angleVector) override;
    void scale(const RS_Vector& center, const RS_Vector& factor) override;
    void mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) override;

protected:
    /** Extended data. */
    RS_DimAngularData   edata;

private:
    void calcDimension(void);
    void fixDimension(void);
    void extensionLine(const RS_ConstructionLine& dimLine,
                       const RS_Vector& dimPoint,
                       const RS_Vector& dirStart,
                       const RS_Vector& dirEnd,
                       const LC_DimAngularVars& av,
                       const RS_Pen& pen);
    void arrow(const RS_Vector& point,
               const double angle,
               const double direction,
               const bool outsideArrows,
               const LC_DimAngularVars& av,
               const RS_Pen& pen);

    RS_Vector   dimDir1s;
    RS_Vector   dimDir1e;
    RS_Vector   dimDir2s;
    RS_Vector   dimDir2e;
    RS_Vector   dimDirRad;
    RS_ConstructionLine dimLine1;
    RS_ConstructionLine dimLine2;
    double      dimRadius {0.0};
    double      dimAngleL1 {0.0};
    double      dimAngleL2 {0.0};
    double      dimAngle {0.0};     ///< angle to dimension in rad
    RS_Vector   dimCenter;          ///< intersection point of the dimension lines
};

#endif
