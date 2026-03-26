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

#include "lc_action_draw_polyline.h"

#include "lc_creation_arc.h"
#include "lc_polyline_options_widget.h"
#include "lc_polyline_options_filler.h"
#include "lc_undosection.h"
#include "muParser.h"

#include "rs_arc.h"
#include "rs_commandevent.h"
#include "rs_document.h"
#include "rs_line.h"
#include "rs_polyline.h"

#ifdef EMU_C99
#include "emu_c99.h"
#endif

LC_ActionDrawPolyline::LC_ActionDrawPolyline(LC_ActionContext *actionContext)
        :LC_UndoableDocumentModificationAction("ActionDrawPolyline",actionContext, RS2::ActionDrawPolyline)
        , m_actionData(std::make_unique<Points>()){
    reset();
}

LC_ActionDrawPolyline::LC_ActionDrawPolyline(const QString& name, LC_ActionContext* actionContext, RS2::ActionType actionType)
   :LC_UndoableDocumentModificationAction(name,actionContext, actionType)
        , m_actionData(std::make_unique<Points>()){
    reset();
}

LC_ActionDrawPolyline::~LC_ActionDrawPolyline() = default;

void LC_ActionDrawPolyline::doSaveOptions() {
    save("Mode", m_mode);
    save("Radius", m_radius);
    save("Angle", m_angleDegrees);
    save("Reversed", m_reversed);
}

void LC_ActionDrawPolyline::doLoadOptions() {
    int mode = loadInt("Mode", SegmentMode::Line);
    m_mode = static_cast<SegmentMode>(mode);
    m_radius = loadDouble("Radius", 10.0);
    m_angleDegrees = loadDouble("Angle", 90.0);
    m_reversed = loadBool("Reversed", false);
}

bool LC_ActionDrawPolyline::isInVisualSnapStatus(int status) {
    return (status == SetStartpoint) || (status == SetNextPoint);
}

void LC_ActionDrawPolyline::reset() const {
    m_actionData->polyline = nullptr;
    m_actionData->data = {};
    m_actionData->start = {};
    m_actionData->history.clear();
    m_actionData->bHistory.clear();
}

void LC_ActionDrawPolyline::init(const int status){
    reset();
    RS_PreviewActionInterface::init(status);
}


bool LC_ActionDrawPolyline::doTriggerModifications([[maybe_unused]]LC_DocumentModificationBatch& ctx) {
    if (m_actionData->polyline == nullptr) {
        return false;
    }
    // ctx+= m_actionData->polyline;

    moveRelativeZero(m_actionData->polyline->getEndpoint());
    return true;
}

void LC_ActionDrawPolyline::doTriggerCompletion([[maybe_unused]]bool success) {
    m_actionData->polyline = nullptr;
}

void LC_ActionDrawPolyline::onMouseMoveEvent(const int status, const LC_MouseEvent* e) {
    RS_Vector mouse = e->snapPoint;
    switch (status) {
        case SetStartpoint: {
            trySnapToRelZeroCoordinateEvent(e);
            break;
        }
        case SetNextPoint: {
            if (m_mode == Line){
                mouse = getSnapAngleAwarePoint(e, m_actionData->point, mouse, true);
            }
            if (m_actionData->point.valid){
                const bool alternateDirection = e->isControl;
                double bulge = 0.;
                if (alternateDirection && m_mode == ArcFixedAngle){
                    const int originalReversed = m_reversed;
                    m_reversed = m_reversed == -1 ? 1: -1;
                    bulge = solveBulge(mouse);
                    m_reversed = originalReversed;
                }
                else {
                    bulge = solveBulge(mouse);
                }

                if (fabs(bulge) < RS_TOLERANCE || m_mode == Line){
                    previewToCreateLine(m_actionData->point, mouse);
                    if (m_showRefEntitiesOnPreview) {
                        previewRefPoint(m_actionData->point);
                        previewRefSelectablePoint(mouse);
                    }
                } else {
                    RS_ArcData tmpArcData = m_actionData->arcData;
                    if (alternateDirection && m_mode != ArcFixedAngle){
                        tmpArcData.reversed = !tmpArcData.reversed;
                    }
                    const auto arc = previewToCreateArc(tmpArcData);
                    if (m_showRefEntitiesOnPreview) {
                        const RS_Vector &center = arc->getCenter();
                        const RS_Vector &endpoint = arc->getEndpoint();
                        const RS_Vector &startpoint = arc->getStartpoint();
                        previewRefPoint(center);
                        previewRefPoint(startpoint);
                        previewRefSelectablePoint(endpoint);
                        previewRefLine(center, endpoint);
                        previewRefLine(center, startpoint);

                        if (m_mode == ArcFixedAngle || m_mode == TangentalArcFixedAngle){
                            auto refArcData = RS_ArcData(arc->getData());
                            const double radius = arc->getRadius() * 0.2;
                            refArcData.radius = radius;
                            previewRefArc(refArcData);
                        }
                    }
                }
            }
            break;
        }
        default:
            break;
    }
}

