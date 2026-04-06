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

#include "rs_snapper.h"

#include <QMouseEvent>

#include "lc_actioncontext.h"
#include "lc_containertraverser.h"
#include "lc_crosshair.h"
#include "lc_cursoroverlayinfo.h"
#include "lc_defaults.h"
#include "lc_graphicviewport.h"
#include "lc_linemath.h"
#include "lc_overlayentitiescontainer.h"
#include "lc_visual_snap_manager.h"
#include "rs_debug.h"
#include "rs_graphic.h"
#include "rs_graphicview.h"
#include "rs_grid.h"
#include "rs_line.h"
#include "rs_pen.h"
#include "rs_settings.h"
#include "rs_vector.h"

namespace {
    // whether a floating point is positive by tolerance
    bool isPositive(const double x) {
        return x > RS_TOLERANCE;
    }

    // A size vector is valid with a positive size
    bool isSizeValid(const RS_Vector& sizeVector) {
        return isPositive(sizeVector.x) || isPositive(sizeVector.y);
    }

    // The valid size magnitude
    double getValidSize(const RS_Vector& sizeVector) {
        return std::hypot(std::max(sizeVector.x, RS_TOLERANCE), std::max(sizeVector.y, RS_TOLERANCE));
    }

    constexpr int DEFAULT_CATCH_ENTITY_RANGE_PX = 32;
}

/**
  * Disable all snapping.
  *
  * This effectively puts the object into free snap mode.
  *
  * @returns A reference to itself.
  */
const RS_SnapMode& RS_SnapMode::clear() {
    *this = RS_SnapMode{};

    return *this;
}

bool RS_SnapMode::operator ==(const RS_SnapMode& rhs) const {
    return snapIntersection == rhs.snapIntersection && snapOnEntity == rhs.snapOnEntity && snapCenter == rhs.snapCenter && snapDistance ==
        rhs.snapDistance && snapMiddle == rhs.snapMiddle && snapEndpoint == rhs.snapEndpoint && snapGrid == rhs.snapGrid && snapFree == rhs.
        snapFree && restriction == rhs.restriction && snapAngle == rhs.snapAngle;
}

bool RS_SnapMode::operator !=(const RS_SnapMode& rhs) const {
    return !this->operator ==(rhs);
}

/**
  * snap mode to a flag integer
  */
uint RS_SnapMode::toInt(const RS_SnapMode& s) {
    uint ret{0};

    if (s.snapIntersection) {
        ret |= RS_SnapMode::SnapIntersection;
    }
    if (s.snapOnEntity) {
        ret |= RS_SnapMode::SnapOnEntity;
    }
    if (s.snapCenter) {
        ret |= RS_SnapMode::SnapCenter;
    }
    if (s.snapDistance) {
        ret |= RS_SnapMode::SnapDistance;
    }
    if (s.snapMiddle) {
        ret |= RS_SnapMode::SnapMiddle;
    }
    if (s.snapEndpoint) {
        ret |= RS_SnapMode::SnapEndpoint;
    }
    if (s.snapGrid) {
        ret |= RS_SnapMode::SnapGrid;
    }
    if (s.snapFree) {
        ret |= RS_SnapMode::SnapFree;
    }
    if (s.snapAngle) {
        ret |= RS_SnapMode::SnapAngle;
    }

    switch (s.restriction) {
        case RS2::RestrictHorizontal:
            ret |= RS_SnapMode::RestrictHorizontal;
            break;
        case RS2::RestrictVertical:
            ret |= RS_SnapMode::RestrictVertical;
            break;
        case RS2::RestrictOrthogonal:
            ret |= RS_SnapMode::RestrictOrthogonal;
            break;
        default:
            break;
    }

    if (s.snapVisual) {
        ret |= RS_SnapMode::SnapVisual;
    }
    if (s.snapVisualSurvive) {
        ret |= RS_SnapMode::SnapVisualSurvive;
    }

    return ret;
}

/**
  * integer flag to snapMode
  */
RS_SnapMode RS_SnapMode::fromInt(const unsigned int ret) {
    RS_SnapMode s;

    if (RS_SnapMode::SnapIntersection & ret) {
        s.snapIntersection = true;
    }
    if (RS_SnapMode::SnapOnEntity & ret) {
        s.snapOnEntity = true;
    }
    if (RS_SnapMode::SnapCenter & ret) {
        s.snapCenter = true;
    }
    if (RS_SnapMode::SnapDistance & ret) {
        s.snapDistance = true;
    }
    if (RS_SnapMode::SnapMiddle & ret) {
        s.snapMiddle = true;
    }
    if (RS_SnapMode::SnapEndpoint & ret) {
        s.snapEndpoint = true;
    }
    if (RS_SnapMode::SnapGrid & ret) {
        s.snapGrid = true;
    }
    if (RS_SnapMode::SnapFree & ret) {
        s.snapFree = true;
    }
    if (RS_SnapMode::SnapAngle & ret) {
        s.snapAngle = true;
    }

    switch (RS_SnapMode::RestrictOrthogonal & ret) {
        case RS_SnapMode::RestrictHorizontal:
            s.restriction = RS2::RestrictHorizontal;
            break;
        case RS_SnapMode::RestrictVertical:
            s.restriction = RS2::RestrictVertical;
            break;
        case RS_SnapMode::RestrictOrthogonal:
            s.restriction = RS2::RestrictOrthogonal;
            break;
        default:
            s.restriction = RS2::RestrictNothing;
            break;
    }

    if (RS_SnapMode::SnapVisual & ret) {
        s.snapVisual = true;
    }

    if (RS_SnapMode::SnapVisualSurvive & ret) {
        s.snapVisualSurvive = true;
    }
    return s;
}

/**
  * Methods and structs for class RS_Snapper
  */
struct RS_Snapper::Indicator {
    bool drawLines = false;
    int lines_Type = 0;
    RS_Pen lines_Pen;

    bool drawShape = false;
    int shape_Type = 0;
    RS_Pen shape_Pen;

    int pointType = LC_DEFAULTS_PDMode;
    int pointSize = LC_DEFAULTS_PDSize;
};

namespace {
    constexpr double DEFAULT_SNAP_ANGLE_STEP = RS_Math::deg2rad(15.0);
}

/**
 * Constructor.
 */
RS_Snapper::RS_Snapper(LC_ActionContext* actionContext, QObject* parent)
    : QObject(parent), m_document(actionContext->getDocument()), m_graphicView(actionContext->getGraphicView()),
      m_actionContext(actionContext), m_infoCursorOverlayData{std::make_unique<LC_InfoCursorData>()},
      m_impData{std::make_unique<ImpData>()}, m_snapIndicator{std::make_unique<Indicator>()},
      m_visualSnapManager{std::make_unique<LC_VisualSnapManager>(this)}, m_snapToAngleStep{DEFAULT_SNAP_ANGLE_STEP} {
    Q_ASSERT(m_document != nullptr);
    Q_ASSERT(m_graphicView != nullptr);
    m_viewport = m_graphicView->getViewPort();
    m_visualSnapManager->setViewport(m_viewport);
    m_formatter = m_viewport->getFormatter();
    m_infoCursorOverlayPrefs = m_graphicView->getInfoCursorOverlayPreferences();
}

RS_Snapper::~RS_Snapper() = default;

/**
 * Initialize (called by all constructors)
 */
void RS_Snapper::init() {
    m_snapMode = m_graphicView->getDefaultSnapMode();
    m_keyEntity = nullptr;
    m_impData->snapSpot = RS_Vector{false};
    m_impData->snapCoord = RS_Vector{false};
    m_snapDistance = 1.0;
    initSettings();
}

void RS_Snapper::initSettings() {
    initFromSettings();

    RS_Graphic* graphic = m_graphicView->getGraphic();
    if (graphic != nullptr) {
        initFromGraphic(graphic);
    }
}

void RS_Snapper::initFromSettings() {
    LC_GROUP("Appearance");
    {
        const int snapIndicatorLineWidth = static_cast<RS2::LineType>(LC_GET_INT("indicator_lines_line_width", 1));
        m_snapIndicator->drawLines = LC_GET_BOOL("indicator_lines_state", true);
        if (m_snapIndicator->drawLines) {
            m_snapIndicator->lines_Type = LC_GET_INT("indicator_lines_type", 0);
            const auto snapIndicatorLineType = static_cast<RS2::LineType>(LC_GET_INT("indicator_lines_line_type", RS2::DashLine));
            const QString snapColorLines = LC_GET_ONE_STR("Colors", "snap_indicator_lines", RS_Settings::SNAP_INDICATOR_LINES);
            m_snapIndicator->lines_Pen = RS_Pen(RS_Color(snapColorLines), RS2::Width00, snapIndicatorLineType);
            m_snapIndicator->lines_Pen.setScreenWidth(snapIndicatorLineWidth);
        }
        else {
            m_snapIndicator->lines_Type = LC_Crosshair::NoLines;
        }

        m_snapIndicator->drawShape = LC_GET_BOOL("indicator_shape_state", true);
        if (m_snapIndicator->drawShape) {
            m_snapIndicator->shape_Type = LC_GET_INT("indicator_shape_type", 0);
            const QString snapColor = LC_GET_ONE_STR("Colors", "snap_indicator", RS_Settings::SNAP_INDICATOR);
            m_snapIndicator->shape_Pen = RS_Pen(RS_Color(snapColor), RS2::Width00, RS2::SolidLine);
            m_snapIndicator->shape_Pen.setScreenWidth(snapIndicatorLineWidth);
        }
        else {
            m_snapIndicator->shape_Type = LC_Crosshair::NoShape;
        }

        m_ignoreSnapToGridIfNoGrid = LC_GET_BOOL("SnapGridIgnoreIfNoGrid", false);
    }
    LC_GROUP_END();

    LC_GROUP("Snap");
    {
        m_distanceBeforeSwitchToFreeSnap = LC_GET_INT("AdvSnapOnEntitySwitchToFreeDistance", 500) / 100.0;
        m_catchEntityGuiRange = LC_GET_INT("AdvSnapEntityCatchRange", DEFAULT_CATCH_ENTITY_RANGE_PX);
        m_minGridCellSnapFactor = LC_GET_INT("AdvSnapGridCellSnapFactor", 25) / 100.0;
        m_angleSnapSnapToGridLinesIfGrid = LC_GET_BOOL("AngleSnapToLinesIfGrid", true);
    }
    LC_GROUP_END();
}

