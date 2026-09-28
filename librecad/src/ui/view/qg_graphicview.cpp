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

#include "qg_graphicview.h"

#include <QGridLayout>
#include <QMenu>
#include <QNativeGestureEvent>
#include <QPoint>
#include <QPointingDevice>
#include <QScopedValueRollback>
#include <QSignalBlocker>
#include <QTimer>
#include <cmath>
#include <cstdlib>
#include <iostream>

#include "lc_action_modify_move_adjust.h"
#include "lc_action_select_single.h"
#include "lc_actioncontext.h"
#include "lc_eventhandler.h"
#include "lc_graphicviewport.h"
#include "lc_graphicviewrenderer.h"
#include "lc_overlayentitiescontainer.h"
#include "lc_quickinfowidget.h"
#include "lc_rect.h"
#include "lc_relative_point_input_widget.h"
#include "lc_relative_position_editing_widget.h"
#include "lc_ucs_mark.h"
#include "lc_undosection.h"
#include "lc_viewmath.h"
#include "qc_applicationwindow.h"
#include "qg_blockwidget.h"
#include "qg_scrollbar.h"
#include "rs.h"
#include "rs_actiondefault.h"
#include "rs_blocklist.h"
#include "rs_debug.h"
#include "rs_dialogfactoryinterface.h"
#include "rs_entity.h"
#include "rs_entitycontainer.h"
#include "rs_graphic.h"
#include "rs_insert.h"
#include "rs_selection.h"
#include "rs_settings.h"
#include "rs_units.h"

#ifdef EMU_C99
#include "emu_c99.h"
#endif

namespace {
    // Issue #1765: set default cursor size: 32x32
    constexpr int g_cursorSize = 32; // fixme - sand - move to common public place
    // Issue #1787: cursor hot spot at center by using hotX=hotY=-1
    constexpr int HOTSPOT_XY = -1;

    // maximum length for displayed block name in context menu
    constexpr int g_MaxBlockNameLength = 40; // fixme - sand - move to common public place

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
    constexpr double zoomFactor = 1.137; // fixme - to settings
    // zooming factor is wheel angle delta divided by this factor
    constexpr double ZOOM_WHEEL_DIVISOR = 200.; // fixme - to settings

    // Helper function to test validity of a rect
    bool withinValidRange(const double x) {
        return x >= RS_MINDOUBLE && x <= RS_MAXDOUBLE;
    }

    bool withinValidRange(const RS_Vector& vp) {
        return vp.valid && withinValidRange(vp.x) && withinValidRange(vp.y);
    }

    // finite and ordered; a point-like drawing (min == max) is valid content
    bool isRectValid(const RS_Vector& vpMin, const RS_Vector& vpMax) {
        return withinValidRange(vpMin) && withinValidRange(vpMax) && vpMin.x <= vpMax.x && vpMin.y <= vpMax.y;
    }
}

/**
 * @brief snapEntity find the closest entity
 * @param view
 * @param event
 * @param view - the graphic view
 * @param event - the mouse event
 * @return RS_Entity* - the closest entity within the range of g_cursorSize
 *                      returns nullptr, if no entity is found in range
 */
RS_Entity* snapEntity(const QG_GraphicView& view, const QMouseEvent* event) {
    if (event == nullptr) {
        return nullptr;
    }
    const auto doc = view.getDocument();
    if (doc == nullptr) {
        // how it might be???
        return nullptr;
    }
    const QPointF mapped = event->pos();
    double distance = RS_MAXDOUBLE;
    const auto viewPort = view.getViewPort();

    const auto pos = viewPort->toWorldFromUi(mapped.x(), mapped.y());
    const auto entity = doc->getNearestEntity(pos, &distance, RS2::ResolveNone);

    return (viewPort->toGuiDX(distance) <= g_cursorSize) ? entity : nullptr;
}

// fixme - sand - remove, not needed?
// Find an ancestor of the RS_Insert type.
// Return nullptr, if none is found
RS_Insert* getAncestorInsert(RS_Entity* entity) {
    while (entity != nullptr) {
        if (entity->rtti() == RS2::EntityInsert) {
            RS_Insert* parent = getAncestorInsert(entity->getParent());
            return parent != nullptr ? parent : static_cast<RS_Insert*>(entity);
        }
        entity = entity->getParent();
    }
    return nullptr;
}

// fixme - sand - remove, not needed?
// whether the current insert is part of Text
RS_Entity* getParentText(const RS_Insert* insert) {
    if (insert == nullptr || insert->getBlock() != nullptr || insert->getParent() == nullptr) {
        return nullptr;
    }
    switch (insert->getParent()->rtti()) {
        case RS2::EntityText:
        case RS2::EntityMText:
            return insert->getParent();
        default:
            return nullptr;
    }
}

// Show the entity property dialog on the closest entity in range
void QG_GraphicView::showEntityPropertiesDialog(RS_Entity* entity) const {
    if (entity == nullptr) {
        return;
    }

    // snap to the top selected parent
    while (entity != nullptr && entity->getParent() != nullptr && entity->getParent()->isSelected()) {
        entity = entity->getParent();
    }

    launchEditProperty(entity);
}

void QG_GraphicView::launchEditProperty(RS_Entity* entity) const {
    const auto* doc = getDocument();
    if (entity == nullptr || doc == nullptr) {
        return;
    }
    editAction(*entity);

    // delete any temporary highlighting duplicates of the original
    auto* defaultAction = dynamic_cast<RS_ActionDefault*>(getEventHandler()->getDefaultAction());
    if (defaultAction != nullptr) {
        defaultAction->clearHighLighting();
    }
}

// Start the edit action:
// Edit Block for an insert
// Edit entity, otherwise
void QG_GraphicView::editAction(RS_Entity& entity) const {
    const auto doc = getDocument();
    if (doc == nullptr) {
        // fixme - DOC - review what for this check is used
        return;
    }
    switch (entity.rtti()) {
        case RS2::EntityInsert: {
            // fixme - sand - rework block editing. Currently insert may not be changed after insertion! Window is needed!!
            const auto& appWindow = QC_ApplicationWindow::getAppWindow(); // fixme - sand - remove static, it just one of parents?
            RS_BlockList* blockList = appWindow->getBlockWidget()->getBlockList();
            RS_Block* active = (blockList != nullptr) ? blockList->getActive() : nullptr;
            const auto* insert = static_cast<RS_Insert*>(&entity);
            RS_Block* current = insert->getBlockForInsert();
            if (current == active) {
                active = nullptr;
            }
            else if (blockList != nullptr) {
                blockList->activate(current);
            }
            /*// fixme - sand - simplify
            std::shared_ptr<RS_Block*> scoped{
                &active,
                [blockList](RS_Block** pointer) {
                    if (pointer != nullptr && *pointer != nullptr && blockList != nullptr) {
                        blockList->activate(*pointer);
                    }
                }
            };*/
            switchToAction(RS2::ActionBlocksEdit);
            if (active != nullptr && blockList != nullptr) {
                blockList->activate(active);
            }
            break;
        }
        default: {
            m_actionContext->saveContextMenuActionContext(&entity, RS_Vector(false), entity.isSelected());
            switchToAction(RS2::ActionModifyEntity);
            break;
        }
    }
}

// Support auto-panning when the cursor is close to the view border
struct QG_GraphicView::AutoPanData {
    void start(const double interval, QG_GraphicView& view) {
        delayCounter = 0;
        panTimer = std::make_unique<QTimer>(&view);
        panTimer->start(interval);
        connect(panTimer.get(), &QTimer::timeout, &view, &QG_GraphicView::autoPanStep);
    }

    std::unique_ptr<QTimer> panTimer;

    QPoint panOffset;

    unsigned delayCounter = 0U;
    // skip the first events, to avoid unintensional panning
    const unsigned delayCounterMax = 10U;
    const double panOffsetMagnitude = 20.0;

    const double panTimerIntervalMinimum = 20.0;
    const double panTimerIntervalMaximum = 100.0;

    // the sensitive border of the view
    const RS_Vector probedAreaOffset = {50 /* pixels */, 50 /* pixels */};
};

struct QG_GraphicView::UCSHighlightData {
    std::unique_ptr<QTimer> timer;

    double timerInterval = 200.0;
    int blinkNumber = 0;
    int maxBlinkNumber = 15;
    bool inVisiblePhase = false;
    RS_Vector origin;
    double angle = 0.0;
    bool forWCS = false;

    RS_Vector savedViewOffset = RS_Vector(0, 0, 0);
    double savedViewFactor = 0.0;

    void start(const double interval, QG_GraphicView& view) {
        if (timer == nullptr) {
            timer = std::make_unique<QTimer>(&view);
            connect(timer.get(), &QTimer::timeout, &view, &QG_GraphicView::ucsHighlightStep);
        }
        timer->start(interval);
    }

