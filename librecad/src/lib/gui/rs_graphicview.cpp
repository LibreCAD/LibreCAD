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
#include "lc_eventhandler.h"
#include "lc_graphicviewport.h"
#include "lc_relative_point_input_widget.h"
#include "lc_shortcuts_manager.h"
#include "lc_widgetviewportrenderer.h"
#include "rs_actioninterface.h"
#include "rs_entitycontainer.h"
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
RS_GraphicView::RS_GraphicView(QWidget *parent, const Qt::WindowFlags f)
    :QWidget(parent, f)
    , m_eventHandler{std::make_unique<LC_EventHandler>(this)}
    , m_viewport{std::make_unique<LC_GraphicViewport>()}
    , m_defaultSnapMode{std::make_unique<RS_SnapMode>()}
    , m_infoCursorOverlayPreferences{std::make_unique<LC_InfoCursorOverlayPrefs>()}{
    m_viewport->addViewportListener(this);
}

RS_GraphicView::~RS_GraphicView() {
    // LC_ERR << "~RS_GraphicView";
}

void RS_GraphicView::loadSettings() {
    LC_GROUP("Appearance");
    {
        m_panOnZoom = LC_GET_BOOL("PanOnZoom", false);
        m_skipFirstZoom = LC_GET_BOOL("FirstTimeNoZoom", false);

    }
    LC_GROUP_END();

    m_infoCursorOverlayPreferences->loadSettings();
    m_viewport->loadSettings();
    m_renderer->loadSettings();
}

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
void RS_GraphicView::setDocument(RS_Document *c) {
    m_document = c;
    m_viewport->setDocument(c);
}

/**
 * @return Current action or nullptr.
 */
RS_ActionInterface *RS_GraphicView::getDefaultAction() const {
    if (m_eventHandler!=nullptr) {
        return m_eventHandler->getDefaultAction();
    }
    return nullptr;
}

void RS_GraphicView::hideOptions() const {
    if (m_eventHandler != nullptr) {
        const auto defaultAction = m_eventHandler->getDefaultAction();
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
        const QAction* qaction = m_eventHandler->getQAction();
        if (qaction != nullptr){
          // todo - sand - actually, this is bad dependency, should be refactored
          return LC_ShortcutsManager::getPlainActionToolTip(qaction);
        }
    }
    return "";
}

QIcon RS_GraphicView::getCurrentActionIcon() const {
    if (m_eventHandler != nullptr) {
        const QAction* qaction = m_eventHandler->getQAction();
        if (qaction != nullptr){
            return qaction->icon();
        }
    }
    return {};
}

bool RS_GraphicView::setEventHandlerAction(const std::shared_ptr<RS_ActionInterface>& action) const {
    const bool actionActive = m_eventHandler->setCurrentAction(action);
    return actionActive;
}

/**
 * Sets the current action of the event handler.
 */
bool RS_GraphicView::setCurrentAction(const std::shared_ptr<RS_ActionInterface>& action) const {
    if (m_eventHandler != nullptr) {
        m_viewport->markRelativeZero();
        return setEventHandlerAction(action);
    }
    return false;
}

/**
 * Kills all running actions.
 */
void RS_GraphicView::killAllActions() const {
    if (m_eventHandler != nullptr) {
        m_eventHandler->killAllActions();
    }
}

bool RS_GraphicView::killAllActionsWithResult() const {
    if (m_eventHandler != nullptr) {
        return m_eventHandler->killAllActions();
    }
    return true;
}

/**
 * Go back in menu or current action.
 */
void RS_GraphicView::back(Qt::KeyboardModifiers modifiers) const {
    if (m_eventHandler && m_eventHandler->hasAction()) {
        m_eventHandler->back(modifiers);
    }
}

/**
 * Go forward with the current action.
 */
void RS_GraphicView::processEnterKey() const {
    if (m_eventHandler && m_eventHandler->hasAction()) {
        m_eventHandler->enter();
    }
}


void RS_GraphicView::keyPressEvent(QKeyEvent *event) {
    if (m_eventHandler && m_eventHandler->hasAction()) {
        m_eventHandler->keyPressEvent(event);
    }
}

/**
 * Called by the actual GUI class which implements a command line.
 */
void RS_GraphicView::commandEvent(RS_CommandEvent *e) const {
    if (m_eventHandler) {
        m_eventHandler->commandEvent(e);
    }
}

/**
 * Enables coordinate input in the command line.
 */
void RS_GraphicView::enableCoordinateInput() const {
    if (m_eventHandler) {
        m_eventHandler->enableCoordinateInput();
    }
}

/**
 * Disables coordinate input in the command line.
 */
void RS_GraphicView::disableCoordinateInput() const {
    if (m_eventHandler) {
        m_eventHandler->disableCoordinateInput();
    }
}

void RS_GraphicView::zoomAuto(const bool axis) const {
    m_viewport->zoomAuto(axis);
}


void RS_GraphicView::onViewportChanged() {
    adjustOffsetControls();
    adjustZoomControls();
    const QString info = m_viewport->getGrid()->getInfo();
    updateGridStatusWidget(info);
    redraw();
}

void RS_GraphicView::onViewportRedrawNeeded(const RS2::RedrawMethod method) {
    redraw(method);
    if (method & RS2::RedrawImmediately) {
        repaint();
    }
}

