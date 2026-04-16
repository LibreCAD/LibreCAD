/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2015 Dongxu Li (dongxuli2011@gmail.com)
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

#ifndef RS_SNAPPER_H
#define RS_SNAPPER_H

#include <QObject>


#include "rs.h"
#include "rs_vector.h"

struct LC_VisualSnapVertex;
class LC_VisualSnapManager;
struct LC_InfoCursorData;
class LC_GraphicViewport;
class LC_Formatter;
class LC_OverlayInfoCursor;
struct LC_InfoCursorOverlayPrefs;
class LC_ActionContext;
class RS_Graphic;
class RS_Entity;
class RS_GraphicView;
class RS_Preview;
class RS_Document;
class QMouseEvent;
class RS_EntityContainer;

/**
  * This class holds information on how to snap the mouse.
  *
  * @author Kevin Cox
  */
struct RS_SnapMode {
    /* SnapModes for RS_SnapMode to Int conversion and vice versa
     *
     * The conversion is only used for save/restore of active snap modes in application settings.
     * Don't change existing mode order, because this will mess up settings on upgrades
     *
     * When adding new values, take care for correct implementation in \p toInt() and \p fromInt()
     */
    enum SnapModes {
        SnapIntersection   = 1 << 0,
        SnapOnEntity       = 1 << 1,
        SnapCenter         = 1 << 2,
        SnapDistance       = 1 << 3,
        SnapMiddle         = 1 << 4,
        SnapEndpoint       = 1 << 5,
        SnapGrid           = 1 << 6,
        SnapFree           = 1 << 7,
        RestrictHorizontal = 1 << 8,
        RestrictVertical   = 1 << 9,
        RestrictOrthogonal = RestrictHorizontal | RestrictVertical,
        SnapAngle          = 1 << 10,
        SnapVisual         = 1 << 11,
        SnapVisualSurvive  = 1 << 12
    };

    bool snapIntersection{false}; //< Whether to snap to intersections or not.
    bool snapOnEntity{false}; //< Whether to snap to entities or not.
    bool snapCenter{false}; //< Whether to snap to centers or not.
    bool snapDistance{false}; //< Whether to snap to distance from endpoints or not.
    bool snapMiddle{false}; //< Whether to snap to midpoints or not.
    bool snapEndpoint{false}; //< Whether to snap to endpoints or not.
    bool snapGrid{false}; //< Whether to snap to grid or not.
    bool snapFree{false}; //< Whether to snap freely
    bool snapAngle{false}; //< Whether to snap along line under certain angle
    bool snapVisual{false}; // whether visual snap enabled or not
    bool snapVisualSurvive{false}; // whether visual snap solution survives change of action's state

    RS2::SnapRestriction restriction{RS2::RestrictNothing}; /// The restriction on the free snap.

    /**
      * Disable all snapping.
      *
      * This effectively puts the object into free snap mode.
      *
      * @returns A reference to itself.
      */
    const RS_SnapMode& clear();
    bool operator ==(const RS_SnapMode& rhs) const;
    bool operator !=(const RS_SnapMode& rhs) const;

    static unsigned toInt(const RS_SnapMode& s); //< convert to int, to save settings
    static RS_SnapMode fromInt(unsigned int); //< convert from int, to restore settings
};

using EntityTypeList = QList<RS2::EntityType>;

/**
 * This class is used for snapping functions in a graphic view.
 * Actions are usually derived from this base class if they need
 * to catch entities or snap to coordinates. Use the methods to
 * retrieve a graphic coordinate from a mouse coordinate.
 *
 * Possible snapping functions are described in RS_SnapMode.
 *
 * @author Andrew Mustun
 */
class RS_Snapper : public QObject {
    Q_OBJECT public:
    explicit RS_Snapper(LC_ActionContext* actionContext, QObject* parent = nullptr);
    ~RS_Snapper() override;
    void init();
    //!
    //! \brief finish stop using snapper
    //!
    virtual void finish();

