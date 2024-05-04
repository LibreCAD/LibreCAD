/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010-2011 R. van Twisk (librecad@rvt.dds.nl)
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
#include <iostream>

#include <QDebug>
#include <QGridLayout>
#include <QLabel>
#include <QMenu>
#include <QNativeGestureEvent>
#include <QPoint>
#include <QPointingDevice>
#include <QTimer>

#include "qc_applicationwindow.h"

#include "qg_blockwidget.h"
#include "qg_dialogfactory.h"
#include "qg_graphicview.h"
#include "qg_scrollbar.h"

#include "rs_actionblocksedit.h"
#include "rs_actiondefault.h"
#include "rs_actionmodifydelete.h"
#include "rs_actionmodifyentity.h"
#include "rs_actionselectsingle.h"
#include "rs_actionzoomauto.h"
#include "rs_actionzoomin.h"
#include "rs_actionzoompan.h"
#include "rs_actionzoomscroll.h"
#include "rs_blocklist.h"
#include "rs_debug.h"
#include "rs_eventhandler.h"
#include "rs_graphic.h"
#include "rs_insert.h"
#include "rs_math.h"
#include "rs_modification.h"
#include "rs_painterqt.h"
#include "rs_settings.h"

#ifdef EMU_C99
#include "emu_c99.h"
#endif

#if (defined (Q_OS_WIN32) || defined (Q_OS_WIN64))
    #define CURSOR_SIZE 16
#else
    #define CURSOR_SIZE 15
#endif

namespace {
// maximum length for displayed block name in context menu
constexpr int g_MaxBlockNameLength = 40;

/*
         * The zoomFactor effects how quickly the scroll wheel will zoom in & out.
         *
         * Benchmarks:
         * 1.250 - the original; fast & usable, but seems a choppy & a bit 'jarring'
         * 1.175 - still a bit choppy
         * 1.150 - smoother than the original, but still 'quick' enough for good navigation.
         * 1.137 - seems to work well for me
         * 1.125 - about the lowest that would be acceptable and useful, a tad on the slow side for me
         * 1.100 - a very slow & deliberate zooming, but feels very "cautious", "controlled", "safe", and "precise".
         * 1.000 - goes nowhere. :)
         */
constexpr double zoomFactor = 1.137;
// zooming factor is wheel angle delta divided by this factor
constexpr double zoomWheelDivisor = 200.;


/**
 * @brief snapEntity find the closest entity
 * @param QG_GraphicView& view - the graphic view
 * @param const QMouseEvent* event - the mouse event
 * @return RS_Entity* - the closest entity within the range of CURSOR_SIZE
 *                      returns nullptr, if no entity is found in range
 */
RS_Entity* snapEntity(const QG_GraphicView& view, const QMouseEvent* event)
{
    if (event == nullptr)
        return nullptr;
    RS_EntityContainer* container = view.getContainer();
    if (container==nullptr)
        return nullptr;
    const QPointF mapped = event->pos();
    double distance = RS_MAXDOUBLE;
    RS_Entity* entity = container->getNearestEntity(view.toGraph(mapped), &distance);

    return (view.toGuiDX(distance) <= CURSOR_SIZE) ? entity : nullptr;
}

// Find an ancestor of the RS_Insert type.
// Return nullptr, if none is found
RS_Insert* getAncestorInsert(RS_Entity* entity)
{
    while(entity != nullptr) {
        if (entity->rtti() == RS2::EntityInsert) {
            RS_Insert* parent = getAncestorInsert(entity->getParent());
            return parent != nullptr ? parent : static_cast<RS_Insert*>(entity);
        }
        entity = entity->getParent();
    }
    return nullptr;
}

// whether the current insert is part of Text
RS_Entity* getParentText(RS_Insert* insert)
{
    if (insert == nullptr || insert->getBlock() != nullptr || insert->getParent() == nullptr)
        return nullptr;
    switch(insert->getParent()->rtti()) {
    case RS2::EntityText:
    case RS2::EntityMText:
        return insert->getParent();
    default:
        return nullptr;
    }
}

// Start the edit action:
// Edit Block for an insert
// Edit entity, otherwise
void editAction(QG_GraphicView& view, RS_Entity& entity)
{
    RS_EntityContainer* container = view.getContainer();
    if (container==nullptr)
        return;

    switch(entity.rtti()) {
    case RS2::EntityInsert:
    {
        auto& appWindow = QC_ApplicationWindow::getAppWindow();
        RS_BlockList* blockList = appWindow->getBlockWidget()->getBlockList();
        RS_Block* active = (blockList != nullptr) ? blockList->getActive() : nullptr;
        auto* insert = static_cast<RS_Insert*>(&entity);
        RS_Block* current = insert->getBlockForInsert();
        if (current == active)
            active=nullptr;
        else
            blockList->activate(current);
        std::shared_ptr<RS_Block*> scoped{&active, [blockList](RS_Block** pointer) {
                if (pointer && *pointer != nullptr)
                    blockList->activate(*pointer);
            }};
        auto* action = new RS_ActionBlocksEdit(*container, view);
        if (action == nullptr)
            return;
        view.setCurrentAction(action);
    }
        break;
    default:
    {
        auto* action = new RS_ActionModifyEntity(*container, view);
        if (action == nullptr)
            return;
        action->setEntity(&entity);
        view.setCurrentAction(action);
        action->trigger();
        action->finish(false);
    }
    }
}

void launchEditProperty(QG_GraphicView& view, RS_Entity* entity)
{
    RS_EntityContainer* container = view.getContainer();
    if (entity == nullptr || container == nullptr)
        return;

    editAction(view, *entity);

    //container->removeEntity(entity);
    auto* doc = dynamic_cast<RS_Document*>(container);
    if (doc != nullptr)
        doc->startUndoCycle();
    // delete any temporary highlighting duplicates of the original
    auto* defaultAction = dynamic_cast<RS_ActionDefault*>(view.getEventHandler()->getDefaultAction());
    if (defaultAction != nullptr)
    {
        defaultAction->clearHighLighting();
    }
    doc->endUndoCycle();
}

// Show the entity property dialog on the closest entity in range
void showEntityPropertiesDialog(QG_GraphicView& view, RS_Entity* entity)
{
    if (entity == nullptr) return;

    // snap to the top selected parent
    while (entity != nullptr && entity->getParent() != nullptr && entity->getParent()->isSelected())
        entity = entity->getParent();

    launchEditProperty(view, entity);
}
}

