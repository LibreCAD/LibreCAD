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
#include <cstdlib>
#include <iostream>

#include <QDebug>
#include <QGridLayout>
#include <QLabel>
#include <QMenu>
#include <QNativeGestureEvent>
#include <QPoint>
#include <QPointingDevice>
#include <QTimer>

#include "qg_graphicview.h"

#include "lc_actioncontext.h"
#include "lc_actionmodifymoveadjust.h"
#include "lc_eventhandler.h"
#include "lc_graphicviewport.h"
#include "lc_graphicviewrenderer.h"
#include "lc_overlayentitiescontainer.h"
#include "lc_quickinfowidget.h"
#include "lc_rect.h"
#include "lc_ucs_mark.h"
#include "qc_applicationwindow.h"
#include "qg_blockwidget.h"
#include "qg_scrollbar.h"
#include "rs.h"
#include "rs_actiondefault.h"
#include "rs_actionmodifyentity.h"
#include "rs_actionselectsingle.h"
#include "rs_blocklist.h"
#include "rs_debug.h"
#include "rs_dialogfactoryinterface.h"
#include "rs_entity.h"
#include "rs_entitycontainer.h"
#include "rs_graphic.h"
#include "rs_insert.h"
#include "rs_settings.h"

#ifdef EMU_C99
#include "emu_c99.h"
#endif

namespace {
// Issue #1765: set default cursor size: 32x32
constexpr int g_cursorSize=32; // fixme - sand - move to common public place
// Issue #1787: cursor hot spot at center by using hotX=hotY=-1
constexpr int g_hotspotXY=-1;

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
    constexpr double zoomFactor = 1.137;// fixme - to settings
// zooming factor is wheel angle delta divided by this factor
    constexpr double zoomWheelDivisor = 200.; // fixme - to settings


    // Helper function to test validity of a rect
    bool withinValidRange(double x) {
        return x >= RS_MINDOUBLE && x <= RS_MAXDOUBLE;
    }

    bool withinValidRange(const RS_Vector& vp){
        return vp.valid && withinValidRange(vp.x) && withinValidRange(vp.y);
    }

    bool isRectValid(const RS_Vector& vpMin, const RS_Vector& vpMax) {
        return
            withinValidRange(vpMin)
            && withinValidRange(vpMax)
            && vpMin.x < vpMax.x
            && vpMin.y < vpMax.y
            && vpMin.x + 1e6 >= vpMax.x
            && vpMin.y + 1e6 >= vpMax.y;
    }
}

/**
 * @brief snapEntity find the closest entity
 * @param QG_GraphicView& view - the graphic view
 * @param const QMouseEvent* event - the mouse event
 * @return RS_Entity* - the closest entity within the range of g_cursorSize
 *                      returns nullptr, if no entity is found in range
 */