void RS_Snapper::updateSnapAngleStep() {
    const int stepType = LC_GET_ONE_INT("Defaults", "AngleSnapStep", 3);
    double snapStepDegrees;
    switch (stepType) {
        case 0:
            snapStepDegrees = 1.0;
            break;
        case 1:
            snapStepDegrees = 3.0;
            break;
        case 2:
            snapStepDegrees = 5.0;
            break;
        case 3:
            snapStepDegrees = 10.0;
            break;
        case 4:
            snapStepDegrees = 15.0;
            break;
        case 5:
            snapStepDegrees = 18.0;
            break;
        case 6:
            snapStepDegrees = 22.5;
            break;
        case 7:
            snapStepDegrees = 30.0;
            break;
        case 8:
            snapStepDegrees = 45.0;
            break;
        case 9:
            snapStepDegrees = 90.0;
            break;
        default:
            snapStepDegrees = 15.0;
            break;
    }
    m_snapToAngleStep = RS_Math::deg2rad(snapStepDegrees);
}

void RS_Snapper::initFromGraphic(RS_Graphic* graphic) {
    if (graphic != nullptr) {
        m_snapIndicator->pointType = graphic->getVariableInt("$PDMODE", LC_DEFAULTS_PDMode);
        m_snapIndicator->pointSize = graphic->getVariableInt("$PDSIZE", LC_DEFAULTS_PDSize);
    }
}

bool RS_Snapper::isInVisualSnapStatus([[maybe_unused]] int status) {
    return false;
}

bool RS_Snapper::isClearVisualSnapByRMB() {
    return m_visualSnapManager->isClearVisualSnapByRMB();
}

void RS_Snapper::onVisualSnapEntityRegistered([[maybe_unused]] RS_Entity* point) {
}

bool RS_Snapper::isVisualSnapApplicable() {
    return m_snapMode.snapVisual;
}

void RS_Snapper::finish() {
    m_finished = true;
    deleteSnapper();
    deleteInfoCursor();
    m_visualSnapManager->clear();
    m_graphicView->hideRelativeInputWidget();
}

void RS_Snapper::setSnapMode(const RS_SnapMode& snapMode) {
    this->m_snapMode = snapMode;
    m_actionContext->requestSnapDistOptions(&m_snapDistance, snapMode.snapDistance);
    m_actionContext->requestSnapMiddleOptions(&m_middlePoints, snapMode.snapMiddle);
}

const RS_SnapMode* RS_Snapper::getSnapMode() const {
    return &m_snapMode;
}

RS_SnapMode* RS_Snapper::getSnapMode() {
    return &m_snapMode;
}

//get current mouse coordinates
RS_Vector RS_Snapper::snapFree(const QMouseEvent* e) const {
    if (e == nullptr) {
        RS_DEBUG->print(RS_Debug::D_WARNING, "RS_Snapper::snapFree: event is nullptr");
        return RS_Vector(false);
    }
    m_impData->snapSpot = toGraph(e);
    m_impData->snapCoord = m_impData->snapSpot;
    m_snapIndicator->drawLines = true;
    m_impData->entity = nullptr;
    m_impData->entityOther = nullptr;
    return m_impData->snapCoord;
}

void RS_Snapper::snapEndpoint(const RS_Vector& mouseCoord, double& ds2Min) const {
    if (m_snapMode.snapEndpoint) {
        RS_Entity* endpointEntity = nullptr;
        const RS_Vector snap = snapEndpoint(mouseCoord, &endpointEntity);
        const double ds2 = mouseCoord.squaredTo(snap);

        if (snap.valid && ds2 < ds2Min) {
            ds2Min = ds2;
            m_impData->snapSpot = snap;
            m_impData->snapType = RS2::SnapType::ENDPOINT;
            m_impData->entity = endpointEntity;
            m_impData->entityOther = nullptr;
        }
    }
}

void RS_Snapper::snapCenter(const RS_Vector& mouseCoord, double& ds2Min) const {
    if (m_snapMode.snapCenter) {
        RS_Entity* centerEntity = nullptr;
        const RS_Vector snap = snapCenter(mouseCoord, &centerEntity);
        const double ds2 = mouseCoord.squaredTo(snap);
        if (ds2 < ds2Min) {
            ds2Min = ds2;
            m_impData->snapSpot = snap;
            m_impData->snapType = RS2::SnapType::CENTER;
            m_impData->entity = centerEntity;
            m_impData->entityOther = nullptr;
        }
    }
}

void RS_Snapper::snapMiddle(const RS_Vector& mouseCoord, double& ds2Min) {
    if (m_snapMode.snapMiddle) {
        //this is still brutal force
        //todo: accept value from widget QG_SnapMiddleOptions

        m_actionContext->requestSnapMiddleOptions(&m_middlePoints, m_snapMode.snapMiddle);
        const RS_Vector snap = snapMiddle(mouseCoord);
        const double ds2 = mouseCoord.squaredTo(snap);
        if (ds2 < ds2Min) {
            ds2Min = ds2;
            m_impData->snapSpot = snap;
            m_impData->snapType = RS2::SnapType::MIDDLE;
            m_impData->entity = nullptr;
            m_impData->entityOther = nullptr;
        }
    }
}

void RS_Snapper::snapDistance(const RS_Vector mouseCoord, double& ds2Min) {
    if (m_snapMode.snapDistance) {
        //this is still brutal force
        //todo: accept value from widget QG_SnapDistOptions
        m_actionContext->requestSnapDistOptions(&m_snapDistance, m_snapMode.snapDistance);
        const RS_Vector snap = snapDist(mouseCoord);
        const double ds2 = mouseCoord.squaredTo(snap);
        if (ds2 < ds2Min) {
            ds2Min = ds2;
            m_impData->snapSpot = snap;
            m_impData->snapType = RS2::SnapType::DISTANCE;
            m_impData->entity = nullptr;
            m_impData->entityOther = nullptr;
        }
    }
}

void RS_Snapper::snapIntersection(const RS_Vector& mouseCoord, double& ds2Min) const {
    m_impData->entityOther = nullptr;
    if (m_snapMode.snapIntersection) {
        RS_Entity* entity1 = nullptr;
        RS_Entity* entity2 = nullptr;
        const RS_Vector snap = snapIntersection(mouseCoord, &entity1, &entity2);
        const double ds2 = mouseCoord.squaredTo(snap);
        if (ds2 < ds2Min) {
            ds2Min = ds2;
            m_impData->snapSpot = snap;
            m_impData->snapType = RS2::SnapType::INTERSECTION;
            m_impData->entity = entity1;
            m_impData->entityOther = entity2;
        }
    }
}

void RS_Snapper::snapOnEntity(const RS_Vector& mouseCoord, double& ds2Min) {
    if (m_snapMode.snapOnEntity && m_impData->snapSpot.distanceTo(mouseCoord) > m_distanceBeforeSwitchToFreeSnap) {
        RS_Entity* entity = nullptr;
        const RS_Vector snap = snapOnEntity(mouseCoord, &entity);
        const double ds2 = mouseCoord.squaredTo(snap);
        if (ds2 < ds2Min) {
            ds2Min = ds2;
            m_impData->snapSpot = snap;
            m_impData->snapType = RS2::SnapType::ENTITY;
            m_impData->entity = entity;
            m_impData->entityOther = nullptr;
        }
    }
}

void RS_Snapper::snapGrid(const RS_Vector& mouseCoord, double ds2Min) const {
    if (isSnapToGrid()) {
        const RS_Vector snap = snapGrid(mouseCoord);
        const double ds2 = mouseCoord.squaredTo(snap);
        if (ds2 < ds2Min) {
            //            ds2Min=ds2;
            m_impData->snapSpot = snap;
            m_impData->snapType = RS2::SnapType::GRID;
            m_impData->entity = nullptr;
            m_impData->entityOther = nullptr;
        }
    }
}

RS_Vector RS_Snapper::snapVisualRayOrLine(const RS_Vector& mouseCoord, RS_Entity* entity, double& dist) {
    RS_Vector snap = entity->getNearestPointOnEntity(mouseCoord, true, &dist);
    if (isSnapToGrid()) {
        const RS_Vector startPoint = entity->getStartpoint();
        const RS_Vector endPoint = entity->getEndpoint();
        snap = snapGrid(snap, startPoint, endPoint);
        m_impData->restrictedSnapType = RS2::SnapType::GRID;
    }
    return snap;
}

