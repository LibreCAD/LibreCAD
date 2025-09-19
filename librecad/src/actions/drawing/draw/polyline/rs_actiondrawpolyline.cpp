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

#include "rs_actiondrawpolyline.h"

#include "muParser.h"
#include "qg_polylineoptions.h"
#include "rs_arc.h"
#include "rs_commandevent.h"
#include "rs_debug.h"
#include "rs_line.h"
#include "rs_polyline.h"

#ifdef EMU_C99
#include "emu_c99.h"
#endif

RS_ActionDrawPolyline::RS_ActionDrawPolyline(LC_ActionContext *actionContext)
        :RS_PreviewActionInterface("Draw polylines",actionContext, RS2::ActionDrawPolyline)
        , m_actionData(std::make_unique<Points>()){
    reset();
}

RS_ActionDrawPolyline::~RS_ActionDrawPolyline() = default;

void RS_ActionDrawPolyline::reset(){
    m_actionData->polyline = nullptr;
    m_actionData->data = {};
    m_actionData->start = {};
    m_actionData->history.clear();
    m_actionData->bHistory.clear();
}

void RS_ActionDrawPolyline::init(int status){
    reset();
    RS_PreviewActionInterface::init(status);
}

void RS_ActionDrawPolyline::doTrigger() {
    if (!m_actionData->polyline) {
        return;
    }

    moveRelativeZero(m_actionData->polyline->getEndpoint());
    undoCycleAdd(m_actionData->polyline, false); // todo - check whether we actially should not add to container

    RS_DEBUG->print("RS_ActionDrawLinePolyline::trigger(): polyline added: %lu",m_actionData->polyline->getId());

    m_actionData->polyline = nullptr;
}

