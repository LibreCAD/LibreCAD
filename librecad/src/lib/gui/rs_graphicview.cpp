/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2015 A. Stebich (librecad@mail.lordofbikes.de)
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


#include "rs_graphicview.h"

#include "lc_cursoroverlayinfo.h"
#include "lc_graphicviewport.h"
#include "lc_shortcuts_manager.h"
#include "lc_widgetviewportrenderer.h"
#include "rs_actioninterface.h"
#include "rs_entitycontainer.h"
#include "rs_eventhandler.h"
#include "rs_graphic.h"
#include "rs_grid.h"
#include "rs_linetypepattern.h"
#include "rs_selection.h"
#include "rs_settings.h"
#include "rs_snapper.h"

#ifdef EMU_C99
#include "emu_c99.h"
#endif


/**
 * Constructor.
 */
RS_GraphicView::RS_GraphicView(QWidget *parent, Qt::WindowFlags f)
    :QWidget(parent, f)
    , m_eventHandler{std::make_unique<RS_EventHandler>(this)}
    , m_viewport{std::make_unique<LC_GraphicViewport>()}
    , defaultSnapMode{std::make_unique<RS_SnapMode>()}
    , infoCursorOverlayPreferences{std::make_unique<LC_InfoCursorOverlayPrefs>()}{
    m_viewport->addViewportListener(this);
}

void RS_GraphicView::loadSettings() {
    LC_GROUP("Appearance");
    {
        m_panOnZoom = LC_GET_BOOL("PanOnZoom", false);
        m_skipFirstZoom = LC_GET_BOOL("FirstTimeNoZoom", false);

    }
    LC_GROUP_END();

    infoCursorOverlayPreferences->loadSettings();
    m_viewport->loadSettings();
    m_renderer->loadSettings();
}

RS_GraphicView::~RS_GraphicView() = default;

/**
 * Must be called by any derived class in the destructor.
 */
void RS_GraphicView::cleanUp() {
    m_bIsCleanUp = true;
}

/**
 * Sets the pointer to the graphic which contains the entities
 * which are visualized by this widget.
 */
void RS_GraphicView::setContainer(RS_EntityContainer *c) {
    container = c;
    m_viewport->setContainer(c);
//adjustOffsetControls();
}

/**
 * @return Current action or nullptr.
 */
RS_ActionInterface *RS_GraphicView::getDefaultAction() const {
    if (m_eventHandler!=nullptr) {
        return m_eventHandler->getDefaultAction();
    } else {
        return nullptr;
    }
}

void RS_GraphicView::hideOptions() const {
    if (m_eventHandler != nullptr) {
        auto defaultAction = m_eventHandler->getDefaultAction();
        if (defaultAction != nullptr) {
            defaultAction->hideOptions();
        }
    }
}

/**
 * Sets the default action of the event handler.
 */
void RS_GraphicView::setDefaultAction(RS_ActionInterface *action) const {
    if (m_eventHandler !=nullptr) {
        m_eventHandler->setDefaultAction(action);
    }
}

/**
 * @return Current action or nullptr.
 */
RS_ActionInterface *RS_GraphicView::getCurrentAction() const {
    return (nullptr != m_eventHandler) ? m_eventHandler->getCurrentAction() : nullptr;
}

QString RS_GraphicView::getCurrentActionName() const {
    if (m_eventHandler !=nullptr) {
        QAction* qaction = m_eventHandler->getQAction();
        if (qaction != nullptr){
          // todo - sand - actually, this is bad dependency, should be refactored
          return LC_ShortcutsManager::getPlainActionToolTip(qaction);
        }
    }
    return "";
}

QIcon RS_GraphicView::getCurrentActionIcon() const {
    if (m_eventHandler != nullptr) {
        QAction* qaction = m_eventHandler->getQAction();
        if (qaction != nullptr){
            return qaction->icon();
        }
    }
    return {};
}

bool RS_GraphicView::setEventHandlerAction(std::shared_ptr<RS_ActionInterface> action){
    bool actionActive = m_eventHandler->setCurrentAction(action);
    if (actionActive) {
        if (m_eventHandler->hasAction()) {
            emit currentActionChanged(action.get());
        }
        else {
            notifyNoActiveAction();
        }
    }
    return actionActive;
}

/**
 * Sets the current action of the event handler.
 */
bool RS_GraphicView::setCurrentAction(std::shared_ptr<RS_ActionInterface> action) {
    if (m_eventHandler != nullptr) {
        m_viewport->markRelativeZero();
        return setEventHandlerAction(action);
    }

    return false;
}

/**
 * Kills all running selection actions. Called when a selection action
 * is launched to reduce confusion.
 */
// fixme - sand - files - review this method again. Why it is so special and different from killAllActions?
// actually, selection may be performed by any inherited class of RS_ActionSelectBase...
// leave if for now as it, yet return later.
void RS_GraphicView::killSelectActions() const {
    if (m_eventHandler != nullptr) {
        m_eventHandler->killSelectActions();
    }
}

/**
 * Kills all running actions.
 */
void RS_GraphicView::killAllActions() const {
    if (m_eventHandler != nullptr && forcedActionKillAllowed) {
        m_eventHandler->killAllActions();
    }
}

/**
 * Go back in menu or current action.
 */
void RS_GraphicView::back() const {
    if (m_eventHandler && m_eventHandler->hasAction()) {
        m_eventHandler->back();
    }
}

/**
 * Go forward with the current action.
 */
void RS_GraphicView::processEnterKey() {
    if (m_eventHandler && m_eventHandler->hasAction()) {
        m_eventHandler->enter();
    }
}

void keyPressEvent(QKeyEvent *event);

