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

#include <cmath>

#include <QMouseEvent>

#include "muParser.h"

#include "rs_actiondrawpolyline.h"
#include "rs_arc.h"
#include "rs_commandevent.h"
#include "rs_commands.h"
#include "rs_coordinateevent.h"
#include "rs_debug.h"
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_line.h"
#include "rs_math.h"
#include "rs_polyline.h"
#include "rs_preview.h"
#include "qg_polylineoptions.h"

#ifdef EMU_C99
#include "emu_c99.h"
#endif



RS_ActionDrawPolyline::RS_ActionDrawPolyline(RS_EntityContainer& container,
                                     RS_GraphicView& graphicView)
        :RS_PreviewActionInterface("Draw polylines",
						   container, graphicView)
        , pPoints(std::make_unique<Points>()){
	actionType=RS2::ActionDrawPolyline;
    reset();
}

RS_ActionDrawPolyline::~RS_ActionDrawPolyline() = default;

void RS_ActionDrawPolyline::reset(){
    pPoints->polyline = nullptr;
    pPoints->data = {};
    pPoints->start = {};
    pPoints->history.clear();
    pPoints->bHistory.clear();
}

void RS_ActionDrawPolyline::init(int status){
    reset();
    RS_PreviewActionInterface::init(status);
}

void RS_ActionDrawPolyline::doTrigger() {
    if (!pPoints->polyline) return;

    moveRelativeZero(pPoints->polyline->getEndpoint());
    undoCycleAdd(pPoints->polyline, false); // todo - check whether we actially should not add to container

    RS_DEBUG->print("RS_ActionDrawLinePolyline::trigger(): polyline added: %lu",pPoints->polyline->getId());

    pPoints->polyline = nullptr;
}