void RS_ActionDrawPolyline::onMouseMoveEvent(int status, LC_MouseEvent *e) {
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
                bool alternateDirection = e->isControl;
                double bulge = 0.;
                if (alternateDirection && m_mode == Ang){
                    int originalReversed = m_reversed;
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
                    RS_ArcData tmpArcData = m_actionData->arc_data;
                    if (alternateDirection && m_mode != Ang){
                        tmpArcData.reversed = !tmpArcData.reversed;
                    }
                    auto arc = previewToCreateArc(tmpArcData);
                    if (m_showRefEntitiesOnPreview) {
                        const RS_Vector &center = arc->getCenter();
                        const RS_Vector &endpoint = arc->getEndpoint();
                        const RS_Vector &startpoint = arc->getStartpoint();
                        previewRefPoint(center);
                        previewRefPoint(startpoint);
                        previewRefSelectablePoint(endpoint);
                        previewRefLine(center, endpoint);
                        previewRefLine(center, startpoint);

                        if (m_mode == Ang || m_mode == TanAng){
                            auto refArcData = RS_ArcData(arc->getData());
                            double radius = arc->getRadius() * 0.2;
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

void RS_ActionDrawPolyline::onMouseLeftButtonRelease([[maybe_unused]]int status, LC_MouseEvent *e) {
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
        RS_Vector snap = e->snapPoint;
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

void RS_ActionDrawPolyline::onMouseRightButtonRelease(int status, [[maybe_unused]]LC_MouseEvent *e) {
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

double RS_ActionDrawPolyline::solveBulge(const RS_Vector &mouse){
    double b(0.);
    bool suc = false;
    RS_Arc arc{};
    RS_Line line{};
    double direction;
    RS_AtomicEntity *lastentity = nullptr;
    m_calculatedSegment = false;

    switch (m_mode) {
//     case Line:
//        b=0.0;
//        break;
        case Tangential:
            if (m_actionData->polyline){
                if (m_prepend) {
                    lastentity = dynamic_cast<RS_AtomicEntity *>(m_actionData->polyline->firstEntity());
                    direction = RS_Math::correctAngle(lastentity->getDirection1() + M_PI);
                }
                else{
                    lastentity = dynamic_cast<RS_AtomicEntity *>(m_actionData->polyline->lastEntity());
                    direction = RS_Math::correctAngle(lastentity->getDirection2() + M_PI);
                }

                line.setStartpoint(m_actionData->point);
                line.setEndpoint(mouse);
                double const direction2 = RS_Math::correctAngle(line.getDirection2() + M_PI);
                double const delta = direction2 - direction;
                if (std::abs(std::remainder(delta, M_PI)) > RS_TOLERANCE_ANGLE){
                    b = std::tan(delta / 2);
                    suc = arc.createFrom2PBulge(m_actionData->point, mouse, b);
                    if (suc)
                        m_actionData->arc_data = arc.getData();
                    else
                        b = 0;
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
        case TanRad: {
            if (m_actionData->polyline){
                if (m_prepend){
                    lastentity = dynamic_cast<RS_AtomicEntity *>(m_actionData->polyline->firstEntity());
                    direction = RS_Math::correctAngle(lastentity->getDirection1() + M_PI);
                }
                else{
                    lastentity = dynamic_cast<RS_AtomicEntity *>(m_actionData->polyline->lastEntity());
                    direction = RS_Math::correctAngle(lastentity->getDirection2() + M_PI);
                }
                suc = arc.createFrom2PDirectionRadius(m_actionData->point, mouse,
                                                      direction, m_radius);
                if (suc){
                    m_actionData->arc_data = arc.getData();
                    b = arc.getBulge();
                    m_actionData->calculatedEndpoint = arc.getEndpoint();
                    m_calculatedSegment = true;

                }
//            else
//                b=0;
            }
//        else
//          b=0;
            break;
        }
        case TanAng: {
            if (m_actionData->polyline){
                if (m_prepend){
                    lastentity = dynamic_cast<RS_AtomicEntity *>(m_actionData->polyline->firstEntity());
                    direction = RS_Math::correctAngle(lastentity->getDirection1() + M_PI);
                }
                else{
                    lastentity = dynamic_cast<RS_AtomicEntity *>(m_actionData->polyline->lastEntity());
                    direction = RS_Math::correctAngle(lastentity->getDirection2() + M_PI);
                }
                suc = arc.createFrom2PDirectionAngle(m_actionData->point, mouse,
                                                      direction, RS_Math::deg2rad(m_angleDegrees));
                if (suc){
                    m_actionData->arc_data = arc.getData();
                    b = arc.getBulge();
                    m_actionData->calculatedEndpoint = arc.getEndpoint();
                    m_calculatedSegment = true;
                }
//            else
//                b=0;
            }
//        else
//          b=0;
            break;
        }
/*     case TanAng:
        b = std::tan(Reversed*m_angle*M_PI/720.0);
        break;
     case TanRadAng:
        b = std::tan(Reversed*m_angle*M_PI/720.0);
        break;*/
        case Ang: {
            if (m_prepend){
                b = std::tan(m_reversed * m_angleDegrees * M_PI / 720.0);
//                b = std::tan(m_reversed * -1 * m_angle * M_PI / 720.0);
                suc = arc.createFrom2PBulge( mouse, m_actionData->point,b);
//                suc = arc.createFrom2PBulge(pPoints->point, mouse, b);
            }
            else{
               b = std::tan(m_reversed * m_angleDegrees * M_PI / 720.0);
               suc = arc.createFrom2PBulge(m_actionData->point, mouse, b);
            }
            if (suc) {
                m_actionData->arc_data = arc.getData();
            }
            else {
                b = 0;
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

void RS_ActionDrawPolyline::onCoordinateEvent(int status, [[maybe_unused]]bool isZero, const RS_Vector &pos) {
    RS_Vector mouse = pos;

    if (m_calculatedSegment) {
        mouse = m_actionData->calculatedEndpoint;
    }

    switch (status) {
        case SetStartpoint: {
            if (!m_startPointSettingOn){
                //	data.startpoint = mouse;
                //printf ("SetStartpoint\n");
                m_actionData->point = mouse;
                m_actionData->history.clear();
                m_actionData->history.append(mouse);
                m_actionData->bHistory.clear();
                m_actionData->bHistory.append(0.0);
                m_actionData->start = m_actionData->point;
                updateMouseButtonHints();
            } else {
                m_startPointSettingOn = false;
                m_endPointSettingOn = true;
                m_startPointX = mouse.x;
                m_startPointY = mouse.y;
                m_shiftY = true;
                updateMouseWidgetTRBack(tr("Enter the end point x")); // fixme - check if this is correct
            }
            drawSnapper();
            moveRelativeZero(mouse);
            setStatus(SetNextPoint);
            break;
        }
        case SetNextPoint: {
            if (!m_endPointSettingOn){
                double bulge = 0.;
                if (m_alternateArc && m_mode == Ang){
                    int originalReversed = m_reversed;
                    m_reversed = m_reversed == -1 ? 1: -1;
                    bulge = solveBulge(mouse);
                    m_reversed = originalReversed;
                }
                else {
                    bulge = solveBulge(mouse);
                }

                if (m_alternateArc && m_mode != Ang && m_mode != Line){
                    RS_ArcData tmpArcData = m_actionData->arc_data;
                    tmpArcData.reversed = !tmpArcData.reversed;
                    RS_Arc arc = RS_Arc(nullptr, tmpArcData);
                    bulge = arc.getBulge();
                }
                m_alternateArc = false;
                m_actionData->point = mouse;
                m_actionData->history.append(mouse);
                m_actionData->bHistory.append(bulge);
                if (!m_actionData->polyline){
                    m_actionData->polyline = new RS_Polyline(m_container, m_actionData->data);
                    m_actionData->polyline->addVertex(m_actionData->start, 0.0);
                }
                if (m_actionData->polyline){
                    m_actionData->polyline->setNextBulge(bulge);
                    m_actionData->polyline->addVertex(mouse, 0.0);
                    m_actionData->polyline->setEndpoint(mouse);
                    if (m_actionData->polyline->count() == 1){
                        setPenAndLayerToActive(m_actionData->polyline);
                        m_container->addEntity(m_actionData->polyline);
                    }
                    deletePreview();
                    deleteSnapper();
                    redraw();
                }
                updateMouseButtonHints();
            } else {
                m_endPointSettingOn = false;
                m_stepSizeSettingOn = true;
                m_endPointX = mouse.x;
                m_endPointY = mouse.y;
                updateMouseWidgetTRBack(tr("Enter number of polylines")); // fixme - check if this is correct
            }
            drawSnapper();
            moveRelativeZero(mouse);
            break;
        }
        default:
            break;
    }
}

void RS_ActionDrawPolyline::setMode(SegmentMode m){
    m_mode = m;
    updateMouseButtonHints();
}

int RS_ActionDrawPolyline::getMode() const{
    return m_mode;
}

void RS_ActionDrawPolyline::setRadius(double r){
    m_radius = r;
}

double RS_ActionDrawPolyline::getRadius() const{
    return m_radius;
}

void RS_ActionDrawPolyline::setAngleDegrees(double a){
    m_angleDegrees = a;
}

double RS_ActionDrawPolyline::getAngle() const{
    return m_angleDegrees;
}

void RS_ActionDrawPolyline::setReversed(bool c){
    m_reversed = c ? -1 : 1;
}

bool RS_ActionDrawPolyline::isReversed() const{
    return m_reversed == -1;
}

bool RS_ActionDrawPolyline::doUpdateAngleByInteractiveInput(const QString& tag, double angleRad) {
    if (tag == "angle") {
        setAngleDegrees(RS_Math::rad2deg(angleRad));
        return true;
    }
    return false;
}

bool RS_ActionDrawPolyline::doUpdateDistanceByInteractiveInput(const QString& tag, double distance) {
    if (tag == "radius") {
        setRadius(distance);
        return true;
    }
    return false;
}

QString RS_ActionDrawPolyline::prepareCommand(RS_CommandEvent *e) const {
    QString c = e->getCommand().toLower().replace(" ", "");
    return c;
}

bool RS_ActionDrawPolyline::doProcessCommand(int status, const QString &c) {
    bool accept = false;
    // fixme - sand - register in commands
    if (checkCommand("li", c)){
        m_mode = Line;
        updateOptions();
        accept = true;
    }
    else if (checkCommand("tan",c)){
        m_mode = Tangential;
        updateOptions();
        accept = true;
    }
    else if (checkCommand("tar", c)){
        m_mode = TanRad;
        updateOptions();
        accept = true;
    }
    else if (checkCommand("taa", c)){
        m_mode = TanRad;
        updateOptions();
        accept = true;
    }
    else if (checkCommand("aa", c)){
        m_mode = Ang;
        updateOptions();
        accept = true;
    }
    if (accept) {
        return accept;
    }
    switch (status) {
        case SetStartpoint: {
            if (checkCommand("close", c)){
                close();
                accept = true;
            }
            break;
        }
        case SetNextPoint: {
            if (checkCommand("undo", c)){
                undo();
                updateMouseButtonHints();
                accept = true;
            }
            else if (checkCommand("close", c)){
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

    if ((m_mode == Line) && (checkCommand("equation", c))) {
        updateMouseWidgetTRBack(tr("Enter an equation, f(x)"));
        m_equationSettingOn = true;
        return true;
    }

    if (m_equationSettingOn) {
        m_equationSettingOn = false;

        m_shiftX = false;

        try {
            QString cRef = c;
            const QString someRandomNumber = "123.456";
            cRef.replace(tr("x"), someRandomNumber);
            setParserExpression(cRef);
            const double parseTestValue = m_muParserObject->Eval();
            if (parseTestValue) { /* This is to counter the 'unused variable' warning. */ }
            updateMouseWidgetTRBack(tr("Enter the start point x"));
            m_startPointSettingOn = true;
            m_actionData->equation = c;
        }
        catch (...) {
            commandMessage(tr("The entered x is invalid."));
            updateMouseButtonHints();
        }
        return true;
    }

    if (m_startPointSettingOn) {
        if (getPlottingX(c, m_startPointX)) {
            m_endPointSettingOn = true;
            m_startPointSettingOn = false;
            m_shiftY = false;
            updateMouseWidgetTRBack(tr("Enter the end point x"));
        }
        return true;
    }

    if (m_endPointSettingOn){
        if (getPlottingX(c, m_endPointX) && std::abs(m_endPointX - m_startPointX) > RS_TOLERANCE){
            m_endPointSettingOn = false;
            m_stepSizeSettingOn = true;
            updateMouseWidgetTRBack(tr("Enter number of polylines"));
        }
        return true;
    }

    if (m_stepSizeSettingOn){
        m_stepSizeSettingOn = false;

        int numberOfPolylines = 0;

        try {
            setParserExpression(c);
            numberOfPolylines = RS_Math::round(m_muParserObject->Eval());

            if (numberOfPolylines <= 0) throw -1;
        }
        catch (...) {
            commandMessage(tr("The step size entered is invalid."));
            updateMouseButtonHints();

            return true;
        }

        drawEquation(numberOfPolylines);
        updateMouseButtonHints();
        setStatus(SetNextPoint);
        return true;
    }
    return false;
}

bool RS_ActionDrawPolyline::getPlottingX(QString command, double& x){
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
        updateMouseButtonHints();
        return false;
    }

    return true;
}

void RS_ActionDrawPolyline::drawEquation(int numberOfPolylines) {
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
            m_actionData->polyline = new RS_Polyline(m_container, m_actionData->data);
            m_actionData->polyline->addVertex(m_actionData->start, 0.0);
        }

        m_actionData->polyline->addVertex(m_actionData->point, 0.0);
        m_actionData->polyline->setEndpoint(m_actionData->point);

        if (m_actionData->polyline->count() == 1) {
            setPenAndLayerToActive(m_actionData->polyline);
            m_container->addEntity(m_actionData->polyline);
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

QStringList RS_ActionDrawPolyline::getAvailableCommands() {
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

void RS_ActionDrawPolyline::updateMouseButtonHints() {
    if (m_equationSettingOn || m_startPointSettingOn || m_endPointSettingOn || m_stepSizeSettingOn) {
        return;
    }

    switch (getStatus()) {
        case SetStartpoint: {
            updateMouseWidgetTRCancel(tr("Specify first point"), MOD_SHIFT_RELATIVE_ZERO);
            break;
        }
        case SetNextPoint: {
            updateMouseButtonHintsForNextPoint();
            break;
        }
        default:
            updateMouseWidget();
            break;
    }
}

void RS_ActionDrawPolyline::updateMouseButtonHintsForNextPoint() {
    QString msg = "";
    LC_ModifiersInfo modifiers = MOD_NONE;
    if (m_mode == Line) {
        modifiers = MOD_SHIFT_ANGLE_SNAP;
    }
    else {
        modifiers = MOD_CTRL(tr("Alternative Arc"));
    }

    qsizetype size = m_actionData->history.size();
    if (size >= 3) {
        msg += command("close");
        msg += "/";
    }

    if (size >= 2) {
        msg += command("undo");
        updateMouseWidgetTRBack(tr("Specify next point or [%1]").arg(msg), modifiers);
    } else {
        updateMouseWidgetTRBack(tr("Specify next point"), modifiers);
    }
}

RS2::CursorType RS_ActionDrawPolyline::doGetMouseCursor([[maybe_unused]] int status){
    return RS2::CadCursor;
}

void RS_ActionDrawPolyline::close(){
    if (m_actionData->history.size() > 2 && m_actionData->start.valid){
        if (m_actionData->polyline){
            if (m_mode == TanRad){
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

void RS_ActionDrawPolyline::undo(){
    if (m_actionData->history.size() > 1){
        m_actionData->history.removeLast();
        m_actionData->bHistory.removeLast();
        deletePreview();
        m_actionData->point = m_actionData->history.last();

        if (m_actionData->history.size() == 1){
            moveRelativeZero(m_actionData->history.front());
            //remove polyline from container,
            //container calls delete over polyline
            m_container->removeEntity(m_actionData->polyline);
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

void RS_ActionDrawPolyline::setParserExpression(const QString& expression){
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

RS_Polyline *&RS_ActionDrawPolyline::getPolyline() const{
    return m_actionData->polyline;
}

RS_PolylineData &RS_ActionDrawPolyline::getData() const{
    return m_actionData->data;
}

RS_Vector &RS_ActionDrawPolyline::getPoint() const{
    return m_actionData->point;
}

RS_Vector &RS_ActionDrawPolyline::getStart() const{
    return m_actionData->start;
}

QList<RS_Vector> &RS_ActionDrawPolyline::getHistory() const{
    return m_actionData->history;
}

QList<double> &RS_ActionDrawPolyline::getBHistory() const{
    return m_actionData->bHistory;
}

LC_ActionOptionsWidget* RS_ActionDrawPolyline::createOptionsWidget(){
    return new QG_PolylineOptions();
}
