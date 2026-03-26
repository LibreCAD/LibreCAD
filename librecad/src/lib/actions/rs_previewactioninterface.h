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

#ifndef RS_PREVIEWACTIONINTERFACE_H
#define RS_PREVIEWACTIONINTERFACE_H

#include "dxf_format.h"
#include "rs_actioninterface.h"
#include "rs_entity.h"
#include "rs_vector.h"
#include "rs_preview.h"

class RS_Point;
class LC_OverlayDrawable;
class LC_ActionInfoMessageBuilder;
class LC_Highlight;
class LC_RefEllipse;

class RS_Arc;
class RS_Circle;
class RS_ConstructionLine;
class RS_Ellipse;
class RS_Line;
struct RS_ArcData;
struct RS_CircleData;
struct RS_EllipseData;
struct RS_LineData;

struct LC_MouseEvent{
    RS_Vector snapPoint;
    RS_Vector graphPoint;
    QPoint uiPosition;
    bool isControl = false;
    bool isShift = false;
    bool isAlt = false;
    QMouseEvent* originalEvent = nullptr;
};


/**
 * This is the interface that must be implemented for all
 * action classes which need a preview.
 *
 * @author Andrew Mustun
 */
class RS_PreviewActionInterface : public RS_ActionInterface {
public:
    void init(int status) override;
    void finish() override;
    void suspend() override;
    void resume() override;
    void trigger() override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void addSnappedPointToVisualSnap(const RS_Vector& v, bool clearOther = false);
    void keyPressEvent(QKeyEvent* e) override;
    QStringList getAvailableCommands() override;
    bool isClearVisualSnapMarks();
    void setStatus(int status) override;
protected:
    RS_PreviewActionInterface(const QString& actionName,LC_ActionContext *actionContext,RS2::ActionType actionType = RS2::ActionNone);
    ~RS_PreviewActionInterface() override;
    std::unique_ptr<LC_ActionInfoMessageBuilder> m_msgBuilder;
    // fixme - sand - tmp -  move to overlay!!!
    int m_angleSnapMarkerSize = 20;
    /**
     * Preview that holds the entities to be previewed.
     */
    std::unique_ptr<RS_Preview> m_preview;
    bool m_hasPreview = true;//whether preview is in use

    std::unique_ptr<LC_Highlight> m_highlight;

    double m_refPointSize = 2.0;
    int m_refPointMode = DXF_FORMAT_PDMode_EncloseSquare(DXF_FORMAT_PDMode_CentreDot);
    bool m_showRefEntitiesOnPreview = false;
    bool m_highlightEntitiesOnHover = false;
    bool m_highlightEntitiesRefPointsOnHover = false;
    bool m_doNotAllowNonDecimalAnglesInput = false;
    LC_MouseEvent m_lastMouseMoveEvent;
    /**
    * This is "major" status of action - it is used for determining, to which status state should be changed after various intermediate statuses (mostly,
    * this is needed for support of command events);
    */
    int m_mainStatus  = 0;

    // main status support
    void setMainStatus(const int status) {m_mainStatus = status; setStatus(status);}
    void restoreMainStatus(){setStatus(m_mainStatus);}


    virtual void doTrigger(){}
    void deletePreview();
    void deleteHighlights() const;
    void deletePreviewAndHighlights();
    void drawPreview();
    void drawHighlights() const;
    void drawPreviewAndHighlights();
    bool isVisualSnapApplicable();

    void addToHighlights(RS_Entity *e, bool enable = true) const;

    bool trySnapToRelZeroCoordinateEvent(const LC_MouseEvent *e);

    RS_Vector getRelZeroAwarePoint(const LC_MouseEvent *e, const RS_Vector &pos) const;
    RS_Vector getSnapAngleAwarePoint(const LC_MouseEvent *e, const RS_Vector &basepoint, const RS_Vector &pos, bool drawMark = false);
    RS_Vector getSnapAngleAwarePoint(const LC_MouseEvent *e, const RS_Vector &basepoint, const RS_Vector &pos, bool drawMark, bool force);
    RS_Vector getFreeSnapAwarePoint(const LC_MouseEvent *e, const RS_Vector &pos) const;