    /**
     * @return Pointer to the entity which was the key entity for the
     * last successful snapping action. If the snap mode is "end point"
     * the key entity is the entity whose end point was caught.
     * If the snap mode didn't require an entity (e.g. free, grid) this
     * method will return NULL.
     */
    RS_Entity* getKeyEntity() const {
        return m_keyEntity;
    }

    /** Sets a new snap mode. */
    void setSnapMode(const RS_SnapMode& snapMode);
    const RS_SnapMode* getSnapMode() const;
    RS_SnapMode* getSnapMode();

    /** Sets a new snap restriction. */
    [[deprecated]] void setSnapRestriction(RS2::SnapRestriction /*snapRes*/) {
        //this->snapRes = snapRes;
    }

    /**
     * Sets the snap range in pixels for catchEntity().
     *
     * @see catchEntity()
     */
    void setSnapRange(const int r) {
        m_catchEntityGuiRange = r;
    }

    /**manually set snapPoint*/
    bool isSnapToGrid() const;
    /**
     * Suspends this snapper while another action takes place.
     */
    virtual void suspend();

    /**
     * Resumes this snapper after it has been suspended.
     */
    virtual void resume();
    void hideSnapOptions() const;
    virtual void drawSnapper();
    void drawInfoCursor();
    bool hasNonDefaultAnglesBasis() const;

    LC_GraphicViewport* getViewPort() const {
        return m_viewport;
    }

    LC_ActionContext* getActionContext() const {
        return m_actionContext;
    }

    int getSnapMiddlePoints() const;
    double getSnapDistance() const;

    bool hasVisualSnap();
    void stopVisualSnap() const;
    void removePrevioustVisualSnapAddition();
    double getAngleStep() {return m_snapToAngleStep;}
    const RS_Vector& getRelativeZero() const;
    virtual void refreshBySettings();
    void lockVisualSnap(bool performLock) const;
protected:
    struct ImpData {
        RS_Vector snapCoord{false};
        RS_Vector snapSpot{false};
        double angle = 0.;
        RS2::SnapType snapType = RS2::SnapType::GRID;
        RS2::SnapType restrictedSnapType = RS2::SnapType::GRID;
        int restriction = RS2::RestrictNothing;
        RS_Entity* entity{nullptr};
        RS_Entity* entityOther{nullptr};
        RS2::LC_VisualSnapIntersectionInfo visualSnapType;

        void update(const RS_Vector& snap, RS2::SnapType type, RS_Entity* ent = nullptr, RS_Entity* ent1 = nullptr) {
            snapSpot = snap;
            snapType = type;
            entity = ent;
            entityOther = ent1;
        }
    };

    void deleteSnapper() const;
    void deleteInfoCursor() const;
    double getSnapRange() const;
    RS_Document* m_document = nullptr;
    RS_GraphicView* m_graphicView = nullptr;
    LC_GraphicViewport* m_viewport = nullptr;
    RS_Entity* m_keyEntity = nullptr;
    RS_SnapMode m_snapMode{};
    LC_ActionContext* m_actionContext{nullptr};

    double m_distanceBeforeSwitchToFreeSnap{5.0}; //< The distance to snap before defaulting to free snapping.
    double m_minGridCellSnapFactor = 0.25;
    /**
     * Snap distance for snapping to points with a
     * given distance from endpoints.
     */
    double m_snapDistance = 1.;
    /**
     * Snap to equidistant middle points
     * default to 1, i.e., equidistant to start/end points
     */
    int m_middlePoints = 1;
    /**
     * Snap range for catching entities. In GUI units
     */
    int m_catchEntityGuiRange = 32;
    bool m_finished{false};

    LC_InfoCursorOverlayPrefs* m_infoCursorOverlayPrefs = nullptr;
    std::unique_ptr<LC_InfoCursorData> m_infoCursorOverlayData;

    LC_Formatter* m_formatter{nullptr};
    bool m_ignoreSnapToGridIfNoGrid = false;

    std::unique_ptr<LC_VisualSnapManager> m_visualSnapManager;

    std::unique_ptr<ImpData> m_impData;
    struct Indicator;
    std::unique_ptr<Indicator> m_snapIndicator;

    double m_snapToAngleStep;
    bool m_angleSnapSnapToGridLinesIfGrid = true;