void LC_ActionDrawPolyline::onMouseLeftButtonRelease([[maybe_unused]] const int status, const LC_MouseEvent* e) {
    RS_Vector mouse = e->snapPoint;
    if (status == SetNextPoint) {
        if (m_mode == Line) {
            mouse = getSnapAngleAwarePoint(e, m_actionData->point, mouse, true);
        } else {
            m_alternateArc = e->isControl;
        }
    }
    if (m_equationSettingOn || m_stepSizeSettingOn) {
        return;
    }

    if (m_startPointSettingOn || m_endPointSettingOn){
        const RS_Vector snap = e->snapPoint;
        QString pointNumberString(QString::number(snap.x)); // fixme - review and check the logic

        if (e->isControl){
            pointNumberString = QString::number(snap.x - getRelativeZero().x).prepend("@@");
        }

        RS_CommandEvent equationCommandEventObject(pointNumberString);
        commandEvent(&equationCommandEventObject);
        return;
    }

    fireCoordinateEvent(mouse);
}

void LC_ActionDrawPolyline::onMouseRightButtonRelease(const int status, [[maybe_unused]] const LC_MouseEvent* e) {
    if (m_equationSettingOn || m_startPointSettingOn || m_endPointSettingOn || m_stepSizeSettingOn){
        m_equationSettingOn = false;
        m_startPointSettingOn = false;
        m_endPointSettingOn = false;
        m_stepSizeSettingOn = false;
    }
    else {
        if (status == SetNextPoint) {
            trigger();
        }
        deletePreview();
        deleteSnapper();
        initPrevious(status);
    }
}

