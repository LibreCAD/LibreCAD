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


/**
 * Translates a vector in real coordinates to a vector in screen coordinates.
 */
/*RS_Vector RS_GraphicView::toGui(RS_Vector v) const {
    double ucsX, ucsY;
    if(m_hasUcs){
        ucs.toUCS(v.x, v.y, ucsX, ucsY);
    }
    else{
        ucsX = v.x;
        ucsY = v.y;
    }
    RS_Vector result = RS_Vector(ucsX * factor.x + offsetX, -ucsY * factor.y + getHeight() - offsetY, 0);
    return result;
}

void RS_GraphicView::toGui(const RS_Vector &v, double& x, double& y) const {
    if(m_hasUcs){
        ucs.toUCS(v.x, v.y, x, y);
    }
    else{
        x = v.x;
        y = v.y;
    }
    x = x * factor.x + offsetX;
    y = -y * factor.y + getHeight() - offsetY;
}

RS_Vector RS_GraphicView::toUCSFromGui(const QPointF& pos) const{
    return RS_Vector(toGraphX(pos.x()), toGraphY(pos.y()));
}

RS_Vector RS_GraphicView::toUCSFromGui(double x, double y) const{
    return RS_Vector(toGraphX(x), toGraphY(y));
}

RS_Vector RS_GraphicView::toGuiFromUCS(const RS_Vector &ucs) const {
    RS_Vector result = RS_Vector(ucs.x * factor.x + offsetX, -ucs.y * factor.y + getHeight() - offsetY, 0);
    return result;
}

RS_Vector RS_GraphicView::toGuiFromUCS(double x, double y) const {
    RS_Vector result = RS_Vector(x * factor.x + offsetX, -y * factor.y + getHeight() - offsetY, 0);
    return result;
}


RS_Vector RS_GraphicView::toGui(double x, double y) const{
    double ucsX, ucsY;
    if(m_hasUcs){
        ucs.toUCS(x, y, ucsX, ucsY);
    }
    else{
        ucsX = x;
        ucsY = y;
    }
    RS_Vector result = RS_Vector(ucsX * factor.x + offsetX, -ucsY * factor.y + getHeight() - offsetY, 0);
    return result;
}

RS_Vector RS_GraphicView::toGuiD(RS_Vector v) const {
    return RS_Vector(toGuiDX(v.x), toGuiDY(v.y));
}
*/
/**
 * Translates a real coordinate in X to a screen coordinate X.
 * @param visible Pointer to a boolean which will contain true
 * after the call if the coordinate is within the visible range.
 */
/*
double RS_GraphicView::toGuiX(double x) const {
    return x * factor.x + offsetX;
}

/**
 * Translates a real coordinate in Y to a screen coordinate Y.
 */
/*double RS_GraphicView::toGuiY(double y) const {
    return -y * factor.y + getHeight() - offsetY;
}*/

/**
 * Translates a real coordinate distance to a screen coordinate distance.
 */
/*double RS_GraphicView::toGuiDY(double d) const {
    return d * factor.y;
}*/

/**
 * Translates a vector in screen coordinates to a vector in real coordinates.
 */
/*RS_Vector RS_GraphicView::toGraph(const RS_Vector &v) const {
//    return RS_Vector(toGraphX(RS_Math::round(v.x)),
//                     toGraphY(RS_Math::round(v.y)));

    double ucsX = (v.x - offsetX) / factor.x;
    double ucsY = -(v.y - getHeight() + offsetY) / factor.y;

    double worldX, worldY;

    if (m_hasUcs){
        ucs.toWorld(ucsX, ucsY, worldX, worldY);
    }
    else{
        worldX = ucsX;
        worldY = ucsY;
    }

    RS_Vector result = RS_Vector(worldX, worldY, 0);
    return result;
}

*/


/**
 * Translates a real coordinate distance to a screen coordinate distance.
 */
/*double RS_GraphicView::toGuiDX(double d) const {
    return d * factor.x;
}

*/
/**
 * Translates two screen coordinates to a vector in real coordinates.
 */
/*
RS_Vector RS_GraphicView::toGraph(const QPointF &position) const {
    return toGraph(position.x(), position.y());
}
*/
/*
RS_Vector RS_GraphicView::toUCS(const QPointF &position) const {
    return toUCS(RS_Vector(position.x(), position.y()));
}*/

/**
 * Translates two screen coordinates to a vector in real coordinates.
 */
/*RS_Vector RS_GraphicView::toGraph(int x, int y) const {
//    return RS_Vector(toGraphX(x), toGraphY(y));
     RS_Vector gui = RS_Vector(x, y, 0);
     return toGraph(gui);
}*/

/**
 * Translates a screen coordinate in X to a real coordinate X.
 */
/*double RS_GraphicView::toGraphX(int x) const {
    return (x - offsetX) / factor.x;
}*/

/**
 * Translates a screen coordinate in Y to a real coordinate Y.
 */
/*
double RS_GraphicView::toGraphY(int y) const {
    return -(y - getHeight() + offsetY) / factor.y;
}
*/

/**
 * Translates a screen coordinate distance to a real coordinate distance.
 */
/*double RS_GraphicView::toGraphDX(int d) const {
    return d / factor.x;
}*/

/**
 * Translates a screen coordinate distance to a real coordinate distance.
 */
/*double RS_GraphicView::toGraphDY(int d) const {
    return d / factor.y;
}*/

/*RS_Vector RS_GraphicView::toGraphD(int x, int y) const{
    return RS_Vector(x /factor.x, y / factor.y);
}*/








/*RS_Grid *RS_GraphicView::getGrid() const {
    return grid.get();
}*/

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