    RS_Vector toGraph(const QMouseEvent* e) const;
    void updateCoordinateWidget(const RS_Vector& abs, const RS_Vector& rel) const;
    void updateCoordinateWidgetByRelZero(const RS_Vector& abs) const;
    void updateCoordinateWidgetFormat() const;
    void invalidateSnapSpot() const;
    QString getCurrentSnapName() const;
    QString getSnapName(RS2::SnapType snapType) const;
    QString getVisualSnapName(RS2::VisualSnapGuideEntityType entityType, double rayAngle) const;
    QString getRestrictionName(int restriction);
    QString getCurrentRestrictionName();
    void preparePositionsInfoCursorOverlay(const RS_Vector& abs, const RS_Vector& relative) const;
    LC_OverlayInfoCursor* obtainInfoCursor() const;
    LC_InfoCursorOverlayPrefs* getInfoCursorOverlayPrefs() const;

    RS_Vector doSnapToAngle(const RS_Vector& currentCoord, const RS_Vector& referenceCoord, double angularResolution);

    QString formatLinear(double value) const;
    QString formatWCSAngle(double wcsAngle) const;
    QString formatAngleRaw(double angle) const;
    QString formatVector(const RS_Vector& value) const;
    QString formatVectorWCS(const RS_Vector& value) const;
    QString formatRelative(const RS_Vector& value) const;
    QString formatPolar(const RS_Vector& value) const;
    QString formatRelativePolar(const RS_Vector& wcsAngle) const;
    void forceUpdateInfoCursor(const RS_Vector& pos) const;
    bool isInfoCursorForModificationEnabled() const;
    double toWorldAngle(double ucsAbsAngle) const;
    double toWorldAngleDegrees(double ucsAbsAngleDegrees) const;
    double toUCSAngle(double wcsAngle) const;
    double toUCSBasisAngle(double wcsAngle) const;
    double toUCSBasisAngleDegrees(double wcsAngle) const;
    double ucsAbsToBasisAngle(double ucsAbsAngle) const;
    double ucsBasisToAbsAngle(double ucsRelAngle) const;
    double adjustRelativeAngleSignByBasis(double relativeAngle) const;
    double toWorldAngleFromUCSBasisDegrees(double ucsBasisAngleDegrees) const;
    double toWorldAngleFromUCSBasis(double ucsBasisAngle) const;
    RS_Vector toWorld(const RS_Vector& ucsPos) const;
    RS_Vector toWorldDelta(const RS_Vector& ucsDelta) const;
    RS_Vector toUCS(const RS_Vector& worldPos) const;
    RS_Vector toUCSDelta(const RS_Vector& worldDelta) const;
    void calcRectCorners(const RS_Vector& worldCorner1, const RS_Vector& worldCorner3, RS_Vector& worldCorner2,
                         RS_Vector& worldCorner4) const;
    double getCatchDistance(double catchDistance, int catchEntityGuiRange) const;
    double toGuiDX(double wcsDX) const;
    double toGraphDX(int wcsDX) const;
    void redraw(RS2::RedrawMethod method = RS2::RedrawMethod::RedrawAll) const;
    void redrawImmediately(RS2::RedrawMethod method) const;
    void redrawDrawing() const;
    void redrawAll() const;
    void enableCoordinateInput() const;
    void disableCoordinateInput() const;
    void initSettings();
    virtual void initFromSettings();
    void updateSnapAngleStep();
    virtual void initFromGraphic(RS_Graphic* graphic);
    virtual bool isInVisualSnapStatus(int status);
    bool isClearVisualSnapByRMB();

    virtual void onVisualSnapPointRegistered([[maybe_unused]]LC_VisualSnapVertex* point, [[maybe_unused]]bool remove) {
    };

    virtual void onVisualSnapEntityRegistered([[maybe_unused]]RS_Entity* point);

    virtual void onVisualSnapSolutionRefresh() {
    };
    virtual bool isVisualSnapApplicable();
    virtual void clearVisualSnap() const;