bool RS_Snapper::snapVisual(const RS_Vector& mouseCoord, RS_Entity** restrictingEntity, RS2::VisualSnapGuideEntityType& restrictingType) {
    bool visualSnapFound = false;

    // double range = getCatchDistance(getSnapRange(), m_catchEntityGuiRange);
    const double range = getSnapRange();
    m_visualSnapManager->setSnapRange(range);
    if (isVisualSnapApplicable() && hasVisualSnap()) {
        VisualSnapSolution* visualSnapSolution = m_visualSnapManager->solveVisualSnap(mouseCoord);
        visualSnapSolution->restrictedPoint.valid = false;

        if (visualSnapSolution != nullptr) {
            if (visualSnapSolution->hasFoundSnapPoint()) {
                // exact snap found, it's either intersection or vertex point
                const RS_Vector snap = visualSnapSolution->foundSnapPoint;
                m_impData->snapSpot = snap;
                m_impData->snapCoord = snap;
                m_impData->snapType = RS2::SnapType::VISUAL_SNAP;
                m_impData->visualSnapType = visualSnapSolution->foundSnapPointInfo;
                const RS2::SnapType originalSnapType = visualSnapSolution->restrictedOriginalSnapType;
                if (originalSnapType != RS2::SnapType::NO_SNAP) {
                    m_impData->restrictedSnapType = originalSnapType;
                }
                m_impData->entity = nullptr;
                m_impData->entityOther = nullptr;
                visualSnapFound = true;
            }
            else {
                if (!visualSnapSolution->guidingEntities.empty()) {
                    const int size = visualSnapSolution->guidingEntities.size();
                    // LC_ERR << "VSnap entities size: " << size;
                    EntityHolder holder;
                    if (size == 1) {
                        holder = visualSnapSolution->guidingEntities.front();
                    }
                    else {
                        // find closest entity
                        double minDist = RS_MAXDOUBLE;
                        for (const auto& h : visualSnapSolution->guidingEntities) {
                            auto e = h.entity;
                            double dist;
                            e->getNearestPointOnEntity(mouseCoord, true, &dist);
                            // LC_ERR << "VSNAP_nearest_dist: " << dist;
                            // LC_ERR << "holder.snapEntityType: " << h.snapEntityType;
                            if (dist < minDist) {
                                minDist = dist;
                                holder = h;
                            }
                        }
                        if (minDist > range) {
                            return false; // otherwise, it will be more than strict snap to nearest line...
                        }
                    }
                    const auto snapEntityType = holder.snapEntityType;
                    // LC_ERR << "VSnap - snapped type : " << snapEntityType;
                    double distance = RS_MAXDOUBLE;
                    RS_Entity* guideEntity = holder.entity;
                    switch (snapEntityType) {
                        case RS2::VSNAP_LINE_VERTEX_VERTICAL:
                        case RS2::VSNAP_LINE_VERTEX_HORIZONTAL:
                        case RS2::VSNAP_POINT_RELATIVE_VERTICAL_DX:
                        case RS2::VSNAP_POINT_RELATIVE_HORIZONTAL_DY: {
                            restrictingType = snapEntityType;
                            *restrictingEntity = holder.entity;
                            break;
                        }
                        case RS2::VSNAP_LINE_ENDPOINT_TANGENT:
                        case RS2::VSNAP_LINE_ENDPOINT_NORMAL:
                        case RS2::VSNAP_LINE_TANGENT1:
                        case RS2::VSNAP_LINE_TANGENT2:
                        case RS2::VSNAP_LINE_RAY:
                        case RS2::VSNAP_LINE_ENDPOINT_ANGLE_STEP:
                        case RS2::VSNAP_LINE_VERTEX_ANGLE_STEP:
                        case RS2::VSNAP_LINE_VERTEX_VERTEX:
                        case RS2::VSNAP_POINT_RELATIVE_ANGLE_RAY:
                        case RS2::VSNAP_POINT_RELATIVE_NORMAL: {
                            double distanceToRay;
                            RS_Vector snap = guideEntity->getNearestPointOnEntity(mouseCoord, true, &distanceToRay);
                            snapEndpoint(mouseCoord, distance);
                            snapIntersection(mouseCoord, distance);
                            snapCenter(mouseCoord, distance);

                            if (distance > range) {
                                if (distanceToRay < range) {
                                    if (isSnapToGrid()) {
                                        const RS_Vector startPoint = guideEntity->getStartpoint();
                                        const RS_Vector endPoint = guideEntity->getEndpoint();
                                        snap = snapGrid(snap, startPoint, endPoint);
                                        m_impData->restrictedSnapType = RS2::SnapType::GRID;
                                    }
                                    else {
                                        // LC_ERR << "VS RAY: ";
                                    }

                                    m_impData->snapSpot = snap;
                                    m_impData->snapCoord = snap;

                                    m_impData->snapType = RS2::SnapType::VISUAL_SNAP;
                                    RS2::LC_VisualSnapIntersectionInfo snapInfo;
                                    snapInfo.entity1 = RS2::VisualSnapGuideEntityType::VSNAP_NONE;
                                    snapInfo.entity2 = snapEntityType;
                                    if (snapEntityType == RS2::VSNAP_LINE_VERTEX_ANGLE_STEP || snapEntityType ==
                                        RS2::VSNAP_LINE_ENDPOINT_ANGLE_STEP) {
                                        snapInfo.rayAngle2 = holder.wcsRayAngleRad;
                                    }
                                    m_impData->visualSnapType = snapInfo;
                                    m_impData->entity = nullptr;
                                    m_impData->entityOther = nullptr;
                                    visualSnapFound = true;
                                    // LC_ERR << "VS POINT ON RAY:  " << snap;
                                }
                            }
                            else {
                                // visualSnapFound = true;
                                // m_impData->snapCoord = m_impData->snapSpot;
                                visualSnapFound = false;
                                // LC_ERR << "VS POINT FOUND: " << m_impData->snapCoord;
                            }
                            break;
                        }
                        default: {
                            const RS_Entity* ent = holder.entity;
                            RS_Vector snap(false);
                            snap = ent->getNearestPointOnEntity(mouseCoord, true, &distance);
                            if (distance < range) {
                                m_impData->snapSpot = snap;
                                m_impData->snapCoord = snap;
                                m_impData->snapType = RS2::SnapType::VISUAL_SNAP;
                                RS2::LC_VisualSnapIntersectionInfo snapInfo;
                                snapInfo.entity1 = RS2::VisualSnapGuideEntityType::VSNAP_NONE;
                                snapInfo.entity2 = snapEntityType;
                                if (snapEntityType == RS2::VSNAP_LINE_VERTEX_ANGLE_STEP || snapEntityType ==
                                    RS2::VSNAP_LINE_ENDPOINT_ANGLE_STEP) {
                                    snapInfo.rayAngle2 = holder.wcsRayAngleRad;
                                    }
                                m_impData->visualSnapType = snapInfo;
                                m_impData->entity = nullptr;
                                m_impData->entityOther = nullptr;
                                visualSnapFound = true;
                            }
                        }
                    }
                }
            }
        }
    }
    return visualSnapFound;
}

/**
 * Snap to a coordinate in the drawing using the current snap mode.
 *
 * @param e A mouse event.
 * @return The coordinates of the point or an invalid vector.
 */