void RS_GraphicView::keyPressEvent(QKeyEvent *event) {
    if (m_eventHandler && m_eventHandler->hasAction()) {
        m_eventHandler->keyPressEvent(event);
    }
}

/**
 * Called by the actual GUI class which implements a command line.
 */
void RS_GraphicView::commandEvent(RS_CommandEvent *e) {
    if (m_eventHandler) {
        m_eventHandler->commandEvent(e);
    }
}

/**
 * Enables coordinate input in the command line.
 */
void RS_GraphicView::enableCoordinateInput() {
    if (m_eventHandler) {
        m_eventHandler->enableCoordinateInput();
    }
}

/**
 * Disables coordinate input in the command line.
 */
void RS_GraphicView::disableCoordinateInput() {
    if (m_eventHandler) {
        m_eventHandler->disableCoordinateInput();
    }
}

void RS_GraphicView::zoomAuto(bool axis){
    m_viewport->zoomAuto(axis);
}

void RS_GraphicView::onViewportChanged() {
    adjustOffsetControls();
    adjustZoomControls();
    QString info = m_viewport->getGrid()->getInfo();
    updateGridStatusWidget(info);
    redraw();
}

void RS_GraphicView::onViewportRedrawNeeded() {
    redraw(RS2::RedrawDrawing);
}

void RS_GraphicView::onUCSChanged(LC_UCS* ucs) {
    emit ucsChanged(ucs);
    QString info = m_viewport->getGrid()->getInfo();
    updateGridStatusWidget(info);
    redraw();
}

void RS_GraphicView::notifyNoActiveAction(){
    emit currentActionChanged(nullptr);
}

void RS_GraphicView::onRelativeZeroChanged(const RS_Vector &pos) {
    emit relativeZeroChanged(pos);
    redraw(RS2::RedrawOverlay);
}

/**
 * @return Pointer to the static pattern struct that belongs to the
 * given pattern type or nullptr.
 */
const RS_LineTypePattern *RS_GraphicView::getPattern(RS2::LineType t) {
    return RS_LineTypePattern::getPattern(t);
}

RS2::SnapRestriction RS_GraphicView::getSnapRestriction() const {
    return defaultSnapRes;
}

RS_SnapMode RS_GraphicView::getDefaultSnapMode() const {
    return *defaultSnapMode;
}

/**
 * Sets the default snap mode used by newly created actions.
 */
void RS_GraphicView::setDefaultSnapMode(RS_SnapMode sm) {
    *defaultSnapMode = sm;
    if (m_eventHandler) {
        m_eventHandler->setSnapMode(sm);
    }
}

/**
 * Sets a snap restriction (e.g. orthogonal).
 */
void RS_GraphicView::setSnapRestriction(RS2::SnapRestriction sr) {
    defaultSnapRes = sr;

    if (m_eventHandler) {
        m_eventHandler->setSnapRestriction(sr);
    }
}

RS_EventHandler *RS_GraphicView::getEventHandler() const {
    return m_eventHandler.get();
}

RS_Graphic *RS_GraphicView::getGraphic() const {
    if (container && container->rtti() == RS2::EntityGraphic) {
        return static_cast<RS_Graphic *>(container);
    }

    return nullptr;
}

RS_EntityContainer *RS_GraphicView::getContainer() const {
    return container;
}

void RS_GraphicView::switchToDefaultAction() {
    killAllActions();
    RS_Selection s(*container, m_viewport.get());
    s.selectAll(false);
    redraw(RS2::RedrawAll);
}

bool RS_GraphicView::isCleanUp(void) const {
    return m_bIsCleanUp;
}

/* Sets the hidden state for the relative-zero marker. */
void RS_GraphicView::setRelativeZeroHiddenState(bool isHidden) {
    return m_viewport->setRelativeZeroHiddenState(isHidden);
}

bool RS_GraphicView::isRelativeZeroHidden() {
    return m_viewport->isRelativeZeroHidden();
}

RS2::EntityType RS_GraphicView::getTypeToSelect() const {
    return typeToSelect;
}

void RS_GraphicView::setTypeToSelect(RS2::EntityType mType) {
    typeToSelect = mType;
}

void RS_GraphicView::setForcedActionKillAllowed(bool enabled) {
    forcedActionKillAllowed = enabled;
}

QString RS_GraphicView::obtainEntityDescription([[maybe_unused]]RS_Entity *entity, [[maybe_unused]]RS2::EntityDescriptionLevel descriptionLevel) {
    return "";
}

void RS_GraphicView::setShowEntityDescriptionOnHover(bool show) {
    showEntityDescriptionOnHover = show;
}

bool RS_GraphicView::getPanOnZoom() const{
    return m_panOnZoom;
}

bool RS_GraphicView::getSkipFirstZoom() const{
    return m_skipFirstZoom;
}

LC_InfoCursorOverlayPrefs* RS_GraphicView::getInfoCursorOverlayPreferences(){
    return infoCursorOverlayPreferences.get();
}

void RS_GraphicView::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    m_viewport->setSize(getWidth(), getHeight());
}

bool RS_GraphicView::isPrintPreview() const {
    return printPreview;
}

void RS_GraphicView::setPrintPreview(bool pv) {
    printPreview = pv;
}

void RS_GraphicView::setLineWidthScaling(bool state){
    m_renderer->setLineWidthScaling(state);
}

bool RS_GraphicView::getLineWidthScaling() const{
    return m_renderer->getLineWidthScaling();
}

LC_WidgetViewPortRenderer* RS_GraphicView::getRenderer() const{
    return m_renderer.get();
}

void RS_GraphicView::setRenderer(std::unique_ptr<LC_WidgetViewPortRenderer> renderer){
    m_renderer = std::move(renderer);
}
