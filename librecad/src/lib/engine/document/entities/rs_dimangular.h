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

#include "rs_constructionline.h"
#include "rs_dimension.h"

/**
 * Holds the data that defines a angular dimension entity.
 */
// fixme - sand - no copy assignment operator!
struct RS_DimAngularData
{
    RS_DimAngularData();
    RS_DimAngularData(const RS_DimAngularData& ed);

    /**
     * Constructor with initialisation.
     *
     * @param definitionPoint1 Definition point of the angular dimension.
     * @param definitionPoint2
     * @param definitionPoint3
     * @param definitionPoint4
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
// fixme - sand - no copy assignment operator!
struct LC_DimAngularVars
{
    explicit LC_DimAngularVars(double dimscale, double dimexo, double dimexe, double dimtxt, double dimgap, double arrowSize, double tickSize);

    explicit LC_DimAngularVars(const LC_DimAngularVars& av);

    double scale() const {
        return dimscale;
    }
    double exo() const {
        return dimexo;
    }
    double exe() const {
        return dimexe;
    }
    double txt() const {
        return dimtxt;
    }
    double gap() const {
        return dimgap;
    }
    double arrow() const {
        return arrowSize;
    }

    double tickSize() const {
        return m_tickSize;
    }

private:
    double dimscale     {1.0};  ///< general scale (DIMSCALE)
    double dimexo       {0.0};  ///< distance from entities (DIMEXO)
    double dimexe       {0.0};  ///< extension line extension (DIMEXE)
    double dimtxt       {0.0};  ///< text height (DIMTXT)
    double dimgap       {0.0};  ///< text distance to line (DIMGAP)
    double arrowSize    {0.0};  ///< arrow length
    double m_tickSize     {0.0};
};

std::ostream& operator << (std::ostream& os, const LC_DimAngularVars& dd);

/**
 * Class for angular dimension entities.
 *
 * @author Andrew Mustun
 */
// fixme - sand - no copy assignment operator!
class RS_DimAngular : public RS_Dimension
{
    friend std::ostream& operator << (std::ostream& os, const RS_DimAngular& d);

public:
    RS_DimAngular(RS_EntityContainer* parent,
                  const RS_DimensionData& d,
                  const RS_DimAngularData& ed);
        RS_DimAngular(const RS_DimAngular& other);

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
        return m_dimAngularData;
    }

    QString getMeasuredLabel() override;
    RS_Vector getCenter() const override;


    RS_Vector getDefinitionPoint1() const {
        return m_dimAngularData.definitionPoint1;
    }

    void setDefinitionPoint1(const RS_Vector& p) {
        m_dimAngularData.definitionPoint1 = p;
    }

    RS_Vector getDefinitionPoint2() const {
        return m_dimAngularData.definitionPoint2;
    }

    void setDefinitionPoint2(const RS_Vector& p) {
        m_dimAngularData.definitionPoint2 = p;
    }

    RS_Vector getDefinitionPoint3() const {
        return m_dimAngularData.definitionPoint3;
    }

    void setDefinitionPoint3(const RS_Vector& p) {
        m_dimAngularData.definitionPoint3 = p;
    }

    RS_Vector getDefinitionPoint4() const {
        return m_dimAngularData.definitionPoint4;
    }

    void setDefinitionPoint4(const RS_Vector& p) {
        m_dimAngularData.definitionPoint4 = p;
    }

    void update() override;
    void move(const RS_Vector& offset) override;
    void rotate(const RS_Vector& center, double angle) override;
    void rotate(const RS_Vector& center, const RS_Vector& angleVector) override;
    void scale(const RS_Vector& center, const RS_Vector& factor) override;
    void mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) override;

protected:
    /** Extended data. */
    RS_DimAngularData   m_dimAngularData;
    void doUpdateDim() override;

private:
    void calcDimension();
    void fixDimension();
    void extensionLine(const RS_ConstructionLine& dimLine,
                       const RS_Vector& dimPoint,
                       const RS_Vector& dirStart,
                       const RS_Vector& dirEnd,
                       const LC_DimAngularVars& av);
    void arrow(const RS_Vector& point, double angle, double direction, bool outsideArrows,
               const LC_DimAngularVars& av);

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