    void snapEndpoint(const RS_Vector& mouseCoord, double& ds2Min) const;
    void snapCenter(const RS_Vector& mouseCoord, double& ds2Min) const;
    void snapMiddle(const RS_Vector& mouseCoord, double& ds2Min);
    void snapDistance(RS_Vector mouseCoord, double& ds2Min);
    void snapIntersection(const RS_Vector& mouseCoord, double& ds2Min) const;
    void snapOnEntity(const RS_Vector& mouseCoord, double& ds2Min);
    void snapGrid(const RS_Vector& mouseCoord, double ds2Min) const;
    RS_Vector snapVisualRayOrLine(const RS_Vector& mouseCoord, RS_Entity* entity, double& dist);
    bool snapVisual(const RS_Vector& mouseCoord, RS_Entity** restrictingEntity, RS2::VisualSnapGuideEntityType& restrictingType);
    RS_Vector snapFree(const RS_Vector& coord);
    RS_Vector snapGrid(const RS_Vector& coord) const;
    RS_Vector snapGrid(const RS_Vector& coord, const RS_Vector& rayStart, const RS_Vector& rayEnd) const;
    RS_Vector snapGrid(const RS_Vector& coord, RS_Entity*) const;
    RS_Vector snapEndpoint(const RS_Vector& coord, RS_Entity** entity) const;
    RS_Vector snapOnEntity(const RS_Vector& coord, RS_Entity** entity);
    RS_Vector snapCenter(const RS_Vector& coord, RS_Entity** entity) const;
    RS_Vector snapMiddle(const RS_Vector& coord) const;
    RS_Vector snapDist(const RS_Vector& coord) const;
    RS_Vector snapIntersection(const RS_Vector& coord, RS_Entity** entity, RS_Entity** otherEntity) const;
    RS_Vector snapToAngle(const RS_Vector& currentCoord, const RS_Vector& referenceCoord, double angularResolution = 15.);
    RS_Vector obtainEndPointForAngleSnap(const RS_Vector& currentCoord, const RS_Vector& referenceCoord, const double angularResolution,
                                         double& wcsAngleSnapped, double& ucsAngleSnapped);
    RS_Vector snapToRelativeAngle(double baseAngle, const RS_Vector& currentCoord, const RS_Vector& referenceCoord,
                                  double angularResolution = 15.);

    RS_Vector setSnapPoint(const RS_Vector& coord, bool setSpot = false);
    RS_Vector snapPoint(const QMouseEvent* e);
    RS_Vector snapFree(const QMouseEvent* e) const;

    RS_Vector restrictOrthogonal(const RS_Vector& coord) const;
    RS_Vector restrictHorizontal(const RS_Vector& coord) const;
    RS_Vector restrictVertical(const RS_Vector& coord) const;
    RS_Vector restrictHorizontal(const RS_Vector& base, const RS_Vector& coord) const;
    RS_Vector restrictVertical(const RS_Vector& base, const RS_Vector& coord) const;
    RS_Vector restrictAngle(const RS_Vector& basePoint, const RS_Vector& snap, double angle) const;
    RS_Entity* catchEntity(const RS_Vector& pos, RS2::ResolveLevel level = RS2::ResolveNone) const;
    RS_Entity* catchEntity(const QMouseEvent* e, RS2::ResolveLevel level = RS2::ResolveNone) const;
    // catch Entity closest to pos and of the given entity type of enType, only search for a particular entity type
    RS_Entity* catchEntity(const RS_Vector& pos, RS2::EntityType enType, RS2::ResolveLevel level = RS2::ResolveNone) const;
    RS_Entity* catchEntity(const QMouseEvent* e, RS2::EntityType enType, RS2::ResolveLevel level = RS2::ResolveNone) const;
    RS_Entity* catchEntity(const QMouseEvent* e, const EntityTypeList& enTypeList, RS2::ResolveLevel level = RS2::ResolveNone) const;
    RS_Entity* catchEntity(const RS_Vector& pos, const EntityTypeList& enTypeList, RS2::ResolveLevel level = RS2::ResolveNone) const;

    virtual void resumeRelativeInputWidget() {};
    virtual void suspendRelativeInputWidget() {};
    friend LC_VisualSnapManager;
};
#endif
