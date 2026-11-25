// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 LibreCAD.org
// Copyright (C) 2026 Dongxu Li

// Typed numbers and coordinates (back-port of #3294): a mixed number such as
// 1 1/2 is 1.5, a polar point on an axis is exact, and a polar coordinate
// is not reported as an unknown command. The line tools leave a typed
// coordinate to the event handler instead of reading it as a value.

#include <vector>

#include <QtTest>
#include <QtWidgets>

#include "lc_abstractactiondrawline.h"
#include "lc_actiondrawlinepoints.h"
#include "lc_actiondrawlinesnake.h"
#include "qg_commandwidget.h"
#include "rs_actioninterface.h"
#include "rs_block.h"
#include "rs_commandevent.h"
#include "rs_coordinateevent.h"
#include "rs_entitycontainer.h"
#include "rs_graphicview.h"
#include "rs_line.h"
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

// sends typed text to the view, as the command line does
bool type(RS_GraphicView& view, const char* text) {
    RS_CommandEvent event(QString::fromLatin1(text));
    view.commandEvent(&event);
    return event.isAccepted();
}

// a line tool drawing into a block, which is a document as the tools need;
// unlike RS_Graphic, a block writes no settings
struct LineToolFixture {
    RS_Block block{nullptr, RS_BlockData(QStringLiteral("line tool"), RS_Vector(0., 0.), false)};
    TestView view;

    LineToolFixture() {
        view.setContainer(&block);
        view.setDefaultAction(new PointProbe(block, view));
    }
    // the actions go before the view, also when a check fails
    ~LineToolFixture() { view.killAllActions(); }
};

int countLines(const RS_EntityContainer& container) {
    int lines = 0;
    for (const RS_Entity* entity : container) {
        if (entity->rtti() == RS2::EntityLine) {
            ++lines;
        }
    }
    return lines;
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

    void lineToolsTellValuesFromCoordinates() {
        struct Case {
            const char* input;
            bool value;
        };
        for (const auto& [input, value] : {Case{"10", true}, Case{"10*2", true}, Case{"max(3,4)", true},
                                           Case{"sum(1,2,3)", true}, Case{"1 1/2", true}, Case{"-5", true},
                                           Case{"10,20", false}, Case{"@10,20", false}, Case{"10<45", false},
                                           Case{"@10<45", false},
                                           // '<' is a polar coordinate even inside parentheses
                                           Case{"(10<45)", false}, Case{"min(10<45,3)", false},
                                           Case{"max(1,2),3", false}, Case{"(1,2", false}, Case{"3)+(4", false}}) {
            QVERIFY2(LC_AbstractActionDrawLine::isScalarExpressionCandidate(QString::fromLatin1(input)) == value, input);
        }
    }

    void linePointsTakeTypedCoordinates() {
        LineToolFixture fixture;
        RS_GraphicView& view = fixture.view;
        view.setRelativeZero(RS_Vector(1., 2.));
        auto* action = new LC_ActionDrawLinePoints(fixture.block, view);
        view.setCurrentAction(action);
        // one point between the ends, which end where they are typed
        action->setFixedDistanceMode(false);
        action->setPointsCount(1);
        action->setEdgePointsMode(LC_ActionDrawLinePoints::DRAW_EDGE_BOTH);
        action->setAngle(0.);
        action->setAngleIsRelative(false);

        // the start point
        QVERIFY(type(view, "10,20"));
        QCOMPARE(view.getRelativeZero(), RS_Vector(10., 20.));

        // along X, a coordinate gives the end point, not a distance of 5
        action->setSetXDirectionState();
        QCOMPARE(action->getStatus(), int(LC_AbstractActionDrawLine::SetDistance));
        QVERIFY(type(view, "30,5"));
        QCOMPARE(view.getRelativeZero(), RS_Vector(30., 20.));
        QCOMPARE(fixture.block.count(), 3u);

        // when an angle is asked for, a coordinate still gives the end point
        QVERIFY(type(view, "0,0"));
        QCOMPARE(view.getRelativeZero(), RS_Vector(0., 0.));
        action->setSetAngleDirectionState();
        QCOMPARE(action->getStatus(), int(LC_AbstractActionDrawLine::SetAngle));
        QVERIFY(type(view, "10,20"));
        QCOMPARE(view.getRelativeZero(), RS_Vector(10., 0.));

        // text that is neither a value nor a coordinate is not taken
        QVERIFY(type(view, "0,0"));
        action->setSetAngleDirectionState();
        QVERIFY(!type(view, "notanangle"));
        QCOMPARE(action->getStatus(), int(LC_AbstractActionDrawLine::SetAngle));
    }

    void snakeLengthsKeepFunctionsButNotComparisons() {
        LineToolFixture fixture;
        RS_GraphicView& view = fixture.view;
        const RS_EntityContainer& container = fixture.block;
        auto* action = new LC_ActionDrawLineSnake(fixture.block, view);
        view.setCurrentAction(action);

        QVERIFY(type(view, "0,0"));
        action->setSetXDirectionState();
        QCOMPARE(action->getStatus(), int(LC_AbstractActionDrawLine::SetDistance));

        // a function with several arguments is a length
        QVERIFY(type(view, "max(3,4)"));
        QCOMPARE(countLines(container), 1);
        QCOMPARE(view.getRelativeZero(), RS_Vector(4., 0.));

        // a comparison in parentheses is no length of 1: the coordinate reader rejects it
        type(view, "(10<45)");
        QCOMPARE(countLines(container), 1);
        QCOMPARE(view.getRelativeZero(), RS_Vector(4., 0.));
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