    bool mayTick() {
        blinkNumber++;
        inVisiblePhase = !inVisiblePhase;
        return blinkNumber <= maxBlinkNumber;
    }

    void stop() {
        blinkNumber = 0;
        inVisiblePhase = false;
        timer->stop();
    }
};

void createViewRenderer();

void QG_GraphicView::initRelativePointInputWidget() {
    m_relativePointWidgetHolder = new LC_RelativePointInputWidget(this, m_actionContext);
    m_relativePointWidgetHolder->setVisible(false);
}

/**
 * Constructor.
 */
// fixme - sand - files - init by action context???
QG_GraphicView::QG_GraphicView(QWidget* parent, RS_Document* doc, LC_ActionContext* actionContext)
    : RS_GraphicView(parent, {}), m_device("Mouse"),
      m_cursorCad(new QCursor(QPixmap(":cursors/cur_cad_bmp.png"), HOTSPOT_XY, HOTSPOT_XY)),
      m_cursorDel(new QCursor(QPixmap(":cursors/cur_del_bmp.png"), HOTSPOT_XY, HOTSPOT_XY)),
      m_cursorSelect(new QCursor(QPixmap(":cursors/cur_select_bmp.png"), HOTSPOT_XY, HOTSPOT_XY)),
      m_cursorMagnifier(new QCursor(QPixmap(":cursors/cur_glass_bmp.png"), HOTSPOT_XY, HOTSPOT_XY)),
      m_cursorHand(new QCursor(QPixmap(":cursors/cur_hand_bmp.png"), HOTSPOT_XY, HOTSPOT_XY)), m_isSmoothScrolling(false),
      m_ucsMarkOptions{std::make_unique<LC_UCSMarkOptions>()}, m_panData{std::make_unique<AutoPanData>()},
      m_ucsHighlightData{std::make_unique<UCSHighlightData>()} {
    RS_DEBUG->print("QG_GraphicView::QG_GraphicView()..");

    if (doc != nullptr) {
        setDocument(doc);
        doc->setGraphicView(this);
        actionContext->setDocumentAndView(doc, this);
        setDefaultAction(new RS_ActionDefault(actionContext));
    }

    m_actionContext = actionContext;

    getViewPort()->justSetOffsetAndFactor(0, 0, 4.0);
    getViewPort()->setBorders(10, 10, 10, 10);

    setMouseTracking(true);
    setFocusPolicy(Qt::NoFocus);

    // SourceForge issue 45 (Left-mouse drag shrinks window)
    setAttribute(Qt::WA_NoMousePropagation);

    // Issue #2264: prevents macOS from applying text-related features like the Caps Lock indicator to the non-text canvas
#ifdef Q_OS_MAC
    setAttribute(Qt::WA_InputMethodEnabled, false); setInputMethodHints(Qt::ImhNone);
#endif
}

void QG_GraphicView::initView() {
    createViewRenderer();
    initRelativePointInputWidget();
}

void QG_GraphicView::createViewRenderer() {
    if (getViewPort() != nullptr) {
        getViewPort()->setSize(width(), height()); // fixme - sand - merge - CHECK THIS
        setRenderer(std::make_unique<LC_GraphicViewRenderer>(getViewPort(), this));
    }
}

void QG_GraphicView::layerToggled(RS_Layer*) {
    redraw(RS2::RedrawDrawing);
}

/**
 * Destructor
 */
QG_GraphicView::~QG_GraphicView() {
    try {
        // LC_ERR << "QG_GraphicView destructor";
        cleanUp();
        // LC_ERR << "QG_GraphicView destructor 1";
    }
    catch (...) {
        LC_ERR << __func__ << "(): received exception";
    }
}

/**
 * @return width of widget.
 */
int QG_GraphicView::getWidth() const {
    if (m_scrollbars) {
        return width() - m_vScrollBar->sizeHint().width();
    }
    return width();
}

/**
 * @return height of widget.
 */
int QG_GraphicView::getHeight() const {
    if (m_scrollbars) {
        return height() - m_hScrollBar->sizeHint().height();
    }
    return height();
}

/**
 * Sets the mouse cursor to the given type.
 */
void QG_GraphicView::setMouseCursor(const RS2::CursorType cursorType) {
    switch (cursorType) {
        default: case RS2::ArrowCursor:
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
            m_cursorHiding ? setCursor(Qt::BlankCursor) : setCursor(*m_cursorCad);
            break;
        case RS2::DelCursor:
            setCursor(*m_cursorDel);
            break;
        case RS2::SelectCursor:
            m_selectCursorHiding ? setCursor(Qt::BlankCursor) : setCursor(*m_cursorSelect);
            break;
        case RS2::MagnifierCursor:
            setCursor(*m_cursorMagnifier);
            break;
        case RS2::MovingHandCursor:
            setCursor(*m_cursorHand);
            break;
    }
}

/**
 * Sets the text for the grid status widget in the left bottom corner.
 */
void QG_GraphicView::updateGridStatusWidget(QString text) {
    emit gridStatusChanged(std::move(text));
}

void QG_GraphicView::dragEnterEvent(QDragEnterEvent* event) {
    RS_GraphicView::dragEnterEvent(event);

    /*
     *   fixme - sand - remove later, experiments with d&d
     */
    /*  if (event->mimeData()->formats().contains("application/x-qabstractitemmodeldatalist")) {
          // QStandardItemModel model;
          // model.dropMimeData(event->mimeData(), Qt::CopyAction, 0,0, QModelIndex());
          // auto item = model.item(0.0);
          // LC_ERR <<  item->text();

           QDrag::cancel();
           QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
           QC_ApplicationWindow::getAppWindow()->getLibraryWidget()->insert();
      }*/
}

/**
 * Redraws the widget.
 */
void QG_GraphicView::redraw(const RS2::RedrawMethod method, bool immediately) {
    getRenderer()->invalidate(method);
    update(); // Paint when ready to paint
    if (immediately) {
        repaint();
    }
}

void QG_GraphicView::resizeEvent(QResizeEvent* e) {
    RS_GraphicView::resizeEvent(e);
    RS_DEBUG->print("QG_GraphicView::resizeEvent begin");
    adjustOffsetControls();
    adjustZoomControls();
    //     updateGrid();
    // Small hack, delete the snapper during resizes
    getViewPort()->clearOverlayDrawablesContainer(RS2::Snapper);
    redraw();
    RS_DEBUG->print("QG_GraphicView::resizeEvent end");
}

void QG_GraphicView::switchToAction(const RS2::ActionType actionType, void* data) const {
    m_actionContext->setCurrentAction(actionType, data);
}

RS_Entity* QG_GraphicView::catchContextEntity(const QMouseEvent* event, RS_Vector& clickPos) const {
    const auto doc = getDocument();
    if (doc == nullptr || event == nullptr) {
        return nullptr;
    }

    const QPointF mapped = event->pos();
    double distance = RS_MAXDOUBLE;
    const LC_GraphicViewport* viewPort = getViewPort();

    clickPos = viewPort->toWorldFromUi(mapped.x(), mapped.y());
    RS_Entity* entity = doc->getNearestEntity(clickPos, &distance, RS2::ResolveNone);

    if (viewPort->toGuiDX(distance) <= g_cursorSize) {
        return entity;
    }
    return nullptr;
}

bool QG_GraphicView::invokeContextMenuForMouseEvent(QMouseEvent* e) {
    bool result = false;
    RS_Vector clickPos;
    RS_Entity* entity = catchContextEntity(e, clickPos);
    const auto contextMenu = QC_ApplicationWindow::getAppWindow()->createGraphicViewContentMenu(e, this, entity, clickPos);
    if (contextMenu != nullptr) {
        if (!contextMenu->isEmpty()) {
            auto actions = contextMenu->actions();
            if (actions.size() == 1) {
                const auto action = actions.front();
                action->trigger();
                result = true;
            }
            else {
                contextMenu->exec(mapToGlobal(e->pos()));
                result = true;
            }
        }
        delete contextMenu;
    }
    return result;
}

void QG_GraphicView::mousePressEvent(QMouseEvent* event) {
    // LC_ERR << "MOUSE PRESS";
    // pan zoom with middle mouse button
    if (event->button() == Qt::MiddleButton && event->modifiers() == Qt::NoModifier) {
        switchToAction(RS2::ActionZoomPan);
        getCurrentAction()->mousePressEvent(event);
    }
    else {
        getEventHandler()->mousePressEvent(event);
    }
}