void RS_GraphicView::onUCSChanged(LC_UCS* ucs) {
    emit ucsChanged(ucs);
    const QString info = m_viewport->getGrid()->getInfo();
    updateGridStatusWidget(info);
    redraw();
}

void RS_GraphicView::notifyCurrentActionChanged(const RS2::ActionType actionType) {
    emit currentActionChanged(actionType);
}

bool RS_GraphicView::hasAction() const {
    return getEventHandler()->hasAction();
}

void RS_GraphicView::notifyLastActionFinished() const {
    return getEventHandler()->notifyLastActionFinished();
}

void RS_GraphicView::onSwitchToDefaultAction(const bool actionIsDefault, const RS2::ActionType actionRtti, const RS2::ActionType prevActionRtti) {
    emit defaultActionActivated(actionIsDefault, actionRtti, prevActionRtti);
}

void RS_GraphicView::onRelativeZeroChanged(const RS_Vector &pos) {
    emit relativeZeroChanged(pos);
    redraw(RS2::RedrawOverlay);
}

/**
 * @return Pointer to the static pattern struct that belongs to the
 * given pattern type or nullptr.
 */
const RS_LineTypePattern *RS_GraphicView::getPattern(const RS2::LineType t) {
    return RS_LineTypePattern::getPattern(t);
}

RS2::SnapRestriction RS_GraphicView::getSnapRestriction() const {
    return m_defaultSnapRes;
}

RS_SnapMode RS_GraphicView::getDefaultSnapMode() const {
    return *m_defaultSnapMode;
}

/**
 * Sets the default snap mode used by newly created actions.
 */
void RS_GraphicView::setDefaultSnapMode(const RS_SnapMode sm) const {
    *m_defaultSnapMode = sm;
    if (m_eventHandler) {
        m_eventHandler->setSnapMode(sm);
    }
}

/**
 * Sets a snap restriction (e.g. orthogonal).
 */
void RS_GraphicView::setSnapRestriction(const RS2::SnapRestriction sr) {
    m_defaultSnapRes = sr;

    if (m_eventHandler) {
        m_eventHandler->setSnapRestriction(sr);
    }
}

LC_EventHandler *RS_GraphicView::getEventHandler() const {
    return m_eventHandler.get();
}

bool RS_GraphicView::isCurrentActionRunning(const RS_ActionInterface* action) const {
    return m_eventHandler->isValid(action);
}

RS_Graphic *RS_GraphicView::getGraphic(const bool resolve) const {
    if (m_document != nullptr){
        if (resolve) {
            return m_document->getGraphic();
        }
        if (m_document->rtti() == RS2::EntityGraphic) {
            return static_cast<RS_Graphic *>(m_document);
        }
    }
    return nullptr;
}

RS_Document *RS_GraphicView::getDocument() const {
    return m_document;
}

void RS_GraphicView::switchToDefaultAction() {
   if (killAllActionsWithResult()) {
       RS_Selection::unselectAllInDocument(m_document, m_viewport.get());
   }
    redraw(RS2::RedrawAll);
}

bool RS_GraphicView::isCleanUp() const {
    return m_bIsCleanUp;
}

/* Sets the hidden state for the relative-zero marker. */
void RS_GraphicView::setRelativeZeroHiddenState(const bool isHidden) const {
    return m_viewport->setRelativeZeroHiddenState(isHidden);
}

bool RS_GraphicView::isRelativeZeroHidden() const {
    return m_viewport->isRelativeZeroHidden();
}

RS2::EntityType RS_GraphicView::getTypeToSelect() const {
    return m_typeToSelect;
}

void RS_GraphicView::setTypeToSelect(const RS2::EntityType mType) {
    m_typeToSelect = mType;
}


QString RS_GraphicView::obtainEntityDescription([[maybe_unused]] const RS_Entity* entity, [[maybe_unused]]RS2::EntityDescriptionLevel descriptionLevel) {
    return "";
}

void RS_GraphicView::setShowEntityDescriptionOnHover(const bool show) {
    m_showEntityDescriptionOnHover = show;
}

bool RS_GraphicView::getPanOnZoom() const{
    return m_panOnZoom;
}

bool RS_GraphicView::getSkipFirstZoom() const{
    return m_skipFirstZoom;
}

LC_InfoCursorOverlayPrefs* RS_GraphicView::getInfoCursorOverlayPreferences() const {
    return m_infoCursorOverlayPreferences.get();
}

void RS_GraphicView::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    m_viewport->setSize(getWidth(), getHeight());
}

bool RS_GraphicView::isPrintPreview() const {
    return m_printPreview;
}

void RS_GraphicView::setPrintPreview(const bool pv) {
    m_printPreview = pv;
}

void RS_GraphicView::setLineWidthScaling(const bool state) const {
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

void RS_GraphicView::showRelativeInputWidget(const RS_Vector& wcsPos, const RS_Vector& basePoint, bool baseIsRelativePoint, RS2::RelativePointParam param) {
    m_relativePointWidgetHolder->show(wcsPos, basePoint,baseIsRelativePoint, param);
}

void RS_GraphicView::hideRelativeInputWidget() {
    m_relativePointWidgetHolder->hide();
}

void RS_GraphicView::restoreRelativeInputWidget() {
    m_relativePointWidgetHolder->updatePosition(true);
    m_relativePointWidgetHolder->setVisible(true);
}

bool RS_GraphicView::isInRelativePointInput() const {
    return m_relativePointWidgetHolder->isVisible();
}
