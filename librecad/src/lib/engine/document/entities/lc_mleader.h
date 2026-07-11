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

struct RS_MTextData;
struct RS_InsertData;

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

  /* DWG-only references preserved for native MLEADER re-export. */
  unsigned int dwgStyleHandle = 0;
  unsigned int dwgLeaderLineTypeHandle = 0;
  unsigned int dwgArrowHeadHandle = 0;
  unsigned int dwgTextStyleHandle = 0;
  unsigned int dwgBlockHandle = 0;
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

    const LC_MLeaderData &getData() const { return m_data; }
    const std::vector<LC_MLeaderRoot> &getRoots() const { return m_data.roots; }
    QString getStyleName() const { return m_data.styleName; }

    void calculateBorders() override;
    void draw(RS_Painter *painter) override;


    void move(const RS_Vector &offset) override;
    void rotate(const RS_Vector &center, double angle) override;
    void rotate(const RS_Vector &center, const RS_Vector &angleVector) override;
    void scale(const RS_Vector &center, const RS_Vector &factor) override;
    void mirror(const RS_Vector &axisPoint1,
                const RS_Vector &axisPoint2) override;
    RS_Entity &shear([[maybe_unused]] double k) override { return *this; }

    /**
     * Build the RS_MTextData for the text annotation from the leader data.
     * @return false when there is no renderable text content. Exposed (testable)
     * because it is the error-prone data mapping; the painting itself is GUI.
     */
    bool textContentData(RS_MTextData &out) const;

    /**
     * Build the RS_InsertData for block content (the block symbol the leader
     * points at).  @return false when there is no renderable block content.
     */
    bool blockContentData(RS_InsertData &out) const;

  protected:
    LC_MLeaderData m_data;

    RS_Vector doGetNearestPointOnEntity(const RS_Vector& coord, bool onEntity, double* dist,
                                        RS_Entity** entity) const override;
    bool doIsPointOnEntity(const RS_Vector& coord, double tolerance) const override;
    double doGetDistanceToPoint(const RS_Vector& coord, RS_Entity** entity, RS2::ResolveLevel level,
                                double solidDist) const override;
    RS_Vector doGetNearestEndpoint(const RS_Vector& coord, double* dist, RS_Entity** entity) const override;
    RS_Vector doGetNearestRef(const RS_Vector& coord, double* dist) const override;
    RS_Vector doGetNearestMiddle(const RS_Vector& coord, double* dist, int middlePoints) const override;
    RS_Vector doGetNearestSelectedRef(const RS_Vector& coord, double* dist) const override;
    RS_Vector doGetNearestDist(double distance, const RS_Vector& coord, double* dist) const override;
    RS_Vector doGetNearestCenter(const RS_Vector& coord, double* dist, RS_Entity** centerEntity) const override;

    /** Render the MText annotation transiently (see definition). */
    void drawTextContent(RS_Painter *painter);
    /** Render the block content transiently (see definition). */
    void drawBlockContent(RS_Painter *painter);
};

#endif
