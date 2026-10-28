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

#include "lc_hatchdensity.h"
#include "lc_secondmoment.h"
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
    RS_HatchData(const bool solid, const double scale, const double angle, QString pattern)
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
        HATCH_AREA_TOO_BIG,    ///< Excessive area ratio (legacy; no longer produced)
        HATCH_TOO_DENSE        ///< Pattern pieces would exceed the piece budget
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
    RS_HatchData getData() const { return m_data; }

    /**
     * Validates and optimizes boundary loops into subcontainers.
     * @return true on success.
     */
    bool validate();

    /**
     * @return Number of optimized loops (subcontainers).
     */
    int countLoops() const;

    /**
     * @return Total number of boundary containers (roots + all nested
     * children). This counts every loop in the hierarchy, whereas countLoops()
     * counts only roots.
     */
    int countAllLoops() const;

    /** @return true if solid hatch (either flag set or pattern name is "SOLID"). */
    bool isSolid() const { return m_data.solid || m_data.pattern.compare("SOLID", Qt::CaseInsensitive) == 0; }
    // validate() turns the loops of a pattern hatch by its angle: these need the loops again
    void setSolid(const bool solid) { m_data.solid = solid; m_needOptimization = true; }

    QString getPattern() const { return m_data.pattern; }
    void setPattern(const QString& patternName) { m_data.pattern = patternName; m_needOptimization = true; }

    double getScale() const { return m_data.scale; }
    void setScale(const double scale) { m_data.scale = scale; }

    double getAngle() const { return m_data.angle; }
    /// Stores the angle normalised to [0, 2 pi), matching rotate() and
    /// mirror() so a negative input and 2 pi + x behave the same as the
    /// transform that would produce it.
    void setAngle(double angle);

    /// The per-hatch ε cached by the last successful validate(). Zero
    /// before validate() has run or after a failed validate().
    double getTolerance() const { return m_tolerance; }

    /**
     * @return Total enclosed area (0th moment) of loops.
     */
    double getTotalArea() const;

    /**
     * @return Centroid of the hatch region in WCS, (cx, cy) = (∬x dA/A, ∬y dA/A).
     * Returns an invalid RS_Vector if area is zero or update() has not run.
     */
    RS_Vector getCentroid() const;

    /**
     * @brief getMomentOfInertia - the three second central moments of area
     * for the hatch region in WCS axes, about the centroid:
     *   ixx = ∬ (x - cx)² dA,   iyy = ∬ (y - cy)² dA,   ixy = ∬ (x-cx)(y-cy) dA
     *
     * The full inertia tensor about the centroid is:
     *   I = | iyy  -ixy |
     *       | -ixy  ixx |
     *
     * Must be called after update() has run.
     */
    LC_SecondMoment getMomentOfInertia() const;

    void calculateBorders() override;
    void update() override;

    /**
     * @return Last update error code.
     */
    RS_HatchError getUpdateError() const { return m_updateError; }

    /**
     * @return Diagnostic density report from the last pattern update attempt.
     * Zero-initialised at the start of update(); fields are filled by
     * updatePatternHatch(). Readers used by the density tint, dialog,
     * Quick Info and property sheet.
     */
    const LC_HatchDensityReport& getDensityReport() const { return m_densityReport; }

    /**
     * Toggles visibility of boundary contours in subcontainers.
     * @param visible true to show boundaries.
     */
    void activateContour(bool visible) const;

    /**
     * @return Subcontainer for the boundary loop at index (0-based).
     */
    RS_EntityContainer* getBoundaryContainer(int loopIndex) const;

    void draw(RS_Painter* painter) override;
    void move(const RS_Vector& offset) override;
    void rotate(const RS_Vector& center, double angle) override;
    void rotate(const RS_Vector& center, const RS_Vector& angleVector) override;
    void scale(const RS_Vector& center, const RS_Vector& factor) override;
    void mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) override;
    void stretch(const RS_Vector& firstCorner, const RS_Vector& secondCorner,
                 const RS_Vector& offset) override;

    friend std::ostream& operator<<(std::ostream& os, const RS_Hatch& hatchEntity);

protected:
    RS_HatchData m_data;
    /**
     * Distance from `coord` (in WCS) to the hatch region. For a pattern hatch
     * the cached loops live in the frame rotated by -m_loopAngle about
     * m_pivot (see validate()); the query is mapped forward with the stored
     * pair so picking agrees with what is drawn, at every angle. Returns 0 if
     * the query is inside and RS_MAXDOUBLE otherwise. The out-parameter
     * `entity` is set to `this` on a hit.
     */
    double doGetDistanceToPoint(const RS_Vector& coord, RS_Entity** entity, RS2::ResolveLevel level, double solidDist) const override;
private:
    void drawPatternLines(RS_Painter* painter) const;
    void drawSolidFill(RS_Painter* painter) const;
    /// Fills the region with the resolved pen colour and the given alpha
    /// channel; shared by drawSolidFill (alpha = 1) and drawDensityTint
    /// (alpha derived from the density report), so paper matches screen.
    void drawFill(RS_Painter* painter, double alpha) const;
    /// Draws the HATCH_TOO_DENSE tint. Called by draw() when the pattern
    /// generation ran out of budget and the pattern pieces were discarded.
    void drawDensityTint(RS_Painter* painter) const;
    void updatePatternHatch(RS_Layer* layer, const RS_Pen& pen);
    void updateSolidHatch(RS_Layer* layer, const RS_Pen& pen) const;
    mutable double m_area = RS_MAXDOUBLE;
    /// WCS centroid of the hatch region, mapped from the loop frame through
    /// (m_pivot, m_loopAngle); set by update() after a successful validate().
    mutable RS_Vector m_centroid{false};
    /// Central second-moment tensor about m_centroid, in WCS axes.
    mutable LC_SecondMoment m_centralMoment;
    mutable bool m_secondMomentValid = false;
    /// Diagnostic report of the last pattern update attempt; read-only to
    /// callers through getDensityReport(). update() resets it to a
    /// default-constructed value before validate() runs.
    LC_HatchDensityReport m_densityReport{};
    RS_HatchError m_updateError = HATCH_UNDEFINED;
    bool m_updateRunning = false;
    bool m_needOptimization = true;
    bool m_updated = false;
    mutable std::shared_ptr<std::vector<LC_LoopUtils::LC_Loops>> m_orderedLoops;
    // The frame of m_orderedLoops: validate() rotated the loops by -m_loopAngle about m_pivot.
    RS_Vector m_pivot{0.0, 0.0};  ///< WCS centre of the combined bbox of the loop containers.
    double m_loopAngle = 0.0;     ///< isSolid() ? 0 : m_data.angle at the last successful validate().
    double m_tolerance = 0.0;     ///< Per-hatch ε cached by validate().

    /**
     * The hatch's lattice origin in WCS. Every root uses one lattice through
     * this point; the pattern's minimum corner is placed on every lattice
     * point anchor + (i*W, k*H). Fixed at (0, 0) on 2.2.2 (D30); H18 makes
     * this read a stored, transform-tracked origin.
     */
    RS_Vector latticeOrigin() const { return RS_Vector{0.0, 0.0}; }

    // Internal: Vector of boundary subcontainers (one per loop, recursive)
    mutable std::vector<std::shared_ptr<RS_EntityContainer>> m_boundaryContainers;
};

#endif