// Support auto-panning when the cursor is close to the view border
struct QG_GraphicView::AutoPanData
{
    void start(double interval, QG_GraphicView &view)
    {
        m_delayCounter = 0;
        panTimer = std::make_unique<QTimer>(&view);
        panTimer->start(interval);
        connect(panTimer.get(), &QTimer::timeout, &view, &QG_GraphicView::autoPanStep);
    }

    std::unique_ptr<QTimer> panTimer;

    QPoint panOffset;

    unsigned m_delayCounter = 0u;
    // skip the first events, to avoid unintensional panning
    const unsigned delayCounterMax = 10u;
    const double panOffsetMagnitude = 20.0;

    const double panTimerInterval_minimum = 20.0;
    const double panTimerInterval_maximum = 100.0;

    // the sensitive border of the view
    const RS_Vector probedAreaOffset = {50 /* pixels */, 50 /* pixels */};
};


/**
 * Constructor.
 */
QG_GraphicView::QG_GraphicView(QWidget* parent, Qt::WindowFlags f, RS_Document* doc)
    :RS_GraphicView(parent, f)
    ,device("Mouse")
    ,curCad(new QCursor(QPixmap(":ui/cur_cad_bmp.png"), CURSOR_SIZE, CURSOR_SIZE))
    ,curDel(new QCursor(QPixmap(":ui/cur_del_bmp.png"), CURSOR_SIZE, CURSOR_SIZE))
    ,curSelect(new QCursor(QPixmap(":ui/cur_select_bmp.png"), CURSOR_SIZE, CURSOR_SIZE))
    ,curMagnifier(new QCursor(QPixmap(":ui/cur_glass_bmp.png"), CURSOR_SIZE, CURSOR_SIZE))
    ,curHand(new QCursor(QPixmap(":ui/cur_hand_bmp.png"), CURSOR_SIZE, CURSOR_SIZE))
    ,redrawMethod(RS2::RedrawAll)
    ,isSmoothScrolling(false)
    , m_panData{std::make_unique<AutoPanData>()}
{
    RS_DEBUG->print("QG_GraphicView::QG_GraphicView()..");

    if (doc != nullptr)
    {
        setContainer(doc);
        doc->setGraphicView(this);
        setDefaultAction(new RS_ActionDefault(*doc, *this));
    }

    setFactorX(4.0);
    setFactorY(4.0);
    setBorders(10, 10, 10, 10);

    setMouseTracking(true);
    setFocusPolicy(Qt::NoFocus);

    // SourceForge issue 45 (Left-mouse drag shrinks window)
    setAttribute(Qt::WA_NoMousePropagation);

    view_rect = LC_Rect(toGraph(0, 0), toGraph(getWidth(), getHeight()));
}



/**
 * Destructor
 */
QG_GraphicView::~QG_GraphicView() {
	cleanUp();
}



/**
 * @return width of widget.
 */
int QG_GraphicView::getWidth() const
{
    if (scrollbars)
        return width() - vScrollBar->sizeHint().width();
    else
        return width();
}



/**
 * @return height of widget.
 */
int QG_GraphicView::getHeight() const
{
    if (scrollbars)
        return height() - hScrollBar->sizeHint().height();
    else
        return height();
}


/**
 * Changes the current background color of this view.
 */
void QG_GraphicView::setBackground(const RS_Color& bg) {
    RS_GraphicView::setBackground(bg);

    QPalette palette;
    palette.setColor(backgroundRole(), bg);
    setPalette(palette);
}


/**
 * Sets the mouse cursor to the given type.
 */
void QG_GraphicView::setMouseCursor(RS2::CursorType cursorType) {

    switch (cursorType) {
    default:
    case RS2::ArrowCursor:
        setCursor(Qt::ArrowCursor);
        break;
    case RS2::UpArrowCursor:
        setCursor(Qt::UpArrowCursor);
        break;
    case RS2::CrossCursor:
        setCursor(Qt::CrossCursor);
        break;
    case RS2::WaitCursor:
        setCursor(Qt::WaitCursor);
        break;
    case RS2::IbeamCursor:
        setCursor(Qt::IBeamCursor);
        break;
    case RS2::SizeVerCursor:
        setCursor(Qt::SizeVerCursor);
        break;
    case RS2::SizeHorCursor:
        setCursor(Qt::SizeHorCursor);
        break;
    case RS2::SizeBDiagCursor:
        setCursor(Qt::SizeBDiagCursor);
        break;
    case RS2::SizeFDiagCursor:
        setCursor(Qt::SizeFDiagCursor);
        break;
    case RS2::SizeAllCursor:
        setCursor(Qt::SizeAllCursor);
        break;
    case RS2::BlankCursor:
        setCursor(Qt::BlankCursor);
        break;
    case RS2::SplitVCursor:
        setCursor(Qt::SplitVCursor);
        break;
    case RS2::SplitHCursor:
        setCursor(Qt::SplitHCursor);
        break;
    case RS2::PointingHandCursor:
        setCursor(Qt::PointingHandCursor);
        break;
    case RS2::ForbiddenCursor:
        setCursor(Qt::ForbiddenCursor);
        break;
    case RS2::WhatsThisCursor:
        setCursor(Qt::WhatsThisCursor);
        break;
    case RS2::OpenHandCursor:
        setCursor(Qt::OpenHandCursor);
        break;
    case RS2::ClosedHandCursor:
        setCursor(Qt::ClosedHandCursor);
        break;
    case RS2::CadCursor:
        cursor_hiding
            ? setCursor(Qt::BlankCursor)
            : setCursor(*curCad);
        break;
    case RS2::DelCursor:
        setCursor(*curDel);
        break;
    case RS2::SelectCursor:
        setCursor(*curSelect);
        break;
    case RS2::MagnifierCursor:
        setCursor(*curMagnifier);
        break;
    case RS2::MovingHandCursor:
        setCursor(*curHand);
        break;

    }
}