RS_Vector RS_Snapper::snapPoint(const QMouseEvent* e) {
    m_impData->snapType = RS2::NO_SNAP;
    m_impData->restrictedSnapType = RS2::NO_SNAP;
    m_impData->entity = nullptr;
    m_impData->entityOther = nullptr;
    m_impData->restriction = RS2::RestrictNothing;
    m_impData->snapCoord = RS_Vector(false);
    m_impData->snapSpot = RS_Vector(false);
    m_impData->visualSnapType.entity1 = RS2::VSNAP_NONE;
    m_impData->visualSnapType.entity2 = RS2::VSNAP_NONE;

    if (e == nullptr) {
        RS_DEBUG->print(RS_Debug::D_WARNING, "RS_Snapper::snapPoint: event is nullptr");
        return m_impData->snapSpot;
    }

    const RS_Vector mouseCoord = toGraph(e);
    double ds2Min = RS_MAXDOUBLE * RS_MAXDOUBLE;

    RS_Entity* restrictingEntity{nullptr};
    RS2::VisualSnapGuideEntityType restrictingEntityType;

    const bool visualSnapFound = snapVisual(mouseCoord, &restrictingEntity, restrictingEntityType);

    if (!visualSnapFound) {
        snapEndpoint(mouseCoord, ds2Min);
        snapCenter(mouseCoord, ds2Min);
        snapMiddle(mouseCoord, ds2Min);
        snapDistance(mouseCoord, ds2Min);
        snapIntersection(mouseCoord, ds2Min);
        snapOnEntity(mouseCoord, ds2Min);
        snapGrid(mouseCoord, ds2Min);

        if (!m_impData->snapSpot.valid) {
            m_impData->snapSpot = mouseCoord; //default to snapFree
            m_impData->snapType = RS2::SnapType::FREE;
        }
        else {
            //retreat to snapFree when distance is more than quarter grid
            // issue #1631: snapFree issues: defines getSnapFree as the minimum graph distance to allow SnapFree
            if (m_snapMode.snapFree) {
                const double snapRange = getSnapRange();
                // compare the current graph distance to the closest snap point to the minimum snapping free distance
                if ((mouseCoord - m_impData->snapSpot).magnitude() >= snapRange) {
                    m_impData->snapSpot = mouseCoord;
                    m_impData->snapType = RS2::SnapType::FREE;
                }
            }
        }

        // handle snap restrictions that can be activated in addition
        // to the ones above:
        //apply restriction
        RS_Vector vpv;
        RS_Vector vph;
        if (m_snapMode.restriction != RS2::RestrictNothing) {
            const RS_Vector rz = m_viewport->getRelativeZero();
            if (m_viewport->hasUCS()) {
                const RS_Vector ucsRZ = m_viewport->toUCS(rz);
                const RS_Vector ucsSnap = m_viewport->toUCS(m_impData->snapSpot);
                vpv = m_viewport->toWorld(RS_Vector(ucsRZ.x, ucsSnap.y));
                vph = m_viewport->toWorld(RS_Vector(ucsSnap.x, ucsRZ.y));
            }
            else {
                vpv = RS_Vector(rz.x, m_impData->snapSpot.y);
                vph = RS_Vector(m_impData->snapSpot.x, rz.y);
            }
        }

        switch (m_snapMode.restriction) {
            case RS2::RestrictOrthogonal: {
                m_impData->snapCoord = (mouseCoord.distanceTo(vpv) < mouseCoord.distanceTo(vph)) ? vpv : vph;
                m_impData->restriction = RS2::RestrictOrthogonal;
                break;
            }
            case RS2::RestrictHorizontal: {
                m_impData->snapCoord = vph;
                m_impData->restriction = RS2::RestrictHorizontal;
                break;
            }
            case RS2::RestrictVertical: {
                m_impData->snapCoord = vpv;
                m_impData->restriction = RS2::RestrictVertical;
                break;
            }
            //case RS2::RestrictNothing:
            default: {
                m_impData->snapCoord = m_impData->snapSpot;
                m_impData->restriction = RS2::RestrictNothing;
                break;
            }
        }
    }
    if (restrictingEntity != nullptr) {
        VisualSnapSolution* visualSnapSolution = m_visualSnapManager->getCurrentSolution();
        visualSnapSolution->restrictedPoint = m_impData->snapSpot;
        visualSnapSolution->restrictedOriginalSnapType = m_impData->snapType;
        m_impData->restrictedSnapType = m_impData->snapType;
        m_impData->snapType = RS2::SnapType::VISUAL_SNAP;
        RS2::LC_VisualSnapIntersectionInfo snapInfo;
        snapInfo.entity1 = RS2::VisualSnapGuideEntityType::VSNAP_NONE;
        snapInfo.entity2 = restrictingEntityType;
        m_impData->visualSnapType = snapInfo;

        if (restrictingEntityType == RS2::VSNAP_LINE_VERTEX_VERTICAL || restrictingEntityType == RS2::VSNAP_POINT_RELATIVE_VERTICAL_DX) {
            m_impData->snapSpot = restrictVertical(restrictingEntity->getStartpoint(), m_impData->snapSpot);
            m_impData->snapCoord = m_impData->snapSpot;
        }
        else if (restrictingEntityType == RS2::VSNAP_LINE_VERTEX_HORIZONTAL || restrictingEntityType ==
            RS2::VSNAP_POINT_RELATIVE_HORIZONTAL_DY) {
            m_impData->snapSpot = restrictHorizontal(restrictingEntity->getStartpoint(), m_impData->snapSpot);
            m_impData->snapCoord = m_impData->snapSpot;
        }
        visualSnapSolution->foundSnapPoint = m_impData->snapSpot;
    }

    snapPoint(m_impData->snapSpot, false);

    return m_impData->snapCoord;
}

/**manually set snapPoint*/
RS_Vector RS_Snapper::snapPoint(const RS_Vector& coord, const bool setSpot) {
    if (coord.valid) {
        m_impData->snapSpot = coord;
        if (setSpot) {
            m_impData->snapCoord = coord;
        }
        // fixme - sand - it seems that the code below is meaning for preview only?
        updateCoordinateWidgetByRelZero(m_impData->snapCoord);
        drawSnapper();
        drawInfoCursor();
    }
    return coord;
}

double RS_Snapper::getSnapRange() const {
    // issue #1631: redefine this method to the minimum graph distance to allow "Snap Free"
    // When the closest of any other snapping point is beyond this distance, free snapping is used.

    double minGraph = RS_MAXDOUBLE;
    double minGrid = RS_MAXDOUBLE;
    double minSize = RS_MAXDOUBLE;
    if (m_graphicView != nullptr) {
        minGraph = toGraphDX(DEFAULT_CATCH_ENTITY_RANGE_PX);
        // if grid is on, less than one quarter of the cell vector
        //        if (viewport->isGridOn()) {
        // todo - sand - check whether it's correct apply this check only if "Snap to Grid" is enabled
        if (m_viewport->isGridOn() && m_snapMode.snapGrid) {
            const RS_Grid* grid = m_viewport->getGrid();
            const RS_Vector& cellVector = grid->getCellVector();
            minGrid = cellVector.magnitude() * m_minGridCellSnapFactor;
        }
    }
    if (isSizeValid(m_document->getSize())) {
        // The size bounding box
        minSize = getValidSize(m_document->getSize());
    }
    if (std::min(minGraph, minGrid) < 0.99 * RS_MAXDOUBLE) {
        return std::min(minGraph, minGrid);
    }
    if (minSize < 0.99 * RS_MAXDOUBLE) {
        return minSize;
    }
    // shouldn't happen: no graphicview or a valid size
    // Allow free snapping by returning the floating point tolerance
    return RS_TOLERANCE;
}

/**
 * Snaps to a free coordinate.
 *
 * @param coord The mouse coordinate.
 * @return The coordinates of the point or an invalid vector.
 */
RS_Vector RS_Snapper::snapFree(const RS_Vector& coord) {
    m_keyEntity = nullptr;
    return coord;
}

/**
 * Snaps to the closest endpoint.
 *
 * @param coord The mouse coordinate.
 * @return The coordinates of the point or an invalid vector.
 */
RS_Vector RS_Snapper::snapEndpoint(const RS_Vector& coord, RS_Entity** entity) const {
    const RS_Vector vec = m_document->getNearestEndpoint(coord, entity, nullptr);
    return vec;
}

/**
 * Snaps to a grid point.
 *
 * @param coord The mouse coordinate.
 * @return The coordinates of the point or an invalid vector.
 */
RS_Vector RS_Snapper::snapGrid(const RS_Vector& coord) const {
    //    RS_DEBUG->print("RS_Snapper::snapGrid begin");

    //    std::cout<<__FILE__<<" : "<<__func__<<" : line "<<__LINE__<<std::endl;
    //    std::cout<<" mouse: = "<<coord<<std::endl;
    //    std::cout<<" snapGrid: = "<<graphicView->getGrid()->snapGrid(coord)<<std::endl;
    return m_viewport->snapGrid(coord);
}

RS_Vector RS_Snapper::snapGrid(const RS_Vector& coord, const RS_Vector& rayStart, const RS_Vector& rayEnd) const {
    return m_viewport->snapGrid(coord, rayStart, rayEnd);
}

// fixme - note 1) similarly to ray, actually any entity may be used with snap to grid if it will be possisble to find
// intersection point points of constuction lines with this grid. However, for UCS - it's necessary to transform the entity
// accordingly (or, alternativealy, transform a grid cell.. ).
// Fixme - note 2) it might be practical add method that returns grid cell - and do transformation in viewport
RS_Vector RS_Snapper::snapGrid(const RS_Vector& coord, RS_Entity* entity) const {
    return m_viewport->snapGrid(coord, entity);
}

/**
 * Snaps to a point on an entity.
 *
 * @param coord The mouse coordinate.
 * @return The coordinates of the point or an invalid vector.
 */
RS_Vector RS_Snapper::snapOnEntity(const RS_Vector& coord, RS_Entity** entity) {
    const RS_Vector vec = m_document->getNearestPointOnEntity(coord, true, nullptr, &m_keyEntity);
    *entity = m_keyEntity;
    return vec;
}

/**
 * Snaps to the closest center.
 *
 * @param coord The mouse coordinate.
 * @return The coordinates of the point or an invalid vector.
 */
RS_Vector RS_Snapper::snapCenter(const RS_Vector& coord, RS_Entity** centerEntity) const {
    const RS_Vector vec = m_document->getNearestCenter(coord, nullptr, centerEntity);
    return vec;
}

/**
 * Snaps to the closest middle.
 *
 * @param coord The mouse coordinate.
 * @return The coordinates of the point or an invalid vector.
 */
RS_Vector RS_Snapper::snapMiddle(const RS_Vector& coord) const {
    //std::cout<<"RS_Snapper::snapMiddle(): middlePoints="<<middlePoints<<std::endl;
    return m_document->getNearestMiddle(coord, nullptr, m_middlePoints);
}

int RS_Snapper::getSnapMiddlePoints() const {
    return m_middlePoints;
}

double RS_Snapper::getSnapDistance() const {
    return m_snapDistance;
}