double LC_ActionDrawPolyline::solveBulge(const RS_Vector &mouse){
    double b(0.);
    bool success = false;
    RS_Line line{};
    double direction;
    RS_AtomicEntity *lastEntity = nullptr;
    m_calculatedSegment = false;

    switch (m_mode) {
//     case Line:
//        b=0.0;
//        break;
        case Tangential:
            if (m_actionData->polyline){
                if (m_prepend) {
                    lastEntity = dynamic_cast<RS_AtomicEntity *>(m_actionData->polyline->firstEntity());
                    direction = RS_Math::correctAngle(lastEntity->getDirection1() + M_PI);
                }
                else{
                    lastEntity = dynamic_cast<RS_AtomicEntity *>(m_actionData->polyline->lastEntity());
                    direction = RS_Math::correctAngle(lastEntity->getDirection2() + M_PI);
                }

                line.setStartpoint(m_actionData->point);
                line.setEndpoint(mouse);
                const double direction2 = RS_Math::correctAngle(line.getDirection2() + M_PI);
                const double delta = direction2 - direction;
                if (std::abs(std::remainder(delta, M_PI)) > RS_TOLERANCE_ANGLE){
                    b = std::tan(delta / 2);
                    success = LC_CreationArc::createFrom2PBulge(m_actionData->point, mouse, b, m_actionData->arcData);
                    if (!success) {
                        m_actionData->arcData = RS_ArcData{};
                        b = 0;
                    }
                }
                break;
//            if(delta<RS_TOLERANCE_ANGLE ||
//                (delta<M_PI+RS_TOLERANCE_ANGLE &&
//                delta>M_PI-RS_TOLERANCE_ANGLE))
//                b=0;
//            else{
//                b = std::tan((direction2-direction)/2);
//                suc = arc.createFrom2PBulge(point,mouse,b);
//                if (suc)
//                    arc_data = arc.getData();
//                else
//                    b=0;
//            }
            }
//        else
//            b=0;
//        break;
            // fall-through
        case TangentalArcFixedRadius: {
            if (m_actionData->polyline){
                if (m_prepend){
                    lastEntity = dynamic_cast<RS_AtomicEntity *>(m_actionData->polyline->firstEntity());
                    direction = RS_Math::correctAngle(lastEntity->getDirection1() + M_PI);
                }
                else{
                    lastEntity = dynamic_cast<RS_AtomicEntity *>(m_actionData->polyline->lastEntity());
                    direction = RS_Math::correctAngle(lastEntity->getDirection2() + M_PI);
                }
                success = LC_CreationArc::createFrom2PDirectionRadius(m_actionData->point, mouse,
                                                      direction, m_radius, m_actionData->arcData);
                if (success){
                    b = m_actionData->arcData.getBulge();
                    m_actionData->calculatedEndpoint = m_actionData->arcData.getEndpoint();
                    m_calculatedSegment = true;

                }
            }
            break;
        }
        case TangentalArcFixedAngle: {
            if (m_actionData->polyline){
                if (m_prepend){
                    lastEntity = dynamic_cast<RS_AtomicEntity *>(m_actionData->polyline->firstEntity());
                    direction = RS_Math::correctAngle(lastEntity->getDirection1() + M_PI);
                }
                else{
                    lastEntity = dynamic_cast<RS_AtomicEntity *>(m_actionData->polyline->lastEntity());
                    direction = RS_Math::correctAngle(lastEntity->getDirection2() + M_PI);
                }
                success = LC_CreationArc::createFrom2PDirectionAngle(m_actionData->point, mouse,
                                                      direction, RS_Math::deg2rad(m_angleDegrees),m_actionData->arcData);
                if (success){
                    b = m_actionData->arcData.getBulge();
                    m_actionData->calculatedEndpoint = m_actionData->arcData.getEndpoint();
                    m_calculatedSegment = true;
                }
                else {
                    m_actionData->arcData = RS_ArcData{};
                }
            }
            break;
        }
/*     case TanAng:
        b = std::tan(Reversed*m_angle*M_PI/720.0);
        break;
     case TanRadAng:
        b = std::tan(Reversed*m_angle*M_PI/720.0);
        break;*/
        case ArcFixedAngle: {
            if (m_prepend){
                b = std::tan(m_reversed * m_angleDegrees * M_PI / 720.0);
//                b = std::tan(m_reversed * -1 * m_angle * M_PI / 720.0);
                success = LC_CreationArc::createFrom2PBulge( mouse, m_actionData->point,b, m_actionData->arcData);
//                suc = arc.createFrom2PBulge(pPoints->point, mouse, b);
            }
            else{
               b = std::tan(m_reversed * m_angleDegrees * M_PI / 720.0);
               success = LC_CreationArc::createFrom2PBulge(m_actionData->point, mouse, b,m_actionData->arcData);
            }
            if (!success) {
                b = 0;
                m_actionData->arcData = RS_ArcData{};
            }
            break;
        }
        default:
            break;
            /*     case RadAngEndp:
            b=tan(Reversed*m_angle*M_PI/720.0);
            break;
         case RadAngCenp:
            b=tan(Reversed*m_angle*M_PI/720.0);*/
    }
    return b;
}