/**
 * Sets the text for the grid status widget in the left bottom corner.
 */
void QG_GraphicView::updateGridStatusWidget(QString text)
{
    emit gridStatusChanged(std::move(text));
}


/**
 * Redraws the widget.
 */
void QG_GraphicView::redraw(RS2::RedrawMethod method) {
        redrawMethod=(RS2::RedrawMethod ) (redrawMethod | method);
        update(); // Paint when reeady to pain
//	repaint(); //Paint immediate
}


void QG_GraphicView::resizeEvent(QResizeEvent* /*e*/) {
    RS_DEBUG->print("QG_GraphicView::resizeEvent begin");
    adjustOffsetControls();
    adjustZoomControls();
//     updateGrid();
        // Small hack, delete the snapper during resizes
        getOverlayContainer(RS2::Snapper)->clear();
        redraw();
    RS_DEBUG->print("QG_GraphicView::resizeEvent end");
}

void QG_GraphicView::mousePressEvent(QMouseEvent* event)
{
    // pan zoom with middle mouse button
    if (event->button()==Qt::MiddleButton)
    {
        setCurrentAction(new RS_ActionZoomPan(*container, *this));
    }
    eventHandler->mousePressEvent(event);
}

void QG_GraphicView::mouseDoubleClickEvent(QMouseEvent* e)
{
    switch(e->button())
    {
        default:
            break;
        case Qt::MiddleButton:
            setCurrentAction(new RS_ActionZoomAuto(*container, *this));
            break;
        case Qt::LeftButton:
            if (menus.contains("Double-Click"))
            {
                killAllActions();
                menus["Double-Click"]->popup(mapToGlobal(e->pos()));
            } else {
                // double click on an entity to edit entity properties
                showEntityPropertiesDialog(*this, getDefaultAction()->catchEntity(e));
            }
            break;
    }
    e->accept();
}


void QG_GraphicView::mouseReleaseEvent(QMouseEvent* event)
{
    RS_DEBUG->print("QG_GraphicView::mouseReleaseEvent");

    event->accept();

    switch (event->button())
    {
    case Qt::RightButton:
        if (event->modifiers()==Qt::ControlModifier)
        {
            if (menus.contains("Ctrl+Right-Click"))
            {
                menus["Ctrl+Right-Click"]->popup(mapToGlobal(event->pos()));
                break;
            }
        }
        if (event->modifiers()==Qt::ShiftModifier)
        {
            if (menus.contains("Shift+Right-Click"))
            {
                menus["Shift+Right-Click"]->popup(mapToGlobal(event->pos()));
                break;
            }
        }

        if (!eventHandler->hasAction())
        {
            if (menus.contains("Right-Click"))
            {
                menus["Right-Click"]->popup(mapToGlobal(event->pos()));
            }
            else
            {
                QMenu* context_menu = new QMenu(this);
                context_menu->setAttribute(Qt::WA_DeleteOnClose);
                if (!recent_actions.empty())
                    context_menu->addActions(recent_actions);

                // "Edit Entity" entry
                addEditEntityEntry(event, *context_menu);
                // Add drawing preferences
                QAction* OptionsDrawing = QC_ApplicationWindow::getAppWindow()->getAction("OptionsDrawing");
                if (OptionsDrawing != nullptr)
                    context_menu->addAction(OptionsDrawing);
                if (!context_menu->isEmpty())
                    context_menu->exec(mapToGlobal(event->pos()));
                else
                    delete context_menu;

            }
        }
        else back();
        break;

    case Qt::XButton1:
        enter();
        emit xbutton1_released();
        break;

    default:
        eventHandler->mouseReleaseEvent(event);
        break;
    }
    RS_DEBUG->print("QG_GraphicView::mouseReleaseEvent: OK");
}

void QG_GraphicView::addEditEntityEntry(QMouseEvent* event, QMenu& contextMenu)
{
    RS_Entity* entity = snapEntity(*this, event);
    if (entity == nullptr)
    return;
    if (container==nullptr)
        return;
    RS_Insert* insert = getAncestorInsert(entity);
    // MText/Text should not be edited as blocks
    RS_Entity* toEdit = getParentText(insert) != nullptr ? getParentText(insert) : nullptr;
    if (toEdit != nullptr) {
        insert = nullptr;
        entity = toEdit;
    }
    QAction* action = (insert != nullptr) ?
                // For an insert, show the menu entry to edit the block instead
                new QAction(QIcon(":/ui/blockedit.png"),
                            QString{"%1: %2"}.arg(tr("Edit Block")).arg(insert->getName().left(g_MaxBlockNameLength)),
                            &contextMenu) :
                new QAction(QIcon(":/extui/modifyentity.png"),
                            tr("Edit Properties"), &contextMenu);

    contextMenu.addAction(action);
    connect(action, &QAction::triggered, this, [this, insert, entity](){
        launchEditProperty(*this, insert != nullptr ? insert : entity);
    });
}

void QG_GraphicView::mouseMoveEvent(QMouseEvent* event)
{
    if (isAutoPan(event)) {
        startAutoPanTimer(event);
        event->accept();
        return;
    }
    m_panData->panTimer.reset();
    // handle auto-panning
    event->accept();
    eventHandler->mouseMoveEvent(event);
}

