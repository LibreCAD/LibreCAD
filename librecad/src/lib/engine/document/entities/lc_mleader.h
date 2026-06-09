/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2025 LibreCAD.org
** Copyright (C) 2025 sand1024
** Copyright (C) 2026 LibreCAD contributors
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file gpl-2.0.txt included in the
** packaging of this file.
**
**********************************************************************/

#ifndef LC_MLEADER_H
#define LC_MLEADER_H

#include <vector>

#include "rs_atomicentity.h"

/**
 * One leader-line within an LC_MLeader root: an ordered point list that
 * the renderer connects with straight or spline segments per leader type.
 */
struct LC_MLeaderLine {
  std::vector<RS_Vector> points;
  int leaderLineIndex = 0;
};

/**
 * One leader root attaching to the content (text or block) and emanating
 * one or more leader lines.
 */
struct LC_MLeaderRoot {
  RS_Vector connectionPoint;
  RS_Vector direction;
  double landingDistance = 0.0;
  std::vector<LC_MLeaderLine> leaderLines;
  int attachmentDirection = 0; // 0=horizontal, 1=vertical
};

/**
 * Holds the data that defines an MLEADER entity.  Mirrors
 * DRW_MLeader/DRW_MLeaderAnnotContext at the LibreCAD level.  Style
 * resolution (against LC_MLeaderStyleList) happens at render time —
 * Phase 7; until then the resolved-fields below are taken directly from
 * the entity-level overrides.
 */
struct LC_MLeaderData {
  LC_MLeaderData() = default;

  /* AnnotContext geometry. */
  std::vector<LC_MLeaderRoot> roots;
  RS_Vector contentBasePoint;
  RS_Vector basePoint;

  /* Content branch: either text label or block reference. */
  bool hasTextContents = false;
  bool hasBlockContents = false;
  QString textLabel;     /*!< MText body when hasTextContents */
  QString textStyleName; /*!< resolved at parse time if available */
  RS_Vector textLocation;
  double textHeight = 0.0;
  double textRotation = 0.0;
  double boundaryWidth = 0.0;
  double boundaryHeight = 0.0;
  int textColor = 0;
  QString blockName; /*!< when hasBlockContents */
  RS_Vector blockLocation;
  RS_Vector blockScale{1, 1, 1};
  double blockRotation = 0.0;

  /* Entity-level fields (also serve as effective values until style
   * resolution is wired). */
  QString styleName;  /*!< MLEADERSTYLE referenced */
  int leaderType = 1; /*!< 0=invisible, 1=line, 2=spline */
  int leaderColor = 0;
  double landingDistance = 0.0;
  double arrowSize = 0.0;
  bool landingEnabled = true;
  bool doglegEnabled = true;
  int contentType = 2; /*!< 0=None,1=Block,2=MText,3=Tolerance */
  double scaleFactor = 1.0;
};

/**
 * MLEADER entity (AcDbMLeader).  Modern AutoCAD callout system: one or
 * more leader roots emanating leader lines that point at text or block
 * content.  See ODA spec §20.4.48 / §20.4.86 for the underlying format.
 */
class LC_MLeader : public RS_AtomicEntity {
public:
    LC_MLeader();
    LC_MLeader(RS_EntityContainer *parent, LC_MLeaderData d);

    RS_Entity *clone() const override;

    /** @return RS2::EntityMLeader */
    RS2::EntityType rtti() const override { return RS2::EntityMLeader; }

    const LC_MLeaderData &getData() const { return data; }
    const std::vector<LC_MLeaderRoot> &getRoots() const { return data.roots; }
    QString getStyleName() const { return data.styleName; }

    void calculateBorders() override;
    void draw(RS_Painter *painter) override;

    RS_Vector getNearestEndpoint(const RS_Vector &coord,
                                 double *dist = nullptr) const override;
    RS_Vector
    getNearestPointOnEntity(const RS_Vector &coord, bool onEntity = true,
                            double *dist = nullptr,
                            RS_Entity **entity = nullptr) const override;
    RS_Vector getNearestCenter(const RS_Vector &coord,
                               double *dist = nullptr) const override;
    RS_Vector getNearestMiddle(const RS_Vector &coord, double *dist = nullptr,
                               int middlePoints = 1) const override;
    RS_Vector getNearestDist(double distance, const RS_Vector &coord,
                             double *dist = nullptr) const override;
    double getDistanceToPoint(const RS_Vector &coord,
                              RS_Entity **entity = nullptr,
                              RS2::ResolveLevel level = RS2::ResolveNone,
                              double solidDist = RS_MAXDOUBLE) const override;

    void move(const RS_Vector &offset) override;
    void rotate(const RS_Vector &center, double angle) override;
    void rotate(const RS_Vector &center, const RS_Vector &angleVector) override;
    void scale(const RS_Vector &center, const RS_Vector &factor) override;
    void mirror(const RS_Vector &axisPoint1,
                const RS_Vector &axisPoint2) override;
    RS_Entity &shear([[maybe_unused]] double k) override { return *this; }

  protected:
    LC_MLeaderData data;
};

#endif