    void addOverlay(LC_OverlayDrawable* drawable, RS2::OverlayGraphics position) const;

    void previewEntity(const RS_Entity *en) const;
    RS_Circle* previewCircle(const RS_CircleData& circleData) const;
    RS_Arc *previewArc(const RS_ArcData &arcData) const;
    RS_Ellipse *previewEllipse(const RS_EllipseData &ellipseData) const;
    RS_Point* previewPoint(const RS_Vector &coord) const;
    void previewLine(const RS_Vector &start, const RS_Vector &end) const;
    RS_Line* obtainPreviewLine(const RS_Vector& start, const RS_Vector& end) const;
    RS_Line* previewLine(const RS_LineData &data) const;
    void previewRefLine(const RS_Vector &start, const RS_Vector &end) const;
    RS_Line* obtainPreviewRefLine(const RS_Vector &start, const RS_Vector &end) const;
    void previewRefConstructionLine(const RS_Vector &start, const RS_Vector &end) const;
    RS_ConstructionLine* obtainPreviewRefConstructionLine(const RS_Vector& start, const RS_Vector& end) const;
    void previewRefLines(const std::vector<RS_LineData>& points) const;
    void previewRefSelectableLine(const RS_Vector &start, const RS_Vector &end) const;
    void previewRefPoint(const RS_Vector &coord) const;
    void previewRefSelectablePoint(const RS_Vector &coord) const;
    void previewRefPoints(const std::vector<RS_Vector>& points) const;
    void previewRefArc(const RS_Vector &center, const RS_Vector &startPoint, const RS_Vector &mouse, bool determineReversal) const;
    RS_Arc* obtainPreviewRefArc(const RS_Vector& center, const RS_Vector& startPoint, const RS_Vector& mouse, bool determineReversal) const;
    RS_Arc* previewRefArc(bool reversed, const RS_Vector &center, const RS_Vector &startPoint, const RS_Vector &mouse) const;
    RS_Circle* previewRefCircle(const RS_Vector &center, double radius) const;
    void previewRefArc(const RS_ArcData &arcData) const;
    RS_Arc* obtainPreviewRefArc(const RS_ArcData& arcData) const;
    void previewRefEllipse(const RS_EllipseData &arcData) const;
    LC_RefEllipse* obtainPreviewRefEllipse(const RS_EllipseData& arcData) const;

    void initRefEntitiesMetrics();

    void highlightHover(const RS_Entity* e) const;
    void highlightHoverWithRefPoints(const RS_Entity* e, bool value) const;
    void highlightSelected(RS_Entity *e, bool enable=true) const;

    virtual void moveRelativeZero(const RS_Vector &zero);
    void markRelativeZero() const;

    void createVisualSnapGuidesForCurrentPoint();

    bool is(const RS_Entity* e, RS2::EntityType type) const;
    bool isLine(const RS_Entity*  e) const{return is(e, RS2::EntityLine);}
    bool isPolyline(const RS_Entity*  e) const{return is(e, RS2::EntityPolyline);}
    bool isCircle(const RS_Entity*  e) const {return is(e, RS2::EntityCircle);}
    bool isArc(const RS_Entity*  e) const {return is(e, RS2::EntityArc);}
    bool isInsert(const RS_Entity*  e) const {return is(e, RS2::EntityInsert);}
    bool isEllipse(const RS_Entity*  e) const {return is(e, RS2::EntityEllipse);}
    bool isAtomic(const RS_Entity* e) const {return e != nullptr && e->isAtomic();}

    void previewSnapAngleMark(const RS_Vector &center, double angle) const;
    void previewSnapAngleMark(const RS_Vector &center, const RS_Vector &refPoint) const;
    void previewSnapAngleMark(const RS_Vector& center, const double angle, double angleBase, bool isAnglesCounterClockWise) const;

    RS_Entity *catchModifiableEntity(const LC_MouseEvent *e, const EntityTypeList &enTypeList) const;
    RS_Entity *catchModifiableEntity(const LC_MouseEvent *e, RS2::EntityType enType) const;

    RS_Entity *catchModifiableEntity(const RS_Vector &coord, RS2::EntityType enType) const;