void QG_GraphicView::mouseDoubleClickEvent(QMouseEvent* e) {
    // LC_ERR << "MOUSE DOUBLE CLICK";
    if (getEventHandler()->hasAction()) {
    }
    else {
        const auto defaultAction = getEventHandler()->getDefaultAction();
        RS_Vector clickPos;
        RS_Entity* entity = catchContextEntity(e, clickPos);
        if (entity == nullptr) {
            if (defaultAction == nullptr) {
                invokeContextMenuForMouseEvent(e);
            }
            else if (defaultAction->getStatus() == RS_ActionInterface::InitialActionStatus) {
                invokeContextMenuForMouseEvent(e);
            }
        }
        else {
            if (e->button() == Qt::LeftButton && e->modifiers() == Qt::NoModifier) {
                if (defaultAction == nullptr) {
                    showEntityPropertiesDialog(entity);
                }
                else if (defaultAction->getStatus() == RS_ActionInterface::InitialActionStatus) {
                    showEntityPropertiesDialog(entity);
                }
            }
            else {
                invokeContextMenuForMouseEvent(e);
            }
        }
    }
    /*else {
        switch(e->button()){
            case Qt::MiddleButton:
                switchToAction(RS2::ActionZoomAuto);
                break;
            case Qt::LeftButton:
                // double click on an entity to edit entity properties

                showEntityPropertiesDialog(entity);
                break;
            default:
                break;
        }
    }*/
    e->accept();
}

void QG_GraphicView::mouseReleaseEvent(QMouseEvent* event) {
    RS_DEBUG->print("QG_GraphicView::mouseReleaseEvent");

    event->accept();
    if (getEventHandler()->hasAction()) {
        switch (event->button()) {
            case Qt::RightButton: {
                if (getEventHandler()->hasAction()) {
                    back(event->modifiers());
                }
                break;
            }
            case Qt::XButton1:
                processEnterKey();
                emit xbutton1_released();
                break;
            default:
                getEventHandler()->mouseReleaseEvent(event);
                break;
        }
    }
    else {
        const auto defaultAction = getEventHandler()->getDefaultAction();
        if (defaultAction != nullptr) {
            const int defaultActionStatus = defaultAction->getStatus();
            if (defaultActionStatus == RS_ActionInterface::InitialActionStatus) {
                if (isMouseReleaseEventForDefaultAction(event)) {
                    defaultAction->mouseReleaseEvent(event);
                }
                else {
                    invokeContextMenuForMouseEvent(event);
                }
            }
            else {
                defaultAction->mouseReleaseEvent(event);
            }
        }
        else {
            invokeContextMenuForMouseEvent(event);
        }
    }
    RS_DEBUG->print("QG_GraphicView::mouseReleaseEvent: OK");
}

bool QG_GraphicView::isMouseReleaseEventForDefaultAction(const QMouseEvent* event) {
    // should correspond to LC_DlgMenuAssigner::validateShortcut()
    if (event->button() == Qt::LeftButton) {
        const auto modifiers = event->modifiers();
        if (modifiers == Qt::NoModifier) {
            // select
            return true;
        }
        const bool control = modifiers & Qt::ControlModifier;
        const bool alt = modifiers & Qt::AltModifier;
        const bool shift = modifiers & Qt::ShiftModifier;
        if (control && !alt && !shift) {
            // pan
            return true;
        }
        if (shift && !alt && !control) {
            // select contour
            return true;
        }
    }
    return false;
}

void QG_GraphicView::mouseMoveEvent(QMouseEvent* event) {
    if (isClosing()) {
        event->accept();
        return;
    }
    // LC_ERR << "OWN MOUSE MOVE";
    if (isAutoPan(event)) {
        startAutoPanTimer(event);
        event->accept();
        return;
    }
    m_panData->panTimer.reset();
    // handle auto-panning
    event->accept();
    getEventHandler()->mouseMoveEvent(event);
}

bool QG_GraphicView::proceedEvent(QEvent* event) {
    // skip events without a default action
    // Hatch preview in qg_dlghatch doesn't have its default action
    if (dynamic_cast<QInputEvent*>(event) == nullptr || getDefaultAction() != nullptr) {
        return QWidget::event(event);
    } // LC_ERR<< "Event Skipped";
    return true;
}

bool QG_GraphicView::event(QEvent* event) {
    if (event->type() == QEvent::NativeGesture) {
        const auto* nge = static_cast<QNativeGestureEvent*>(event);

        if (nge->gestureType() == Qt::ZoomNativeGesture) {
            const double v = nge->value();
            const RS2::ZoomDirection direction = std::signbit(v) ? RS2::Out : RS2::In;
            const double factor = 1. + std::abs(v);

            // It seems the NativeGestureEvent::pos() incorrectly reports global coordinates
            const QPointF g = mapFromGlobal(nge->globalPosition().toPoint());
            const RS_Vector mouse = getViewPort()->toWorldFromUi(g.x(), g.y());
            doZoom(direction, mouse, factor);
        }
        return true;
    }
    if (event->type() == QEvent::LayoutDirectionChange) {
        keepScrollbarsOnDrawingSide();
    }
    return proceedEvent(event);
}

/**
 * The drawing is painted from x = 0 to getWidth(), so the vertical scrollbar stays on the
 * right in every locale: QGridLayout mirrors columns in a right-to-left UI unless its origin
 * corner is mirrored too.
 */
void QG_GraphicView::keepScrollbarsOnDrawingSide() const {
    if (m_layout != nullptr) {
        m_layout->setOriginCorner(isRightToLeft() ? Qt::TopRightCorner : Qt::TopLeftCorner);
    }
}

void QG_GraphicView::doZoom(const RS2::ZoomDirection direction, const RS_Vector& center, const double zoomFactor) const {
    if (direction == RS2::In) {
        getViewPort()->zoomIn(zoomFactor, center);
    }
    else {
        getViewPort()->zoomOut(zoomFactor, center);
    }
}

/**
 * support for the wacom graphic tablet.
 */