bool QG_GraphicView::event(QEvent *event)
{
    if (event->type() == QEvent::NativeGesture) {
        QNativeGestureEvent *nge = static_cast<QNativeGestureEvent *>(event);

        if (nge->gestureType() == Qt::ZoomNativeGesture) {
            double v = nge->value();
            RS2::ZoomDirection direction;
            double factor;

            if (v < 0) {
                direction = RS2::Out;
                factor = 1-v;
            } else {
                direction = RS2::In;
                factor = 1+v;
            }

            // It seems the NativeGestureEvent::pos() incorrectly reports global coordinates
            QPoint g = mapFromGlobal(nge->globalPosition().toPoint());
            RS_Vector mouse = toGraph(g.x(), g.y());
            setCurrentAction(new RS_ActionZoomIn(*container, *this, direction,
												 RS2::Both, &mouse, factor));
        }

        return true;
    }
    return QWidget::event(event);
}

/**
 * support for the wacom graphic tablet.
 */
void QG_GraphicView::tabletEvent(QTabletEvent* e) {
    if (testAttribute(Qt::WA_UnderMouse)) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        switch(e->pointerType()) {
        case QPointingDevice::PointerType::Eraser:
            if (e->type()==QEvent::TabletRelease) {
                if (container) {

                    RS_ActionSelectSingle* a =
                        new RS_ActionSelectSingle(*container, *this);
                    setCurrentAction(a);
                    QMouseEvent ev(QEvent::MouseButtonRelease, e->position(), e->globalPosition(),
                                   Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);//RLZ
                    mouseReleaseEvent(&ev);
                    a->finish();

                    if (container->countSelected()>0) {
                        setCurrentAction(
                            new RS_ActionModifyDelete(*container, *this));
                    }
                }
            }
            break;

        case QPointingDevice::PointerType::Generic:
        case QPointingDevice::PointerType::Pen:
        case QPointingDevice::PointerType::Cursor:
            if (e->type()==QEvent::TabletPress) {
                QMouseEvent ev(QEvent::MouseButtonPress, e->position(), e->globalPosition(),
                               Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);//RLZ
                mousePressEvent(&ev);
            } else if (e->type()==QEvent::TabletRelease) {
                QMouseEvent ev(QEvent::MouseButtonRelease, e->position(), e->globalPosition(),
                               Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);//RLZ
                mouseReleaseEvent(&ev);
            } else if (e->type()==QEvent::TabletMove) {
                QMouseEvent ev(QEvent::MouseMove, e->position(), e->globalPosition(),
                               Qt::NoButton, {}, Qt::NoModifier);//RLZ
                mouseMoveEvent(&ev);
            }
            break;
        default:
            break;


        }

#else
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
        switch (e->deviceType()) {
#else
        switch (e->device()) {
#endif
        case QTabletEvent::Eraser:
            if (e->type()==QEvent::TabletRelease) {
                if (container) {

                    RS_ActionSelectSingle* a =
                        new RS_ActionSelectSingle(*container, *this);
                    setCurrentAction(a);
                    QMouseEvent ev(QEvent::MouseButtonRelease, e->position(),
                                   Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);//RLZ
                    mouseReleaseEvent(&ev);
                    a->finish();

                    if (container->countSelected()>0) {
                        setCurrentAction(
                            new RS_ActionModifyDelete(*container, *this));
                    }
                }
            }
            break;

        case QTabletEvent::Stylus:
        case QTabletEvent::Puck:
            if (e->type()==QEvent::TabletPress) {
                QMouseEvent ev(QEvent::MouseButtonPress, e->position(),
                               Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);//RLZ
                mousePressEvent(&ev);
            } else if (e->type()==QEvent::TabletRelease) {
                QMouseEvent ev(QEvent::MouseButtonRelease, e->position(),
                               Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);//RLZ
                mouseReleaseEvent(&ev);
            } else if (e->type()==QEvent::TabletMove) {
                QMouseEvent ev(QEvent::MouseMove, e->position(),
                               Qt::NoButton, {}, Qt::NoModifier);//RLZ
                mouseMoveEvent(&ev);
            }
            break;

        default:
            break;
        }
#endif
    }

    // a 'mouse' click:
    /*if (e->pressure()>10 && lastPressure<10) {
        QMouseEvent e(QEvent::MouseButtonPress, e->pos(),
           Qt::LeftButton, Qt::LeftButton);
        mousePressEvent(&e);
}
    else if (e->pressure()<10 && lastPressure>10) {
        QMouseEvent e(QEvent::MouseButtonRelease, e->pos(),
           Qt::LeftButton, Qt::LeftButton);
        mouseReleaseEvent(&e);
}	else if (lastPos!=e->pos()) {
        QMouseEvent e(QEvent::MouseMove, e->pos(),
           Qt::NoButton, 0);
        mouseMoveEvent(&e);
}

    lastPressure = e->pressure();
    lastPos = e->pos();
    */
}

void QG_GraphicView::leaveEvent(QEvent* e) {
    // stop auto-panning
    m_panData->panTimer.reset();

    eventHandler->mouseLeaveEvent();
    QWidget::leaveEvent(e);
}

void QG_GraphicView::enterEvent(QEnterEvent* e) {
    eventHandler->mouseEnterEvent();
    QWidget::enterEvent(e);
}


void QG_GraphicView::focusOutEvent(QFocusEvent* e) {
    QWidget::focusOutEvent(e);
}


void QG_GraphicView::focusInEvent(QFocusEvent* e) {
    eventHandler->mouseEnterEvent();
    QWidget::focusInEvent(e);
}

/**
 * mouse wheel event. zooms in/out or scrolls when
 * shift or ctrl is pressed.
 */
