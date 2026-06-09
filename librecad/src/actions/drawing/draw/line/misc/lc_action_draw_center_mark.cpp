/****************************************************************************
**
* Action that draws a cross in the center of selected arc, circle,
* ellipse or ellipse arc

Copyright (C) 2024 LibreCAD.org
Copyright (C) 2024 sand1024

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**********************************************************************/

#include "lc_action_draw_center_mark.h"

#include "lc_center_mark_options_filler.h"
#include "lc_center_mark_options_widget.h"
#include "rs_document.h"
#include "rs_ellipse.h"
#include "rs_line.h"
#include "rs_pen.h"

/**
 * Structure that contains information about cross lines
 */
struct LC_CrossData {
    LC_CrossData() {}

    LC_CrossData(const RS_Vector& horPoint1,
                 const RS_Vector& horPoint2,
                 const RS_Vector& vertPoint1,
                 const RS_Vector& vertPoint2,
                 const RS_Vector& center) :
        horizontal( horPoint1, horPoint2),
        vertical( vertPoint1, vertPoint2),
        centerPoint(center){}

    // horizontal line
    RS_LineData horizontal;
    // vertical line
    RS_LineData vertical;
    // center point
    RS_Vector centerPoint;
};

// todo - it is possible to duplicate UI by commands, but it seems that's not too practical
// todo - the action should be initiated by mouse anyway, so in order to make the action fully scriptable,
// todo - it is necessary either have command for entity selection or skip commands for now

/**
 * Simple action that draws a cross located in the center of selected circle, arc, ellipse or ellipse arc.
 * Size and angle of cross is controlled by options.
 * @param actionContext
 */
LC_ActionDrawCenterMark::LC_ActionDrawCenterMark(LC_ActionContext *actionContext)
    :LC_AbstractActionWithPreview("ActionDrawCenterMark", actionContext,RS2::ActionDrawCenterMark){
}

LC_ActionDrawCenterMark::~LC_ActionDrawCenterMark() = default;

void LC_ActionDrawCenterMark::doSaveOptions() {
    save("X", m_lenX);
    save("Y", m_lenY);
    save("Angle", m_ucsBasisAngleDegrees);
    save("Mode", m_crossSizeMode);
}

void LC_ActionDrawCenterMark::doLoadOptions() {
    m_lenX = loadDouble("X", 1.0);
    m_lenY = loadDouble("Y", 1.0);
    m_ucsBasisAngleDegrees = loadDouble("Angle", 0.0);
    m_crossSizeMode = loadInt("Mode", 1);
}

void LC_ActionDrawCenterMark::doInitWithContextEntity(RS_Entity* contextEntity, [[maybe_unused]]const RS_Vector& clickPos) {
    if (isAcceptSelectedEntityToTriggerOnInit(contextEntity)) {
        m_entity = contextEntity;
    }
}

void LC_ActionDrawCenterMark::collectEntitiesForTriggerOnInit(QList<RS_Entity*> &entitiesForTrigger) {
    if (m_entity == nullptr) {
        LC_AbstractActionWithPreview::collectEntitiesForTriggerOnInit(entitiesForTrigger);
    }
    else {
        entitiesForTrigger << m_entity;
    }
}

// support of creation cross for already selected entities on action invocation
bool LC_ActionDrawCenterMark::doCheckMayTriggerOnInit(const int status){
    return status == SetEntity;
}

bool LC_ActionDrawCenterMark::isAcceptSelectedEntityToTriggerOnInit(RS_Entity *pEntity){
    // from all selected entities, we'll accept only ones that suits for the cross creation
    const int rtti = pEntity->rtti();
    return rtti == RS2::EntityCircle || rtti == RS2::EntityEllipse || rtti == RS2::EntityArc;
}

void LC_ActionDrawCenterMark::doCreateEntitiesOnTrigger(RS_Entity *en, QList<RS_Entity *> &list){
    const LC_CrossData crossData = createCrossDataForEntity(en);
    addCrossDataEntities(list, crossData);
}

bool LC_ActionDrawCenterMark::isSetActivePenAndLayerOnTrigger() {
    return false;
}

void LC_ActionDrawCenterMark::setupCrossLinePenAndLayer(RS_Line* line) const {
    line->setLayerToActive(); // fixme - sand - change to some annotation layer?
    const RS2::LineType lineType = getLineTypeForCenterLine();
    RS_Pen pen = m_document->getActivePen();
    if (lineType != RS2::LineTypeUnchanged) {
        pen.setLineType(lineType);
    }
    line->setPen(pen);
}

RS2::LineType LC_ActionDrawCenterMark::getLineTypeForCenterLine() const{
    return RS2::CenterLine2; // fixme - retrieve from settings (CENTERLTYPE)
}

/**
 * Creates entities (2 lines) that will added as result of trigger to the drawing
 */

bool LC_ActionDrawCenterMark::doTriggerEntitiesPrepare(LC_DocumentModificationBatch& ctx) {
    doCreateEntitiesOnTrigger(m_entity, ctx.entitiesToAdd);
    return true;
}

