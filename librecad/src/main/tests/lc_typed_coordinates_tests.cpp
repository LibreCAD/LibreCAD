// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 LibreCAD.org
// Copyright (C) 2026 Dongxu Li

// Typed numbers and coordinates (back-port of #3294): a mixed number such as
// 1 1/2 is 1.5, a polar point on an axis is exact, and a polar coordinate
// is not reported as an unknown command.

#include <vector>

#include <QtTest>
#include <QtWidgets>

#include "qg_commandwidget.h"
#include "rs_actioninterface.h"
#include "rs_commandevent.h"
#include "rs_coordinateevent.h"
#include "rs_entitycontainer.h"
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
