/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2026 LibreCAD (librecad.org)
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
**********************************************************************/

// Shared fixture for headless navigation tests of QG_GraphicView.

#ifndef LC_NAVTEST_SUPPORT_H
#define LC_NAVTEST_SUPPORT_H

#include <functional>

#include <QApplication>
#include <QMouseEvent>
#include <QScrollBar>
#include <QStyle>
#include <QStyleOptionSlider>
#include <QWheelEvent>

#include "lc_actioncontext.h"
#include "lc_actiontestsupport.h"
#include "lc_graphicviewport.h"
#include "lc_graphicviewportlistener.h"
#include "qg_graphicview.h"
#include "rs_graphic.h"
#include "rs_line.h"

namespace lc::navtest {

struct CountingListener : LC_GraphicViewPortListener {
    int changed = 0;
    void onViewportChanged() override { ++changed; }
};

/// Reaches QScrollBar::initStyleOption (protected) to query the style's thumb geometry.
struct SliderAccess : QScrollBar {
    static void initOption(const QScrollBar* bar, QStyleOptionSlider* option) {
        auto member = &SliderAccess::initStyleOption;
        (bar->*member)(option);
    }
};

/// Reaches a few of QG_GraphicView's protected members (v2 stale-snapshot and
/// UCS-highlight tests), the same way SliderAccess reaches QScrollBar's.
struct GraphicViewAccess : QG_GraphicView {
    static void triggerSlotHScrolled(QG_GraphicView* view, const int value) {
        auto member = &GraphicViewAccess::slotHScrolled;
        (view->*member)(value);
    }
    static void triggerSlotVScrolled(QG_GraphicView* view, const int value) {
        auto member = &GraphicViewAccess::slotVScrolled;
        (view->*member)(value);
    }
    static void triggerUcsHighlightStep(QG_GraphicView* view) {
        auto member = &GraphicViewAccess::ucsHighlightStep;
        (view->*member)();
    }
};

inline QRect subControlRect(const QScrollBar* bar, const QStyle::SubControl control) {
    QStyleOptionSlider option;
    SliderAccess::initOption(bar, &option);
    return bar->style()->subControlRect(QStyle::CC_ScrollBar, &option, control, bar);
}

inline QRect thumbRect(const QScrollBar* bar) {
    return subControlRect(bar, QStyle::SC_ScrollBarSlider);
}

inline QRect grooveRect(const QScrollBar* bar) {
    return subControlRect(bar, QStyle::SC_ScrollBarGroove);
}

/// thumb length along the bar
inline int thumbLength(const QScrollBar* bar) {
    const QRect t = thumbRect(bar);
    return bar->orientation() == Qt::Horizontal ? t.width() : t.height();
}

/// pixels the thumb can travel along the groove
inline int thumbTravel(const QScrollBar* bar) {
    const QRect g = grooveRect(bar);
    return (bar->orientation() == Qt::Horizontal ? g.width() : g.height()) - thumbLength(bar);
}

inline int thumbStart(const QScrollBar* bar) {
    const QRect t = thumbRect(bar);
    return bar->orientation() == Qt::Horizontal ? t.x() : t.y();
}

inline void pump() {
    for (int i = 0; i < 3; ++i) {
        QCoreApplication::processEvents();
    }
}

inline void sendMouse(QWidget* widget, const QEvent::Type type, const QPoint pos, const Qt::MouseButton button,
                      const Qt::MouseButtons buttons) {
    QMouseEvent event(type, QPointF(pos), QPointF(widget->mapToGlobal(pos)), button, buttons, Qt::NoModifier);
    QApplication::sendEvent(widget, &event);
}

/// Sends a wheel event to \p widget; returns whether it was accepted.
inline bool sendWheel(QWidget* widget, const QPoint angleDelta, const Qt::KeyboardModifiers modifiers = Qt::NoModifier) {
    const QPoint pos = widget->rect().center();
    QWheelEvent event(QPointF(pos), QPointF(widget->mapToGlobal(pos)), QPoint(), angleDelta, Qt::NoButton, modifiers,
                      Qt::NoScrollPhase, false);
    QApplication::sendEvent(widget, &event);
    return event.isAccepted();
}

/**
 * Drags the thumb of \p bar from its centre by \p steps moves of \p stepPx each, along the bar,
 * calling \p afterStep after each move.
 */
inline void dragThumb(QScrollBar* bar, const int steps, const int stepPx,
                      const std::function<void(int)>& afterStep = nullptr) {
    const bool horizontal = bar->orientation() == Qt::Horizontal;
    const QPoint press = thumbRect(bar).center();
    auto along = [horizontal](const int d) {
        return horizontal ? QPoint(d, 0) : QPoint(0, d);
    };
    sendMouse(bar, QEvent::MouseButtonPress, press, Qt::LeftButton, Qt::LeftButton);
    for (int k = 1; k <= steps; ++k) {
        sendMouse(bar, QEvent::MouseMove, press + along(k * stepPx), Qt::NoButton, Qt::LeftButton);
        if (afterStep) {
            afterStep(k);
        }
    }
    sendMouse(bar, QEvent::MouseButtonRelease, press + along(steps * stepPx), Qt::LeftButton, Qt::NoButton);
}

/// A 100 x 50 rectangle with its lower left corner at the origin.
inline void addRectangle(RS_Graphic* graphic, const RS_Vector& min = RS_Vector(0, 0),
                         const RS_Vector& max = RS_Vector(100, 50)) {
    graphic->addEntity(new RS_Line(graphic, min, RS_Vector(max.x, min.y)));
    graphic->addEntity(new RS_Line(graphic, RS_Vector(max.x, min.y), max));
    graphic->addEntity(new RS_Line(graphic, max, RS_Vector(min.x, max.y)));
    graphic->addEntity(new RS_Line(graphic, RS_Vector(min.x, max.y), min));
}

/// the default drawing of ViewFixture: the 100 x 50 rectangle
inline void addDefaultDrawing(RS_Graphic* graphic) {
    addRectangle(graphic);
}

/**
 * A shown QG_GraphicView (offscreen) with its own drawing.
 *
 * The window is placed away from the (offscreen) cursor: an Enter event on the view
 * resumes RS_ActionDefault, which would create the QC_ApplicationWindow singleton.
 */
struct ViewFixture {
    const bool qtReady{lc::test::application() != nullptr};
    RS_Graphic* graphic = nullptr;
    LC_ActionContext context;
    QWidget* parent = nullptr;
    QG_GraphicView* view = nullptr;
    QScrollBar* hBar = nullptr;
    QScrollBar* vBar = nullptr;