void LC_ActionDrawPolyline::onCoordinateEvent(const int status, [[maybe_unused]]bool isZero, const RS_Vector &pos) {
    RS_Vector mouse = pos;

    if (m_calculatedSegment) {
        mouse = m_actionData->calculatedEndpoint;
    }

    switch (status) {
        case SetStartpoint: {
            if (!m_startPointSettingOn){

                m_actionData->point = mouse;
                m_actionData->history.clear();
                m_actionData->history.append(mouse);
                m_actionData->bHistory.clear();
                m_actionData->bHistory.append(0.0);
                m_actionData->start = m_actionData->point;
                updateActionPrompt();
            } else {
                m_startPointSettingOn = false;
                m_endPointSettingOn = true;
                m_startPointX = mouse.x;
                m_startPointY = mouse.y;
                m_shiftY = true;
                updatePromptTRBack(tr("Enter the end point x")); // fixme - check if this is correct
            }
            drawSnapper();
            moveRelativeZero(mouse);
            addSnappedPointToVisualSnap(mouse);
            setStatus(SetNextPoint);
            break;
        }
        case SetNextPoint: {
            if (!m_endPointSettingOn){
                double bulge = 0.;
                if (m_alternateArc && m_mode == ArcFixedAngle){
                    const int originalReversed = m_reversed;
                    m_reversed = m_reversed == -1 ? 1: -1;
                    bulge = solveBulge(mouse);
                    m_reversed = originalReversed;
                }
                else {
                    bulge = solveBulge(mouse);
                }

                if (m_alternateArc && m_mode != ArcFixedAngle && m_mode != Line){
                    RS_ArcData tmpArcData = m_actionData->arcData;
                    tmpArcData.reversed = !tmpArcData.reversed;
                    const auto arc = RS_Arc(nullptr, tmpArcData);
                    bulge = arc.getBulge();
                }
                m_alternateArc = false;
                m_actionData->point = mouse;
                m_actionData->history.append(mouse);
                m_actionData->bHistory.append(bulge);

                RS_Polyline* polylineToUse = nullptr;
                bool addPolylineToDocument = false;

                if (m_actionData->polyline == nullptr){
                    polylineToUse = new RS_Polyline(m_document, m_actionData->data);
                    polylineToUse->addVertex(m_actionData->start, 0.0);
                    addPolylineToDocument = true;
                }
                else {
                    polylineToUse = m_actionData->polyline;
                }

                polylineToUse->setNextBulge(bulge);
                polylineToUse->addVertex(mouse, 0.0);
                polylineToUse->setEndpoint(mouse);

                if (addPolylineToDocument) {
                    const LC_UndoSection undo(m_document, m_viewport);
                    undo.undoableExecute([this, polylineToUse](LC_DocumentModificationBatch& ctx)-> bool {
                        ctx += polylineToUse;
                        if (m_actionData->polyline != nullptr) {
                            ctx -= m_actionData->polyline;
                        }
                        return true;
                    });
                }

                m_actionData->polyline = polylineToUse;

                deletePreview();
                deleteSnapper();
                redraw();

                updateActionPrompt();
            } else {
                m_endPointSettingOn = false;
                m_stepSizeSettingOn = true;
                m_endPointX = mouse.x;
                m_endPointY = mouse.y;
                updatePromptTRBack(tr("Enter number of polylines")); // fixme - check if this is correct
            }
            drawSnapper();
            addSnappedPointToVisualSnap(mouse);
            moveRelativeZero(mouse);
            break;
        }
        default:
            break;
    }
}

void LC_ActionDrawPolyline::setMode(const SegmentMode m){
    m_mode = m;
    updateActionPrompt();
}

int LC_ActionDrawPolyline::getMode() const{
    return m_mode;
}

void LC_ActionDrawPolyline::setRadius(const double r){
    m_radius = r;
}

double LC_ActionDrawPolyline::getRadius() const{
    return m_radius;
}

void LC_ActionDrawPolyline::setAngleDegrees(const double a){
    m_angleDegrees = a;
}

