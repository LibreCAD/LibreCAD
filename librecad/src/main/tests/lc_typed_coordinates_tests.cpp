// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 LibreCAD.org
// Copyright (C) 2026 Dongxu Li

// Typed numbers and coordinates (back-port of #3294): a mixed number such as
// 1 1/2 is 1.5, a polar point on an axis is exact, and a polar coordinate
// is not reported as an unknown command.
// Issue #3302: the tools that read a number from the command line leave a typed
// point (10,10) to the event handler, which the expression parser would read as 10.

#include <initializer_list>
#include <vector>

#include <QScopeGuard>
#include <QtTest>
#include <QtWidgets>

#include "lc_actiondrawlinepoints.h"
#include "lc_actiondrawlinesnake.h"
#include "lc_actiondrawrectangle1point.h"
#include "lc_actiondrawrectangle2points.h"
#include "lc_actiondrawrectangle3points.h"
#include "lc_actiondrawstar.h"
#include "qg_commandwidget.h"
#include "rs_actioninterface.h"
#include "rs_commandevent.h"
#include "rs_coordinateevent.h"
#include "rs_entitycontainer.h"
#include "rs_graphic.h"
#include "rs_graphicview.h"
#include "rs_math.h"

namespace {

// a view that draws nothing
class TestView final : public RS_GraphicView {
public:
    int getWidth() const override { return 640; }
    int getHeight() const override { return 480; }
    void redraw(RS2::RedrawMethod = RS2::RedrawAll) override {}
    void adjustOffsetControls() override {}
    void adjustZoomControls() override {}
    void setMouseCursor(RS2::CursorType) override {}
    RS_Vector getMousePosition() const override { return RS_Vector(0., 0.); }
    void updateGridStatusWidget(QString) override {}
};

// keeps the points it is given
class PointProbe final : public RS_ActionInterface {
public:
    PointProbe(RS_EntityContainer& container, RS_GraphicView& view)
        : RS_ActionInterface("point probe", container, view) {}
    void coordinateEvent(RS_CoordinateEvent* e) override { points.push_back(e->getCoordinate()); }
    std::vector<RS_Vector> points;
};


// what the tool answers to a typed text. The event handler reads a point only when the tool
// leaves the text alone: a tool that takes it as a value never sees the point.
template <class Tool>
bool takesTypedText(RS_EntityContainer& container, RS_GraphicView& view, const char* text) {
    auto* tool = new Tool(container, view);
    view.setCurrentAction(tool);
    RS_CommandEvent event(QString::fromLatin1(text));
    tool->commandEvent(&event);
    const bool taken = event.isAccepted();
    view.killAllActions();
    return taken;
}

// the tool, after the commands, for a look at its state
template <class Tool>
Tool* startedTool(RS_EntityContainer& container, RS_GraphicView& view, std::initializer_list<const char*> commands) {
    auto* tool = new Tool(container, view);
    view.setCurrentAction(tool);
    for (const char* command : commands) {
        RS_CommandEvent event(QString::fromLatin1(command));
        view.commandEvent(&event);
    }
    return tool;
}
}

class LC_TypedCoordinatesTests : public QObject {
    Q_OBJECT

private slots:
    void mixedNumberIsWholePlusFraction() {
        struct Case {
            const char* input;
            double value;
        };
        for (const auto& [input, value] : {Case{"1 1/2", 1.5}, Case{"2 1/4", 2.25}, Case{"1  1/2", 1.5},
                                           Case{"-1 1/2", -1.5}, Case{"10 3/8\"", 10.375},
                                           Case{" 3 3 / 4 in ", 3.75},
                                           // unchanged
                                           Case{"2-1/4\"", 2.25}, Case{"2-1/4", 1.75}, Case{"1/2", 0.5},
                                           Case{"2*3", 6.}}) {
            bool ok = false;
            const double result = RS_Math::eval(QString::fromLatin1(input), &ok);
            QVERIFY2(ok, input);
            QVERIFY2(qAbs(result - value) < 1e-12, qPrintable(QString("%1 = %2").arg(input).arg(result, 0, 'g', 17)));
        }
    }

    void typedPointsUseMixedNumbersAndAreExactOnTheAxes() {
        RS_EntityContainer container;
        TestView view;
        view.setContainer(&container);
        view.setRelativeZero(RS_Vector(1., 2.));
        // an action to return to, as the application's default action is
        view.setDefaultAction(new PointProbe(container, view));
        auto* probe = new PointProbe(container, view);
        view.setCurrentAction(probe);

        struct Case {
            const char* input;
            double x;
            double y;
        };
        const std::vector<Case> cases{
            {"100<0", 100., 0.},      {"100<90", 0., 100.},     {"100<180", -100., 0.},
            {"100<270", 0., -100.},   {"100<-90", 0., -100.},   {"100<450", 0., 100.},
            {"@100<90", 1., 102.},    {"@100<-90", 1., -98.},   {"1 1/2,2", 1.5, 2.},
            {"@1 1/2,0", 2.5, 2.},    {"1 1/2<0", 1.5, 0.},
        };
        for (const auto& c : cases) {
            RS_CommandEvent event(QString::fromLatin1(c.input));
            view.commandEvent(&event);
            QVERIFY2(event.isAccepted(), c.input);
            QVERIFY2(!probe->points.empty(), c.input);
            const RS_Vector point = probe->points.back();
            // exact, not within a tolerance
            QVERIFY2(point.x == c.x && point.y == c.y,
                     qPrintable(QString("%1: (%2, %3)").arg(c.input).arg(point.x, 0, 'g', 17).arg(point.y, 0, 'g', 17)));
        }

        // off the axes, the same as RS_Vector::polar()
        RS_CommandEvent event(QStringLiteral("100<30"));
        view.commandEvent(&event);
        const RS_Vector expected = RS_Vector::polar(100., M_PI / 6.);
        QVERIFY(probe->points.back().distanceTo(expected) < 1e-12);
        view.killAllActions();
    }