RS_Entity* snapEntity(const QG_GraphicView& view, const QMouseEvent* event) {
    if (event == nullptr) {
        return nullptr;
    }
    RS_EntityContainer* container = view.getContainer();
    if (container == nullptr) {
        return nullptr;
    }
    const QPointF mapped = event->pos();
    double distance = RS_MAXDOUBLE;
    const LC_GraphicViewport* viewPort = view.getViewPort();

    auto pos = viewPort->toWorldFromUi(mapped.x(), mapped.y());
    RS_Entity* entity = container->getNearestEntity(pos, &distance, RS2::ResolveNone);

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
RS_Entity* getParentText(RS_Insert* insert) {
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
void QG_GraphicView::showEntityPropertiesDialog(RS_Entity* entity){
    if (entity == nullptr) {
        return;
    }

    // snap to the top selected parent
    while (entity != nullptr && entity->getParent() != nullptr && entity->getParent()->isSelected()) {
        entity = entity->getParent();
    }

    launchEditProperty(entity);
}

void QG_GraphicView::launchEditProperty(RS_Entity* entity){
    RS_EntityContainer* container = getContainer();
    if (entity == nullptr || container == nullptr) {
        return;
    }
    editAction( *entity);

    //container->removeEntity(entity);
    auto* doc = dynamic_cast<RS_Document*>(container);
    if (doc != nullptr) {
        doc->startUndoCycle();
    }
    // delete any temporary highlighting duplicates of the original
    auto* defaultAction = dynamic_cast<RS_ActionDefault*>(getEventHandler()->getDefaultAction());
    if (defaultAction != nullptr){
        defaultAction->clearHighLighting();
    }
    doc->endUndoCycle();
}

// Start the edit action:
// Edit Block for an insert
// Edit entity, otherwise
void QG_GraphicView::editAction( RS_Entity& entity){
    RS_EntityContainer* container = getContainer();
    if (container==nullptr) {
        return;
    }
    switch(entity.rtti()) {
        case RS2::EntityInsert: {
            auto& appWindow = QC_ApplicationWindow::getAppWindow(); // fixme - sand - remove static, it just one of parents?
            RS_BlockList* blockList = appWindow->getBlockWidget()->getBlockList();
            RS_Block* active = (blockList != nullptr) ? blockList->getActive() : nullptr;
            auto* insert = static_cast<RS_Insert*>(&entity);
            RS_Block* current = insert->getBlockForInsert();
            if (current == active) {
                active=nullptr;
            }
            else if (blockList != nullptr) {
                blockList->activate(current);
            }
            std::shared_ptr<RS_Block*> scoped{&active, [blockList](RS_Block** pointer) {
                if (pointer != nullptr && *pointer != nullptr && blockList != nullptr)
                    blockList->activate(*pointer);
            }};
            switchToAction(RS2::ActionBlocksEdit);
            break;
        }
        default:{
            m_actionContext->saveContextMenuActionContext(&entity,RS_Vector(false), entity.isSelected());
            switchToAction(RS2::ActionModifyEntity);
        }
    }
}

// Support auto-panning when the cursor is close to the view border
struct QG_GraphicView::AutoPanData{
    void start(double interval, QG_GraphicView &view){
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

struct QG_GraphicView::UCSHighlightData {
    std::unique_ptr<QTimer> m_timer;

    double m_timerInterval = 200.0;
    int m_blinkNumber = 0;
    int m_maxBlinkNumber = 15;
    bool m_inVisiblePhase = false;
    RS_Vector origin;
    double angle = 0.0;
    bool forWCS = false;


    RS_Vector m_savedViewOffset = RS_Vector(0, 0, 0);
    double m_savedViewFactor = 0.0;

    void start(double interval, QG_GraphicView &view) {
        if (m_timer == nullptr) {
            m_timer = std::make_unique<QTimer>(&view);
            connect(m_timer.get(), &QTimer::timeout, &view, &QG_GraphicView::ucsHighlightStep);
        }
        m_timer->start(interval);
    }

    bool mayTick(){
        m_blinkNumber++;
        m_inVisiblePhase = !m_inVisiblePhase;
        return m_blinkNumber <= m_maxBlinkNumber;
    }

    void stop(){
        m_blinkNumber = 0;
        m_inVisiblePhase = false;
        m_timer->stop();
    }
};

void createViewRenderer();

/**
 * Constructor.
 */
// fixme - sand - files - init by action context???
QG_GraphicView::QG_GraphicView(QWidget* parent, RS_Document* doc, LC_ActionContext* actionContext)
    :RS_GraphicView(parent, {})
    ,m_device("Mouse")
    ,m_cursorCad(new QCursor(QPixmap(":cursors/cur_cad_bmp.png"), g_hotspotXY, g_hotspotXY))
    ,m_cursorDel(new QCursor(QPixmap(":cursors/cur_del_bmp.png"), g_hotspotXY, g_hotspotXY))
    ,m_cursorSelect(new QCursor(QPixmap(":cursors/cur_select_bmp.png"), g_hotspotXY, g_hotspotXY))
    ,m_cursorMagnifier(new QCursor(QPixmap(":cursors/cur_glass_bmp.png"), g_hotspotXY, g_hotspotXY))
    ,m_cursorHand(new QCursor(QPixmap(":cursors/cur_hand_bmp.png"), g_hotspotXY, g_hotspotXY))
    ,m_isSmoothScrolling(false)
    , m_ucsMarkOptions{std::make_unique<LC_UCSMarkOptions>()}
    , m_panData{std::make_unique<AutoPanData>()}
    , m_ucsHighlightData{std::make_unique<UCSHighlightData>()}
{
    RS_DEBUG->print("QG_GraphicView::QG_GraphicView()..");

    if (doc != nullptr){
        setContainer(doc);
        doc->setGraphicView(this);
        actionContext->setDocumentAndView(doc, this);
        setDefaultAction(new RS_ActionDefault(actionContext));
    }

    m_actionContext = actionContext;

    getViewPort()->justSetOffsetAndFactor(0,0,4.0);
    getViewPort()->setBorders(10, 10, 10, 10);

    setMouseTracking(true);
    setFocusPolicy(Qt::NoFocus);

    // SourceForge issue 45 (Left-mouse drag shrinks window)
    setAttribute(Qt::WA_NoMousePropagation);

    // Issue #2264: prevents macOS from applying text-related features like the Caps Lock indicator to the non-text canvas
#ifdef Q_OS_MAC
    setAttribute(Qt::WA_InputMethodEnabled, false);
    setInputMethodHints(Qt::ImhNone);
#endif
}

void QG_GraphicView::initView() {
    createViewRenderer();
}

void QG_GraphicView::createViewRenderer() {
    if (getViewPort()) {
        getViewPort()->setSize(width(), height()); // fixme - sand - merge - CHECK THIS
        setRenderer(std::make_unique<LC_GraphicViewRenderer>(getViewPort(), this));
    }
}

void QG_GraphicView::layerToggled(RS_Layer *) {
    const RS_EntityContainer::LC_SelectionInfo &info = getContainer()->getSelectionInfo();
    m_actionContext->updateSelectionWidget(info.count, info.length);
    // RS_DIALOGFACTORY->updateSelectionWidget(info.count, info.length);
    redraw(RS2::RedrawDrawing);
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
int QG_GraphicView::getWidth() const{
    if (m_scrollbars) {
        return width() - m_vScrollBar->sizeHint().width();
    }
    else {
        return width();
    }
}

/**
 * @return height of widget.
 */
int QG_GraphicView::getHeight() const{
    if (m_scrollbars) {
        return height() - m_hScrollBar->sizeHint().height();
    }
    else {
        return height();
    }
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
            m_cursor_hiding ? setCursor(Qt::BlankCursor) : setCursor(*m_cursorCad);
            break;
        case RS2::DelCursor:
            setCursor(*m_cursorDel);
            break;
        case RS2::SelectCursor:
            m_selectCursor_hiding ? setCursor(Qt::BlankCursor) : setCursor(*m_cursorSelect);
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
void QG_GraphicView::updateGridStatusWidget(QString text){
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
void QG_GraphicView::redraw(RS2::RedrawMethod method) {
    getRenderer()->invalidate(method);
    update(); // Paint when reeady to pain
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


void QG_GraphicView::switchToAction(RS2::ActionType actionType, void* data) const {
    m_actionContext->setCurrentAction(actionType, data);
}

RS_Entity* QG_GraphicView::catchContextEntity(QMouseEvent* event, RS_Vector& clickPos) {
    auto container = getContainer();
    if (container == nullptr || event == nullptr) {
        return nullptr;
    }

    const QPointF mapped = event->pos();
    double distance = RS_MAXDOUBLE;
    const LC_GraphicViewport* viewPort = getViewPort();

    clickPos = viewPort->toWorldFromUi(mapped.x(), mapped.y());
    RS_Entity* entity = container->getNearestEntity(clickPos, &distance, RS2::ResolveNone);

    if (viewPort->toGuiDX(distance) <= g_cursorSize) {
        return entity;
    }
    else {
        return nullptr;
    }
}

bool QG_GraphicView::invokeContextMenuForMouseEvent(QMouseEvent* e) {
    bool result = false;
    RS_Vector clickPos;
    RS_Entity* entity = catchContextEntity(e, clickPos);
    auto contextMenu = QC_ApplicationWindow::getAppWindow()->createGraphicViewContentMenu(e, this, entity, clickPos);
    if (contextMenu != nullptr) {
        if (!contextMenu->isEmpty()) {
            auto actions = contextMenu->actions();
            if (actions.size() == 1) {
                auto action = actions.front();
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

void QG_GraphicView::mousePressEvent(QMouseEvent* event){
    // LC_ERR << "MOUSE PRESS";
    // pan zoom with middle mouse button
    if (event->button()==Qt::MiddleButton && event->modifiers() == Qt::NoModifier){
        switchToAction(RS2::ActionZoomPan);
        getCurrentAction()->mousePressEvent(event);
    }
    else {
        getEventHandler()->mousePressEvent(event);
    }
}

void QG_GraphicView::mouseDoubleClickEvent(QMouseEvent* e){
    // LC_ERR << "MOUSE DOUBLE CLICK";
    if (getEventHandler()->hasAction()) {

    }
    else {
        auto defaultAction = getEventHandler()->getDefaultAction();
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

void QG_GraphicView::mouseReleaseEvent(QMouseEvent* event){
    RS_DEBUG->print("QG_GraphicView::mouseReleaseEvent");

    event->accept();
    if (getEventHandler()->hasAction()) {
        switch (event->button()) {
            case Qt::RightButton: {
                if (getEventHandler()->hasAction()) {
                    back();
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
        auto defaultAction = getEventHandler()->getDefaultAction();
        if (defaultAction != nullptr) {
            int defaultActionStatus = defaultAction->getStatus();
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

bool QG_GraphicView::isMouseReleaseEventForDefaultAction(QMouseEvent* event) {
    // should correspond to LC_DlgMenuAssigner::validateShortcut()
    if (event->button() == Qt::LeftButton) {
        auto modifiers = event->modifiers();
        if (modifiers == Qt::NoModifier) { // select
            return true;
        }
        bool control = modifiers & Qt::ControlModifier;
        bool alt = modifiers & Qt::AltModifier;
        bool shift = modifiers & Qt::ShiftModifier;
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

void QG_GraphicView::mouseMoveEvent(QMouseEvent* event){
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
    if (dynamic_cast<QInputEvent*>(event) == nullptr || getDefaultAction() != nullptr){
        return QWidget::event(event);
    }
    else {
        // LC_ERR<< "Event Skipped";
    }
    return true;
}

    bool QG_GraphicView::event(QEvent *event){

    if (event->type() == QEvent::NativeGesture) {
        auto *nge = static_cast<QNativeGestureEvent *>(event);

        if (nge->gestureType() == Qt::ZoomNativeGesture) {
            double v = nge->value();
            RS2::ZoomDirection direction = std::signbit(v) ? RS2::Out : RS2::In;
            double factor = 1. + std::abs(v);

            // It seems the NativeGestureEvent::pos() incorrectly reports global coordinates
            QPointF g = mapFromGlobal(nge->globalPosition().toPoint());
            RS_Vector mouse = getViewPort()->toWorldFromUi(g.x(), g.y());
            doZoom(direction, mouse, factor);
        }
        return true;
    }
    return proceedEvent(event);
}

void QG_GraphicView::doZoom(RS2::ZoomDirection direction, RS_Vector& center, double zoom_factor) {
    if (direction==RS2::In) {
        getViewPort()->zoomIn(zoom_factor, center);
    } else {
        getViewPort()->zoomOut(zoom_factor, center);
    }
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
                if (getContainer() != nullptr) {
			        auto a = std::make_shared<RS_ActionSelectSingle>(m_actionContext);
                    setCurrentAction(a);
                    QMouseEvent ev(QEvent::MouseButtonRelease, e->position(), e->globalPosition(),
                                   Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);//RLZ
                    mouseReleaseEvent(&ev);
                    a->finish();

                    if (getContainer()->countSelected()>0) {
                        switchToAction(RS2::ActionModifyDelete);
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
                if (getContainer() != nullptr) {

                    RS_ActionSelectSingle* a =
                        new RS_ActionSelectSingle(*getContainer(), *this);
                    setCurrentAction(a);
                    QMouseEvent ev(QEvent::MouseButtonRelease, e->position(),
                                   Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);//RLZ
                    mouseReleaseEvent(&ev);
                    a->finish();

                    if (getContainer()->countSelected()>0) {
                        setCurrentAction(
                            new RS_ActionModifyDelete(*getContainer(), *this));
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
    getEventHandler()->mouseLeaveEvent();
    QWidget::leaveEvent(e);
}

void QG_GraphicView::enterEvent(QEnterEvent* e) {
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
void QG_GraphicView::wheelEvent(QWheelEvent *e) {
    // LC_ERR << "OWN WHEEL";
    //RS_DEBUG->print("wheel: %d", e->delta());

    //printf("state: %d\n", e->state());
    //printf("ctrl: %d\n", Qt::ControlButton);

    if (getContainer() == nullptr) {
        return;
    }

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
//    RS_Vector mouse = toGraph(e->position());
    const QPointF &uiEventPosition = e->position();
    RS_Vector mouse = getViewPort()->toUCSFromGui(uiEventPosition.x(), uiEventPosition.y());
#else
    RS_Vector mouse = toGraph(e->position());
#endif

    if (m_device == "Trackpad") {
        QPoint numPixels = e->pixelDelta();

        // high-resolution scrolling triggers Pan instead of Zoom logic
        m_isSmoothScrolling |= !numPixels.isNull();

        if (m_isSmoothScrolling){
            if (e->phase() == Qt::ScrollEnd) m_isSmoothScrolling = false;
        }
        else // Trackpads that without high-resolution scrolling
             // e.g. libinput-XWayland trackpads
        {
            numPixels = e->angleDelta() / 4;
        }

        if (!numPixels.isNull()){
            if (e->modifiers()==Qt::ControlModifier){
                // Hold ctrl to zoom. 1 % per pixel
                double v = (m_invertZoomDirection) ? (numPixels.y() / zoomWheelDivisor) : (-numPixels.y() / zoomWheelDivisor);
                RS2::ZoomDirection direction;
                if (v < 0) {
                    direction = RS2::Out;
                } else {
                    direction = RS2::In;
                }

                double zoomFactor = 1. + std::abs(v);
                doZoom(direction, mouse, zoomFactor);
            }
            else{
                int hDelta = (m_invertHorizontalScroll) ? -numPixels.x() : numPixels.x();
                int vDelta = (m_invertVerticalScroll) ? -numPixels.y() : numPixels.y();

                // scroll by scrollbars: issue #479 (it has its own issues)
                if (m_scrollbars){
                    m_hScrollBar->setValue(m_hScrollBar->value() - hDelta);
                    m_vScrollBar->setValue(m_vScrollBar->value() - vDelta);
                }
                else {
                    getViewPort()->zoomPan(hDelta, vDelta);
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
    int angleDeltaY = e->angleDelta().y(); // delta for VERTICAL mouse wheel
    int angleDeltaX = e->angleDelta().x(); // delta for HORIZONTAL mouse wheel

    // for zoom, let's use just vertical scrolling, so below we'll rely on AngleDeltaY only.
    // otherwise, horizontal scroll will not work :(  Basically, that's a side-effect for porting to QT6
    // so here let's use simpler logic
    angleDeltaX = angleDeltaY;
    if (e->modifiers() == Qt::ControlModifier) {
        scroll = true;
        direction= (angleDeltaY > 0) ? RS2::Up : RS2::Down;
    }
    else if(e->modifiers() == Qt::ShiftModifier){
        scroll = true;
        direction= (angleDeltaX > 0) ? RS2::Left : RS2::Right;
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
		//scroll by scrollbars: issue #479

        int delta = 0;

        switch(direction){
            case RS2::Left:
            case RS2::Right:
                delta = (m_invertHorizontalScroll) ? -angleDeltaX : angleDeltaX;
                m_hScrollBar->setValue(m_hScrollBar->value()+delta);
                break;
            default:
                delta = (m_invertVerticalScroll) ? -angleDeltaY : angleDeltaY;
                m_vScrollBar->setValue(m_vScrollBar->value()+delta);
        }
    }
    // zoom in / out:
    else if (e->modifiers()==0) {

//        LC_ERR << " AngleDelta Y "  << angleDeltaY;

        RS2::ZoomDirection zoomDirection = ((angleDeltaY > 0) != m_invertZoomDirection) ? RS2::In : RS2::Out;

        const QPoint viewCenter{getWidth()/2, getHeight()/2};
        const QPoint delta = viewCenter - uiEventPosition.toPoint();

        if (getPanOnZoom()) {
            QCursor::setPos(mapToGlobal(viewCenter));
            getViewPort()->zoomPan(delta.x(), delta.y());
        }
        if (!getPanOnZoom() || !getSkipFirstZoom() || (abs(delta.x())<32 && abs(delta.y())<32)) {
            RS_Vector& zoomCenter = mouse;
//            LC_ERR << " Mouse "  << mouse << " Direction: " << (zoomDirection == RS2::In ? "In" : "Out");

            /*// todo - well, actually this is one-shot action... and it will lead to full action processing chain in action handler
            // todo - are we REALLY need it there? alternatively, zoom may be part of this class)
            auto zoomAction = std::make_unique<RS_ActionZoomIn>(m_actionContext, zoomDirection, RS2::Both, &zoomCenter,m_scrollZoomFactor);
            zoomAction->trigger();*/
            doZoom(zoomDirection, zoomCenter, m_scrollZoomFactor);
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
void QG_GraphicView::keyPressEvent(QKeyEvent * e) {
    if (getContainer() == nullptr) {
        return;
    }
    if (m_allowScrollAndMoveAdjustByKeys) {
        RS2::Direction direction = RS2::Up;
        bool scroll = e->modifiers() == Qt::NoModifier;
        bool shift = e->modifiers() & Qt::ShiftModifier;
        bool control = e->modifiers() & Qt::ControlModifier;

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
            e->accept();
        }
    }
    if (!e->isAccepted()){
        getEventHandler()->keyPressEvent(e);
    }
}

void QG_GraphicView::keyReleaseEvent(QKeyEvent * e){
    getEventHandler()->keyReleaseEvent(e);
}

/**
* Called whenever the graphic view has changed.
* Adjusts the scrollbar ranges / steps.
*/
void QG_GraphicView::adjustOffsetControls(){
    if (!m_scrollbars)
        return;

    std::unique_lock<std::mutex> lock(m_scrollbarMutex, std::defer_lock);
    if (!lock.try_lock()) {
        return;
    }

    if (getContainer()==nullptr || m_hScrollBar==nullptr || m_vScrollBar==nullptr) {
        return;
    }
    LC_LOG<<__func__<<"(): begin";

    getContainer()->forcedCalculateBorders();
    RS_Vector vpMin = getContainer()->getMin();
    RS_Vector vpMax = getContainer()->getMax();

    // no drawing yet - still allow to scroll
    if (!isRectValid(vpMin, vpMax)) {
        vpMin = RS_Vector(-10,-10);
        vpMax = RS_Vector(100,100);
    }

    int ox = getViewPort()->getOffsetX();
    int oy = getViewPort()->getOffsetY();

    int minVal = int(-1.25 * getWidth() - ox);
    int maxVal = int( 0.25 * getWidth() - ox);

    LC_LOG<<__func__<<"(): x scrollbar range["<<minVal<<", "<<maxVal<<"]: "<<getViewPort()->getOffsetX();
    if (minVal<=maxVal) {
        m_hScrollBar->setRange(minVal, maxVal);
    }

    minVal = int(0.75 * getHeight() - oy);
    maxVal = int(0.25 * getHeight() - oy);

    if (minVal<=maxVal) {
        m_vScrollBar->setRange(minVal, maxVal);
    }

    m_hScrollBar->setPageStep(getWidth());
    m_vScrollBar->setPageStep(getHeight());

    m_hScrollBar->setValue(-ox);
    m_vScrollBar->setValue(oy);
    LC_LOG<<__func__<<"(): y scrollbar range["<<minVal<<", "<<maxVal<<"]: "<<oy;

    slotHScrolled(-ox);
    slotVScrolled(oy);


    //        RS_DEBUG->print("H min: %d / max: %d / step: %d / value: %d\n",
    //                        hScrollBar->minimum(), hScrollBar->maximum(),
    //                        hScrollBar->pageStep(), ox);

    //        RS_DEBUG->print(/*RS_Debug::D_WARNING, */"V min: %d / max: %d / step: %d / value: %d\n",
    //                        vScrollBar->minimum(), vScrollBar->maximum(),
    //                        vScrollBar->pageStep(), oy);
    LC_LOG<<__func__<<"(): end";

}


/**
 * override this to adjust controls and widgets that
 * control the zoom factor of the graphic.
 */
void QG_GraphicView::adjustZoomControls()
{
}

/**
 * Slot for horizontal scroll events.
 */
void QG_GraphicView::slotHScrolled(int value) {
    // Scrollbar behaviour tends to change with every Qt version..
    // so let's keep old code in here for now

    auto viewport = getViewPort();
    //static int running = false;
    //if (!running) {
    //running = true;
    ////RS_DEBUG->print("value x: %d\n", value);
    if (m_hScrollBar->maximum()==m_hScrollBar->minimum()) {
        getContainer()->calculateBorders();
        RS_Vector min = getContainer()->getMin();
        RS_Vector max = getContainer()->getMax();
        RS_Vector ucsMin;
        RS_Vector ucsMax;
        viewport->ucsBoundingBox(min, max, ucsMin, ucsMax);
        RS_Vector containerSize = ucsMax - ucsMin;
        viewport->centerOffsetX(ucsMin, containerSize);
    } else {
        viewport->setOffsetX(-value);
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

    if (m_vScrollBar->maximum()==m_vScrollBar->minimum()) {
        getContainer()->calculateBorders();
        RS_Vector min = getContainer()->getMin();
        RS_Vector max = getContainer()->getMax();
        RS_Vector ucsMin;
        RS_Vector ucsMax;
        getViewPort()->ucsBoundingBox(min, max, ucsMin, ucsMax);
        RS_Vector containerSize = ucsMax - ucsMin;
        getViewPort()->centerOffsetY(ucsMin, containerSize);
    } else {
        getViewPort()->setOffsetY(value);
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
void QG_GraphicView::setOffset([[maybe_unused]]int ox, [[maybe_unused]]int oy) {
    getViewPort()->setOffsetX(ox);
    getViewPort()->setOffsetY(oy);
    // need to adjust offset control for scrollbars when setting graphicview offset
    adjustOffsetControls();
}

void QG_GraphicView::layerActivated(RS_Layer *layer) {
    bool toActivated = LC_GET_ONE_BOOL("Modify", "ModifyEntitiesToActiveLayer");

    if (toActivated) {
        RS_EntityContainer *container = getContainer();
        RS_Graphic *graphic = getGraphic();
        if (graphic != nullptr) {
            QList<RS_Entity *> clones;

            graphic->startUndoCycle();

            for (auto en: std::as_const(*container)) { // fixme - sand - iterating all elements in container
                if (en != nullptr) {
                    if (en->isSelected()) {
                        RS_Entity *cl = en->clone();
                        cl->setLayer(layer);
                        en->setSelected(false);
                        cl->setSelected(false);
                        clones << cl;

                        en->setUndoState(true);
                        graphic->addUndoable(en);
                    }
                }
            }

            for (auto cl: std::as_const(clones)) {
                container->addEntity(cl);
                graphic->addUndoable(cl);
            }

            graphic->endUndoCycle();
            graphic->updateInserts();

            container->calculateBorders();
            container->setSelected(false);
            redraw(RS2::RedrawDrawing);
        }
    }
}

/**
 * Handles paint events by redrawing the graphic in this view.
 * usually that's very fast since we only paint the buffer we
 * have from the last call..
 */
void QG_GraphicView::paintEvent(QPaintEvent *){
    getRenderer()->render();
}


#define HIDE_SELECT_CURSOR false

void QG_GraphicView::loadSettings() {
    RS_GraphicView::loadSettings();

    {
        LC_GROUP_GUARD("Appearance");
        int zoomFactor1000 = LC_GET_INT("ScrollZoomFactor", 1137);
        m_scrollZoomFactor = zoomFactor1000 / 1000.0;

        m_ucsHighlightData->m_maxBlinkNumber = LC_GET_INT("UCSHighlightBlinkCount",10)*2; // one blink includes both for visible and invisible phase
        m_ucsHighlightData->m_timerInterval =  LC_GET_INT("UCSHighlightBlinkDelay",250);
    }

    {
        LC_GROUP_GUARD("Defaults");
        m_invertZoomDirection = LC_GET_ONE_BOOL("Defaults", "InvertZoomDirection");
        m_invertHorizontalScroll = LC_GET_BOOL("WheelScrollInvertH");
        m_invertVerticalScroll = LC_GET_BOOL("WheelScrollInvertV");
    }

    m_allowScrollAndMoveAdjustByKeys = LC_GET_ONE_BOOL("Keyboard", "AllowScrollMoveAdjustByKeys", true);

    LC_GROUP("Appearance");
    {
        m_cursor_hiding = LC_GET_BOOL("cursor_hiding", false);
        bool showSnapIndicatorLines = LC_GET_BOOL("indicator_lines_state", true);
        bool showSnapIndicatorShape = LC_GET_BOOL("indicator_shape_state", true);
        if (HIDE_SELECT_CURSOR) {
            // potentially, select cursor may be also hidden and so snapper will be used instead of cursor.
            // however, this will require review and modifications of significant amount of actions, so
            // probably I'll return to this later. In such case, the code within this "if" will be handy for such support
            m_selectCursor_hiding = m_cursor_hiding && (showSnapIndicatorLines || showSnapIndicatorShape);
        }
        else {
            m_selectCursor_hiding = false;
        }
    }
    LC_GROUP_END();
    m_ucsMarkOptions->loadSettings();
}

void QG_GraphicView::setAntialiasing(bool state){
    getRenderer()->setAntialiasing(state);
}

bool QG_GraphicView::isDraftMode() const {
    auto* viewRenderer = dynamic_cast<LC_GraphicViewRenderer*>(getRenderer());
    return (viewRenderer != nullptr) ?  viewRenderer->isDraftMode() : false;
}

void QG_GraphicView::setDraftMode(bool dm) {
    auto* viewRenderer = dynamic_cast<LC_GraphicViewRenderer*>(getRenderer());
    if (viewRenderer != nullptr) {
        viewRenderer->setDraftMode(dm);
        redraw();
    }
}

void QG_GraphicView::setDraftLinesMode(bool mode) {
    auto* viewRenderer = dynamic_cast<LC_GraphicViewRenderer*>(getRenderer());
    if (viewRenderer != nullptr) {
        viewRenderer->setLineWidthScaling(!mode);
    }
}

void QG_GraphicView::addScrollbars(){
    m_scrollbars = true;

    m_hScrollBar = new QG_ScrollBar(Qt::Horizontal, this);
    m_vScrollBar = new QG_ScrollBar(Qt::Vertical, this);
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

    m_hScrollBar->setSingleStep(50);
    m_hScrollBar->setCursor(Qt::ArrowCursor);
    m_layout->addWidget(m_hScrollBar, 1, 0);
    connect(m_hScrollBar, &QG_ScrollBar::valueChanged, this, &QG_GraphicView::slotHScrolled);

    m_vScrollBar->setSingleStep(50);
    m_vScrollBar->setCursor(Qt::ArrowCursor);
    m_layout->addWidget(m_vScrollBar, 0, 1);
    connect(m_vScrollBar, &QG_ScrollBar::valueChanged, this, &QG_GraphicView::slotVScrolled);
}

bool QG_GraphicView::hasScrollbars(){
    return m_scrollbars;
}

void QG_GraphicView::setCursorHiding(bool state){
    m_cursor_hiding = state;
}

void QG_GraphicView::setCurrentQAction(QAction* q_action){
    getEventHandler()->setQAction(q_action);

    if (m_recent_actions.contains(q_action)){
        m_recent_actions.removeOne(q_action);
    }
    m_recent_actions.prepend(q_action);
}


void QG_GraphicView::startAutoPanTimer(QMouseEvent *event){
    if (event == nullptr) {
        return;
    }
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

bool QG_GraphicView::isAutoPan(QMouseEvent *event) const{
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
    const LC_Rect cadArea_unprobed(cadArea_minCoord + m_panData->probedAreaOffset,
                                   cadArea_maxCoord - m_panData->probedAreaOffset);
    if (cadArea_unprobed.width() < 0. || cadArea_unprobed.height() < 0.) {
        return false;
    }

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

void QG_GraphicView::deleteActionContext() {
    delete m_actionContext;
}

/*
    Auto-pans the CAD area.
    - by Melwyn Francis Carlo <carlo.melwyn@outlook.com>
*/
void QG_GraphicView::autoPanStep(){
    // skip first steps to avoid unintensional panning
    m_panData->m_delayCounter = std::min(++ m_panData->m_delayCounter, m_panData->delayCounterMax);
    if (m_panData->m_delayCounter < m_panData->delayCounterMax) {
        return;
    }

    RS_DEBUG->print(RS_Debug::D_INFORMATIONAL, "%s(): Timer is ticking!", __func__);
    getViewPort()->zoomPan(m_panData->panOffset.x(), m_panData->panOffset.y());
}


QString QG_GraphicView::obtainEntityDescription(RS_Entity *entity, RS2::EntityDescriptionLevel shortDescription) {
    LC_QuickInfoWidget *entityInfoWidget = QC_ApplicationWindow::getAppWindow()->getEntityInfoWidget();
    if (entityInfoWidget != nullptr){
        QString result = entityInfoWidget->getEntityDescription(entity, shortDescription);
        return result;
    }
    return "";
}

void QG_GraphicView::ucsHighlightStep(){
    auto overlayContainer = getViewPort()->getOverlaysDrawablesContainer(RS2::OverlayGraphics::ActionPreviewEntity);
    overlayContainer->clear();
    if (m_ucsHighlightData->mayTick()){
        if (m_ucsHighlightData->m_inVisiblePhase) {
            // note - potentially, here we may simply store data for custom ucs mark and create object in renderer....
            // that will eliminate storing ucs mark settings in this class
            auto m_ucsMark = new LC_OverlayUCSMark(m_ucsHighlightData->origin, m_ucsHighlightData->angle,
                                                   m_ucsHighlightData->forWCS, m_ucsMarkOptions.get());
            overlayContainer->add(m_ucsMark);
        }
        else{
        }
    }
    else{
        m_ucsHighlightData->stop();
        // restore current view position
        getViewPort()->justSetOffsetAndFactor(m_ucsHighlightData->m_savedViewOffset.x,
                                         m_ucsHighlightData->m_savedViewOffset.y,
                                         m_ucsHighlightData->m_savedViewFactor);
    }
    redraw(RS2::RedrawOverlay);
    update();
}

void QG_GraphicView::highlightUCSLocation(LC_UCS *ucs){
    if (ucs == nullptr){
        return;
    }

    auto viewport = getViewPort();
    // save current view position
    m_ucsHighlightData->m_savedViewOffset.x = viewport->getOffsetX();
    m_ucsHighlightData->m_savedViewOffset.y = viewport->getOffsetY();
    m_ucsHighlightData->m_savedViewFactor = viewport->getFactor().x;

    RS_Vector origin = ucs->getOrigin();
    double angle = ucs->getXAxisDirection();

    // try to ensure that origin of UCS is visible if it's outside of visible part of drawing
    double AXIS_SIZE = viewport->toUcsDX(20); // fixme - ucs - or toUcsX?
    viewport->zoomAutoEnsurePointsIncluded(origin, origin.relative(AXIS_SIZE, angle),  origin.relative(AXIS_SIZE, angle+M_PI_2));

    double uiOriginPointX=0., uiOriginPointY=0.;
    viewport->toUI(origin, uiOriginPointX, uiOriginPointY);

    double ucsXAxisAngleInUCS = viewport->toUCSAngle(angle);

    m_ucsHighlightData->origin = RS_Vector(uiOriginPointX, uiOriginPointY);
    m_ucsHighlightData->angle = -ucsXAxisAngleInUCS;
    m_ucsHighlightData->forWCS = ! ucs->isUCS();
    double timerInterval = m_ucsHighlightData->m_timerInterval;
    m_ucsHighlightData->start(timerInterval, *this);
}