double LC_ActionDrawPolyline::getAngleDegrees() const{
    return m_angleDegrees;
}

void LC_ActionDrawPolyline::setReversed(const bool c){
    m_reversed = c ? -1 : 1;
}

bool LC_ActionDrawPolyline::isReversed() const{
    return m_reversed == -1;
}

bool LC_ActionDrawPolyline::doUpdateAngleByInteractiveInput(const QString& tag, const double angleRad) {
    if (tag == "angle") {
        setAngleDegrees(RS_Math::rad2deg(angleRad));
        return true;
    }
    return false;
}

bool LC_ActionDrawPolyline::doUpdateDistanceByInteractiveInput(const QString& tag, const double distance) {
    if (tag == "radius") {
        setRadius(distance);
        return true;
    }
    return false;
}

QString LC_ActionDrawPolyline::prepareCommand(RS_CommandEvent *e) const {
    QString c = e->getCommand().toLower().replace(" ", "");
    return c;
}

bool LC_ActionDrawPolyline::doProcessCommand(const int status, const QString &command) {
    bool accept = false;
    // fixme - sand - register in commands
    if (checkCommand("li", command)){
        m_mode = Line;
        updateOptions();
        accept = true;
    }
    else if (checkCommand("tan",command)){
        m_mode = Tangential;
        updateOptions();
        accept = true;
    }
    else if (checkCommand("tar", command)){
        m_mode = TangentalArcFixedRadius;
        updateOptions();
        accept = true;
    }
    else if (checkCommand("taa", command)){
        m_mode = TangentalArcFixedRadius;
        updateOptions();
        accept = true;
    }
    else if (checkCommand("aa", command)){
        m_mode = ArcFixedAngle;
        updateOptions();
        accept = true;
    }
    if (accept) {
        return accept;
    }
    switch (status) {
        case SetStartpoint: {
            if (checkCommand("close", command)){
                close();
                accept = true;
            }
            break;
        }
        case SetNextPoint: {
            if (checkCommand("undo", command)){
                undo();
                updateActionPrompt();
                accept = true;
            }
            else if (checkCommand("close", command)){
                close();
                accept = true;
            }
            break;
        }
        default:
            break;
    }
    if (accept) {
        return accept;
    }

    if ((m_mode == Line) && checkCommand("equation", command)) {
        updatePromptTRBack(tr("Enter an equation, f(x)"));
        m_equationSettingOn = true;
        return true;
    }

    if (m_equationSettingOn) {
        m_equationSettingOn = false;

        m_shiftX = false;

        try {
            QString cRef = command;
            const QString someRandomNumber = "123.456";
            cRef.replace(tr("x"), someRandomNumber);
            setParserExpression(cRef);
            const double parseTestValue = m_muParserObject->Eval();
            if (parseTestValue) { /* This is to counter the 'unused variable' warning. */ }
            updatePromptTRBack(tr("Enter the start point x"));
            m_startPointSettingOn = true;
            m_actionData->equation = command;
        }
        catch (...) {
            commandMessage(tr("The entered x is invalid."));
            updateActionPrompt();
        }
        return true;
    }

    if (m_startPointSettingOn) {
        if (getPlottingX(command, m_startPointX)) {
            m_endPointSettingOn = true;
            m_startPointSettingOn = false;
            m_shiftY = false;
            updatePromptTRBack(tr("Enter the end point x"));
        }
        return true;
    }

    if (m_endPointSettingOn){
        if (getPlottingX(command, m_endPointX) && std::abs(m_endPointX - m_startPointX) > RS_TOLERANCE){
            m_endPointSettingOn = false;
            m_stepSizeSettingOn = true;
            updatePromptTRBack(tr("Enter number of polylines"));
        }
        return true;
    }

    if (m_stepSizeSettingOn){
        m_stepSizeSettingOn = false;

        int numberOfPolylines = 0;

        try {
            setParserExpression(command);
            numberOfPolylines = RS_Math::round(m_muParserObject->Eval());

            if (numberOfPolylines <= 0) {
                throw -1;
            }
        }
        catch (...) {
            commandMessage(tr("The step size entered is invalid."));
            updateActionPrompt();

            return true;
        }

        drawEquation(numberOfPolylines);
        updateActionPrompt();
        setStatus(SetNextPoint);
        return true;
    }
    return false;
}