/**
 * Snaps to the closest point with a given distance to the endpoint.
 *
 * @param coord The mouse coordinate.
 * @return The coordinates of the point or an invalid vector.
 */
RS_Vector RS_Snapper::snapDist(const RS_Vector& coord) const {
    //std::cout<<" RS_Snapper::snapDist(RS_Vector coord): distance="<<distance<<std::endl;
    const RS_Vector vec = m_document->getNearestDist(m_snapDistance, coord, nullptr);
    return vec;
}

/**
 * Snaps to the closest intersection point.
 *
 * @param coord The mouse coordinate.
 * @return The coordinates of the point or an invalid vector.
 */
RS_Vector RS_Snapper::snapIntersection(const RS_Vector& coord, RS_Entity** entity, [[maybe_unused]] RS_Entity** otherEntity) const {
    const RS_Vector vec = m_document->getNearestIntersection(coord, nullptr);
    return vec;
}

/**
 * 'Corrects' the given coordinates to 0, 90, 180, 270 degrees relative to
 * the current relative zero point.
 *
 * @param coord The uncorrected coordinates.
 * @return The corrected coordinates.
 */
RS_Vector RS_Snapper::restrictOrthogonal(const RS_Vector& coord) const {
    const RS_Vector rz = m_viewport->getRelativeZero();
    RS_Vector ret(coord);

    const auto retx = RS_Vector(rz.x, ret.y);
    const auto rety = RS_Vector(ret.x, rz.y);

    if (retx.distanceTo(ret) > rety.distanceTo(ret)) {
        ret = rety;
    }
    else {
        ret = retx;
    }

    return ret;
}

/**
 * 'Corrects' the given coordinates to 0, 180 degrees relative to
 * the current relative zero point.
 *
 * @param coord The uncorrected coordinates.
 * @return The corrected coordinates.
 */

RS_Vector RS_Snapper::restrictHorizontal(const RS_Vector& coord) const {
    return m_viewport->restrictHorizontal(m_viewport->getRelativeZero(), coord);
}

/**
 * 'Corrects' the given coordinates to 90, 270 degrees relative to
 * the current relative zero point.
 *
 * @param coord The uncorrected coordinates.
 * @return The corrected coordinates.
 */
RS_Vector RS_Snapper::restrictVertical(const RS_Vector& coord) const {
    return m_viewport->restrictVertical(m_viewport->getRelativeZero(), coord);
}

RS_Vector RS_Snapper::restrictVertical(const RS_Vector& base, const RS_Vector& coord) const {
    return m_viewport->restrictVertical(base, coord);
}

RS_Vector RS_Snapper::restrictHorizontal(const RS_Vector& base, const RS_Vector& coord) const {
    return m_viewport->restrictHorizontal(base, coord);
}

RS_Vector RS_Snapper::restrictAngle(const RS_Vector& basePoint, const RS_Vector& snap, const double angle) const {
    const double realAngle = toWorldAngle(angle);
    const RS_Vector infiniteTickEndPoint = basePoint.relative(10.0, realAngle);
    const RS_Vector pointOnInfiniteTick = LC_LineMath::getNearestPointOnInfiniteLine(snap, basePoint, infiniteTickEndPoint);

    const RS_Vector possibleEndPoint = pointOnInfiniteTick;
    return possibleEndPoint;
}

/**
 * Catches an entity which is close to the given position 'pos'.
 *
 * @param pos A graphic coordinate.
 * @param level The level of resolving for iterating through the entity
 *        container
 * @return Pointer to the entity or nullptr.
 */
RS_Entity* RS_Snapper::catchEntity(const RS_Vector& pos, const RS2::ResolveLevel level) const {
    // set default distance for points inside solids
    double dist(0.);
    //    std::cout<<"getSnapRange()="<<getSnapRange()<<"\tsnap distance = "<<dist<<std::endl;

    RS_Entity* entity = m_document->getNearestEntity(pos, &dist, level);

    int idx = -1;
    if (entity != nullptr && entity->getParent()) {
        idx = entity->getParent()->findEntity(entity);
    }

    if (entity != nullptr && dist <= getCatchDistance(getSnapRange(), m_catchEntityGuiRange)) {
        // highlight:
        RS_DEBUG->print("RS_Snapper::catchEntity: found: %d", idx);
        return entity;
    }
    RS_DEBUG->print("RS_Snapper::catchEntity: not found");
    return nullptr;
}

/**
 * Catches an entity which is close to the given position 'pos'.
 *
 * @param pos A graphic coordinate.
 * @param enType
 * @param level The level of resolving for iterating through the entity
 *        container
 * @enType, only search for a particular entity type
 * @return Pointer to the entity or nullptr.
 */
RS_Entity* RS_Snapper::catchEntity(const RS_Vector& pos, const RS2::EntityType enType, const RS2::ResolveLevel level) const {
    RS_DEBUG->print("RS_Snapper::catchEntity");
    //  std::cout<<"RS_Snapper::catchEntity(): enType= "<<enType<<std::endl;

    // set default distance for points inside solids
    RS_EntityContainer ec(nullptr, false);
    //isContainer
    bool isContainer{false};
    switch (enType) {
        case RS2::EntityPolyline:
        case RS2::EntityContainer:
        case RS2::EntitySpline:
            isContainer = true;
            break;
        default:
            break;
    }

    const auto traversedEntities = lc::LC_ContainerTraverser{*m_document, level}.entities();
    // fixme - iteration over all elements of drawing
    for (const RS_Entity* en : traversedEntities) {
        if (!en->isVisible()) {
            continue;
        }
        if (en->rtti() != enType && isContainer) {
            //whether this entity is a member of member of the type enType
            const RS_Entity* parent(en->getParent());
            bool matchFound{false};
            while (parent) {
                if (parent->rtti() == enType) {
                    matchFound = true;
                    ec.addEntity(en);
                    break;
                }
                parent = parent->getParent();
            }
            if (!matchFound) {
                continue;
            }
        }
        if (en->rtti() == enType) {
            ec.addEntity(en);
        }
    }
    if (ec.count() == 0) {
        return nullptr;
    }
    double dist(0.);

    RS_Entity* entity = ec.getNearestEntity(pos, &dist, RS2::ResolveNone);

    int idx = -1;
    if (entity != nullptr && entity->getParent()) {
        idx = entity->getParent()->findEntity(entity);
    }

    if (entity != nullptr && dist <= getCatchDistance(getSnapRange(), m_catchEntityGuiRange)) {
        // highlight:
        RS_DEBUG->print("RS_Snapper::catchEntity: found: %d", idx);
        return entity;
    }
    RS_DEBUG->print("RS_Snapper::catchEntity: not found");
    return nullptr;
}

/**
 * Catches an entity which is close to the mouse cursor.
 *
 * @param e A mouse event.
 * @param level The level of resolving for iterating through the entity
 *        container
 * @return Pointer to the entity or nullptr.
 */
RS_Entity* RS_Snapper::catchEntity(const QMouseEvent* e, const RS2::ResolveLevel level) const {
    RS_Entity* entity = catchEntity(toGraph(e), level);
    return entity;
}

/**
 * Catches an entity which is close to the mouse cursor.
 *
 * @param e A mouse event.
 * @param enType
 * @param level The level of resolving for iterating through the entity
 *        container
 * @enType, only search for a particular entity type
 * @return Pointer to the entity or nullptr.
 */
RS_Entity* RS_Snapper::catchEntity(const QMouseEvent* e, const RS2::EntityType enType, const RS2::ResolveLevel level) const {
    return catchEntity(toGraph(e), enType, level);
}

RS_Entity* RS_Snapper::catchEntity(const QMouseEvent* e, const EntityTypeList& enTypeList, const RS2::ResolveLevel level) const {
    const RS_Vector coord = toGraph(e);
    return catchEntity(coord, enTypeList, level);
}

RS_Entity* RS_Snapper::catchEntity(const RS_Vector& pos, const EntityTypeList& enTypeList, const RS2::ResolveLevel level) const {
    RS_Entity* pten = nullptr;
    switch (enTypeList.size()) {
        case 0:
            return catchEntity(pos, level);
        default: {
            RS_EntityContainer ec(nullptr, false);
            for (const auto t0 : enTypeList) {
                const RS_Entity* en = catchEntity(pos, t0, level);
                if (en != nullptr) {
                    // fixme - sand - due to some unknown reasons, there is a duplication of entity to be added on catch!!! Investigate and fix!!
                    if (ec.findEntity(en) == -1) {
                        ec.addEntity(en);
                    }
                }
                //			if(en) {
                //            std::cout<<__FILE__<<" : "<<__func__<<" : lines "<<__LINE__<<std::endl;
                //            std::cout<<"caught id= "<<en->getId()<<std::endl;
                //            }
            }
            if (ec.count() > 0) {
                ec.getDistanceToPoint(pos, &pten, RS2::ResolveNone);
                return pten;
            }
        }
    }
    return nullptr;
}

void RS_Snapper::suspend() {
    m_impData->snapSpot = RS_Vector{false};
    m_impData->snapCoord = RS_Vector{false};
    suspendRelativeInputWidget();
}

void RS_Snapper::resume() {
    drawSnapper();
    initSettings();
    // fixme - review/rework this, load from settings as other overlays action(??)
    m_infoCursorOverlayPrefs = m_graphicView->getInfoCursorOverlayPreferences();
    resumeRelativeInputWidget();
}