    void drawingToolsLeaveTypedPointsToTheEventHandler() {
        RS_Graphic graphic;
        graphic.newDoc();
        TestView view;
        view.setContainer(&graphic);
        view.setDefaultAction(new PointProbe(graphic, view));

        // The expression parser reads 10,10 as the number 10 and 10<30 as true, and 0 is a number as
        // well: a tool that asks for a point takes none of them for a value.
        for (const char* text : {"10,10", "-3,4.5", "@10,5", "100<90", "@100<90", "10<30", "0"}) {
            const auto name = [text](const char* tool) { return QString("%1: %2").arg(tool, text).toLatin1(); };
            QVERIFY2(!takesTypedText<LC_ActionDrawRectangle2Points>(graphic, view, text),
                     name("Rectangle (2 points)").constData());
            QVERIFY2(!takesTypedText<LC_ActionDrawRectangle3Points>(graphic, view, text),
                     name("Rectangle (3 points)").constData());
            QVERIFY2(!takesTypedText<LC_ActionDrawRectangle1Point>(graphic, view, text),
                     name("Rectangle (corner, size)").constData());
            QVERIFY2(!takesTypedText<LC_ActionDrawStar>(graphic, view, text), name("Star").constData());
            QVERIFY2(!takesTypedText<LC_ActionDrawLinePoints>(graphic, view, text), name("Line Points").constData());
            QVERIFY2(!takesTypedText<LC_ActionDrawLineSnake>(graphic, view, text), name("Snake").constData());
        }
    }

    void drawingToolsStillTakeNumbersWhereANumberIsAsked() {
        RS_Graphic graphic;
        graphic.newDoc();
        TestView view;
        view.setContainer(&graphic);
        view.setDefaultAction(new PointProbe(graphic, view));
        const auto finish = qScopeGuard([&view] { view.killAllActions(); });

        auto* rectangle = startedTool<LC_ActionDrawRectangle2Points>(graphic, view, {"angle", "30", "radius", "2.5"});
        QCOMPARE(rectangle->getAngle(), 30.);
        QCOMPARE(rectangle->getRadius(), 2.5);
        view.killAllActions();

        auto* star = startedTool<LC_ActionDrawStar>(graphic, view, {"number", "7"});
        QCOMPARE(star->getRaysNumber(), 7);
        view.killAllActions();

        auto* points = startedTool<LC_ActionDrawLinePoints>(graphic, view, {"number", "4"});
        QCOMPARE(points->getPointsCount(), 4);
    }

    void aTypedFirstCornerOfARectangleMovesTheRelativeZero() {
        RS_Graphic graphic;
        graphic.newDoc();
        TestView view;
        view.setContainer(&graphic);
        view.setDefaultAction(new PointProbe(graphic, view));
        const auto finish = qScopeGuard([&view] { view.killAllActions(); });
        view.setRelativeZero(RS_Vector(0., 0.));

        // as a click on the first corner does: the next @x,y is measured from the corner
        auto* rectangle = startedTool<LC_ActionDrawRectangle2Points>(graphic, view, {"10,10"});
        QCOMPARE(rectangle->getStatus(), static_cast<int>(LC_AbstractActionDrawRectangle::SetPoint2));
        QCOMPARE(view.getRelativeZero().x, 10.);
        QCOMPARE(view.getRelativeZero().y, 10.);
    }

    void coordinatesAreNoUnknownCommands() {
        QG_CommandWidget widget(nullptr, "command_widget");
        for (const char* coordinate : {"0,0", "@10,5", "100<-90", "@100<45"}) {
            widget.teHistory->clear();
            widget.handleCommand(QString::fromLatin1(coordinate));
            QVERIFY2(!widget.teHistory->toPlainText().contains("Unknown command"), coordinate);
        }
        widget.teHistory->clear();
        widget.handleCommand(QStringLiteral("notacommand"));
        QVERIFY(widget.teHistory->toPlainText().contains("Unknown command"));

        widget.teHistory->clear();
        widget.handleCommand(QString());
        QVERIFY(widget.teHistory->toPlainText().isEmpty());
    }
};

QTEST_MAIN(LC_TypedCoordinatesTests)
#include "lc_typed_coordinates_tests.moc"