void QG_GraphicView::tabletEvent(QTabletEvent* e) {
    if (testAttribute(Qt::WA_UnderMouse)) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        switch (e->pointerType()) {
            case QPointingDevice::PointerType::Eraser:
                if (e->type() == QEvent::TabletRelease) {
                    if (getDocument() != nullptr) {
                        auto a = std::make_shared<LC_ActionSelectSingle>(m_actionContext);
                        setCurrentAction(a);
                        QMouseEvent ev(QEvent::MouseButtonRelease, e->position(), e->globalPosition(), Qt::LeftButton, Qt::LeftButton,
                                       Qt::NoModifier); //RLZ
                        mouseReleaseEvent(&ev);
                        a->finish();

                        if (getDocument()->hasSelection()) {
                            switchToAction(RS2::ActionModifyDelete);
                        }
                    }
                }
                break;

            case QPointingDevice::PointerType::Generic:
            case QPointingDevice::PointerType::Pen:
            case QPointingDevice::PointerType::Cursor:
                if (e->type() == QEvent::TabletPress) {
                    QMouseEvent ev(QEvent::MouseButtonPress, e->position(), e->globalPosition(), Qt::LeftButton, Qt::LeftButton,
                                   Qt::NoModifier); //RLZ
                    mousePressEvent(&ev);
                }
                else if (e->type() == QEvent::TabletRelease) {
                    QMouseEvent ev(QEvent::MouseButtonRelease, e->position(), e->globalPosition(), Qt::LeftButton, Qt::LeftButton,
                                   Qt::NoModifier); //RLZ
                    mouseReleaseEvent(&ev);
                }
                else if (e->type() == QEvent::TabletMove) {
                    QMouseEvent ev(QEvent::MouseMove, e->position(), e->globalPosition(), Qt::NoButton, {}, Qt::NoModifier); //RLZ
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
        if (e->type() == QEvent::TabletRelease) {
            if (getDocument() != nullptr) {
                RS_ActionSelectSingle* a = new RS_ActionSelectSingle(*getDocument(), *this);
                setCurrentAction(a);
                QMouseEvent ev(QEvent::MouseButtonRelease, e->position(), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier); //RLZ
                mouseReleaseEvent(&ev);
                a->finish();

                if (getDocument()->hasSelection()) {
                    setCurrentAction(new RS_ActionModifyDelete(*getDocument(), *this));
                }
            }
        } break; case QTabletEvent::Stylus: case QTabletEvent::Puck:
        if (e->type() == QEvent::TabletPress) {
            QMouseEvent ev(QEvent::MouseButtonPress, e->position(), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier); //RLZ
            mousePressEvent(&ev);
        }
        else if (e->type() == QEvent::TabletRelease) {
            QMouseEvent ev(QEvent::MouseButtonRelease, e->position(), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier); //RLZ
            mouseReleaseEvent(&ev);
        }
        else if (e->type() == QEvent::TabletMove) {
            QMouseEvent ev(QEvent::MouseMove, e->position(), Qt::NoButton, {}, Qt::NoModifier); //RLZ
            mouseMoveEvent(&ev);
        } break; default:
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
    getEventHandler()->mouseLeaveEvent();
    QWidget::leaveEvent(e);
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
void QG_GraphicView::enterEvent(QEnterEvent* e) {
#else
void QG_GraphicView::enterEvent(QEvent* e) {
#endif
    getEventHandler()->mouseEnterEvent();
    QWidget::enterEvent(e);
}

void QG_GraphicView::focusOutEvent(QFocusEvent* e) {
    QWidget::focusOutEvent(e);
}

void QG_GraphicView::focusInEvent(QFocusEvent* e) {
    getEventHandler()->mouseEnterEvent();
    QWidget::focusInEvent(e);
}

/**
 * mouse wheel event. zooms in/out or scrolls when
 * shift or ctrl is pressed.
 */
void QG_GraphicView::wheelEvent(QWheelEvent* e) {
    // LC_ERR << "OWN WHEEL";
    //RS_DEBUG->print("wheel: %d", e->delta());

    //printf("state: %d\n", e->state());
    //printf("ctrl: %d\n", Qt::ControlButton);

    if (getDocument() == nullptr) {
        return;
    }

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    //    RS_Vector mouse = toGraph(e->position());
    const QPointF& uiEventPosition = e->position();
    const RS_Vector mouse = getViewPort()->toUCSFromGui(uiEventPosition.x(), uiEventPosition.y());
#else
    RS_Vector mouse = toGraph(e->position());
#endif

    if (m_device == "Trackpad") {
        QPoint numPixels = e->pixelDelta();

        // high-resolution scrolling triggers Pan instead of Zoom logic
        m_isSmoothScrolling |= !numPixels.isNull();

        if (m_isSmoothScrolling) {
            if (e->phase() == Qt::ScrollEnd) {
                m_isSmoothScrolling = false;
            }
        }
        else // Trackpads that without high-resolution scrolling
        // e.g. libinput-XWayland trackpads
        {
            numPixels = e->angleDelta() / 4;
        }

        if (!numPixels.isNull()) {
            if (e->modifiers() == Qt::ControlModifier) {
                // Hold ctrl to zoom. 1 % per pixel
                const double v = (m_invertZoomDirection) ? (numPixels.y() / ZOOM_WHEEL_DIVISOR) : (-numPixels.y() / ZOOM_WHEEL_DIVISOR);
                RS2::ZoomDirection direction;
                if (v < 0) {
                    direction = RS2::Out;
                }
                else {
                    direction = RS2::In;
                }

                const double zoomFact = 1. + std::abs(v);
                doZoom(direction, mouse, zoomFact);
            }
            else {
                const int hDelta = (m_invertHorizontalScroll) ? -numPixels.x() : numPixels.x();
                const int vDelta = (m_invertVerticalScroll) ? -numPixels.y() : numPixels.y();

                // pan the viewport, not the scrollbars: a bar is bounded, panning is not (#2945)
                getViewPort()->zoomPan(hDelta, vDelta);
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
    const int angleDeltaY = e->angleDelta().y(); // delta for VERTICAL mouse wheel
    int angleDeltaX = e->angleDelta().x(); // delta for HORIZONTAL mouse wheel

    // for zoom, let's use just vertical scrolling, so below we'll rely on AngleDeltaY only.
    // otherwise, horizontal scroll will not work :(  Basically, that's a side-effect for porting to QT6
    // so here let's use simpler logic
    angleDeltaX = angleDeltaY;
    if (e->modifiers() == Qt::ControlModifier) {
        scroll = true;
        direction = (angleDeltaY > 0) ? RS2::Up : RS2::Down;
    }
    else if (e->modifiers() == Qt::ShiftModifier) {
        scroll = true;
        direction = (angleDeltaX > 0) ? RS2::Left : RS2::Right;
    }

    // fixme - potentially, we can support mouses with two mouse wheels later if this will be reasonable.
    // fixme - however, it looks as a kind of overkill - using on single vertical mouse wheel for scroll seems to be fine//
    /*
        if (e->modifiers() == Qt::ControlModifier) {
            scroll = true;
            if (angleDeltaY == 0){
            //case Qt::Horizontal:
                direction= (angleDeltaX > 0) ? RS2::Left : RS2::Right;
            } else {
            //case Qt::Vertical:
                direction= (angleDeltaY > 0) ? RS2::Up : RS2::Down;
            }
        }
        // scroll left / right:
        else if	(e->modifiers()==Qt::ShiftModifier) {
            scroll = true;
            if (angleDeltaY == 0){
            //case Qt::Horizontal:
                direction= (angleDeltaX > 0) ? RS2::Up : RS2::Down;
            } else {
            //case Qt::Vertical:
                direction= (angleDeltaX > 0) ? RS2::Left : RS2::Right;
            }
        }*/

    if (scroll && m_scrollbars) {
        // pan the viewport with the direction the scrollbars used to give (issue #479);
        // a bar is bounded, panning is not (#2945)
        int delta = 0;

        switch (direction) {
            case RS2::Left:
            case RS2::Right:
                delta = (m_invertHorizontalScroll) ? -angleDeltaX : angleDeltaX;
                getViewPort()->zoomPan(-delta, 0);
                break;
            default:
                delta = (m_invertVerticalScroll) ? -angleDeltaY : angleDeltaY;
                getViewPort()->zoomPan(0, -delta);
                break;
        }
    }
    // zoom in / out:
    else if (e->modifiers() == 0) {
        //        LC_ERR << " AngleDelta Y "  << angleDeltaY;

        const RS2::ZoomDirection zoomDirection = ((angleDeltaY > 0) != m_invertZoomDirection) ? RS2::In : RS2::Out;

        const QPoint viewCenter{getWidth() / 2, getHeight() / 2};
        const QPoint delta = viewCenter - uiEventPosition.toPoint();

        if (getPanOnZoom()) {
            QCursor::setPos(mapToGlobal(viewCenter));
            getViewPort()->zoomPan(delta.x(), delta.y());
        }
        if (!getPanOnZoom() || !getSkipFirstZoom() || (abs(delta.x()) < 32 && abs(delta.y()) < 32)) {
            const RS_Vector& zoomCenter = mouse;
            //            LC_ERR << " Mouse "  << mouse << " Direction: " << (zoomDirection == RS2::In ? "In" : "Out");

            /*// todo - well, actually this is one-shot action... and it will lead to full action processing chain in action handler
            // todo - are we REALLY need it there? alternatively, zoom may be part of this class)
            auto zoomAction = std::make_unique<RS_ActionZoomIn>(m_actionContext, zoomDirection, RS2::Both, &zoomCenter,m_scrollZoomFactor);
            zoomAction->trigger();*/
            const double zoomFactor = std::pow(m_scrollZoomFactor, std::abs(angleDeltaY) / 120.0);
            doZoom(zoomDirection, zoomCenter, zoomFactor);
        }
    }
    redraw();

    /*    QMouseEvent event
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

        e->accept();*/
}

// fixme - sand -  move by keyboard support!!!
void QG_GraphicView::keyPressEvent(QKeyEvent* e) {
    if (getDocument() == nullptr) {
        return;
    }
    if (m_relativePointWidgetHolder->isVisible()) {
        return;
    }
    bool eventProcessed = false; // due to some weird reasons, even incoming event is already accepted (Win10)... so using own flag
    if (m_allowScrollAndMoveAdjustByKeys) {
        RS2::Direction direction = RS2::Up;
        bool scroll = e->modifiers() == Qt::NoModifier;
        const bool shift = e->modifiers() & Qt::ShiftModifier;
        const bool control = e->modifiers() & Qt::ControlModifier;

        bool move = shift || control;

        switch (e->key()) {
            case Qt::Key_Left:
                direction = RS2::Right;
                break;
            case Qt::Key_Right:
                direction = RS2::Left;
                break;
            case Qt::Key_Up:
                direction = RS2::Up;
                break;
            case Qt::Key_Down:
                direction = RS2::Down;
                break;
            default:
                scroll = false;
                move = false;
                break;
        }

        if (scroll) {
            getViewPort()->zoomScroll(direction);
            eventProcessed = true;
            e->accept();
        }
        else if (move) {
            LC_ActionModifyMoveAdjust::MovementInfo::Step step = LC_ActionModifyMoveAdjust::MovementInfo::GRID;
            if (control) {
                if (shift) {
                    step = LC_ActionModifyMoveAdjust::MovementInfo::META_GRID;
                }
                else {
                    step = LC_ActionModifyMoveAdjust::MovementInfo::SUB_GRID;
                }
            }
            else if (shift) {
                step = LC_ActionModifyMoveAdjust::MovementInfo::GRID;
            }

            LC_ActionModifyMoveAdjust::MovementInfo info(direction, step);
            switchToAction(RS2::ActionModifyMoveAdjust, &info);
            eventProcessed = true;
            e->accept();
        }
    }
    if (!eventProcessed) {
        getEventHandler()->keyPressEvent(e);
    }
}

void QG_GraphicView::keyReleaseEvent(QKeyEvent* e) {
    getEventHandler()->keyReleaseEvent(e);
}

/**
 * Drawing extents used as the scrollable content, in WCS.
 * @return false when the drawing has no extents (empty drawing)
 */
bool QG_GraphicView::scrollContentExtents(RS_Vector& wcsMin, RS_Vector& wcsMax) const {
    auto* viewport = getViewPort();
    if (viewport == nullptr || !viewport->getViewBorders(wcsMin, wcsMax)) {
        return false;
    }
    return isRectValid(wcsMin, wcsMax);
}

namespace {
    void applyScrollState(QG_ScrollBar* bar, const LC_ScrollModel::State& state) {
        // the bars only display the viewport: changing them must not move it
        const QSignalBlocker blocker(bar);
        bar->setRange(0, state.maximum);
        bar->setPageStep(state.pageStep);
        bar->setSingleStep(state.singleStep);
        bar->setValue(state.value);
        // the drawing extents in the same ticks, so the band and the thumb share one mapping
        bar->setContentBand(state.hasContent, state.tickFor(state.contentMin), state.tickFor(state.contentMax));
    }

    // The viewport fingerprint a scroll snapshot was built from (see LC_ScrollViewportKey).
    // The UCS is read by value (hasUCS()/getUcsOrigin()/getXAxisAngle()), never via
    // getCurrentUCS(), which heap-allocates a fresh LC_UCS on every call and would leak
    // here -- this function runs on essentially every viewport change.
    LC_ScrollViewportKey viewportKey(const LC_GraphicViewport& viewport) {
        const RS_Vector factor = viewport.getFactor();
        return {viewport.getOffsetX(), viewport.getOffsetY(), factor.x, factor.y,
                viewport.getWidth(), viewport.getHeight(),
                viewport.hasUCS(), viewport.getUcsOrigin(), viewport.getXAxisAngle()};
    }
}

/**
 * Builds one axis's LC_ScrollModel::Axis/State from the viewport quantities that feed
 * it: the shared math behind both adjustOffsetControls() (which then pushes it into the
 * QScrollBar widgets) and rebaseScrollIfStale() (which only updates the internal
 * snapshot -- see its comment). \p ox / \p oy / \p width / \p height are the viewport's
 * CURRENT offsets and size, so the axis this builds always starts from the view that is
 * actually on screen right now.
 *
 * The int -> double conversions below keep the subtraction/negation itself in double
 * (rather than negating/subtracting as int first and widening the result), so this has
 * no int-overflow risk regardless of how extreme ox/oy/height are.
 */
LC_ScrollModel::State QG_GraphicView::computeAxisState(const bool isHorizontal, const bool hasContent,
                                                       const RS_Vector& ucsMin, const RS_Vector& ucsMax,
                                                       const RS_Vector& factor, const int ox, const int oy,
                                                       const int width, const int height) const {
    if (isHorizontal) {
        return LC_ScrollModel::compute({hasContent, ucsMin.x * factor.x, ucsMax.x * factor.x,
                                        -static_cast<double>(ox), static_cast<double>(width)});
    }
    return LC_ScrollModel::compute({hasContent, -ucsMax.y * factor.y, -ucsMin.y * factor.y,
                                    static_cast<double>(oy) - static_cast<double>(height),
                                    static_cast<double>(height)});
}

/**
* Called whenever the graphic view has changed.
* Projects the viewport onto the scrollbars: the scrollable region is
* (drawing extents +/- half a view) united with the current view, so the bars
* never clamp or move the view. See LC_ScrollModel.
*/
void QG_GraphicView::adjustOffsetControls() {
    if (!m_scrollbars || m_hScrollBar == nullptr || m_vScrollBar == nullptr || getDocument() == nullptr) {
        return;
    }
    // a bar slot is moving the view, or we are already syncing: keep the frozen snapshot
    if (m_scrollSyncing) {
        return;
    }
    // the range stays frozen while a thumb is held; sliderReleased re-syncs
    if (m_hScrollBar->isSliderDown() || m_vScrollBar->isSliderDown()) {
        return;
    }
    auto* viewport = getViewPort();
    const int width = viewport->getWidth();
    const int height = viewport->getHeight();
    if (width <= 0 || height <= 0) {
        return;
    }
    const QScopedValueRollback<bool> guard(m_scrollSyncing, true);

    RS_Vector wcsMin;
    RS_Vector wcsMax;
    RS_Vector ucsMin;
    RS_Vector ucsMax;
    const bool hasContent = scrollContentExtents(wcsMin, wcsMax);
    if (hasContent) {
        viewport->ucsBoundsOfWcsBox(wcsMin, wcsMax, ucsMin, ucsMax);
    }
    const RS_Vector factor = viewport->getFactor();
    const int ox = viewport->getOffsetX();
    const int oy = viewport->getOffsetY();

    m_hScroll = computeAxisState(true, hasContent, ucsMin, ucsMax, factor, ox, oy, width, height);
    m_vScroll = computeAxisState(false, hasContent, ucsMin, ucsMax, factor, ox, oy, width, height);
    m_scrollKey = viewportKey(*viewport);
    m_scrollTipHasContent = hasContent;
    m_scrollTipUcsMin = ucsMin;
    m_scrollTipUcsMax = ucsMax;
    if (m_hScroll.valid) {
        applyScrollState(m_hScrollBar, m_hScroll);
    }
    if (m_vScroll.valid) {
        applyScrollState(m_vScrollBar, m_vScroll);
    }
    LC_LOG << __func__ << "(): H [0, " << m_hScroll.maximum << "] page " << m_hScroll.pageStep << " value "
        << m_hScroll.value << "; V [0, " << m_vScroll.maximum << "] page " << m_vScroll.pageStep << " value "
        << m_vScroll.value;
}

/**
 * Some paths change the viewport without going through adjustOffsetControls() (a silent
 * VPORT restore, applyUCSAfterLoad(), a keyboard zoom while a thumb is held). When the
 * viewport no longer matches the fingerprint the snapshot was taken from, this rebuilds
 * m_hScroll/m_vScroll's SCALE (pixelsPerTick, maximum, pageStep) from the CURRENT content
 * extents, factor, offsets and size -- the same inputs and the same computeAxisState()
 * adjustOffsetControls() uses -- so a factor that changed silently is no longer stale
 * (the bug this fixes: only the origin used to be corrected, never the scale).
 *
 * The origin is then RE-ANCHORED, not taken from that fresh compute() as is: the bar
 * widget itself is not (and must not be, see below) touched here, so it still shows
 * whatever value it last displayed (m_hScroll.value, unchanged since); origin is solved
 * for so that viewStartFor(that unchanged value) reproduces the CURRENT actual view start
 * exactly, now through the fresh pixelsPerTick. This is what keeps a rebase with no value
 * change from moving the view, and what makes a value change from here measure correctly
 * in fresh, current pixels-per-tick rather than in whatever pixels-per-tick happened to
 * be in effect when the bar was last synced for real.
 *
 * This must NOT touch the QScrollBar widgets (applyScrollState()/setRange()/setValue()):
 * it runs from inside slotHScrolled()/slotVScrolled(), themselves called from a bar's own
 * valueChanged, so writing back to the bar here would recurse. The widgets simply stay
 * at their last-synced (possibly now stale) range/page-step until the next real resync,
 * either adjustOffsetControls() on sliderReleased or a later notified viewport change;
 * only the internal snapshot used to map THIS slot call's value needs to be correct.
 *
 * Returns the freshly computed viewport key (whether or not it differed from
 * m_scrollKey), so a caller that already needs it (a slot patching just the one field
 * that changed) does not have to call viewportKey() again from scratch.
 */
LC_ScrollViewportKey QG_GraphicView::rebaseScrollIfStale() {
    auto* viewport = getViewPort();
    const LC_ScrollViewportKey key = viewportKey(*viewport);
    if (key == m_scrollKey) {
        return key;
    }
    const int width = viewport->getWidth();
    const int height = viewport->getHeight();
    if (width <= 0 || height <= 0) {
        // No sane axis state can be built at a degenerate size: leave m_hScroll/m_vScroll
        // untouched, and hand back the OLD m_scrollKey (not the freshly computed one) so a
        // caller that stores our return value back into m_scrollKey does not mark this
        // change as already handled. The next call, once the size is sane again, must still
        // see the fingerprint mismatch and recompute for real.
        return m_scrollKey;
    }
    RS_Vector wcsMin;
    RS_Vector wcsMax;
    RS_Vector ucsMin;
    RS_Vector ucsMax;
    const bool hasContent = scrollContentExtents(wcsMin, wcsMax);
    if (hasContent) {
        viewport->ucsBoundsOfWcsBox(wcsMin, wcsMax, ucsMin, ucsMax);
    }
    const RS_Vector factor = viewport->getFactor();
    const int ox = viewport->getOffsetX();
    const int oy = viewport->getOffsetY();

    if (m_hScroll.valid) {
        const double currentStart = -static_cast<double>(ox);
        const double anchorValue = m_hScroll.value;
        m_hScroll = computeAxisState(true, hasContent, ucsMin, ucsMax, factor, ox, oy, width, height);
        if (m_hScroll.valid) {
            m_hScroll.origin = currentStart - anchorValue * m_hScroll.pixelsPerTick;
            m_hScroll.value = static_cast<int>(anchorValue);
        }
    }
    if (m_vScroll.valid) {
        const double currentStart = static_cast<double>(oy) - static_cast<double>(height);
        const double anchorValue = m_vScroll.value;
        m_vScroll = computeAxisState(false, hasContent, ucsMin, ucsMax, factor, ox, oy, width, height);
        if (m_vScroll.valid) {
            m_vScroll.origin = currentStart - anchorValue * m_vScroll.pixelsPerTick;
            m_vScroll.value = static_cast<int>(anchorValue);
        }
    }
    m_scrollKey = key;
    return key;
}

/**
 * Built on demand (QG_ScrollBar asks on QEvent::ToolTip), from the drawing's UCS box
 * cached at the last bar sync and the current view, in the drawing's units and linear
 * format. The view range follows computeAxisState()'s mapping: a UCS coordinate c is
 * the scroll-space pixel c * factor.x (x) or -c * factor.y (y). Empty, so no tooltip,
 * while the band is switched off (Appearance/ScrollBarContentBand).
 */
QString QG_GraphicView::scrollBarToolTip(const bool horizontal) const {
    const auto* viewport = getViewPort();
    if (!m_scrollBarContentBand || viewport == nullptr) {
        return {};
    }
    const RS_Vector factor = viewport->getFactor();
    const double scale = horizontal ? factor.x : factor.y;
    const double length = horizontal ? viewport->getWidth() : viewport->getHeight();
    if (!(scale > 0.0) || !(length > 0.0)) {
        return {};
    }
    const double offset = horizontal ? viewport->getOffsetX() : viewport->getOffsetY();
    const double viewMin = -offset / scale;
    const double viewMax = (length - offset) / scale;

    const RS_Graphic* graphic = getGraphic();
    const RS2::Unit unit = graphic != nullptr ? graphic->getUnit() : RS2::None;
    const RS2::LinearFormat format = graphic != nullptr ? graphic->getLinearFormat() : RS2::Decimal;
    const int precision = graphic != nullptr ? graphic->getLinearPrecision() : 4;
    auto linear = [unit, format, precision](const double value) {
        return RS_Units::formatLinear(value, unit, format, precision);
    };
    const QString axis = horizontal ? QStringLiteral("X") : QStringLiteral("Y");
    const QString separator = QStringLiteral(" · ");
    const QString view = tr("View %1 %2..%3").arg(axis, linear(viewMin), linear(viewMax));
    if (!m_scrollTipHasContent) {
        return tr("The drawing is empty") + separator + view;
    }
    const double drawingMin = horizontal ? m_scrollTipUcsMin.x : m_scrollTipUcsMin.y;
    const double drawingMax = horizontal ? m_scrollTipUcsMax.x : m_scrollTipUcsMax.y;
    const QString drawing = tr("Drawing %1 %2..%3").arg(axis, linear(drawingMin), linear(drawingMax));

    // distance in view lengths: one decimal while it is small, whole numbers beyond
    auto views = [horizontal, viewLength = viewMax - viewMin](const double distance) {
        const double count = distance / viewLength;
        if (count < 0.1) {
            return horizontal ? tr("less than 0.1 view widths") : tr("less than 0.1 view heights");
        }
        const QString number = count < 10.0 ? QString::number(count, 'f', 1)
                                            : QString::number(std::round(count), 'f', 0);
        return horizontal ? tr("≈%1 view widths").arg(number) : tr("≈%1 view heights").arg(number);
    };
    QString where;
    if (viewMin <= drawingMin && drawingMax <= viewMax) {
        where = tr("view shows the whole drawing");
    } else if (drawingMax <= viewMin) {  // touching counts as off, as for the band (bandIsInformative)
        const double distance = viewMin - drawingMax;
        where = horizontal ? tr("drawing is %1 to the left (%2)").arg(linear(distance), views(distance))
                           : tr("drawing is %1 down (%2)").arg(linear(distance), views(distance));
    } else if (drawingMin >= viewMax) {
        const double distance = drawingMin - viewMax;
        where = horizontal ? tr("drawing is %1 to the right (%2)").arg(linear(distance), views(distance))
                           : tr("drawing is %1 up (%2)").arg(linear(distance), views(distance));
    } else {
        where = tr("drawing is in view");
    }
    return drawing + separator + view + separator + where;
}

void QG_GraphicView::setScrollBarToolTips(const bool enabled) {
    for (QG_ScrollBar* bar : {m_hScrollBar, m_vScrollBar}) {
        if (bar == nullptr) {
            continue;
        }
        if (enabled) {
            const bool horizontal = bar->orientation() == Qt::Horizontal;
            bar->setToolTipProvider([this, horizontal] {
                return scrollBarToolTip(horizontal);
            });
        } else {
            bar->setToolTipProvider(nullptr);
        }
    }
}

/**
 * override this to adjust controls and widgets that
 * control the zoom factor of the graphic.
 */
void QG_GraphicView::adjustZoomControls() {
}


/**
 * Shared body of slotHScrolled()/slotVScrolled(): rebase \p axisState if the viewport
 * changed silently, then map \p value through it and apply the result via \p
 * applyViewStart, which sets the viewport's offset for this axis and returns the
 * (int) offset it actually used. Only that one key field (ox for H, oy for V) is then
 * patched into a copy of the key rebaseScrollIfStale() already computed, rather than
 * calling viewportKey() again from scratch (see LC_ScrollViewportKey's comment).
 *
 * The H/V axes differ in more than one line each (Y is flipped and offset by the view
 * height, X is not), so those differences stay as the two explicit one-line lambdas at
 * each call site; this shares only what is byte-identical between them.
 */
void QG_GraphicView::driveScrollAxis(LC_ScrollModel::State& axisState, const int value,
                                     const std::function<int(double)>& applyViewStart,
                                     int LC_ScrollViewportKey::* const keyField) {
    if (!axisState.valid) {
        return;
    }
    LC_ScrollViewportKey key = rebaseScrollIfStale();
    const QScopedValueRollback<bool> driving(m_scrollSyncing, true);
    key.*keyField = applyViewStart(axisState.viewStartFor(value));
    axisState.value = value;
    m_scrollKey = key;
}

/**
 * Slot for horizontal scroll events: maps the bar value through the snapshot
 * taken at the last sync. The viewport notification redraws the view.
 */
void QG_GraphicView::slotHScrolled(const int value) {
    driveScrollAxis(m_hScroll, value, [this](const double viewStart) {
        const int ox = -LC_ViewMath::saturatingRound(viewStart);
        getViewPort()->setOffsetX(ox);
        return ox;
    }, &LC_ScrollViewportKey::ox);
}

/**
 * Slot for vertical scroll events.
 */
void QG_GraphicView::slotVScrolled(const int value) {
    driveScrollAxis(m_vScroll, value, [this](const double viewStart) {
        auto* viewport = getViewPort();
        const int oy = LC_ViewMath::saturatingRound(viewStart + viewport->getHeight());
        viewport->setOffsetY(oy);
        return oy;
    }, &LC_ScrollViewportKey::oy);
}

/**
 * @brief setOffset
 * @param ox offset X
 * @param oy offset Y
 */
void QG_GraphicView::setOffset([[maybe_unused]] const int ox, [[maybe_unused]] const int oy) {
    getViewPort()->setOffsetX(ox);
    getViewPort()->setOffsetY(oy);
    // need to adjust offset control for scrollbars when setting graphicview offset
    adjustOffsetControls();
}

void QG_GraphicView::layerActivated(RS_Layer* layer) {
    const bool applyLayerToSelectedEntities = LC_GET_ONE_BOOL("Modify", "ModifyEntitiesToActiveLayer");

    if (applyLayerToSelectedEntities) {
        RS_Graphic* graphic = getGraphic();
        if (graphic != nullptr) {
            const auto doc = getDocument();
            const auto selection = doc->getSelection();
            if (!selection->isEmpty()) {
                QList<RS_Entity*> selected;
                selection->collectSelectedEntities(selected);
                if (!selected.isEmpty()) {
                    doc->undoableModify(getViewPort(), [selected, layer](LC_DocumentModificationBatch& ctx)-> bool {
                                            for (const auto en : std::as_const(selected)) {
                                                if (en != nullptr && en->isAlive()) {
                                                    RS_Entity* clone = en->clone();
                                                    clone->setLayer(layer);
                                                    clone->setPen(en->getPen(false));
                                                    clone->clearSelectionFlag();
                                                    clone->update();
                                                    ctx += clone;
                                                    ctx -= en;
                                                }
                                            }
                                            ctx.dontSetActiveLayerAndPen();
                                            return true;
                                        }, [this]([[maybe_unused]] LC_DocumentModificationBatch& ctx, RS_Document* d)-> void {
                                            RS_Selection::unselectAllInDocument(d, getViewPort());
                                        });
                }
            }

            graphic->updateInserts();
            doc->calculateBorders();
            doc->clearSelectionFlag();
            redraw(RS2::RedrawDrawing);
        }
    }
}

/**
 * Handles paint events by redrawing the graphic in this view.
 * usually that's very fast since we only paint the buffer we
 * have from the last call..
 */
void QG_GraphicView::paintEvent(QPaintEvent*) {
    getRenderer()->render();
}

#define HIDE_SELECT_CURSOR false

void QG_GraphicView::loadSettings() {
    RS_GraphicView::loadSettings();

    {
        LC_GROUP_GUARD("Appearance");
        const int zoomFactor1000 = LC_GET_INT("ScrollZoomFactor", 1137);
        m_scrollZoomFactor = zoomFactor1000 / 1000.0;

        m_ucsHighlightData->maxBlinkNumber = LC_GET_INT("UCSHighlightBlinkCount", 10) * 2;
        // one blink includes both for visible and invisible phase
        m_ucsHighlightData->timerInterval = LC_GET_INT("UCSHighlightBlinkDelay", 250);
    }

    {
        LC_GROUP_GUARD("Defaults");
        m_invertZoomDirection = LC_GET_ONE_BOOL("Defaults", "InvertZoomDirection");
        m_invertHorizontalScroll = LC_GET_BOOL("WheelScrollInvertH");
        m_invertVerticalScroll = LC_GET_BOOL("WheelScrollInvertV");
    }

    m_allowScrollAndMoveAdjustByKeys = LC_GET_ONE_BOOL("Keyboard", "AllowScrollMoveAdjustByKeys", true);

    m_scrollBarContentBand = LC_GET_ONE_BOOL("Appearance", "ScrollBarContentBand",
                                             QG_ScrollBar::kContentBandDefault);
    if (m_hScrollBar != nullptr && m_vScrollBar != nullptr) {
        m_hScrollBar->setContentBandEnabled(m_scrollBarContentBand);
        m_vScrollBar->setContentBandEnabled(m_scrollBarContentBand);
    }

    LC_GROUP("Appearance");
    {
        m_cursorHiding = LC_GET_BOOL("cursor_hiding", false);
        bool showSnapIndicatorLines = LC_GET_BOOL("indicator_lines_state", true);
        bool showSnapIndicatorShape = LC_GET_BOOL("indicator_shape_state", true);
        if (HIDE_SELECT_CURSOR) {
            // potentially, select cursor may be also hidden and so snapper will be used instead of cursor.
            // however, this will require review and modifications of significant amount of actions, so
            // probably I'll return to this later. In such case, the code within this "if" will be handy for such support
            m_selectCursorHiding = m_cursorHiding && (showSnapIndicatorLines || showSnapIndicatorShape);
        }
        m_selectCursorHiding = false;
    }
    LC_GROUP_END();
    m_ucsMarkOptions->loadSettings();

    LC_GROUP("Colors");
    {
        const RS_Color bgColor(LC_GET_STR("RelativePositionAssistantBackground", RS_Settings::RELATIVE_POSITION_BACKGROUND));
        const RS_Color txtColor(LC_GET_STR("RelativePositionAssistantText", RS_Settings::RELATIVE_POSITION_BACKGROUND));
        m_relativePointWidgetHolder->setWidgetColors(bgColor, txtColor);
    }
    LC_GROUP_END();

    LC_GROUP("RelativePositionAssistant");
    {
        const int fontSize = LC_GET_INT("FontSize", 10);
        const QString fontName = LC_GET_STR("FontName", "Helvetica");
        m_relativePointWidgetHolder->setFont(fontName, fontSize);
    }
    LC_GROUP_END();
}

void QG_GraphicView::setAntialiasing(const bool state) const {
    getRenderer()->setAntialiasing(state);
}

bool QG_GraphicView::isAntialiasing() const {
    return getRenderer()->isAntialiasing();
}

bool QG_GraphicView::isDraftMode() const {
    const auto* viewRenderer = dynamic_cast<LC_GraphicViewRenderer*>(getRenderer());
    return (viewRenderer != nullptr) ? viewRenderer->isDraftMode() : false;
}

void QG_GraphicView::setDraftMode(const bool dm) {
    auto* viewRenderer = dynamic_cast<LC_GraphicViewRenderer*>(getRenderer());
    if (viewRenderer != nullptr) {
        viewRenderer->setDraftMode(dm);
        redraw();
    }
}

void QG_GraphicView::setDraftLinesMode(const bool mode) const {
    auto* viewRenderer = dynamic_cast<LC_GraphicViewRenderer*>(getRenderer());
    if (viewRenderer != nullptr) {
        viewRenderer->setLineWidthScaling(mode);
    }
}

bool QG_GraphicView::isDraftLinesMode() const {
    const auto* viewRenderer = dynamic_cast<LC_GraphicViewRenderer*>(getRenderer());
    if (viewRenderer != nullptr) {
        return !viewRenderer->getLineWidthScaling();
    }
    return false;
}

void QG_GraphicView::addScrollbars() {
    m_scrollbars = true;

    m_hScrollBar = new QG_ScrollBar(Qt::Horizontal, this);
    m_vScrollBar = new QG_ScrollBar(Qt::Vertical, this);
    m_hScrollBar->setContentBandEnabled(m_scrollBarContentBand);
    m_vScrollBar->setContentBandEnabled(m_scrollBarContentBand);
    m_layout = new QGridLayout(this);

    setOffset(50, 50);

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    m_layout->setContentsMargins(QMargins{});
#else
    layout->setMargin(0);
#endif
    m_layout->setSpacing(0);
    m_layout->setColumnStretch(0, 1);
    m_layout->setColumnStretch(1, 0);
    m_layout->setColumnStretch(2, 0);
    m_layout->setRowStretch(0, 1);
    m_layout->setRowStretch(1, 0);
    keepScrollbarsOnDrawingSide();

    // adjustOffsetControls() freezes the range on sliderReleased through a direct
    // connection, relying on sliderReleased firing after the final SliderMove. With
    // tracking off, Qt emits sliderReleased before that move (qabstractslider.cpp),
    // and a resync there would discard the drag. QG_ScrollBar::init() enables tracking
    // unconditionally, so this invariant holds in release builds too, not only where
    // Q_ASSERT compiles in.
    m_hScrollBar->setCursor(Qt::ArrowCursor);
    m_layout->addWidget(m_hScrollBar, 1, 0);
    connect(m_hScrollBar, &QG_ScrollBar::valueChanged, this, &QG_GraphicView::slotHScrolled);
    connect(m_hScrollBar, &QG_ScrollBar::sliderReleased, this, &QG_GraphicView::adjustOffsetControls);

    m_vScrollBar->setCursor(Qt::ArrowCursor);
    m_layout->addWidget(m_vScrollBar, 0, 1);
    connect(m_vScrollBar, &QG_ScrollBar::valueChanged, this, &QG_GraphicView::slotVScrolled);
    connect(m_vScrollBar, &QG_ScrollBar::sliderReleased, this, &QG_GraphicView::adjustOffsetControls);
}

bool QG_GraphicView::hasScrollbars() const {
    return m_scrollbars;
}

void QG_GraphicView::setCursorHiding(const bool state) {
    m_cursorHiding = state;
}

void QG_GraphicView::setCurrentQAction(QAction* q_action) {
    getEventHandler()->setQAction(q_action);

    if (m_recentActions.contains(q_action)) {
        m_recentActions.removeOne(q_action);
    }
    m_recentActions.prepend(q_action);
}

void QG_GraphicView::startAutoPanTimer(const QMouseEvent* event) {
    if (event == nullptr) {
        return;
    }
    const RS_Vector cadArea_minCoord(0., 0.);
    const RS_Vector cadArea_maxCoord(getWidth(), getHeight());
    const LC_Rect cadArea_actual(cadArea_minCoord, cadArea_maxCoord);
    const LC_Rect cadArea_unprobed(cadArea_minCoord + m_panData->probedAreaOffset, cadArea_maxCoord - m_panData->probedAreaOffset);

    RS_Vector mouseCoord{event->position()};
    mouseCoord.y = cadArea_actual.height() - mouseCoord.y;

    const RS_Vector cadArea_centerPoint((cadArea_minCoord + cadArea_maxCoord) / 2.0);
    RS_Vector offset = mouseCoord - cadArea_centerPoint;
    offset = {std::abs(offset.x) - cadArea_unprobed.width() / 2., std::abs(offset.y) - cadArea_unprobed.height() / 2.};
    offset = {std::max(offset.x, 1.), std::max(offset.y, 1.)};

    const double panOffset_angle{cadArea_centerPoint.angleTo(mouseCoord)};

    /* It would be better if the below value was calculated in the code that deals with resizing the CAD area. */
    const double quarterAngle = cadArea_centerPoint.angleTo(cadArea_actual.upperRightCorner());

    double percentageFactor;

    if (((panOffset_angle > quarterAngle) && (panOffset_angle <= (M_PI - quarterAngle))) || ((panOffset_angle > (quarterAngle + M_PI)) && (
        panOffset_angle <= (M_PI + M_PI - quarterAngle)))) {
        percentageFactor = (std::abs((mouseCoord - cadArea_centerPoint).y) - cadArea_unprobed.height() / 2.0) / (cadArea_actual.height() /
            2.0 - cadArea_unprobed.height() / 2.0);
    }
    else {
        percentageFactor = (std::abs((mouseCoord - cadArea_centerPoint).x) - cadArea_unprobed.width() / 2.0) / (cadArea_actual.width() / 2.0
            - cadArea_unprobed.width() / 2.0);
    }

    const double panTimerInterval{
        m_panData->panTimerIntervalMinimum + ((m_panData->panTimerIntervalMaximum - m_panData->panTimerIntervalMinimum) * (1.0 -
            percentageFactor))
    };

    offset = RS_Vector::polar(offset.magnitude(), M_PI - panOffset_angle);
    m_panData->panOffset = {static_cast<int>(offset.x), static_cast<int>(offset.y)};

    if (m_panData->panTimer != nullptr) {
        m_panData->panTimer->setInterval(panTimerInterval);
    }
    else {
        m_panData->start(panTimerInterval, *this);
    }

    if (RS_DEBUG->getLevel() >= RS_Debug::D_INFORMATIONAL) {
        std::cout << " CAD area centre point                = " << cadArea_centerPoint << std::endl <<
            " Actual CAD area quarter angle (deg)  = " << quarterAngle * 180.0 / M_PI << std::endl <<
            " Percentage factor                    = " << percentageFactor << std::endl << " Pan offset angle (radians)           = " <<
            panOffset_angle << std::endl << " Pan offset angle (degrees)           = " << panOffset_angle * 180.0 / M_PI << std::endl <<
            " Pan offset vector                    = " << m_panData->panOffset.x() << ", " << m_panData->panOffset.y() << std::endl
            //<< " Pan timer interval (ms)              = " << m_panData->panTimer->interfac
            << std::endl << " Mouse (cursor) position (adjusted)   = " << mouseCoord << std::endl <<
            " Mouse position w.r.t. centre point   = " << mouseCoord - cadArea_centerPoint << std::endl << std::endl << std::endl;
    }
}

bool QG_GraphicView::isAutoPan(const QMouseEvent* event) const {
    if (event == nullptr) {
        return false;
    }

    const bool autopanEnabled = LC_GET_ONE_BOOL("Appearance", "Autopanning");

    if (!autopanEnabled) {
        return false;
    }

    const RS_Vector cadArea_minCoord(0., 0.);
    const RS_Vector cadArea_maxCoord(getWidth(), getHeight());
    const LC_Rect cadArea_actual(cadArea_minCoord, cadArea_maxCoord);
    const LC_Rect cadArea_unprobed(cadArea_minCoord + m_panData->probedAreaOffset, cadArea_maxCoord - m_panData->probedAreaOffset);
    if (cadArea_unprobed.width() < 0. || cadArea_unprobed.height() < 0.) {
        return false;
    }

    const RS_Vector mouseCoord{event->position()};

    if (RS_DEBUG->getLevel() >= RS_Debug::D_INFORMATIONAL) {
        std::cout << " Unprobed CAD area width and height = " << cadArea_unprobed.width() << "/" << cadArea_unprobed.height() << std::endl
            << " Actual   CAD area width and height = " << cadArea_actual.width() << "/" << cadArea_actual.height() << std::endl <<
            " Mouse (cursor) position            = " << mouseCoord << std::endl << std::endl;
    }

    return cadArea_actual.inArea(mouseCoord) && !cadArea_unprobed.inArea(mouseCoord);
}

void QG_GraphicView::deleteActionContext() const {
     delete m_actionContext;
}

/*
    Auto-pans the CAD area.
    - by Melwyn Francis Carlo <carlo.melwyn@outlook.com>
*/
void QG_GraphicView::autoPanStep() const {
    // skip first steps to avoid unintensional panning
    m_panData->delayCounter = std::min(++m_panData->delayCounter, m_panData->delayCounterMax);
    if (m_panData->delayCounter < m_panData->delayCounterMax) {
        return;
    }

    RS_DEBUG->print(RS_Debug::D_INFORMATIONAL, "%s(): Timer is ticking!", __func__);
    getViewPort()->zoomPan(m_panData->panOffset.x(), m_panData->panOffset.y());
}

QString QG_GraphicView::obtainEntityDescription(RS_Entity* entity, const RS2::EntityDescriptionLevel shortDescription) {
    const LC_QuickInfoWidget* entityInfoWidget = QC_ApplicationWindow::getAppWindow()->getEntityInfoWidget();
    if (entityInfoWidget != nullptr) {
        QString result = entityInfoWidget->getEntityDescription(entity, shortDescription);
        return result;
    }
    return "";
}

void QG_GraphicView::ucsHighlightStep() {
    const auto overlayContainer = getViewPort()->getOverlaysDrawablesContainer(RS2::OverlayGraphics::ActionPreviewEntity);
    overlayContainer->clear();
    if (m_ucsHighlightData->mayTick()) {
        if (m_ucsHighlightData->inVisiblePhase) {
            // note - potentially, here we may simply store data for custom ucs mark and create object in renderer....
            // that will eliminate storing ucs mark settings in this class
            const auto ucsMark = new LC_OverlayUCSMark(m_ucsHighlightData->origin, m_ucsHighlightData->angle, m_ucsHighlightData->forWCS,
                                                         m_ucsMarkOptions.get());
            overlayContainer->add(ucsMark);
        }
        else {
        }
    }
    else {
        m_ucsHighlightData->stop();
        // restore current view position; setOffsetAndFactor (not justSetOffsetAndFactor)
        // notifies, so the scrollbars resync and the drawing layer, not just the overlay,
        // is re-rendered (RedrawDrawing, not only RedrawOverlay as below)
        getViewPort()->setOffsetAndFactor(m_ucsHighlightData->savedViewOffset.x, m_ucsHighlightData->savedViewOffset.y,
                                              m_ucsHighlightData->savedViewFactor);
    }
    redraw(RS2::RedrawOverlay);
    update();
}

void QG_GraphicView::highlightUCSLocation(LC_UCS* ucs) {
    if (ucs == nullptr) {
        return;
    }

    const auto viewport = getViewPort();
    // save current view position
    m_ucsHighlightData->savedViewOffset.x = viewport->getOffsetX();
    m_ucsHighlightData->savedViewOffset.y = viewport->getOffsetY();
    m_ucsHighlightData->savedViewFactor = viewport->getFactor().x;

    const RS_Vector origin = ucs->getOrigin();
    const double angle = ucs->getXAxisDirection();

    // try to ensure that origin of UCS is visible if it's outside of visible part of drawing
    const double AXIS_SIZE = viewport->toUcsDX(20); // fixme - ucs - or toUcsX?
    viewport->zoomAutoEnsurePointsIncluded(origin, origin.relative(AXIS_SIZE, angle), origin.relative(AXIS_SIZE, angle + M_PI_2));

    double uiOriginPointX = 0., uiOriginPointY = 0.;
    viewport->toUI(origin, uiOriginPointX, uiOriginPointY);

    const double ucsXAxisAngleInUCS = viewport->toUCSAngle(angle);

    m_ucsHighlightData->origin = RS_Vector(uiOriginPointX, uiOriginPointY);
    m_ucsHighlightData->angle = -ucsXAxisAngleInUCS;
    m_ucsHighlightData->forWCS = !ucs->isUCS();
    const double timerInterval = m_ucsHighlightData->timerInterval;
    m_ucsHighlightData->start(timerInterval, *this);
}