/**
 * Hides the snapper options. Default implementation does nothing.
 */
void RS_Snapper::hideSnapOptions() const {
    m_actionContext->hideSnapOptions();
}

/**
 * Deletes the snapper from the screen.
 */
void RS_Snapper::deleteSnapper() const {
    if (m_graphicView != nullptr && !m_graphicView->isCleanUp()) {
        m_viewport->clearOverlayDrawablesContainer(RS2::Snapper);
        m_graphicView->redraw(RS2::RedrawOverlay); // redraw will happen in the mouse movement event
    }
}

void RS_Snapper::deleteInfoCursor() const {
    if (m_graphicView != nullptr && !m_graphicView->isCleanUp()) {
        m_viewport->clearOverlayDrawablesContainer(RS2::InfoCursor);
        m_graphicView->redraw(RS2::RedrawOverlay); // redraw will happen in the mouse movement event
    }
}

/**
 * creates the snap indicator
 */
void RS_Snapper::drawSnapper() {
    LC_OverlayDrawablesContainer* snapperOverlay = m_viewport->getOverlaysDrawablesContainer(RS2::Snapper);
    snapperOverlay->clear();
    if (!m_finished && m_impData->snapSpot.valid) {
        if (m_snapIndicator->drawLines || m_snapIndicator->drawShape) {
            auto* crosshair = new LC_Crosshair(m_impData->snapCoord, m_snapIndicator->shape_Type, m_snapIndicator->lines_Type,
                                               m_snapIndicator->lines_Pen, m_snapIndicator->pointSize, m_snapIndicator->pointType);
            crosshair->setShapesPen(m_snapIndicator->shape_Pen);
            snapperOverlay->add(crosshair);
        }
    }
    m_graphicView->redraw(RS2::RedrawOverlay); // redraw will happen in the mouse movement event
}

LC_OverlayInfoCursor* RS_Snapper::obtainInfoCursor() const {
    const auto overlayContainer = m_viewport->getOverlaysDrawablesContainer(RS2::InfoCursor);
    LC_OverlayInfoCursor* result = nullptr;
    // fixme - this is not absolutely safe if someone put another cursor to overlay container! Rework later!!
    const auto entity = overlayContainer->first(); // note - this is not absolutely safe if someone put another cursor to overlay container!
    result = dynamic_cast<LC_OverlayInfoCursor*>(entity);
    if (result == nullptr && m_infoCursorOverlayPrefs != nullptr) {
        result = new LC_OverlayInfoCursor(m_impData->snapCoord, &m_infoCursorOverlayPrefs->options);
        overlayContainer->add(result);
    }
    return result;
}

void RS_Snapper::drawInfoCursor() {
    const auto overlayContainer = m_viewport->getOverlaysDrawablesContainer(RS2::InfoCursor);
    if (m_infoCursorOverlayPrefs != nullptr && m_infoCursorOverlayPrefs->enabled) {
        // fixme - this is not absolutely safe if someone put another cursor to overlay container! Rework later!!
        const auto entity = overlayContainer->first();
        auto* infoCursor = dynamic_cast<LC_OverlayInfoCursor*>(entity);
        if (infoCursor == nullptr) {
            infoCursor = new LC_OverlayInfoCursor(m_impData->snapCoord, &m_infoCursorOverlayPrefs->options);
            overlayContainer->add(infoCursor);
        }
        else {
            infoCursor->setOptions(&m_infoCursorOverlayPrefs->options);
            infoCursor->setPos(m_impData->snapCoord);
        }
        const auto prefs = getInfoCursorOverlayPrefs();
        if (prefs->showSnapType) {
            if (m_graphicView->isInRelativePointInput()) {
                m_infoCursorOverlayData->setZone2("");
            }
            else {
                QString snapName = getCurrentSnapName();
                QString restrictionName;
                if (m_impData->snapType == RS2::ANGLE || m_impData->snapType == RS2::ANGLE_REL || m_impData->snapType == RS2::ANGLE_ON_ENTITY) {
                    const double ucsAbsSnapAngle = m_impData->angle;
                    const double ucsBasisAngle = m_formatter->toUCSBasisAngleFromUCS(ucsAbsSnapAngle);
                    restrictionName = formatAngleRaw(ucsBasisAngle);
                }
                else {
                    restrictionName = getCurrentRestrictionName();
                }
                if (!restrictionName.isEmpty()) {
                    snapName = snapName + (prefs->multiLine ? "\n" : " | ") + restrictionName;
                }
                m_infoCursorOverlayData->setZone2(snapName);
            }
        }
        else {
            m_infoCursorOverlayData->setZone2("");
        }
        infoCursor->setZonesData(m_infoCursorOverlayData.get());
    }
    m_graphicView->redraw(RS2::RedrawOverlay);
}

QString RS_Snapper::getRestrictionName(const int restriction) {
    switch (restriction) {
        case RS2::RestrictVertical:
            return tr("Vertical");
        case RS2::RestrictHorizontal:
            return tr("Horizontal");
        case RS2::RestrictOrthogonal:
            return tr("Orthogonal");
        default:
            return "";
    }
}

QString RS_Snapper::getCurrentRestrictionName() {
    if (m_impData->snapType == RS2::VISUAL_SNAP) {
        if (m_impData->visualSnapType.entity1 == RS2::VSNAP_NONE) {
            auto secondEntityType = m_impData->visualSnapType.entity2;
            if (secondEntityType == RS2::VSNAP_LINE_VERTEX_VERTICAL) {
                return getSnapName(m_impData->restrictedSnapType);
            }
            if (secondEntityType == RS2::VSNAP_LINE_VERTEX_HORIZONTAL) {
                return getSnapName(m_impData->restrictedSnapType);
            }
            if (secondEntityType == RS2::VSNAP_NONE) {
                return getSnapName(m_impData->restrictedSnapType);
            }
            if (secondEntityType == RS2::VSNAP_POINT_RELATIVE_VERTICAL_DX) {
                return getSnapName(m_impData->restrictedSnapType);
            }
            if (secondEntityType == RS2::VSNAP_POINT_RELATIVE_HORIZONTAL_DY) {
                return getSnapName(m_impData->restrictedSnapType);
            }
        }
        return "";
    }
    else {
        return getRestrictionName(m_impData->restriction);
    }
}

QString RS_Snapper::getCurrentSnapName() const {
    const RS2::SnapType snapType = m_impData->snapType;
    QString msg = getSnapName(snapType);
    if (snapType == RS2::SnapType::VISUAL_SNAP) {
        const auto pointType = m_impData->visualSnapType;
        if (pointType.entity1 == RS2::VSNAP_NONE) {
            // this is point
            msg = msg + " (" + getVisualSnapName(pointType.entity2, pointType.rayAngle2) + ")";
        }
        else {
            // this is intersection
            msg = msg + " (" + getVisualSnapName(pointType.entity1, pointType.rayAngle1) + " x " + getVisualSnapName(
                pointType.entity2, pointType.rayAngle2) + ")";
        }
    }
    return msg;
}

QString RS_Snapper::getSnapName(RS2::SnapType snapType) const {
    switch (snapType) {
        case RS2::GRID:
            return tr("Grid");
        case RS2::ENTITY:
            return tr("Entity");
        case RS2::ENDPOINT:
            return tr("Endpoint");
        case RS2::INTERSECTION:
            return tr("Intersection");
        case RS2::MIDDLE:
            return tr("Middle");
        case RS2::DISTANCE:
            return tr("Distance");
        case RS2::CENTER:
            return tr("Center");
        case RS2::ANGLE:
            return tr("Angle");
        case RS2::ANGLE_REL:
            return tr("Angle Relative");
        case RS2::ANGLE_ON_ENTITY:
            return tr("Angle (on Entity)");
        case RS2::VISUAL_SNAP: {
            QString msg = tr("Visual");
            return msg;
        }
        case RS2::FREE:
            return tr("Free");
        default:
            return "";
    }
}