void QG_GraphicView::wheelEvent(QWheelEvent *e) {
    //RS_DEBUG->print("wheel: %d", e->delta());

    //printf("state: %d\n", e->state());
    //printf("ctrl: %d\n", Qt::ControlButton);

    if (container==nullptr) {
        return;
    }

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    RS_Vector mouse = toGraph(e->position());
#else
    RS_Vector mouse = toGraph(e->position());
#endif

    if (device == "Trackpad")
    {
        QPoint numPixels = e->pixelDelta();

        // high-resolution scrolling triggers Pan instead of Zoom logic
        isSmoothScrolling |= !numPixels.isNull();

        if (isSmoothScrolling)
        {
            if (e->phase() == Qt::ScrollEnd) isSmoothScrolling = false;
        }
        else // Trackpads that without high-resolution scrolling
             // e.g. libinput-XWayland trackpads
        {
            numPixels = e->angleDelta() / 4;
        }

        if (!numPixels.isNull())
        {
            if (e->modifiers()==Qt::ControlModifier)
            {
                RS_SETTINGS->beginGroup("/Defaults");
                bool invZoom = (RS_SETTINGS->readNumEntry("/InvertZoomDirection", 0) == 1);
                RS_SETTINGS->endGroup();

                // Hold ctrl to zoom. 1 % per pixel
                double v = (invZoom) ? (numPixels.y() / zoomWheelDivisor) : (-numPixels.y() / zoomWheelDivisor);
                RS2::ZoomDirection direction;
                double factor;

                if (v < 0) {
                    direction = RS2::Out; factor = 1-v;
                } else {
                    direction = RS2::In;  factor = 1+v;
                }

                setCurrentAction(new RS_ActionZoomIn(*container, *this, direction,
                                                     RS2::Both, &mouse, factor));
            }
            else
            {
                RS_SETTINGS->beginGroup("/Defaults");
                bool inv_h = (RS_SETTINGS->readNumEntry("/WheelScrollInvertH", 0) == 1);
                bool inv_v = (RS_SETTINGS->readNumEntry("/WheelScrollInvertV", 0) == 1);
                RS_SETTINGS->endGroup();

                int hDelta = (inv_h) ? -numPixels.x() : numPixels.x();
                int vDelta = (inv_v) ? -numPixels.y() : numPixels.y();

                // scroll by scrollbars: issue #479 (it has its own issues)
                if (scrollbars)
                {
                    hScrollBar->setValue(hScrollBar->value() - hDelta);
                    vScrollBar->setValue(vScrollBar->value() - vDelta);
                }
                else
                {
                    setCurrentAction(new RS_ActionZoomScroll(hDelta, vDelta,
                                                             *container, *this));
                }
            }
            redraw();
        }
        e->accept();
        return;
    }

    if (e->angleDelta().isNull()) {
        // A zero delta event occurs when smooth scrolling is ended. Ignore this
        e->accept();
        return;
    }

    bool scroll = false;
    RS2::Direction direction = RS2::Up;

    // scroll up / down:
    if (e->modifiers()==Qt::ControlModifier) {
        scroll = true;
        if (e->angleDelta().y() == 0){
        //case Qt::Horizontal:
            direction=(e->angleDelta().x()>0)?RS2::Left : RS2::Right;
        } else {
        //case Qt::Vertical:
            direction=(e->angleDelta().y()>0)?RS2::Up : RS2::Down;
        }
    }

    // scroll left / right:
    else if	(e->modifiers()==Qt::ShiftModifier) {
        scroll = true;
        if (e->angleDelta().y() == 0){
        //case Qt::Horizontal:
            direction=(e->angleDelta().x()>0)?RS2::Up : RS2::Down;
        } else {
        //case Qt::Vertical:
            direction=(e->angleDelta().x()>0)?RS2::Left : RS2::Right;
        }
    }

    if (scroll && scrollbars) {
		//scroll by scrollbars: issue #479

        RS_SETTINGS->beginGroup("/Defaults");
        bool inv_h = (RS_SETTINGS->readNumEntry("/WheelScrollInvertH", 0) == 1);
        bool inv_v = (RS_SETTINGS->readNumEntry("/WheelScrollInvertV", 0) == 1);
        RS_SETTINGS->endGroup();

        int delta = 0;

		switch(direction){
		case RS2::Left:
		case RS2::Right:
            delta = (inv_h) ? -e->angleDelta().x() : e->angleDelta().x();
			hScrollBar->setValue(hScrollBar->value()+delta);
			break;
		default:
            delta = (inv_v) ? -e->angleDelta().y() : e->angleDelta().y();
			vScrollBar->setValue(vScrollBar->value()+delta);
		}

//        setCurrentAction(new RS_ActionZoomScroll(direction,
//                         *container, *this));
    }

    // zoom in / out:
    else if (e->modifiers()==0) {
        auto groupGuard = RS_SETTINGS->beginGroupGuard("/Defaults");
        bool invZoom = RS_SETTINGS->readNumEntry("/InvertZoomDirection", 0) == 1;

        RS2::ZoomDirection zoomDirection = ((e->angleDelta().y() > 0) != invZoom) ? RS2::In : RS2::Out;

        RS_Vector& zoomCenter = mouse;

        setCurrentAction(new RS_ActionZoomIn(*container, *this, zoomDirection, RS2::Both, &zoomCenter, zoomFactor));
    }
    redraw();

    QMouseEvent event
    {
        QEvent::MouseMove,
#if (QT_VERSION >= QT_VERSION_CHECK(5, 15, 0))
            e->position(),
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
            e->globalPosition(),
#endif
#else
            QPointF{static_cast<qreal>(e->x()), static_cast<qreal>(e->y())},
#endif
            Qt::NoButton, Qt::NoButton, Qt::NoModifier
    };
    eventHandler->mouseMoveEvent(&event);

    e->accept();
}


    void QG_GraphicView::keyPressEvent(QKeyEvent * e)
    {
        if (container == nullptr) {
            return;
        }

        bool scroll = false;
        RS2::Direction direction = RS2::Up;

        switch (e->key()) {
        case Qt::Key_Left:
            scroll = true;
            direction = RS2::Right;
            break;
        case Qt::Key_Right:
            scroll = true;
            direction = RS2::Left;
            break;
        case Qt::Key_Up:
            scroll = true;
            direction = RS2::Up;
            break;
        case Qt::Key_Down:
            scroll = true;
            direction = RS2::Down;
            break;
        default:
            scroll = false;
            break;
        }

        if (scroll) {
            setCurrentAction(new RS_ActionZoomScroll(direction, *container, *this));
        }
        eventHandler->keyPressEvent(e);
    }

    void QG_GraphicView::keyReleaseEvent(QKeyEvent * e)
    {
        eventHandler->keyReleaseEvent(e);
    }

    /**
 * Called whenever the graphic view has changed.
 * Adjusts the scrollbar ranges / steps.
 */
    void QG_GraphicView::adjustOffsetControls()
    {
        if (scrollbars) {
            static bool running = false;

            if (running) {
                return;
        }

        running = true;

        if (container==nullptr || hScrollBar==nullptr || vScrollBar==nullptr) {
            return;
        }

        int ox = getOffsetX();
        int oy = getOffsetY();

        RS_Vector min = container->getMin();
        RS_Vector max = container->getMax();

        // no drawing yet - still allow to scroll
        if (max.x < min.x+1.0e-6 ||
                max.y < min.y+1.0e-6 ||
                    max.x > RS_MAXDOUBLE ||
                    max.x < RS_MINDOUBLE ||
                    min.x > RS_MAXDOUBLE ||
                    min.x < RS_MINDOUBLE ||
                    max.y > RS_MAXDOUBLE ||
                    max.y < RS_MINDOUBLE ||
                    min.y > RS_MAXDOUBLE ||
                    min.y < RS_MINDOUBLE ) {
            min = RS_Vector(-10,-10);
            max = RS_Vector(100,100);
        }

        auto factor = getFactor();

        int minVal = (int)(-getWidth()*0.75
                           + std::min(min.x, 0.)*factor.x);
        int maxVal = (int)(-getWidth()*0.25
                           + std::max(max.x, 0.)*factor.x);

        if (minVal<=maxVal) {
            hScrollBar->setRange(minVal, maxVal);
        }

        minVal = (int)(+getHeight()*0.25
                       - std::max(max.y, 0.)*factor.y);
        maxVal = (int)(+getHeight()*0.75
                       - std::min(min.y, 0.)*factor.y);

        if (minVal<=maxVal) {
            vScrollBar->setRange(minVal, maxVal);
        }

        hScrollBar->setPageStep(getWidth());
        vScrollBar->setPageStep(getHeight());

        hScrollBar->setValue(-ox);
        vScrollBar->setValue(oy);


        slotHScrolled(-ox);
        slotVScrolled(oy);


//        RS_DEBUG->print("H min: %d / max: %d / step: %d / value: %d\n",
//                        hScrollBar->minimum(), hScrollBar->maximum(),
//                        hScrollBar->pageStep(), ox);

//        RS_DEBUG->print(/*RS_Debug::D_WARNING, */"V min: %d / max: %d / step: %d / value: %d\n",
//                        vScrollBar->minimum(), vScrollBar->maximum(),
//                        vScrollBar->pageStep(), oy);


        running = false;
    }
}