void RS_ActionDrawPolyline::mouseMoveEvent(QMouseEvent *e){
    deleteHighlights();
    deletePreview();
    RS_DEBUG->print("RS_ActionDrawLinePolyline::mouseMoveEvent begin");
    RS_Vector mouse = snapPoint(e);
    int status = getStatus();
    switch (status) {
        case SetStartpoint: {
            trySnapToRelZeroCoordinateEvent(e);
            break;
        }
        case SetNextPoint: {
            if (m_mode == Line){
                mouse = getSnapAngleAwarePoint(e, pPoints->point, mouse, true);
            }
            if (pPoints->point.valid){
                bool alternateDirection = isControl(e);
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
                    previewToCreateLine(pPoints->point, mouse);
                    if (showRefEntitiesOnPreview) {
                        previewRefPoint(pPoints->point);
                        previewRefSelectablePoint(mouse);
                    }
                } else {
                    RS_ArcData tmpArcData = pPoints->arc_data;
                    if (alternateDirection && m_mode != Ang){
                        tmpArcData.reversed = !tmpArcData.reversed;
                    }
                    auto arc = previewToCreateArc(tmpArcData);
                    if (showRefEntitiesOnPreview) {
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
    RS_DEBUG->print("RS_ActionDrawLinePolyline::mouseMoveEvent end");
    drawHighlights();
    drawPreview();
}

void RS_ActionDrawPolyline::onMouseLeftButtonRelease([[maybe_unused]]int status, QMouseEvent *e) {

    RS_Vector mouse = snapPoint(e);
    if (status == SetNextPoint) {
        if (m_mode == Line) {
            mouse = getSnapAngleAwarePoint(e, pPoints->point, mouse, true);
        } else {
            alternateArc = isControl(e);
        }
    }
    if (equationSettingOn || stepSizeSettingOn) return;

    if (startPointSettingOn || endPointSettingOn){
        QString pointNumberString(QString::number(snapPoint(e).x)); // fixme - review and check the logic

        if (isControl(e)){
            pointNumberString = QString::number(snapPoint(e).x - graphicView->getRelativeZero().x).prepend("@@");
        }

        RS_CommandEvent equationCommandEventObject(pointNumberString);
        commandEvent(&equationCommandEventObject);
        return;
    }

    fireCoordinateEvent(mouse);
}

void RS_ActionDrawPolyline::onMouseRightButtonRelease(int status, [[maybe_unused]]QMouseEvent *e) {
    if (equationSettingOn || startPointSettingOn || endPointSettingOn || stepSizeSettingOn){
        equationSettingOn = false;
        startPointSettingOn = false;
        endPointSettingOn = false;
        stepSizeSettingOn = false;
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
            if (pPoints->polyline){
                if (prepend) {
                    lastentity = dynamic_cast<RS_AtomicEntity *>(pPoints->polyline->firstEntity());
                    direction = RS_Math::correctAngle(lastentity->getDirection1() + M_PI);
                }
                else{
                    lastentity = dynamic_cast<RS_AtomicEntity *>(pPoints->polyline->lastEntity());
                    direction = RS_Math::correctAngle(lastentity->getDirection2() + M_PI);
                }


                line.setStartpoint(pPoints->point);
                line.setEndpoint(mouse);
                double const direction2 = RS_Math::correctAngle(line.getDirection2() + M_PI);
                double const delta = direction2 - direction;
                if (std::abs(std::remainder(delta, M_PI)) > RS_TOLERANCE_ANGLE){
                    b = std::tan(delta / 2);
                    suc = arc.createFrom2PBulge(pPoints->point, mouse, b);
                    if (suc)
                        pPoints->arc_data = arc.getData();
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
            if (pPoints->polyline){
                if (prepend){
                    lastentity = dynamic_cast<RS_AtomicEntity *>(pPoints->polyline->firstEntity());
                    direction = RS_Math::correctAngle(lastentity->getDirection1() + M_PI);
                }
                else{
                    lastentity = dynamic_cast<RS_AtomicEntity *>(pPoints->polyline->lastEntity());
                    direction = RS_Math::correctAngle(lastentity->getDirection2() + M_PI);
                }
                suc = arc.createFrom2PDirectionRadius(pPoints->point, mouse,
                                                      direction, m_radius);
                if (suc){
                    pPoints->arc_data = arc.getData();
                    b = arc.getBulge();
                    pPoints->calculatedEndpoint = arc.getEndpoint();
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
            if (pPoints->polyline){
                if (prepend){
                    lastentity = dynamic_cast<RS_AtomicEntity *>(pPoints->polyline->firstEntity());
                    direction = RS_Math::correctAngle(lastentity->getDirection1() + M_PI);
                }
                else{
                    lastentity = dynamic_cast<RS_AtomicEntity *>(pPoints->polyline->lastEntity());
                    direction = RS_Math::correctAngle(lastentity->getDirection2() + M_PI);
                }
                suc = arc.createFrom2PDirectionAngle(pPoints->point, mouse,
                                                      direction, RS_Math::deg2rad(m_angle));
                if (suc){
                    pPoints->arc_data = arc.getData();
                    b = arc.getBulge();
                    pPoints->calculatedEndpoint = arc.getEndpoint();
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
            if (prepend){
                b = std::tan(m_reversed * m_angle * M_PI / 720.0);
//                b = std::tan(m_reversed * -1 * m_angle * M_PI / 720.0);
                suc = arc.createFrom2PBulge( mouse, pPoints->point,b);
//                suc = arc.createFrom2PBulge(pPoints->point, mouse, b);
            }
            else{
               b = std::tan(m_reversed * m_angle * M_PI / 720.0);
               suc = arc.createFrom2PBulge(pPoints->point, mouse, b);
            }
            if (suc)
                pPoints->arc_data = arc.getData();
            else
                b = 0;
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

    if (m_calculatedSegment)
        mouse = pPoints->calculatedEndpoint;

    switch (status) {
        case SetStartpoint: {
            if (!startPointSettingOn){
                //	data.startpoint = mouse;
                //printf ("SetStartpoint\n");
                pPoints->point = mouse;
                pPoints->history.clear();
                pPoints->history.append(mouse);
                pPoints->bHistory.clear();
                pPoints->bHistory.append(0.0);
                pPoints->start = pPoints->point;
                updateMouseButtonHints();
            } else {
                startPointSettingOn = false;
                endPointSettingOn = true;
                startPointX = mouse.x;
                startPointY = mouse.y;
                shiftY = true;
                updateMouseWidgetTRBack(tr("Enter the end point x")); // fixme - check if this is correct
            }
            drawSnapper();
            moveRelativeZero(mouse);
            setStatus(SetNextPoint);
            break;
        }
        case SetNextPoint: {
            if (!endPointSettingOn){
                double bulge = 0.;
                if (alternateArc && m_mode == Ang){
                    int originalReversed = m_reversed;
                    m_reversed = m_reversed == -1 ? 1: -1;
                    bulge = solveBulge(mouse);
                    m_reversed = originalReversed;
                }
                else {
                    bulge = solveBulge(mouse);
                }

                if (alternateArc && m_mode != Ang && m_mode != Line){
                    RS_ArcData tmpArcData = pPoints->arc_data;
                    tmpArcData.reversed = !tmpArcData.reversed;
                    RS_Arc arc = RS_Arc(nullptr, tmpArcData);
                    bulge = arc.getBulge();
                }
                alternateArc = false;
                pPoints->point = mouse;
                pPoints->history.append(mouse);
                pPoints->bHistory.append(bulge);
                if (!pPoints->polyline){
                    pPoints->polyline = new RS_Polyline(container, pPoints->data);
                    pPoints->polyline->addVertex(pPoints->start, 0.0);
                }
                if (pPoints->polyline){
                    pPoints->polyline->setNextBulge(bulge);
                    pPoints->polyline->addVertex(mouse, 0.0);
                    pPoints->polyline->setEndpoint(mouse);
                    if (pPoints->polyline->count() == 1){
                        setPenAndLayerToActive(pPoints->polyline);
                        container->addEntity(pPoints->polyline);
                    }
                    deletePreview();
                    deleteSnapper();
                    graphicView->redraw();
                }
                updateMouseButtonHints();
            } else {
                endPointSettingOn = false;
                stepSizeSettingOn = true;
                endPointX = mouse.x;
                endPointY = mouse.y;
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

void RS_ActionDrawPolyline::setAngle(double a){
    m_angle = a;
}

double RS_ActionDrawPolyline::getAngle() const{
    return m_angle;
}

void RS_ActionDrawPolyline::setReversed(bool c){
    m_reversed = c ? -1 : 1;
}

bool RS_ActionDrawPolyline::isReversed() const{
    return m_reversed == -1;
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
        equationSettingOn = true;
        return true;
    }

    if (equationSettingOn) {
        equationSettingOn = false;

        shiftX = false;

        try {
            QString cRef = c;
            const QString someRandomNumber = "123.456";

            cRef.replace(tr("x"), someRandomNumber);

            setParserExpression(cRef);

            const double parseTestValue = m_muParserObject->Eval();

            if (parseTestValue) { /* This is to counter the 'unused variable' warning. */ }

            updateMouseWidgetTRBack(tr("Enter the start point x"));

            startPointSettingOn = true;

            pPoints->equation = c;
        }
        catch (...) {
            commandMessage(tr("The entered x is invalid."));
            updateMouseButtonHints();
        }
        return true;
    }

    if (startPointSettingOn) {
        if (getPlottingX(c, startPointX)) {
            endPointSettingOn = true;
            startPointSettingOn = false;
            shiftY = false;
            updateMouseWidgetTRBack(tr("Enter the end point x"));
        }
        return true;
    }

    if (endPointSettingOn){
        if (getPlottingX(c, endPointX) && std::abs(endPointX - startPointX) > RS_TOLERANCE){
            endPointSettingOn = false;
            stepSizeSettingOn = true;
            updateMouseWidgetTRBack(tr("Enter number of polylines"));
        }
        return true;
    }

    if (stepSizeSettingOn){
        stepSizeSettingOn = false;

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

        if (command.startsWith("@")) isRelative = true;

        if (command.startsWith("@@")) shiftX = true;

        setParserExpression(command.remove("@"));

        x = m_muParserObject->Eval();

        if (isRelative) x += graphicView->getRelativeZero().x;

        endPointSettingOn = true;
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
    const double stepSize = (endPointX - startPointX) / numberOfPolylines;

    double equationX = startPointX;
    double plottingX = startPointX;
    m_muParserObject->DefineVar(_T("x"), &equationX);
    setParserExpression(pPoints->equation);

    if (shiftX || shiftY)
        equationX = 0.0;

    if (getStatus() == SetStartpoint) {
        pPoints->point = RS_Vector(startPointX, m_muParserObject->Eval());
        if (shiftY)
            pPoints->point.y += startPointY;
        pPoints->history.clear();
        pPoints->history.append(pPoints->point);
        pPoints->bHistory.clear();
        pPoints->bHistory.append(0.0);
        pPoints->start = pPoints->point;

        setStatus(SetNextPoint);

        plottingX += stepSize;
        equationX += stepSize;
    }

    for (int i = 0; i <= numberOfPolylines; ++i) {
        pPoints->point = RS_Vector(plottingX, m_muParserObject->Eval());
        pPoints->history.append(pPoints->point);

        if (pPoints->polyline == nullptr) {
            pPoints->polyline = new RS_Polyline(container, pPoints->data);
            pPoints->polyline->addVertex(pPoints->start, 0.0);
        }

        pPoints->polyline->addVertex(pPoints->point, 0.0);
        pPoints->polyline->setEndpoint(pPoints->point);

        if (pPoints->polyline->count() == 1) {
            setPenAndLayerToActive(pPoints->polyline);
            container->addEntity(pPoints->polyline);
        }

        plottingX += stepSize;
        equationX += stepSize;
    }
    deletePreview();
    graphicView->redraw();

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
            if (pPoints->history.size() >= 2){
                cmd += command("undo");
            }
            if (pPoints->history.size() >= 3){
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
    if (equationSettingOn || startPointSettingOn || endPointSettingOn || stepSizeSettingOn) return;

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

    qsizetype size = pPoints->history.size();
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
    if (pPoints->history.size() > 2 && pPoints->start.valid){
        if (pPoints->polyline){
            if (m_mode == TanRad){
                m_mode = Line;
            }
            fireCoordinateEvent(pPoints->polyline->getStartpoint());
            pPoints->polyline->setClosed(true);
        }
        trigger();
        setStatus(SetStartpoint);
        moveRelativeZero(pPoints->start);
    } else {
        commandMessage(tr("Cannot close sequence of lines: Not enough entities defined yet."));
    }
}

void RS_ActionDrawPolyline::undo(){
    if (pPoints->history.size() > 1){
        pPoints->history.removeLast();
        pPoints->bHistory.removeLast();
        deletePreview();
        pPoints->point = pPoints->history.last();

        if (pPoints->history.size() == 1){
            moveRelativeZero(pPoints->history.front());
            //remove polyline from container,
            //container calls delete over polyline
            container->removeEntity(pPoints->polyline);
            pPoints->polyline = nullptr;
        }
        if (pPoints->polyline){
            pPoints->polyline->removeLastVertex();
            moveRelativeZero(pPoints->polyline->getEndpoint());
        }
    } else {
        commandMessage(tr("Cannot undo: Not enough entities defined yet."));
    }
    graphicView->redraw();
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
    return pPoints->polyline;
}

RS_PolylineData &RS_ActionDrawPolyline::getData() const{
    return pPoints->data;
}

RS_Vector &RS_ActionDrawPolyline::getPoint() const{
    return pPoints->point;
}

RS_Vector &RS_ActionDrawPolyline::getStart() const{
    return pPoints->start;
}

QList<RS_Vector> &RS_ActionDrawPolyline::getHistory() const{
    return pPoints->history;
}

QList<double> &RS_ActionDrawPolyline::getBHistory() const{
    return pPoints->bHistory;
}

LC_ActionOptionsWidget* RS_ActionDrawPolyline::createOptionsWidget(){
    return new QG_PolylineOptions();
}