QString RS_Snapper::getVisualSnapName(const RS2::VisualSnapGuideEntityType entityType, double rayAngle) const {
    switch (entityType) {
        case RS2::VSNAP_NONE: {
            return tr("None", "visual snap");
        }
        case RS2::VSNAP_LINE_VERTEX_HORIZONTAL: {
            return tr("Horizontal", "visual snap");
        }
        case RS2::VSNAP_LINE_VERTEX_ANGLE_STEP: {
            QString name = tr("Angle Ray");
            const double wcsRayAngle = rayAngle;
            if (wcsRayAngle > 0) {
                const double ucsAbsRayAngle = toUCSBasisAngle(wcsRayAngle);
                const double ucsBasisAngle = m_formatter->toUCSBasisAngleFromUCS(ucsAbsRayAngle);
                QString angleName = formatAngleRaw(ucsBasisAngle);
                name = name + " " + angleName;
            }
            return name;
        }
        case RS2::VSNAP_LINE_ENDPOINT_ANGLE_STEP: {
            QString name = tr("Relative Angle Ray");
            const double wcsRayAngle = rayAngle;
            if (wcsRayAngle > 0) {
                const double ucsAbsRayAngle = toUCSBasisAngle(wcsRayAngle);
                const double ucsBasisAngle = m_formatter->toUCSBasisAngleFromUCS(ucsAbsRayAngle);
                QString angleName = formatAngleRaw(ucsBasisAngle);
                name = name + " " + angleName;
            }
            return name;
        }
        case RS2::VSNAP_LINE_VERTEX_VERTICAL: {
            return tr("Vertical", "visual snap");
        }
        case RS2::VSNAP_LINE_RAY: {
            return tr("Line ray");
        }
        case RS2::VSNAP_LINE_VERTEX_VERTEX: {
            return tr("Vertex-Vertex");
        }
        case RS2::VSNAP_LINE_ENDPOINT_TANGENT: {
            return tr("Endpoint Tangent");
        }
        case RS2::VSNAP_LINE_ENDPOINT_NORMAL: {
            return tr("Endpoint Normal");
        }
        case RS2::VSNAP_LINE_TANGENT1: {
            return tr("Tangent One");
        }
        case RS2::VSNAP_LINE_TANGENT2: {
            return tr("Tangent Two");
        }
        case RS2::VSNAP_POINT_MIDDLE: {
            return tr("Middle", "visual snap");
        }
        case RS2::VSNAP_LINE_VERTEX_ORTHO: {
            return tr("Orthogonal", "visual snap");
        }
        case RS2::VSNAP_POINT_DISTANCE_EXPLICIT: {
            return tr("Distance (Explicit)");
        }
        case RS2::VSNAP_POINT_DISTANCE_VERTEX: {
            return tr("Distance (Vertex)");
        }
        case RS2::VSNAP_DOC_ENTITY: {
            return tr("Entity", "visual snap");
        }
        case RS2::VSNAP_POINT_RELATIVE_NORMAL: {
            return tr("Relative Normal", "visual snap");
        }
        case RS2::VSNAP_POINT_RELATIVE_DISTANCE: {
            return tr("Relative Distance", "visual snap");
        }
        case RS2::VSNAP_POINT_RELATIVE_VERTICAL_DX: {
            return tr("Relative X", "visual snap");
        }
        case RS2::VSNAP_POINT_RELATIVE_HORIZONTAL_DY: {
            return tr("Relative Y", "visual snap");
        }
        case RS2::VSNAP_POINT_RELATIVE_ANGLE_RAY: {
            return tr("Relative Angle", "visual snap");
        }
        default:
            return "";
    }
}

bool RS_Snapper::isSnapToGrid() const {
    bool result = m_snapMode.snapGrid;
    if (result) {
        if (m_ignoreSnapToGridIfNoGrid) {
            result = m_viewport->isGridOn();
        }
    }
    return result;
}

RS_Vector RS_Snapper::snapToRelativeAngle(const double baseAngle, const RS_Vector& currentCoord, const RS_Vector& referenceCoord,
                                          const double angularResolution) {
    if (m_snapMode.restriction != RS2::RestrictNothing || isSnapToGrid()) {
        return currentCoord;
    }

    const double wcsAngleRaw = referenceCoord.angleTo(currentCoord);
    const double ucsAngleRaw = toUCSAngle(wcsAngleRaw);
    const double ucsAngleSnapped = ucsAngleRaw - std::remainder(ucsAngleRaw, angularResolution);
    const double wcsAngleSnappedAbsolute = toWorldAngle(ucsAngleSnapped);

    const double wcsAngleSnapped = wcsAngleSnappedAbsolute + baseAngle; // add base angle, so snap is relative

    RS_Vector res = RS_Vector::polar(referenceCoord.distanceTo(currentCoord), wcsAngleSnapped);
    res += referenceCoord;

    if (m_snapMode.snapOnEntity) {
        const RS_Vector t = m_document->getNearestVirtualIntersection(res, wcsAngleSnapped, nullptr);
        m_impData->snapSpot = t;
        m_impData->snapType = (t == res) ? RS2::SnapType::ANGLE_REL : RS2::SnapType::ANGLE_ON_ENTITY;
        m_impData->angle = ucsAngleSnapped;
        snapPoint(m_impData->snapSpot, true);
        return t;
    }
    m_impData->snapType = RS2::SnapType::ANGLE_REL;
    m_impData->angle = ucsAngleSnapped;
    snapPoint(res, true);
    return res;
}

// fixme - snap to angle - rwork
RS_Vector RS_Snapper::snapToAngle(const RS_Vector& currentCoord, const RS_Vector& referenceCoord, const double angularResolution) {
    if (m_snapMode.restriction != RS2::RestrictNothing || isSnapToGrid()) {
        return currentCoord;
    }
    return doSnapToAngle(currentCoord, referenceCoord, angularResolution);
}

RS_Vector RS_Snapper::obtainEndPointForAngleSnap(const RS_Vector& currentCoord, const RS_Vector& referenceCoord,
                                                 const double angularResolution, double& wcsAngleSnapped, double& ucsAngleSnapped) {
    const double wcsAngleRaw = referenceCoord.angleTo(currentCoord);
    const double ucsAngleAbs = toUCSAngle(wcsAngleRaw);

    // double ucsAngle = ucsAngleAbs - this->m_anglesBase;
    // fixme - fmt - review
    const double ucsAngle = m_formatter->toUCSBasisAngleFromUCS(ucsAngleAbs);

    ucsAngleSnapped = ucsAngleAbs - remainder(ucsAngle, angularResolution);
    wcsAngleSnapped = toWorldAngle(ucsAngleSnapped);

    RS_Vector res = RS_Vector::polar(referenceCoord.distanceTo(currentCoord), wcsAngleSnapped);
    res += referenceCoord;
    return res;
}

RS_Vector RS_Snapper::doSnapToAngle(const RS_Vector& currentCoord, const RS_Vector& referenceCoord, const double angularResolution) {
    double wcsAngleSnapped;
    double ucsAngleSnapped;
    RS_Vector res = obtainEndPointForAngleSnap(currentCoord, referenceCoord, angularResolution, wcsAngleSnapped, ucsAngleSnapped);

    if (m_snapMode.snapOnEntity) {
        const RS_Vector t = m_document->getNearestVirtualIntersection(res, wcsAngleSnapped, nullptr);
        m_impData->snapSpot = t;
        m_impData->snapType = (t == res) ? RS2::ANGLE : RS2::ANGLE_ON_ENTITY;
        m_impData->angle = ucsAngleSnapped;
        snapPoint(m_impData->snapSpot, true);
        return t;
    }
    m_impData->snapType = RS2::ANGLE;
    m_impData->angle = ucsAngleSnapped;
    snapPoint(res, true);
    return res;
}

RS_Vector RS_Snapper::toGraph(const QMouseEvent* e) const {
    const QPointF& pointF = e->position();
    const RS_Vector result = m_viewport->toWorldFromUi(pointF.x(), pointF.y());
    return result;
}

double RS_Snapper::toGuiDX(const double wcsDX) const {
    return m_viewport->toGuiDX(wcsDX);
}

double RS_Snapper::toGraphDX(const int wcsDX) const {
    return m_viewport->toUcsDX(wcsDX);
}

const RS_Vector& RS_Snapper::getRelativeZero() const {
    return m_viewport->getRelativeZero();
}

void RS_Snapper::updateCoordinateWidgetFormat() const {
    m_actionContext->updateCoordinateWidget(toWorld(RS_Vector(0.0, 0.0)), toWorld(RS_Vector(0.0, 0.0)), true);
}

void RS_Snapper::updateCoordinateWidget(const RS_Vector& abs, const RS_Vector& rel) const {
    if (m_infoCursorOverlayPrefs->enabled) {
        preparePositionsInfoCursorOverlay(abs, rel);
    }
    m_actionContext->updateCoordinateWidget(abs, rel, false);
}

void RS_Snapper::updateCoordinateWidgetByRelZero(const RS_Vector& abs) const {
    const RS_Vector& relative = abs - m_viewport->getRelativeZero();
    if (m_infoCursorOverlayPrefs->enabled) {
        preparePositionsInfoCursorOverlay(abs, relative);
    }
    m_actionContext->updateCoordinateWidget(abs, relative, false);
}

LC_InfoCursorOverlayPrefs* RS_Snapper::getInfoCursorOverlayPrefs() const {
    return m_infoCursorOverlayPrefs;
}

bool RS_Snapper::isInfoCursorForModificationEnabled() const {
    return m_infoCursorOverlayPrefs->enabled && m_infoCursorOverlayPrefs->showEntityInfoOnModification;
}