/**
 * override this to adjust controls and widgets that
 * control the zoom factor of the graphic.
 */
void QG_GraphicView::adjustZoomControls() {}


/**
 * Slot for horizontal scroll events.
 */
void QG_GraphicView::slotHScrolled(int value) {
    // Scrollbar behaviour tends to change with every Qt version..
    // so let's keep old code in here for now

    //static int running = false;
    //if (!running) {
    //running = true;
    ////RS_DEBUG->print("value x: %d\n", value);
    if (hScrollBar->maximum()==hScrollBar->minimum()) {
        centerOffsetX();
    } else {
        setOffsetX(-value);
    }
    //if (isUpdateEnabled()) {
//         updateGrid();
    redraw();
}


/**
 * Slot for vertical scroll events.
 */
void QG_GraphicView::slotVScrolled(int value) {
    // Scrollbar behaviour tends to change with every Qt version..
    // so let's keep old code in here for now

    //static int running = false;
    //if (!running) {
    //running = true;
//    DEBUG_HEADER
//	RS_DEBUG->print(/*RS_Debug::D_WARNING,*/ "%s %s(): set vertical offset from %d to %d\n",
//                    __FILE__, __func__, getOffsetY(), value);
    if (vScrollBar->maximum()==vScrollBar->minimum()) {
        centerOffsetY();
    } else {
        setOffsetY(value);
    }
    //if (isUpdateEnabled()) {
  //  updateGrid();
    redraw();
}
/**
 * @brief setOffset
 * @param ox, offset X
 * @param oy, offset Y
 */
void QG_GraphicView::setOffset(int ox, int oy) {
//    DEBUG_HEADER
//    qDebug()<<"adjusting offset from ("<<getOffsetX()<<","<<getOffsetY()<<") to ("<<ox<<" , "<<oy<<")";
    RS_GraphicView::setOffset(ox, oy);
    // need to adjust offset control for scrollbars when setting graphicview offset
    adjustOffsetControls();
}

RS_Vector QG_GraphicView::getMousePosition() const
{
    //find mouse position
    QPoint vp=mapFromGlobal(QCursor::pos());
    //if cursor is not on widget, return the widget center position
    if(!rect().contains(vp))
        vp=QPoint(width()/2, height()/2);
    return toGraph(vp.x(), vp.y());
}

void QG_GraphicView::getPixmapForView(std::unique_ptr<QPixmap>& pm)
{
	QSize const s0(getWidth(), getHeight());
	if(pm && pm->size()==s0)
		return;
	pm.reset(new QPixmap(getWidth(), getHeight()));
}

