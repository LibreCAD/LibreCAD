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

#include<climits>
#include<cmath>

#include <QApplication>
#include <QMouseEvent>
#include <QtAlgorithms>
#include "rs_graphicview.h"

#include "rs_color.h"
#include "rs_debug.h"
#include "rs_dialogfactory.h"
#include "rs_eventhandler.h"
#include "rs_graphic.h"
#include "rs_grid.h"
#include "rs_line.h"
#include "rs_linetypepattern.h"

#include "rs_painter.h"
#include "rs_snapper.h"
#include "rs_settings.h"
#include "rs_units.h"
#include "lc_graphicviewport.h"
#include "lc_widgetviewportrenderer.h"
#include "lc_shortcuts_manager.h"

#ifdef EMU_C99
#include "emu_c99.h"
#endif


/**
 * Constructor.
 */
RS_GraphicView::RS_GraphicView(QWidget *parent, Qt::WindowFlags f)
    :QWidget(parent, f), eventHandler{new RS_EventHandler{this}},
    defaultSnapMode{std::make_unique<RS_SnapMode>()}{
    viewport = new LC_GraphicViewport();
    viewport->addViewportListener(this);
}

void RS_GraphicView::loadSettings() {
    LC_GROUP("Appearance");
    {
        m_panOnZoom = LC_GET_BOOL("PanOnZoom", false);
        m_skipFirstZoom = LC_GET_BOOL("FirstTimeNoZoom", false);

    }
    LC_GROUP_END();

    infoCursorOverlayPreferences.loadSettings();
    viewport->loadSettings();
    renderer->loadSettings();
}