/**
 * Creates line entities based on cross data and adds them to the list
 * @param list
 * @param crossData
 */
void LC_ActionDrawCenterMark::addCrossDataEntities(QList<RS_Entity *> &list, const LC_CrossData &crossData) const{
    // fixme - support more settings for CenterMark - like in AutoCAD.
    // fixme- sand - CENTERMARK size, gap, etc...
    // fixme - sand - rework drawing of the centermark to fit these settings.

    // create horizontal and vertical lines

    const RS_LineData horizontalData = crossData.horizontal;
    auto *horizontalLine = new RS_Line(m_document, horizontalData);

    setupCrossLinePenAndLayer(horizontalLine);

    list << horizontalLine;

    const RS_LineData verticalData = crossData.vertical;
    auto *verticalLine = new RS_Line(m_document, verticalData);

    setupCrossLinePenAndLayer(verticalLine);

    list << verticalLine;

    /*
    // todo - this is temproary code, yet don't delete it so far!!
    // Drawing the centermark from center point using 2 segments for each direction

    RS_LineData horizontalData = crossData.horizontal;

    RS_LineData lineData;
    lineData.startpoint = crossData.centerPoint;

    lineData.endpoint = horizontalData.startpoint;
    auto *hLine1 = new RS_Line(m_container, lineData);
    setupCrossLinePenAndLayer(hLine1);
    list << hLine1;

    lineData.endpoint = horizontalData.endpoint;
    auto *hLine2 = new RS_Line(m_container, lineData);
    setupCrossLinePenAndLayer(hLine2);
    list << hLine2;

    RS_LineData verticalData = crossData.vertical;

    lineData.endpoint = verticalData.startpoint;
    auto *vLine1 = new RS_Line(m_container, lineData);
    setupCrossLinePenAndLayer(vLine1);
    list << vLine1;

    lineData.endpoint = verticalData.endpoint;
    auto *vLine2 = new RS_Line(m_container, lineData);
    setupCrossLinePenAndLayer(vLine2);
    list << vLine2;
    */
}

bool LC_ActionDrawCenterMark::doCheckMayTrigger(){
    return m_entity != nullptr; // we need entity selected
}

RS_Vector LC_ActionDrawCenterMark::doGetRelativeZeroAfterTrigger(){
    return m_entity->getCenter(); // just put in center of circle
}

void LC_ActionDrawCenterMark::doAfterTrigger(){
    m_entity = nullptr; // cleanup
}

/**
 * Major function that calculates sizes for cross lines based on given options and selected entity properties
 * @return data for cross creation
 */
LC_CrossData LC_ActionDrawCenterMark::createCrossDataForEntity(RS_Entity* ent) const{
    Q_ASSERT(ent != nullptr);
    RS_Vector cp = ent->getCenter();
    double lengthX=0., lengthY=0.;
    double ellipseAngle = 0.0;
    RS2::EntityType rtti = ent->rtti();
    bool arcShape = rtti == RS2::EntityArc;
    bool isCircle = rtti == RS2::EntityCircle;
    bool isEllipse = rtti == RS2::EntityEllipse;
    bool isEllipseArcShape = false;

    // check whether we are in ellipse arc
    RS_Ellipse* ellipse = nullptr;
    if (isEllipse){
        ellipse = static_cast<RS_Ellipse *>(ent);
        isEllipseArcShape = ellipse->isEllipticArc();
    }

    double lenYToUse = m_lenY;

    if (std::abs(m_lenY) < RS_TOLERANCE){
        lenYToUse = m_lenX;
    }

    // first, determine size of cross based on specified mode
    switch (m_crossSizeMode) {
        case CROSS_SIZE_EXTEND: // cross should extend shape on specified length
            if (arcShape || isCircle){
                // for arc and circle we rely on radius
                double radius = ent->getRadius();
                lengthX = radius + m_lenX;
                lengthY = radius + lenYToUse;
                ellipseAngle = 0.0; // we'll draw for circle, so no ellipse angle there

            } else if (ellipse != nullptr) {
                // for ellipses - we rely on axis radiuses
                lengthX = ellipse->getMajorRadius() + m_lenX;
                lengthY = ellipse->getMinorRadius() + lenYToUse;
                ellipseAngle = ellipse->getAngle();
            }
            else{
                // should not be possible unless new entities are added
                lengthX = -1;
                lengthY = -1;
            }
            break;
        case CROSS_SIZE_LENGTH:  // cross should have fixed length
            // divide length by 2 because + operator on vector
            // adds the length to both ends of the line.
            lengthX = m_lenX / 2;
            lengthY = lenYToUse / 2;
            if (isEllipse || isEllipseArcShape){
                ellipseAngle = ellipse->getAngle();
            } else {
                ellipseAngle = 0.0;
            }
            break;
        case CROSS_SIZE_PERCENT:  //Length is value in percents of radius
            if (arcShape || isCircle){ // for circle, it will be percent of radius
                double radius = ent->getRadius();
                lengthX = radius * m_lenX / 100.0;
                lengthY = radius * lenYToUse / 100.0;
                ellipseAngle = 0.0;
            } else if (isEllipse || isEllipseArcShape){  // for ellipse, calculate percents for each axis
                lengthX = ellipse->getMajorRadius() * m_lenX / 100.0;
                lengthY = ellipse->getMinorRadius() * lenYToUse / 100.0;
                ellipseAngle = ellipse->getAngle();
            }
            else{
                // should not be possible unless new entities are added
                lengthX = -1;
                lengthY = -1;
            }
            break;
        default:
            break;
    }

    auto v = RS_Vector();

    // points for lines
    RS_Vector horStart;
    RS_Vector horEnd;
    RS_Vector vertStart;
    RS_Vector vertEnd;


    double orientationAngle = toWorldAngleFromUCSBasisDegrees(m_ucsBasisAngleDegrees);
//    ellipseAngle = toUCSAngle(ellipseAngle);

    // determine start and end points for cross lines based on calculated lengths and angle

    // calculate horizontal line
    if (lengthX <= 0.0){ // check for zero
        horStart = cp;
        horEnd = cp;
    } else {
        // normal line - taking care of given angle (plus, take care of ellipse angle, if needed)
        // calculate start point
        v.setPolar(lengthX, orientationAngle + M_PI + ellipseAngle);
        horStart = cp + v;
        // end point of horizontal line
        v.setPolar(lengthX, orientationAngle + ellipseAngle);
        horEnd = cp + v;
    }

    // calculate vertical line
    if (lengthY <= 0.0){
        vertStart = cp;
        vertEnd = cp;
    } else {
        v.setPolar(lengthY, orientationAngle - M_PI / 2 + ellipseAngle);
        vertStart = cp + v;
        v.setPolar(lengthY, orientationAngle + M_PI / 2 + ellipseAngle);
        vertEnd = cp + v;
    }
    // return result
    auto result = LC_CrossData(horStart, horEnd, vertStart, vertEnd, cp);
    return result;
}