void QG_GraphicView::layerActivated(RS_Layer *layer) {
	RS_SETTINGS->beginGroup("/Modify");
	bool toActivated= (RS_SETTINGS->readNumEntry("/ModifyEntitiesToActiveLayer", 0)==1);
	RS_SETTINGS->endGroup();

	if(!toActivated) return;
    RS_EntityContainer *container = this->getContainer();
    RS_Graphic* graphic = this->getGraphic();
    QList<RS_Entity*> clones;

    if (graphic) {
        graphic->startUndoCycle();
    }

    for (auto en: *container) {
        if (!en) continue;
        if (!en->isSelected()) continue;

        RS_Entity* cl = en->clone();
        cl->setLayer(layer);
        this->deleteEntity(en);
        en->setSelected(false);
        cl->setSelected(false);
        clones << cl;

        if (!graphic) continue;

        en->setUndoState(true);
        graphic->addUndoable(en);
    }

    for (auto cl: clones) {
        container->addEntity(cl);
        this->drawEntity(cl);

        if (!graphic) continue;

        graphic->addUndoable(cl);
    }

    if (graphic) {
        graphic->endUndoCycle();
        graphic->updateInserts();
    }

    container->calculateBorders();
    container->setSelected(false);
    redraw(RS2::RedrawDrawing);
}


/**
 * Handles paint events by redrawing the graphic in this view.
 * usually that's very fast since we only paint the buffer we
 * have from the last call..
 */
void QG_GraphicView::paintEvent(QPaintEvent *)
{

    // Re-Create or get the layering pixmaps
    getPixmapForView(PixmapLayer1);
    getPixmapForView(PixmapLayer2);
    getPixmapForView(PixmapLayer3);

    // Draw Layer 1
    if (redrawMethod & RS2::RedrawGrid)
    {
        PixmapLayer1->fill(getBackground());
        RS_PainterQt painter1(PixmapLayer1.get());
        drawLayer1((RS_Painter*)&painter1);
        painter1.end();
    }

    if (redrawMethod & RS2::RedrawDrawing)
    {
        view_rect = LC_Rect(toGraph(0, 0),
                            toGraph(getWidth(), getHeight()));
        // DRaw layer 2
        PixmapLayer2->fill(Qt::transparent);
        RS_PainterQt painter2(PixmapLayer2.get());
        if (antialiasing)
        {
            painter2.setRenderHint(QPainter::Antialiasing);
        }
        painter2.setDrawingMode(drawingMode);
        painter2.setDrawSelectedOnly(false);
        drawLayer2((RS_Painter*)&painter2);
        painter2.setDrawSelectedOnly(true);
        drawLayer2((RS_Painter*)&painter2);
        painter2.end();
    }

    if (redrawMethod & RS2::RedrawOverlay)
    {
        PixmapLayer3->fill(Qt::transparent);
        RS_PainterQt painter3(PixmapLayer3.get());
        if (antialiasing)
        {
            painter3.setRenderHint(QPainter::Antialiasing);
        }
        drawLayer3((RS_Painter*)&painter3);
        painter3.end();
    }

    // Finally paint the layers back on the screen, bitblk to the rescue!
    RS_PainterQt wPainter(this);
    wPainter.drawPixmap(0,0,*PixmapLayer1);
    wPainter.drawPixmap(0,0,*PixmapLayer2);
    wPainter.drawPixmap(0,0,*PixmapLayer3);
    wPainter.end();

    redrawMethod=RS2::RedrawNone;
}

void QG_GraphicView::setAntialiasing(bool state)
{
	antialiasing = state;
}

void QG_GraphicView::addScrollbars()
{
    scrollbars = true;

    hScrollBar = new QG_ScrollBar(Qt::Horizontal, this);
    vScrollBar = new QG_ScrollBar(Qt::Vertical, this);
    layout = new QGridLayout(this);

    setOffset(50, 50);

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    layout->setContentsMargins(QMargins{});
#else
    layout->setMargin(0);
#endif
    layout->setSpacing(0);
    layout->setColumnStretch(0, 1);
    layout->setColumnStretch(1, 0);
    layout->setColumnStretch(2, 0);
    layout->setRowStretch(0, 1);
    layout->setRowStretch(1, 0);

    hScrollBar->setSingleStep(50);
    hScrollBar->setCursor(Qt::ArrowCursor);
    layout->addWidget(hScrollBar, 1, 0);
    connect(hScrollBar, SIGNAL(valueChanged(int)),
            this, SLOT(slotHScrolled(int)));

    vScrollBar->setSingleStep(50);
    vScrollBar->setCursor(Qt::ArrowCursor);
    layout->addWidget(vScrollBar, 0, 1);
    connect(vScrollBar, SIGNAL(valueChanged(int)),
            this, SLOT(slotVScrolled(int)));
}

bool QG_GraphicView::hasScrollbars()
{
    return scrollbars;
}

void QG_GraphicView::setCursorHiding(bool state)
{
    cursor_hiding = state;
}

void QG_GraphicView::setCurrentQAction(QAction* q_action)
{
    eventHandler->setQAction(q_action);

    if (recent_actions.contains(q_action))
    {
        recent_actions.removeOne(q_action);
    }
    recent_actions.prepend(q_action);
}

void QG_GraphicView::destroyMenu(const QString& activator)
{
    if (menus.contains(activator))
    {
        auto menu = menus.take(activator);
        delete menu;
    }
}

void QG_GraphicView::setMenu(const QString& activator, QMenu* menu)
{
    destroyMenu(activator);
    menus[activator] = menu;
}