    RS_Entity *catchEntityByEvent(const LC_MouseEvent *e, RS2::ResolveLevel level = RS2::ResolveNone) const;
    RS_Entity *catchEntityByEvent(const LC_MouseEvent *e, RS2::EntityType enType, RS2::ResolveLevel level = RS2::ResolveNone) const;
    RS_Entity *catchEntityByEvent(const LC_MouseEvent *e, const EntityTypeList &enTypeList, RS2::ResolveLevel level = RS2::ResolveNone) const;

    RS_Entity* catchAndDescribe(const LC_MouseEvent *e, const EntityTypeList &enTypeList, RS2::ResolveLevel level) const;
    RS_Entity* catchAndDescribe(const LC_MouseEvent* e, RS2::ResolveLevel level  = RS2::ResolveNone) const;
    RS_Entity* catchAndDescribe(const LC_MouseEvent *e, RS2::EntityType enType, RS2::ResolveLevel level = RS2::ResolveNone) const;

    RS_Entity* catchAndDescribe(const RS_Vector &pos, RS2::ResolveLevel level = RS2::ResolveNone) const;

    RS_Entity* catchModifiableAndDescribe(const LC_MouseEvent *e, RS2::EntityType enType) const;
    RS_Entity* catchModifiableAndDescribe(const LC_MouseEvent *e, const EntityTypeList &enTypeList) const;

    LC_ActionInfoMessageBuilder& msg(const QString& name, const QString& value) const;
    LC_ActionInfoMessageBuilder& msg(const QString& name) const;
    LC_ActionInfoMessageBuilder& msgStart() const;

    QString obtainEntityDescriptionForInfoCursor(const RS_Entity* e, RS2::EntityDescriptionLevel level) const;
    void prepareEntityDescription(const RS_Entity* entity, RS2::EntityDescriptionLevel level) const;
    void appendInfoCursorZoneMessage(const QString& message, int zoneNumber, bool replaceContent) const;
    void appendInfoCursorEntityCreationMessage(const QString& message) const;

    void previewToCreateCircle(const RS_CircleData &circleData) const;
    RS_Arc *previewToCreateArc(const RS_ArcData &arcData) const;
    RS_Line *previewToCreateLine(const RS_LineData &lineData) const;
    RS_Line *previewToCreateLine(const RS_Vector &start, const RS_Vector &end) const;
    RS_Ellipse *previewToCreateEllipse(const RS_EllipseData &ellipseData) const;
    RS_Point *previewToCreatePoint(const RS_Vector &coord) const;
    void previewEntityToCreate(const RS_Entity* en, bool addToPreview = true) const;

    void fireCoordinateEventForSnap(const LC_MouseEvent *e);

    void onMouseLeftButtonRelease(int status, QMouseEvent *e) override;
    void onMouseRightButtonRelease(int status, QMouseEvent *e) override;
    void onMouseLeftButtonPress(int status, QMouseEvent *e) override;
    void onMouseRightButtonPress(int status, QMouseEvent *e) override;

    virtual void onMouseMoveEvent(int status, const LC_MouseEvent* event);
    virtual void onMouseLeftButtonRelease(int status, const LC_MouseEvent* e);
    virtual void onMouseRightButtonRelease(int status, const LC_MouseEvent* e);
    virtual void onMouseLeftButtonPress(int status, const LC_MouseEvent* e);
    virtual void onMouseRightButtonPress(int status, const LC_MouseEvent* e);
    virtual QStringList doGetAvailableCommands(int status);

    bool parseToWCSAngle(const QString &c, double &wcsAngleRad) const;
    bool parseToUCSBasisAngle(const QString &c, double& ucsBasisAngleRad) const;
    bool parseToRelativeAngle(const QString&c, double &ucsBasisAngleRad) const;
    double evalAngleValue(const QString &c, bool *ok) const;
    void initFromSettings() override;

    void onVisualSnapPointRegistered(LC_VisualSnapVertex* point, bool remove) override;
    void onVisualSnapEntityRegistered(RS_Entity* point) override;
    void onVisualSnapSolutionRefresh() override;

    virtual bool doCheckMayTrigger();
private:
    LC_MouseEvent toLCMouseMoveEvent(QMouseEvent *e);
    friend LC_ActionInfoMessageBuilder;
};


#endif