bool LC_ActionDrawPolyline::getPlottingX(QString command, double& x){
    try {
        bool isRelative = false;

        if (command.startsWith("@")) {
            isRelative = true;
        }
        if (command.startsWith("@@")) {
            m_shiftX = true;
        }

        setParserExpression(command.remove("@"));

        x = m_muParserObject->Eval();
        if (isRelative) {
            x += getRelativeZero().x;
        }

        m_endPointSettingOn = true;
    }
    catch (...) {
        commandMessage(tr("The value x entered is invalid."));
        updateActionPrompt();
        return false;
    }

    return true;
}

// fixme - review polyline equation functionality!!!!!
void LC_ActionDrawPolyline::drawEquation(const int numberOfPolylines) {
    deleteSnapper();
    const double stepSize = (m_endPointX - m_startPointX) / numberOfPolylines;

    double equationX = m_startPointX;
    double plottingX = m_startPointX;
    m_muParserObject->DefineVar(_T("x"), &equationX);
    setParserExpression(m_actionData->equation);

    if (m_shiftX || m_shiftY) {
        equationX = 0.0;
    }

    if (getStatus() == SetStartpoint) {
        m_actionData->point = RS_Vector(m_startPointX, m_muParserObject->Eval());
        if (m_shiftY) {
            m_actionData->point.y += m_startPointY;
        }
        m_actionData->history.clear();
        m_actionData->history.append(m_actionData->point);
        m_actionData->bHistory.clear();
        m_actionData->bHistory.append(0.0);
        m_actionData->start = m_actionData->point;

        setStatus(SetNextPoint);

        plottingX += stepSize;
        equationX += stepSize;
    }

    for (int i = 0; i <= numberOfPolylines; ++i) {
        m_actionData->point = RS_Vector(plottingX, m_muParserObject->Eval());
        m_actionData->history.append(m_actionData->point);

        if (m_actionData->polyline == nullptr) {
            m_actionData->polyline = new RS_Polyline(m_document, m_actionData->data);
            m_actionData->polyline->addVertex(m_actionData->start, 0.0);
        }

        m_actionData->polyline->addVertex(m_actionData->point, 0.0);
        m_actionData->polyline->setEndpoint(m_actionData->point);

        if (m_actionData->polyline->count() == 1) {
            setPenAndLayerToActive(m_actionData->polyline);
            m_document->addEntity(m_actionData->polyline);  // fixme - selection - check UNDO state
        }

        plottingX += stepSize;
        equationX += stepSize;
    }
    deletePreview();
    redraw();

    plottingX -= stepSize;
    equationX -= stepSize;

    moveRelativeZero(RS_Vector(plottingX, m_muParserObject->Eval()));
    drawSnapper();
}

QStringList LC_ActionDrawPolyline::getAvailableCommands() {
    QStringList cmd;

    cmd+=command("li");
    cmd+=command("tan");
    cmd+=command("tar");
    cmd+=command("taa");
    cmd+=command("aa");

    switch (getStatus()) {
        case SetStartpoint:
            break;
        case SetNextPoint:
            if (m_actionData->history.size() >= 2){
                cmd += command("undo");
            }
            if (m_actionData->history.size() >= 3){
                cmd += command("close");
            }
            break;
        default:
            break;
    }

    if (m_mode == Line){
        cmd += command("equation");
    }

    return cmd;
}

void LC_ActionDrawPolyline::updateActionPrompt() {
    if (m_equationSettingOn || m_startPointSettingOn || m_endPointSettingOn || m_stepSizeSettingOn) {
        return;
    }

    switch (getStatus()) {
        case SetStartpoint: {
            updatePromptTRCancel(tr("Specify first point"), MOD_SHIFT_RELATIVE_ZERO);
            break;
        }
        case SetNextPoint: {
            updateMouseButtonHintsForNextPoint();
            break;
        }
        default:
            updatePrompt();
            break;
    }
}

