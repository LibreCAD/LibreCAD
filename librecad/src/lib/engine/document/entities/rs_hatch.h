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

#include <memory>
#include <vector>
#include <QString>

#include "rs_entitycontainer.h"

class QPainterPath;
class RS_Pattern;

namespace LC_LoopUtils {
class LC_Loops;
}

/**
 * Holds the data that defines a hatch entity.
 */
struct RS_HatchData {
    /**
     * Default constructor. Leaves the data object uninitialized.
     */
    RS_HatchData() = default;

    /**
     * @param solid true: solid fill, false: pattern.
     * @param scale Pattern scale or spacing.
     * @param angle Pattern rotation angle.
     * @param pattern Pattern name (ignored for solid).
     */
    RS_HatchData(bool solid, double scale, double angle, QString pattern)
        : solid{solid}, scale{scale}, angle{angle}, pattern{std::move(pattern)} {}

    bool solid = false;
    double scale = 1.0;
    double angle = 0.0;
    QString pattern;
};

std::ostream& operator<<(std::ostream& os, const RS_HatchData& td);

/**
 * Class for a hatch entity.
 *
 * @author Andrew Mustun
 */
class RS_Hatch : public RS_EntityContainer {
public:
    /**
     * Error codes for hatch update operations.
     */
    enum RS_HatchError {
        HATCH_UNDEFINED = -1,  ///< Uninitialized error state
        HATCH_OK,              ///< Successful update
        HATCH_INVALID_CONTOUR, ///< Failed loop optimization
        HATCH_PATTERN_NOT_FOUND, ///< Pattern not in library
        HATCH_TOO_SMALL,       ///< Contour/pattern too tiny
        HATCH_AREA_TOO_BIG     ///< Excessive area ratio
    };

    RS_Hatch() = default;
    ~RS_Hatch() override;

    RS_Hatch(RS_EntityContainer* parent, const RS_HatchData& d);

    RS_Entity* clone() const override;

    /** @return RS2::EntityHatch */
    RS2::EntityType rtti() const override { return RS2::EntityHatch; }

    /**
     * @return true if this is a hatch with lines (pattern), false if solid fill.
     */
    bool isContainer() const override { return !isSolid(); }

    /** @return Copy of data that defines the hatch. */
    RS_HatchData getData() const { return data; }

    /**
     * Validates and optimizes boundary loops into subcontainers.
     * @return true on success.
     */
    bool validate();

    /**
     * @return Number of optimized loops (subcontainers).
     */
    int countLoops() const;

    /** @return true if solid hatch. */
    bool isSolid() const { return data.solid; }
    void setSolid(bool solid) { data.solid = solid; }

    QString getPattern() const { return data.pattern; }
    void setPattern(const QString& patternName) { data.pattern = patternName; }

    double getScale() const { return data.scale; }
    void setScale(double scale) { data.scale = scale; }

    double getAngle() const { return data.angle; }
    void setAngle(double angle) { data.angle = angle; }

    /**
     * @return Total enclosed area of loops.
     */
    double getTotalArea() const;

    void calculateBorders() override;
    void update() override;

    /**
     * @return Last update error code.
     */
    RS_HatchError getUpdateError() const { return updateError; }

    /**
     * Toggles visibility of boundary contours in subcontainers.
     * @param on true to show boundaries.
     */
    void activateContour(bool on);

    /**
     * @return Subcontainer for the boundary loop at index (0-based).
     */
    RS_EntityContainer* getBoundaryContainer(int loopIndex) const;

    void draw(RS_Painter* painter) override;

    double getDistanceToPoint(const RS_Vector& coord, RS_Entity** entity = nullptr,
                              RS2::ResolveLevel level = RS2::ResolveNone,
                              double solidDist = RS_MAXDOUBLE) const override;

    void move(const RS_Vector& offset) override;
    void rotate(const RS_Vector& center, double angle) override;
    void rotate(const RS_Vector& center, const RS_Vector& angleVector) override;
    void scale(const RS_Vector& center, const RS_Vector& factor) override;
    void mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) override;
    void stretch(const RS_Vector& firstCorner, const RS_Vector& secondCorner,
                 const RS_Vector& offset) override;

    friend std::ostream& operator<<(std::ostream& os, const RS_Hatch& p);

protected:
    RS_HatchData data{};

private:
    void debugOutPath(const QPainterPath& tmpPath) const;
    void drawPatternLines(RS_Painter* painter) const;
    void drawSolidFill(RS_Painter* painter);
    void updatePatternHatch(RS_Layer* layer, const RS_Pen& pen);
    void updateSolidHatch(RS_Layer* layer, const RS_Pen& pen);
    void prepareUpdate();

    mutable double m_area = RS_MAXDOUBLE;
    RS_HatchError updateError = HATCH_UNDEFINED;
    bool updateRunning = false;
    bool m_needOptimization = true;
    bool m_updated = false;
    mutable std::shared_ptr<std::vector<LC_LoopUtils::LC_Loops>> m_orderedLoops;
    mutable std::shared_ptr<std::vector<QPainterPath>> m_solidPath;

    // Internal: Vector of boundary subcontainers (one per loop)
    mutable std::vector<std::shared_ptr<RS_EntityContainer>> m_boundaryContainers;
};

#endif