    explicit ViewFixture(const QSize size = QSize(800, 600), const bool scrollbars = true,
                         const std::function<void(RS_Graphic*)>& fill = addDefaultDrawing, QWidget* parentWidget = nullptr)
        : parent(parentWidget) {
        graphic = new RS_Graphic();
        graphic->initForNewDocument();
        if (fill) {
            fill(graphic);
        }
        graphic->calculateBorders();
        view = new QG_GraphicView(parent, graphic, &context);
        view->initView();
        view->loadSettings();
        if (scrollbars) {
            view->addScrollbars();
        }
        QWidget* window = parent != nullptr ? parent : view;
        window->move(4000, 4000);
        window->resize(size);
        if (parent != nullptr) {
            view->resize(size);
        }
        window->show();
        pump();
        for (auto* bar : view->findChildren<QScrollBar*>()) {
            (bar->orientation() == Qt::Horizontal ? hBar : vBar) = bar;
        }
    }

    ~ViewFixture() {
        view->hide();
        delete view;
        graphic->setGraphicView(nullptr);
        delete graphic;
    }

    ViewFixture(const ViewFixture&) = delete;
    ViewFixture& operator=(const ViewFixture&) = delete;

    LC_GraphicViewport* viewport() const { return view->getViewPort(); }
    int ox() const { return viewport()->getOffsetX(); }
    int oy() const { return viewport()->getOffsetY(); }
    double factor() const { return viewport()->getFactor().x; }
};

} // namespace lc::navtest

#endif