void LC_ActionDrawPolyline::updateMouseButtonHintsForNextPoint() {
    QString msg = "";
    LC_ModifiersInfo modifiers = MOD_NONE;
    if (m_mode == Line) {
        modifiers = MOD_SHIFT_ANGLE_SNAP;
    }
    else {
        modifiers = MOD_CTRL(tr("Alternative Arc"));
    }

    const qsizetype size = m_actionData->history.size();
    if (size >= 3) {
        msg += command("close");
        msg += "/";
    }

    if (size >= 2) {
        msg += command("undo");
        updatePromptTRBack(tr("Specify next point or [%1]").arg(msg), modifiers);
    } else {
        updatePromptTRBack(tr("Specify next point"), modifiers);
    }
}

RS2::CursorType LC_ActionDrawPolyline::doGetMouseCursor([[maybe_unused]] int status){
    return RS2::CadCursor;
}

void LC_ActionDrawPolyline::close(){
    if (m_actionData->history.size() > 2 && m_actionData->start.valid){
        if (m_actionData->polyline){
            if (m_mode == TangentalArcFixedRadius){
                m_mode = Line;
            }
            fireCoordinateEvent(m_actionData->polyline->getStartpoint());
            m_actionData->polyline->setClosed(true);
        }
        trigger();
        setStatus(SetStartpoint);
        moveRelativeZero(m_actionData->start);
    } else {
        commandMessage(tr("Cannot close sequence of lines: Not enough entities defined yet."));
    }
}

void LC_ActionDrawPolyline::undo(){
    if (m_actionData->history.size() > 1){
        m_actionData->history.removeLast();
        m_actionData->bHistory.removeLast();
        deletePreview();
        m_actionData->point = m_actionData->history.last();

        if (m_actionData->history.size() == 1){
            moveRelativeZero(m_actionData->history.front());
            //remove polyline from container,
            //container calls delete over polyline
            m_document->removeEntity(m_actionData->polyline);
            m_actionData->polyline = nullptr;
        }
        if (m_actionData->polyline){
            m_actionData->polyline->removeLastVertex();
            moveRelativeZero(m_actionData->polyline->getEndpoint());
        }
    } else {
        commandMessage(tr("Cannot undo: Not enough entities defined yet."));
    }
    redraw();
}

void LC_ActionDrawPolyline::setParserExpression(const QString& expression){
    if (m_muParserObject == nullptr){
        m_muParserObject = std::make_unique<mu::Parser>();
        m_muParserObject->DefineConst(_T("e"), M_E);
        m_muParserObject->DefineConst(_T("pi"), M_PI);
    }

#ifdef _UNICODE
    m_muParserObject->SetExpr(expression.toStdWString());
#else
    m_muParserObject->SetExpr(expression.toStdString());
#endif
}



RS_Polyline *&LC_ActionDrawPolyline::getPolyline() const{
    return m_actionData->polyline;
}

RS_PolylineData &LC_ActionDrawPolyline::getData() const{
    return m_actionData->data;
}

RS_Vector &LC_ActionDrawPolyline::getPoint() const{
    return m_actionData->point;
}

RS_Vector &LC_ActionDrawPolyline::getStart() const{
    return m_actionData->start;
}

QList<RS_Vector> &LC_ActionDrawPolyline::getHistory() const{
    return m_actionData->history;
}

QList<double> &LC_ActionDrawPolyline::getBHistory() const{
    return m_actionData->bHistory;
}

LC_ActionOptionsWidget* LC_ActionDrawPolyline::createOptionsWidget(){
    return new LC_PolylineOptionsWidget();
}

LC_ActionOptionsPropertiesFiller* LC_ActionDrawPolyline::createOptionsFiller() {
    return new LC_PolylineOptionsFiller();
}