bool LC_ActionDrawCenterMark::doCheckMayDrawPreview([[maybe_unused]] const LC_MouseEvent* event, const int status){
    return status == SetEntity;
}

/**
 * creates entities to show in preview
 * @param e
 * @param snap
 * @param list
 * @param status
 */
void LC_ActionDrawCenterMark::doPreparePreviewEntities(const LC_MouseEvent* e, [[maybe_unused]]RS_Vector &snap, QList<RS_Entity *> &list, [[maybe_unused]] int status){
    deleteSnapper();
    RS_Entity *en = catchAndDescribe(e, m_circleType, RS2::ResolveAll);
    // check whether entity is ok for drawing cross
    if (en != nullptr){
        bool isArc = en->isArc();
        if (!isArc){
            if (en->is(RS2::EntityEllipse)){
                // for ellipse arcs
                isArc = true;
            }
        }
        if (isArc){ // can draw
            // handle visual highlighting
            m_entity = en;

            highlightHover(en);

            // prepare data for preview
            const LC_CrossData crossData = createCrossDataForEntity(en);
            // create lines
            addCrossDataEntities(list, crossData);
            if (m_showRefEntitiesOnPreview) {
                // ref point
                createRefPoint(crossData.centerPoint, list);
            }
        }
    }
}

/**
 * Just invoke trigger on left mouse click
 * @param e
 * @param status
 * @param snapPoint
 */
void LC_ActionDrawCenterMark::doOnLeftMouseButtonRelease([[maybe_unused]] const LC_MouseEvent* e, const int status, [[maybe_unused]]const RS_Vector &snapPoint){
    if (status == SetEntity){
        trigger(); // just draw cross on click
        invalidateSnapSpot();
    }
}

void LC_ActionDrawCenterMark::updateActionPrompt(){
    switch (getStatus()) {
        case SetEntity:
            updatePromptTRCancel(tr("Select circle, arc or ellipse"));
            break;
        default:
            updatePrompt();
            break;
    }
}

bool LC_ActionDrawCenterMark::doUpdateAngleByInteractiveInput(const QString& tag, const double angleRad) {
    if (tag == "angle") {
        setCrossAngleDegrees(RS_Math::rad2deg(angleRad));
        return true;
    }
    return false;
}

bool LC_ActionDrawCenterMark::doUpdateDistanceByInteractiveInput(const QString& tag, const double distance) {
    if (tag == "x") {
        setXLength(distance);
        return true;
    }
    if (tag == "y") {
        setYLength(distance);
        return true;
    }
    return false;
}

/**
 * Returns mouse cursor for action
 * @param status
 * @return
 */
RS2::CursorType LC_ActionDrawCenterMark::doGetMouseCursor([[maybe_unused]]int status){
  return RS2::SelectCursor;
}

LC_ActionOptionsWidget* LC_ActionDrawCenterMark::createOptionsWidget(){
    return new LC_CenterMarkOptionsWidget();
}

LC_ActionOptionsPropertiesFiller* LC_ActionDrawCenterMark::createOptionsFiller() {
    return new LC_CenterMarkOptionsFiller();
}