void QG_GraphicView::startAutoPanTimer(QMouseEvent *event)
{
    if (event == nullptr)
        return;
    const RS_Vector cadArea_minCoord(0., 0.);
    const RS_Vector cadArea_maxCoord(getWidth(), getHeight());
    const LC_Rect cadArea_actual(cadArea_minCoord, cadArea_maxCoord);
    const LC_Rect cadArea_unprobed(cadArea_minCoord + m_panData->probedAreaOffset,
                                   cadArea_maxCoord - m_panData->probedAreaOffset);

    RS_Vector mouseCoord{event->position()};
    mouseCoord.y = cadArea_actual.height() - mouseCoord.y;

    const RS_Vector cadArea_centerPoint((cadArea_minCoord + cadArea_maxCoord) / 2.0);
    RS_Vector offset = mouseCoord - cadArea_centerPoint;
    offset = {std::abs(offset.x) - cadArea_unprobed.width() / 2.,
              std::abs(offset.y) - cadArea_unprobed.height() / 2.};
    offset = {std::max(offset.x, 1.), std::max(offset.y, 1.)};

    double panOffset_angle{cadArea_centerPoint.angleTo(mouseCoord)};

    /* It would be better if the below value was calculated in the code that deals with resizing the CAD area. */
    const double quarterAngle = cadArea_centerPoint.angleTo(cadArea_actual.upperRightCorner());

    double percentageFactor = 1.;

    if (((panOffset_angle > quarterAngle) && (panOffset_angle <= (M_PI - quarterAngle)))
        || ((panOffset_angle > (quarterAngle + M_PI))
            && (panOffset_angle <= (M_PI + M_PI - quarterAngle)))) {
        percentageFactor = (std::abs((mouseCoord - cadArea_centerPoint).y)
                            - (cadArea_unprobed.height() / 2.0))
                           / ((cadArea_actual.height() / 2.0) - (cadArea_unprobed.height() / 2.0));
    } else {
        percentageFactor = (std::abs((mouseCoord - cadArea_centerPoint).x)
                            - (cadArea_unprobed.width() / 2.0))
                           / ((cadArea_actual.width() / 2.0) - (cadArea_unprobed.width() / 2.0));
    }

    const double panTimerInterval{
        m_panData->panTimerInterval_minimum
        + ((m_panData->panTimerInterval_maximum - m_panData->panTimerInterval_minimum)
           * (1.0 - percentageFactor))};

    offset = RS_Vector::polar(offset.magnitude(), M_PI - panOffset_angle);
    m_panData->panOffset = {static_cast<int>(offset.x), static_cast<int>(offset.y)};

    if (m_panData->panTimer != nullptr) {
        m_panData->panTimer->setInterval(panTimerInterval);
    } else {
        m_panData->start(panTimerInterval, *this);
    }

    if (RS_DEBUG->getLevel() >= RS_Debug::D_INFORMATIONAL) {
        std::cout << " CAD area centre point                = " << cadArea_centerPoint << std::endl
                  << " Actual CAD area quarter angle (deg)  = " << quarterAngle * 180.0 / M_PI
                  << std::endl
                  << " Percentage factor                    = " << percentageFactor << std::endl
                  << " Pan offset angle (radians)           = " << panOffset_angle << std::endl
                  << " Pan offset angle (degrees)           = " << panOffset_angle * 180.0 / M_PI
                  << std::endl
                  << " Pan offset vector                    = " << m_panData->panOffset.x() << ", "
                  << m_panData->panOffset.y()
                  << std::endl
                  //<< " Pan timer interval (ms)              = " << m_panData->panTimer->interfac
                  << std::endl
                  << " Mouse (cursor) position (adjusted)   = " << mouseCoord << std::endl
                  << " Mouse position w.r.t. centre point   = " << mouseCoord - cadArea_centerPoint
                  << std::endl
                  << std::endl
                  << std::endl;
    }
}


bool QG_GraphicView::isAutoPan(QMouseEvent *event) const
{
    if (event == nullptr)
        return false;
    RS_SETTINGS->beginGroupGuard("/Appearance");
    const bool autopanEnabled = (bool) RS_SETTINGS->readNumEntry("/Autopanning", 0);

    if (!autopanEnabled)
        return false;

    const RS_Vector cadArea_minCoord(0., 0.);

    const RS_Vector cadArea_maxCoord(getWidth(), getHeight());

    const LC_Rect cadArea_actual(cadArea_minCoord, cadArea_maxCoord);

    const LC_Rect cadArea_unprobed(cadArea_minCoord + m_panData->probedAreaOffset,
                                   cadArea_maxCoord - m_panData->probedAreaOffset);
    if (cadArea_unprobed.width() < 0. || cadArea_unprobed.height() < 0.)
        return false;

    RS_Vector mouseCoord{event->position()};

    if (RS_DEBUG->getLevel() >= RS_Debug::D_INFORMATIONAL) {
        std::cout << " Unprobed CAD area width and height = " << cadArea_unprobed.width() << "/"
                  << cadArea_unprobed.height() << std::endl
                  << " Actual   CAD area width and height = " << cadArea_actual.width() << "/"
                  << cadArea_actual.height() << std::endl
                  << " Mouse (cursor) position            = " << mouseCoord << std::endl
                  << std::endl;
    }

    return cadArea_actual.inArea(mouseCoord) && !cadArea_unprobed.inArea(mouseCoord);
}


/*
    Auto-pans the CAD area.
    - by Melwyn Francis Carlo <carlo.melwyn@outlook.com>
*/
void QG_GraphicView::autoPanStep()
{
    // skip first steps to avoid unintensional panning
    m_panData->m_delayCounter = std::min(++ m_panData->m_delayCounter, m_panData->delayCounterMax);
    if (m_panData->m_delayCounter < m_panData->delayCounterMax)
        return;

    RS_DEBUG->print(RS_Debug::D_INFORMATIONAL, "%s(): Timer is ticking!", __func__);

    zoomPan(m_panData->panOffset.x(), m_panData->panOffset.y());
}