RS_GraphicView::~RS_GraphicView(){
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
void RS_GraphicView::setContainer(RS_EntityContainer *c) {
    container = c;
    viewport->setContainer(c);
//adjustOffsetControls();
}

/**
 * @return Current action or nullptr.
 */
RS_ActionInterface *RS_GraphicView::getDefaultAction() {
    if (eventHandler) {
        return eventHandler->getDefaultAction();
    } else {
        return nullptr;
    }
}

/**
 * Sets the default action of the event handler.
 */
void RS_GraphicView::setDefaultAction(RS_ActionInterface *action) {
    if (eventHandler) {
        eventHandler->setDefaultAction(action);
    }
}

/**
 * @return Current action or nullptr.
 */
RS_ActionInterface *RS_GraphicView::getCurrentAction() {
    if (eventHandler) {
        return eventHandler->getCurrentAction();
    } else {
        return nullptr;
    }
}

QString  RS_GraphicView::getCurrentActionName() {
    if (eventHandler) {
        QAction* qaction = eventHandler->getQAction();
        if (qaction != nullptr){
//            return qaction->text();
          // todo - sand - actually, this is bad dependency, should be refactored
          return LC_ShortcutsManager::getPlainActionToolTip(qaction);
        }
    }
    return "";
}

QIcon RS_GraphicView::getCurrentActionIcon() {
    if (eventHandler) {
        QAction* qaction = eventHandler->getQAction();
        if (qaction != nullptr){
            return qaction->icon();
        }
    }
    return QIcon();
}

/**
 * Sets the current action of the event handler.
 */
void RS_GraphicView::setCurrentAction(RS_ActionInterface *action) {
    if (eventHandler) {
        viewport->markRelativeZero();
        eventHandler->setCurrentAction(action);
    }
}

/**
 * Kills all running selection actions. Called when a selection action
 * is launched to reduce confusion.
 */
void RS_GraphicView::killSelectActions() {
    if (eventHandler) {
        eventHandler->killSelectActions();
    }
}

/**
 * Kills all running actions.
 */
void RS_GraphicView::killAllActions() {
    if (eventHandler) {
        if (forcedActionKillAllowed) {
            eventHandler->killAllActions();
        }
    }
}

/**
 * Go back in menu or current action.
 */
void RS_GraphicView::back() {
    if (eventHandler && eventHandler->hasAction()) {
        eventHandler->back();
    }
}

/**
 * Go forward with the current action.
 */
void RS_GraphicView::enter() {
    if (eventHandler && eventHandler->hasAction()) {
        eventHandler->enter();
    }
}

void keyPressEvent(QKeyEvent *event);

void RS_GraphicView::keyPressEvent(QKeyEvent *event) {
    if (eventHandler && eventHandler->hasAction()) {
        eventHandler->keyPressEvent(event);
    }
}

/**
 * Called by the actual GUI class which implements a command line.
 */
void RS_GraphicView::commandEvent(RS_CommandEvent *e) {
    if (eventHandler) {
        eventHandler->commandEvent(e);
    }
}

/**
 * Enables coordinate input in the command line.
 */
void RS_GraphicView::enableCoordinateInput() {
    if (eventHandler) {
        eventHandler->enableCoordinateInput();
    }
}

/**
 * Disables coordinate input in the command line.
 */
void RS_GraphicView::disableCoordinateInput() {
    if (eventHandler) {
        eventHandler->disableCoordinateInput();
    }
}

void RS_GraphicView::zoomAuto(bool axis){
    viewport->zoomAuto(axis);
}

void RS_GraphicView::onViewportChanged() {
    adjustOffsetControls();
    adjustZoomControls();
    QString info = viewport->getGrid()->getInfo();
    updateGridStatusWidget(info);
    redraw();
}

void RS_GraphicView::onViewportRedrawNeeded() {
    redraw(RS2::RedrawDrawing);
}

void RS_GraphicView::onUCSChanged(LC_UCS* ucs) {
    emit ucsChanged(ucs);
    QString info = viewport->getGrid()->getInfo();
    updateGridStatusWidget(info);
    redraw();
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
    if (eventHandler) {
        eventHandler->setSnapMode(sm);
    }
}

/**
 * Sets a snap restriction (e.g. orthogonal).
 */
void RS_GraphicView::setSnapRestriction(RS2::SnapRestriction sr) {
    defaultSnapRes = sr;

    if (eventHandler) {
        eventHandler->setSnapRestriction(sr);
    }
}

RS_EventHandler *RS_GraphicView::getEventHandler() const {
    return eventHandler;
}

RS_Graphic *RS_GraphicView::getGraphic() const {
    if (container && container->rtti() == RS2::EntityGraphic) {
        return static_cast<RS_Graphic *>(container);
    } else {
        return nullptr;
    }
}

RS_EntityContainer *RS_GraphicView::getContainer() const {
    return container;
}

bool RS_GraphicView::isCleanUp(void) const {
    return m_bIsCleanUp;
}

/* Sets the hidden state for the relative-zero marker. */
void RS_GraphicView::setRelativeZeroHiddenState(bool isHidden) {
    return viewport->setRelativeZeroHiddenState(isHidden);
}

bool RS_GraphicView::isRelativeZeroHidden() {
    return viewport->isRelativeZeroHidden();
}

RS2::EntityType RS_GraphicView::getTypeToSelect() const {
    return typeToSelect;
}

void RS_GraphicView::setTypeToSelect(RS2::EntityType mType) {
    typeToSelect = mType;
}

/*
void RS_GraphicView::setHasNoGrid(bool noGrid) {
    hasNoGrid = noGrid;
}*/

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

void RS_GraphicView::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    viewport->setSize(getWidth(), getHeight());
}

bool RS_GraphicView::isPrintPreview() const {
    return printPreview;
}

void RS_GraphicView::setPrintPreview(bool pv) {
    printPreview = pv;
}

void RS_GraphicView::setLineWidthScaling(bool state){
    renderer->setLineWidthScaling(state);
}

bool RS_GraphicView::getLineWidthScaling() const{
    return renderer->getLineWidthScaling();
}