void RS_Snapper::preparePositionsInfoCursorOverlay(const RS_Vector& abs, const RS_Vector& relative) const {
    const LC_InfoCursorOverlayPrefs* prefs = getInfoCursorOverlayPrefs();

    QString coordAbs = "";
    QString coordPolar = "";
    if (prefs != nullptr && (prefs->showAbsolutePosition || prefs->showRelativePositionDistAngle || prefs->showRelativePositionDeltas)) {
        const RS_Graphic* graphic = m_graphicView->getGraphic();
        if (graphic != nullptr) {
            const bool showLabels = prefs->showLabels;
            if (prefs->showAbsolutePosition) {
                const RS_Vector ucs = toUCS(abs);
                const QString absX = (showLabels ? "X: " : "") + formatLinear(ucs.x);
                const QString absY = (showLabels ? "Y: " : "") + formatLinear(ucs.y);
                coordAbs = absX + (prefs->multiLine ? "\n" : showLabels ? " " : " , ") + absY;
            }

            const bool hasUCS = m_viewport->hasUCS();
            if (prefs->showAbsolutePositionWCS && hasUCS) {
                const QString absX = (showLabels ? "WX: " : "W") + formatLinear(abs.x);
                const QString absY = (showLabels ? "WY: " : "") + formatLinear(abs.y);

                const QString coordAbsWCS = absX + (prefs->multiLine ? "\n" : showLabels ? " " : " , ") + absY;

                if (coordAbs.isEmpty()) {
                    coordAbs = coordAbsWCS;
                }
                else {
                    coordAbs = coordAbs + "\n" + coordAbsWCS;
                }
            }

            RS_Vector relativeToUse;
            if (hasUCS) {
                relativeToUse = m_viewport->toUCSDelta(relative);
            }
            else {
                relativeToUse = relative;
            }

            if (prefs->showRelativePositionDistAngle) {
                const QString lenStr = (showLabels ? tr("Dist: ") : "@ ") + formatLinear(relativeToUse.magnitude());
                // as we're in ucs coordinates there, use raw formatAngle instead of method

                const double relativeAngle = relativeToUse.angle();
                const double ucsBasisAngle = ucsAbsToBasisAngle(relativeAngle);
                const QString angleStr = (showLabels ? tr("Angle: ") : "< ") + formatAngleRaw(ucsBasisAngle);

                coordPolar = lenStr + (prefs->multiLine ? "\n" : showLabels ? " " : " ") + angleStr;
            }
            if (prefs->showRelativePositionDeltas) {
                const QString lenStr = (showLabels ? tr("dX: ") : "@ ") + formatLinear(relativeToUse.x);
                const QString angleStr = (showLabels ? tr("dY: ") : "") + formatLinear(relativeToUse.y);

                const QString coordDeltas = lenStr + (prefs->multiLine ? "\n" : showLabels ? " " : " , ") + angleStr;
                if (coordPolar.isEmpty()) {
                    coordPolar = coordDeltas;
                }
                else {
                    coordPolar = coordPolar + "\n" + coordDeltas;
                }
            }
        }
    }

    m_infoCursorOverlayData->setZone1(coordAbs);
    m_infoCursorOverlayData->setZone3(coordPolar);
}

void RS_Snapper::invalidateSnapSpot() const {
    m_impData->snapSpot.valid = false;
}

QString RS_Snapper::formatLinear(const double value) const {
    return m_formatter->formatLinear(value);
}

QString RS_Snapper::formatWCSAngle(const double wcsAngle) const {
    return m_formatter->formatWCSAngle(wcsAngle);
}

QString RS_Snapper::formatAngleRaw(const double angle) const {
    return m_formatter->formatRawAngle(angle);
}

// fixme - ucs-  move to coordinate mapper?
QString RS_Snapper::formatVector(const RS_Vector& value) const {
    double x, y;
    if (m_viewport->hasUCS()) {
        const RS_Vector ucsValue = m_viewport->toUCS(value);
        x = ucsValue.x;
        y = ucsValue.y;
    }
    else {
        x = value.x;
        y = value.y;
    }
    return formatLinear(x).append(" , ").append(formatLinear(y));
}

QString RS_Snapper::formatVectorWCS(const RS_Vector& value) const {
    return QString("W ").append(formatLinear(value.x)).append(" , ").append(formatLinear(value.y));
}

QString RS_Snapper::formatRelative(const RS_Vector& value) const {
    double x, y;
    m_viewport->toUCSDelta(value, x, y);
    return QString("@ ").append(formatLinear(x)).append(" , ").append(formatLinear(y));
}

QString RS_Snapper::formatPolar(const RS_Vector& value) const {
    return formatLinear(value.magnitude()).append(" < ").append(formatWCSAngle(value.angle()));
}

QString RS_Snapper::formatRelativePolar(const RS_Vector& wcsAngle) const {
    return QString("@ ").append(formatLinear(wcsAngle.magnitude())).append(" < ").append(formatWCSAngle(wcsAngle.angle()));
}

void RS_Snapper::forceUpdateInfoCursor(const RS_Vector& pos) const {
    LC_OverlayInfoCursor* infoCursor = obtainInfoCursor();
    if (infoCursor != nullptr) {
        infoCursor->setPos(pos);
        infoCursor->setZonesData(m_infoCursorOverlayData.get());
    }
}

double RS_Snapper::toWorldAngle(const double ucsAbsAngle) const {
    return m_viewport->toWorldAngle(ucsAbsAngle);
}

double RS_Snapper::toWorldAngleDegrees(const double ucsAbsAngleDegrees) const {
    return m_viewport->toWorldAngleDegrees(ucsAbsAngleDegrees);
}

double RS_Snapper::toUCSAngle(const double wcsAngle) const {
    return m_viewport->toUCSAngle(wcsAngle);
}

double RS_Snapper::ucsAbsToBasisAngle(const double ucsAbsAngle) const {
    return m_viewport->toBasisUCSAngle(ucsAbsAngle);
}

double RS_Snapper::ucsBasisToAbsAngle(const double ucsRelAngle) const {
    return m_viewport->toAbsUCSAngle(ucsRelAngle);
}

double RS_Snapper::adjustRelativeAngleSignByBasis(const double relativeAngle) const {
    return m_formatter->adjustRelativeAngleSignByBasis(relativeAngle);
}

double RS_Snapper::toUCSBasisAngleDegrees(const double wcsAngle) const {
    return m_formatter->toUCSBasisAngleDegrees(wcsAngle);
}

double RS_Snapper::toWorldAngleFromUCSBasisDegrees(const double ucsBasisAngleDegrees) const {
    return m_formatter->toWorldAngleFromUCSBasisDegrees(ucsBasisAngleDegrees);
}

double RS_Snapper::toWorldAngleFromUCSBasis(const double ucsBasisAngle) const {
    return m_formatter->toWorldAngleFromUCSBasis(ucsBasisAngle);
}

double RS_Snapper::toUCSBasisAngle(const double wcsAngle) const {
    return m_formatter->toUCSBasisAngle(wcsAngle);
}

RS_Vector RS_Snapper::toWorld(const RS_Vector& ucsPos) const {
    return m_viewport->toWorld(ucsPos);
}

RS_Vector RS_Snapper::toUCS(const RS_Vector& worldPos) const {
    return m_viewport->toUCS(worldPos);
}

RS_Vector RS_Snapper::toWorldDelta(const RS_Vector& ucsDelta) const {
    return m_viewport->toWorldDelta(ucsDelta);
}

RS_Vector RS_Snapper::toUCSDelta(const RS_Vector& worldDelta) const {
    return m_viewport->toUCSDelta(worldDelta);
}

// todo - sand - ucs - move to coordinates mapper?
void RS_Snapper::calcRectCorners(const RS_Vector& worldCorner1, const RS_Vector& worldCorner3, RS_Vector& worldCorner2,
                                 RS_Vector& worldCorner4) const {
    const RS_Vector ucsCorner1 = toUCS(worldCorner1);
    const RS_Vector ucsCorner3 = toUCS(worldCorner3);
    const auto ucsCorner2 = RS_Vector(ucsCorner1.x, ucsCorner3.y);
    const auto ucsCorner4 = RS_Vector(ucsCorner3.x, ucsCorner1.y);
    worldCorner2 = toWorld(ucsCorner2);
    worldCorner4 = toWorld(ucsCorner4);
}

bool RS_Snapper::hasNonDefaultAnglesBasis() const {
    return m_formatter->hasNonDefaultAnglesBasis();
}

void RS_Snapper::stopVisualSnap() const {
    m_visualSnapManager->clear();
}

void RS_Snapper::removePrevioustVisualSnapAddition() {
    m_visualSnapManager->removeLastAddition();
    onVisualSnapSolutionRefresh();
}

bool RS_Snapper::hasVisualSnap() {
    return m_visualSnapManager->hasVisualSnap();
}

// get catching entity distance in graph distance
double RS_Snapper::getCatchDistance(const double catchDistance, const int catchEntityGuiRange) const {
    return (m_graphicView != nullptr) ? std::min(catchDistance, toGraphDX(catchEntityGuiRange)) : catchDistance;
}

// fixme - sand - review
void RS_Snapper::enableCoordinateInput() const {
    m_graphicView->enableCoordinateInput();
}

void RS_Snapper::disableCoordinateInput() const {
    m_graphicView->disableCoordinateInput();
}

void RS_Snapper::redraw(const RS2::RedrawMethod method) const {
    // fixme - sand - ucs - decide how it's better to invoke redraw
    //    viewport->requestRedraw(method);
    m_graphicView->redraw(method);
}

void RS_Snapper::redrawImmediately(const RS2::RedrawMethod method) const {
    m_graphicView->redraw(static_cast<RS2::RedrawMethod>(method + RS2::RedrawImmediately));
}

void RS_Snapper::redrawDrawing() const {
    redraw(RS2::RedrawMethod::RedrawDrawing);
}

void RS_Snapper::redrawAll() const {
    redraw(RS2::RedrawMethod::RedrawAll);
}

// fixme **********************************************************************
// fixme - snap angle and grid enabled!!!!
// fixme **********************************************************************

// Fixme
// options for visual snap
// 1) which rays should be created
// 2) creating not only normal/tangental, but also interval rays (as in angle snap?)
