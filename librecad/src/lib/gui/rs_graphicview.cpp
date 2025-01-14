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
#include "rs_math.h"
#include "rs_painter.h"
#include "rs_snapper.h"
#include "rs_settings.h"
#include "rs_units.h"
#include "lc_linemath.h"
#include "dxf_format.h"
#include "lc_undoablerelzero.h"
#include "lc_defaults.h"

#ifdef EMU_C99
#include "emu_c99.h"
#endif

#ifdef DEBUG_RENDERING
#define DEBUG_RENDERING_DETAILS
#endif

struct RS_GraphicView::ColorData {
    /** background color (any color) */
    RS_Color background;
    /** foreground color (black or white) */
    RS_Color foreground;
    /** grid color */
    RS_Color gridColor = Qt::gray;
    /** meta grid color */
    RS_Color metaGridColor;
    /** selected color */
    RS_Color selectedColor;
    /** highlighted color */
    RS_Color highlightedColor;
    /** Start handle color */
    RS_Color startHandleColor;
    /** Intermediate (not start/end vertex) handle color */
    RS_Color handleColor;
    /** End handle color */
    RS_Color endHandleColor;
    /** reference entities on preview color */
    RS_Color previewReferenceEntitiesColor;

    /** reference entities on preview color */
    RS_Color previewReferenceHighlightedEntitiesColor;

    /** Relative-zero marker color */
    RS_Color relativeZeroColor;

    /** colors for axis extensions */
    RS_Color xAxisExtensionColor;
    RS_Color yAxisExtensionColor;

    /** overlaybox */

    RS_Color overlayBoxLine;
    RS_Color overlayBoxFill;
    RS_Color overlayBoxLineInverted;
    RS_Color overlayBoxFillInverted;

    /* Relative-zero hidden state */
    bool hideRelativeZero = false;
};

namespace{
    // fixme - move to nicer settings
    static const RS_Color printPreviewBorderAndShadowColor = RS_Color(64, 64, 64);
    static const RS_Color printPreviewBackgroundColor = RS_Color(200, 200, 200);
    static const RS_Color printPreviewPaperColor = RS_Color(180, 180, 180);
    static const RS_Color printPreviewPrintAreaColor = RS_Color(255, 255, 255);
}

/**
 * Constructor.
 */
RS_GraphicView::RS_GraphicView(QWidget *parent, Qt::WindowFlags f)
    :QWidget(parent, f), eventHandler{new RS_EventHandler{this}},
    m_colorData{std::make_unique<ColorData>()},
    grid{std::make_unique<RS_Grid>(this)},
    defaultSnapMode{std::make_unique<RS_SnapMode>()},
    drawingMode(RS2::ModeFull),
    savedViews(16),
    previousViewTime{std::make_unique<QDateTime>(QDateTime::currentDateTime())} {
//    loadSettings();
}

void RS_GraphicView::loadSettings() {
    LC_GROUP("Appearance");
    {
        m_colorData->hideRelativeZero = LC_GET_BOOL("hideRelativeZero");
        m_extendAxisLines = LC_GET_BOOL("ExtendAxisLines", false);
        m_entityHandleHalfSize = LC_GET_INT("EntityHandleSize", 4) / 2;
        m_relativeZeroRadius = LC_GET_INT("RelZeroMarkerRadius", 5);
        m_zeroShortAxisMarkSize = LC_GET_INT("ZeroShortAxisMarkSize", 20);
        m_extendAxisModeX = LC_GET_INT("ExtendModeXAxis", 0);
        m_extendAxisModeY = LC_GET_INT("ExtendModeYAxis", 0);
        m_ignoreDraftForHighlight = LC_GET_BOOL("IgnoreDraftForHighlight", false);
        m_draftLinesMode = LC_GET_BOOL("DraftLinesMode", false);
        m_panOnZoom = LC_GET_BOOL("PanOnZoom", false);
        m_skipFirstZoom = LC_GET_BOOL("FirstTimeNoZoom", false);

        m_showUCSZeroMarker = LC_GET_BOOL("ShowUCSZeroMarker", false);
        m_showWCSZeroMarker = LC_GET_BOOL("ShowWCSZeroMarker", true);
        m_csZeroMarkerSize = LC_GET_INT("ZeroMarkerSize", 30);
        m_csZeroMarkerFontSize = LC_GET_INT("ZeroMarkerFontSize", 10);
        m_csZeroMarkerfontName = LC_GET_STR("ZeroMarkerFontName", "Verdana");
        m_csZeroMarkerFont = QFont(m_csZeroMarkerfontName, m_csZeroMarkerFontSize);
        m_ucsApplyingPolicy = LC_GET_INT("UCSApplyPolicy",0);

    } // Appearance group
    LC_GROUP_END();

    LC_GROUP("Render");
    {
        minRenderableTextHeightInPx = LC_GET_INT("MinRenderableTextHeightPx", 4);
        int minArcRadius100 = LC_GET_INT("MinArcRadius", 80);
        minArcDrawingRadius = minArcRadius100 / 100.0;

        int minCircleRadius100 = LC_GET_INT("MinCircleRadius", 200);
        minCircleDrawingRadius = minCircleRadius100 / 100.0;

        int minLineLen100 = LC_GET_INT("MinLineLen", 200);
        minLineDrawingLen = minLineLen100 / 100.0;

        int minEllipseMajor100 = LC_GET_INT("MinEllipseMajor", 200);
        minEllipseMajorRadius = minEllipseMajor100 / 100.0;

        int minEllipseMinor100 = LC_GET_INT("MinEllipseMinor", 200);
        minEllipseMinorRadius = minEllipseMinor100 / 100.0;

        drawTextsAsDraftForPanning = LC_GET_BOOL("DrawTextsAsDraftInPanning", true);
        drawTextsAsDraftForPreview = LC_GET_BOOL("DrawTextsAsDraftInPreview", true);
    } // Render group
    LC_GROUP_END();

    LC_GROUP_GUARD("Colors");
    {
        setBackground(QColor(LC_GET_STR("background", RS_Settings::background)));
        setSelectedColor(QColor(LC_GET_STR("select", RS_Settings::select)));
        setHighlightedColor(QColor(LC_GET_STR("highlight", RS_Settings::highlight)));
        setStartHandleColor(QColor(LC_GET_STR("start_handle", RS_Settings::start_handle)));
        setHandleColor(QColor(LC_GET_STR("handle", RS_Settings::handle)));
        setEndHandleColor(QColor(LC_GET_STR("end_handle", RS_Settings::end_handle)));
        setRelativeZeroColor(QColor(LC_GET_STR("relativeZeroColor", RS_Settings::relativeZeroColor)));
        setPreviewReferenceEntitiesColor(QColor(LC_GET_STR("previewReferencesColor", RS_Settings::previewRefColor)));
        setPreviewReferenceHighlightedEntitiesColor(QColor(LC_GET_STR("previewReferencesHighlightColor", RS_Settings::previewRefHighlightColor)));
        setXAxisExtensionColor(QColor(LC_GET_STR("grid_x_axisColor", "red")));
        setYAxisExtensionColor(QColor(LC_GET_STR("grid_y_axisColor", "green")));

        int overlayTransparency = LC_GET_INT("overlay_box_transparency",90);

        setOverlayBoxLineColor(QColor(LC_GET_STR("overlay_box_line", RS_Settings::overlayBoxLine)));
        QColor tmp = QColor(LC_GET_STR("overlay_box_fill", RS_Settings::overlayBoxFill));
        RS_Color fillColor(tmp.red(), tmp.green(), tmp.blue(), overlayTransparency);
        setOverlayBoxFillColor(fillColor);

        setOverlayBoxLineInvertedColor(QColor(LC_GET_STR("overlay_box_line_inv", RS_Settings::overlayBoxLineInverted)));
        tmp = QColor(LC_GET_STR("overlay_box_fill_inv", RS_Settings::overlayBoxFillInverted));
        RS_Color fillColorInverted(tmp.red(), tmp.green(), tmp.blue(), overlayTransparency);
        setOverlayBoxFillInvertedColor(fillColorInverted);
    } // colors group

    if (grid != nullptr){
        grid->loadSettings();
    }


    LC_GROUP("InfoOverlayCursor");
    {
        infoCursorOverlayPreferences.enabled = LC_GET_BOOL("Enabled", true);
        if (infoCursorOverlayPreferences.enabled) {
            infoCursorOverlayPreferences.showAbsolutePosition = LC_GET_BOOL("ShowAbsolute", true);
            infoCursorOverlayPreferences.showAbsolutePositionWCS = LC_GET_BOOL("ShowAbsoluteWCS", false);

            infoCursorOverlayPreferences.showRelativePositionDistAngle = LC_GET_BOOL("ShowRelativeDA", true);
            infoCursorOverlayPreferences.showRelativePositionDeltas = LC_GET_BOOL("ShowRelativeDD", true);
            infoCursorOverlayPreferences.showSnapType = LC_GET_BOOL("ShowSnapInfo", true);
            infoCursorOverlayPreferences.showCurrentActionName = LC_GET_BOOL("ShowActionName", true);
            infoCursorOverlayPreferences.showCommandPrompt = LC_GET_BOOL("ShowPrompt", true);
            infoCursorOverlayPreferences.showLabels = LC_GET_BOOL("ShowLabels", false);
            infoCursorOverlayPreferences.multiLine = !LC_GET_BOOL("SingleLine", true);

            infoCursorOverlayPreferences.showEntityInfoOnCatch = LC_GET_BOOL("ShowPropertiesCatched", true);
            infoCursorOverlayPreferences.showEntityInfoOnCreation = LC_GET_BOOL("ShowPropertiesCreating", true);
            infoCursorOverlayPreferences.showEntityInfoOnModification = LC_GET_BOOL("ShowPropertiesEdit", true);

            int infoCursorFontSize = LC_GET_INT("FontSize", 10);
            // todo - potentially, we may use different font sizes for different zones later
            infoCursorOverlayPreferences.options.setFontSize(infoCursorFontSize);
            infoCursorOverlayPreferences.options.fontName = LC_GET_STR("FontName", "Helvetica");
            infoCursorOverlayPreferences.options.offset = LC_GET_INT("OffsetFromCursor", 10);
        }
    }

    LC_GROUP("Colors");
    {
        if (infoCursorOverlayPreferences.enabled) {
            infoCursorOverlayPreferences.options.zone1Settings.color = QColor(LC_GET_STR("info_overlay_absolute", RS_Settings::overlayInfoCursorAbsolutePos));
            infoCursorOverlayPreferences.options.zone2Settings.color = QColor(LC_GET_STR("info_overlay_snap", RS_Settings::overlayInfoCursorSnap));
            infoCursorOverlayPreferences.options.zone3Settings.color = QColor(LC_GET_STR("info_overlay_relative", RS_Settings::overlayInfoCursorRelativePos));
            infoCursorOverlayPreferences.options.zone4Settings.color = QColor(LC_GET_STR("info_overlay_prompt", RS_Settings::overlayInfoCursorCommandPrompt));
        }
    }
    LC_GROUP_END();
}

void RS_GraphicView::updateEndCapsStyle(const RS_Graphic *graphic) {//        Lineweight endcaps setting for new objects:
//        0 = none; 1 = round; 2 = angle; 3 = square
    int endCaps = graphic->getGraphicVariableInt("$ENDCAPS", 1);
    switch (endCaps){
        case 0:
            penCapStyle = Qt::FlatCap;
            break;
        case 1:
            penCapStyle = Qt::RoundCap;
            break;
        case 2:
            penCapStyle = Qt::MPenCapStyle;
            break;
        case 3:
            penCapStyle = Qt::SquareCap;
            break;
        default:
            penCapStyle = Qt::FlatCap; // fixme - or round?
    }
}

void RS_GraphicView::updatePointsStyle(RS_Graphic *graphic) {
    pdmode = graphic->getGraphicVariableInt("$PDMODE", LC_DEFAULTS_PDMode);
    pdsize = graphic->getGraphicVariableDouble("$PDSIZE", LC_DEFAULTS_PDSize);
}

void RS_GraphicView::updateJoinStyle(const RS_Graphic *graphic) {//0=none; 1= round; 2 = angle; 3 = flat
    int joinStyle = graphic->getGraphicVariableInt("$JOINSTYLE", 1);

    switch (joinStyle){
        case 0:
            penJoinStyle = Qt::BevelJoin;
            break;
        case 1:
            penJoinStyle = Qt::RoundJoin;
            break;
        case 2:
            penJoinStyle = Qt::MiterJoin;
            break;
        case 3:
            penJoinStyle = Qt::BevelJoin;
            break;
        default:
            penJoinStyle = Qt::RoundJoin;
    }
}

void RS_GraphicView::updateUnitAndDefaultWidthFactors(const RS_Graphic *graphic) {
    unitFactor = RS_Units::convert(1.0, RS2::Millimeter, graphic->getUnit());
    unitFactor100 =  this->unitFactor / 100.0;
    defaultWidthFactor = graphic->getVariableDouble("$DIMSCALE", 1.0);
}

void RS_GraphicView::updateGraphicRelatedSettings(RS_Graphic *graphic) {
    if (graphic != nullptr) {
        updateUnitAndDefaultWidthFactors(graphic);
        updatePointsStyle(graphic);
        updateEndCapsStyle(graphic);
        updateJoinStyle(graphic);
    }
}


RS_GraphicView::~RS_GraphicView(){
    qDeleteAll(overlayEntities);
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
void RS_GraphicView::setContainer(RS_EntityContainer *container) {
    this->container = container;
//adjustOffsetControls();
}

/**
 * Sets the zoom factor in X for this visualization of the graphic.
 */
void RS_GraphicView::setFactorX(double f) {
    if (!zoomFrozen) {
        factor.x = std::abs(f);
        invalidate();
    }
}

/**
 * Sets the zoom factor in Y for this visualization of the graphic.
 */
void RS_GraphicView::setFactorY(double f) {
    if (!zoomFrozen) {
        factor.y = std::abs(f);
        invalidate();
    }
}

void RS_GraphicView::setOffset(int ox, int oy) {
//    DEBUG_HEADER
//    RS_DEBUG->print(/*RS_Debug::D_WARNING, */"set offset from (%d, %d) to (%d, %d)", getOffsetX(), getOffsetY(), ox, oy);
    setOffsetX(ox);
    setOffsetY(oy);
}

/**
 * @return true if the grid is switched on.
 */
bool RS_GraphicView::isGridOn() const {
    if (container) {
        RS_Graphic *graphic = container->getGraphic();
        if (graphic != nullptr) {
            return graphic->isGridOn();
        }
    }
    return true;
}

/**
 * @return true if the grid is isometric
 *
 *@Author: Dongxu Li
 */
bool RS_GraphicView::isGridIsometric() const {
    return grid->isIsometric();
}


void RS_GraphicView::setIsoViewType(RS2::IsoGridViewType chType) {
    grid->setIsoViewType(chType);
}

RS2::IsoGridViewType RS_GraphicView::getIsoViewType() const {
    return grid->getIsoViewType();
}

/**
 * Centers the drawing in x-direction.
 */
void RS_GraphicView::centerOffsetX(const RS_Vector& containerMin, const RS_Vector& containerSize) {
    if (container && !zoomFrozen) {
        offsetX = (int) (((getWidth() - borderLeft - borderRight)
                          - (containerSize.x * factor.x)) / 2.0
                         - (containerMin.x * factor.x)) + borderLeft;
        invalidate();
    }
}

/**
 * Centers the drawing in y-direction.
 */
void RS_GraphicView::centerOffsetY(const RS_Vector& containerMin, const RS_Vector& containerSize) {
    if (container && !zoomFrozen) {
        offsetY = (int) ((getHeight() - borderTop - borderBottom
                          - (containerSize.y * factor.y)) / 2.0
                         - (containerMin.y * factor.y)) + borderBottom;
        invalidate();
    }
}

/**
 * Centers the given coordinate in the view in x-direction.
 */
void RS_GraphicView::centerX(double v) {
    if (!zoomFrozen) {
        offsetX = (int) ((v * factor.x)
                         - (double) (getWidth() - borderLeft - borderRight) / 2.0);
        invalidate();
    }
}

/**
 * Centers the given coordinate in the view in y-direction.
 */
void RS_GraphicView::centerY(double v) {
    if (!zoomFrozen) {
        offsetY = (int) ((v * factor.y)
                         - (double) (getHeight() - borderTop - borderBottom) / 2.0);
        invalidate();
    }
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
        markRelativeZero();
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

/**
 * zooms in by factor f
 */
void RS_GraphicView::zoomIn(double f, const RS_Vector &center) {
    if (f < 1.0e-6) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "RS_GraphicView::zoomIn: invalid factor");
        return;
    }

    RS_Vector zeroCorner = getUCSViewLeftBottom();
    RS_Vector rightTopCorner = getUCSViewRightTop();

    RS_Vector c = center;
    if (!c.valid) {
        //find mouse position
//        c = getMousePosition();
      c = (zeroCorner + rightTopCorner) * 0.5;
      return;
    }

    const RS_Vector scaleVector = RS_Vector(1.0 / f, 1.0 / f);
    zoomWindow(zeroCorner.scale(c, scaleVector), rightTopCorner.scale(c, scaleVector));
}

/**
 * zooms in by factor f in x
 */
void RS_GraphicView::zoomInX(double f) {
    factor.x *= f;
    offsetX = (int) ((offsetX - getWidth() / 2) * f) + getWidth() / 2;
    adjustOffsetControls();
    adjustZoomControls();
    invalidate();
    redraw();
}

/**
 * zooms in by factor f in y
 */
void RS_GraphicView::zoomInY(double f) {
    factor.y *= f;
    offsetY = (int) ((offsetY - getHeight() / 2) * f) + getHeight() / 2;
    adjustOffsetControls();
    adjustZoomControls();
    invalidate();
    redraw();
}

/**
 * zooms out by factor f
 */
void RS_GraphicView::zoomOut(double f, const RS_Vector &center) {
    if (f < 1.0e-6) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "RS_GraphicView::zoomOut: invalid factor");
        return;
    }
    zoomIn(1 / f, center);
}

/**
 * zooms out by factor f in x
 */
void RS_GraphicView::zoomOutX(double f) {
    if (f < 1.0e-6) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "RS_GraphicView::zoomOutX: invalid factor");
        return;
    }
    factor.x /= f;
    offsetX = (int) (offsetX / f);
    adjustOffsetControls();
    adjustZoomControls();
    invalidate();
    redraw();
}

/**
 * zooms out by factor f y
 */
void RS_GraphicView::zoomOutY(double f) {
    if (f < 1.0e-6) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "RS_GraphicView::zoomOutY: invalid factor");
        return;
    }
    factor.y /= f;
    offsetY = (int) (offsetY / f);
    adjustOffsetControls();
    adjustZoomControls();
    invalidate();
    redraw();
}

void RS_GraphicView::ucsBoundingBox(const RS_Vector& min, const RS_Vector&max, RS_Vector& ucsMin, RS_Vector& ucsMax) const{
    if (m_hasUcs) {
        RS_Vector ucsCorner1 = toUCS(min);
        RS_Vector ucsCorner3 = toUCS(max);
        RS_Vector ucsCorner2 = RS_Vector(ucsCorner1.x, ucsCorner3.y);
        RS_Vector ucsCorner4 = RS_Vector(ucsCorner3.x, ucsCorner1.y);

        double minX, maxX;
        double minY, maxY;

        maxX = std::max(ucsCorner1.x, ucsCorner3.x);
        maxX = std::max(ucsCorner2.x, maxX);
        maxX = std::max(ucsCorner4.x, maxX);

        minX = std::min(ucsCorner1.x, ucsCorner3.x);
        minX = std::min(ucsCorner2.x, minX);
        minX = std::min(ucsCorner4.x, minX);

        maxY = std::max(ucsCorner1.y, ucsCorner3.y);
        maxY = std::max(ucsCorner2.y, maxY);
        maxY = std::max(ucsCorner4.y, maxY);

        minY = std::min(ucsCorner1.y, ucsCorner3.y);
        minY = std::min(ucsCorner2.y, minY);
        minY = std::min(ucsCorner4.y, minY);

        ucsMin = RS_Vector(minX, minY);
        ucsMax = RS_Vector(maxX, maxY);
    }
    else{
        ucsMin = min;
        ucsMax = max;
    }
}


void RS_GraphicView::worldBoundingBox(const RS_Vector& ucsMin, const RS_Vector&ucsMax, RS_Vector& worldMin, RS_Vector& worldMax) const{
    if (m_hasUcs) {
        RS_Vector ucsCorner1 = ucsMin;
        RS_Vector ucsCorner3 = ucsMax;
        RS_Vector ucsCorner2 = RS_Vector(ucsCorner1.x, ucsCorner3.y);
        RS_Vector ucsCorner4 = RS_Vector(ucsCorner3.x, ucsCorner1.y);
        
        RS_Vector worldCorner1 = toWorld(ucsCorner1);
        RS_Vector worldCorner2 = toWorld(ucsCorner2);
        RS_Vector worldCorner3 = toWorld(ucsCorner3);
        RS_Vector worldCorner4 = toWorld(ucsCorner4);

        double minX, maxX;
        double minY, maxY;

        maxX = std::max(worldCorner1.x, worldCorner3.x);
        maxX = std::max(worldCorner2.x, maxX);
        maxX = std::max(worldCorner4.x, maxX);

        minX = std::min(worldCorner1.x, worldCorner3.x);
        minX = std::min(worldCorner2.x, minX);
        minX = std::min(worldCorner4.x, minX);

        maxY = std::max(worldCorner1.y, worldCorner3.y);
        maxY = std::max(worldCorner2.y, maxY);
        maxY = std::max(worldCorner4.y, maxY);

        minY = std::min(worldCorner1.y, worldCorner3.y);
        minY = std::min(worldCorner2.y, minY);
        minY = std::min(worldCorner4.y, minY);

        worldMin = RS_Vector(minX, minY);
        worldMax = RS_Vector(maxX, maxY);
    }
    else{
        worldMin = ucsMin;
        worldMax = ucsMax;
    }
}

void RS_GraphicView::zoomAutoEnsurePointsIncluded(const RS_Vector &pos1, const RS_Vector &pos2, const RS_Vector &pos3) {
    if (container) {
        container->calculateBorders();
        RS_Vector min = container->getMin();
        RS_Vector max = container->getMax();

        min = RS_Vector::minimum(min, pos1);
        min = RS_Vector::minimum(min, pos2);
        min = RS_Vector::minimum(min, pos3);

        max = RS_Vector::maximum(max, pos1);
        max = RS_Vector::maximum(max, pos2);
        max = RS_Vector::maximum(max, pos3);
        doZoomAuto(min,max, true, true);
    }
}

/**
 * performs autozoom
 *
 * @param axis include axis in zoom
 * @param keepAspectRatio true: keep aspect ratio 1:1
 *                        false: factors in x and y are stretched to the max
 */
void RS_GraphicView::zoomAuto(bool axis, bool keepAspectRatio) {
    RS_DEBUG->print("RS_GraphicView::zoomAuto");
    if (container) {
        container->calculateBorders();
        RS_Vector min = container->getMin();
        RS_Vector max = container->getMax();
        doZoomAuto(min,max, axis, keepAspectRatio);
    }
    RS_DEBUG->print("RS_GraphicView::zoomAuto OK");
}

void RS_GraphicView::doZoomAuto(const RS_Vector& min, const RS_Vector& max, bool axis, bool keepAspectRatio) {
    double sx, sy;
    RS_Vector containerSize;
    RS_Vector containerMin;

    if (m_hasUcs){
        RS_Vector ucsMin;
        RS_Vector ucsMax;

        ucsBoundingBox(min, max, ucsMin, ucsMax);

        RS_Vector ucsSize = ucsMax - ucsMin;

        sx = ucsSize.x;
        sy = ucsSize.y;

        containerSize = ucsSize;
        containerMin = ucsMin;
    }
    else {
        auto const dV = max - min;
        if (axis) {
            sx = std::max(dV.x, 0.);
            sy = std::max(dV.y, 0.);
        } else {
            sx = dV.x;
            sy = dV.y;
        }
        containerSize = dV;
        containerMin = min;
    }


//		    std::cout<<" RS_GraphicView::zoomAuto("<<sx<<","<<sy<<")"<<std::endl;
//			std::cout<<" RS_GraphicView::zoomAuto("<<axis<<","<<keepAspectRatio<<")"<<std::endl;

    double fx = 1., fy = 1.;
    unsigned short fFlags = 0;

    if (sx > RS_TOLERANCE) {
        fx = (getWidth() - borderLeft - borderRight) / sx;
    } else {
        fFlags += 1; //invalid x factor
    }

    if (sy > RS_TOLERANCE) {
        fy = (getHeight() - borderTop - borderBottom) / sy;
    } else {
        fFlags += 2; //invalid y factor
    }
//    std::cout<<"0: fx= "<<fx<<"\tfy="<<fy<<std::endl;

    RS_DEBUG->print("f: %f/%f", fx, fy);

    switch (fFlags) {
        case 1:
            fx = fy;
            break;
        case 2:
            fy = fx;
            break;
        case 3:
            return; //do not do anything, invalid factors
        default:
            if (keepAspectRatio) {
                fx = fy = std::min(fx, fy);
            }
//                break;
    }
//    std::cout<<"1: fx= "<<fx<<"\tfy="<<fy<<std::endl;

    RS_DEBUG->print("f: %f/%f", fx, fy);
//exclude invalid factors
    fFlags = 0;
    if (fx < RS_TOLERANCE || fx > RS_MAXDOUBLE) {
        fx = 1.0;
        fFlags += 1;
    }
    if (fy < RS_TOLERANCE || fy > RS_MAXDOUBLE) {
        fy = 1.0;
        fFlags += 2;
    }
    if (fFlags == 3) return;
    saveView();
//        std::cout<<"2: fx= "<<fx<<"\tfy="<<fy<<std::endl;
    setFactorX(fx);
    setFactorY(fy);

    RS_DEBUG->print("f: %f/%f", fx, fy);

    adjustZoomControls();
    centerOffsetX(containerMin, containerSize);
    centerOffsetY(containerMin, containerSize);
    adjustOffsetControls();
    redraw();
}

/**
 * Shows previous view.
 */
void RS_GraphicView::zoomPrevious() {

    RS_DEBUG->print("RS_GraphicView::zoomPrevious");

    if (container) {
        restoreView();
    }
}

/**
 * Saves the current view as previous view to which we can
 * switch back later with @see applyUCS().
 */
void RS_GraphicView::saveView() {
    if (getGraphic()) getGraphic()->setModified(true);
    QDateTime noUpdateWindow = QDateTime::currentDateTime().addMSecs(-500);
//do not update view within 500 milliseconds
    if (*previousViewTime > noUpdateWindow) return;
    *previousViewTime = QDateTime::currentDateTime();
    savedViews[savedViewIndex] = std::make_tuple(offsetX, offsetY, factor);
    savedViewIndex = (savedViewIndex + 1) % savedViews.size();
    if (savedViewCount < savedViews.size()) savedViewCount++;

    if (savedViewCount == 1) {
        emit previous_zoom_state(true);
    }
}


/**
 * Restores the view previously saved with
 * @see saveView().
 */
void RS_GraphicView::restoreView() {
    if (savedViewCount == 0) return;
    savedViewCount--;
    if (savedViewCount == 0) {
        emit previous_zoom_state(false);
    }
    savedViewIndex = (savedViewIndex + savedViews.size() - 1) % savedViews.size();

    offsetX = std::get<0>(savedViews[savedViewIndex]);
    offsetY = std::get<1>(savedViews[savedViewIndex]);
    factor = std::get<2>(savedViews[savedViewIndex]);

    adjustOffsetControls();
    adjustZoomControls();
    invalidate();
    redraw();
}


/*	*
 *	Function name:
 *	Description:		Performs autozoom in Y axis only.
 *	Author(s):			..., Claude Sylvain
 *	Created:				?
 *	Last modified:		23 July 2011
 *
 *	Parameters:			bool axis:
 *								Axis in zoom.
 *
 *	Returns:				void
 *	*/

void RS_GraphicView::zoomAutoY(bool axis) {
    if (container) {
        double visibleHeight = 0.0;
        double minY = RS_MAXDOUBLE;
        double maxY = RS_MINDOUBLE;
        bool noChange = false;

        // fixme - sand -- ?? WHY ?? What if there are entities outside lines? This is not reliable at all
        for (auto e: *container) {
            if (e->rtti() == RS2::EntityLine) {
                auto *l = (RS_Line *) e;
                double x1, x2;
                x1 = toGuiX(l->getStartpoint().x);
                x2 = toGuiX(l->getEndpoint().x);

                if (((x1 > 0.0) && (x1 < (double) getWidth())) ||
                    ((x2 > 0.0) && (x2 < (double) getWidth()))) {
                    minY = std::min(minY, l->getStartpoint().y);
                    minY = std::min(minY, l->getEndpoint().y);
                    maxY = std::max(maxY, l->getStartpoint().y);
                    maxY = std::max(maxY, l->getEndpoint().y);
                }
            }
        }

        if (axis) {
            visibleHeight = std::max(maxY, 0.0) - std::min(minY, 0.0);
        } else {
            visibleHeight = maxY - minY;
        }

        if (visibleHeight < 1.0) {
            noChange = true;
        }

        double fy = 1.0;
        if (visibleHeight > 1.0e-6) {
            fy = (getHeight() - borderTop - borderBottom)
                 / visibleHeight;
            if (factor.y < 0.000001) {
                noChange = true;
            }
        }

        if (noChange == false) {
            setFactorY(fy);
            offsetY = (int) ((getHeight() - borderTop - borderBottom
                              - (visibleHeight * factor.y)) / 2.0
                             - (minY * factor.y)) + borderBottom;
            adjustOffsetControls();
            adjustZoomControls();
            invalidate();

        }
        RS_DEBUG->print("Auto zoom y ok");
    }
}

RS_Vector RS_GraphicView::getUCSViewLeftBottom() const{
    return toUCSFromGui(0,0);
}

RS_Vector RS_GraphicView::getUCSViewRightTop() const{
    return toUCSFromGui(getWidth(),getHeight());
}


/**
 * Zooms the area given by v1 and v2.
 *
 * @param keepAspectRatio true: keeps the aspect ratio 1:1
 *                        false: zooms exactly the selected range to the
 *                               current graphic view
 */
void RS_GraphicView::zoomWindow(
    RS_Vector v1, RS_Vector v2,
    bool keepAspectRatio) {

    // Switch left/right and top/bottom is necessary:
  /*  if (v1.x > v2.x) {
        std::swap(v1.x, v2.x);
    }
    if (v1.y > v2.y) {
        std::swap(v1.y, v2.y);
    }

    LC_ERR << "Zoom: Original Diagonal " << v1.distanceTo(v2);
    RS_Vector worldCorner1 = v1;
    RS_Vector worldCorner3 = v2;

    RS_Vector ucsMin;
    RS_Vector ucsMax;

    ucsBoundingBox(worldCorner1, worldCorner3, ucsMin, ucsMax);

    v1 = ucsMin;
    v2 = ucsMax;

    LC_ERR << "Zoom: UCS Diagonal " << v1.distanceTo(v2);*/

    double zoomX = 480.0;    // Zoom for X-Axis
    double zoomY = 640.0;    // Zoom for Y-Axis   (Set smaller one)
    int zoomBorder = 0; // fixme - what for this variable?

// Switch left/right and top/bottom is necessary:
    if (v1.x > v2.x) {
        std::swap(v1.x, v2.x);
    }
    if (v1.y > v2.y) {
        std::swap(v1.y, v2.y);
    }

// Get zoom in X and zoom in Y:
    int width = getWidth();
    if (v2.x - v1.x > 1.0e-6) {
        zoomX = width / (v2.x - v1.x);
    }
    int height = getHeight();
    if (v2.y - v1.y > 1.0e-6) {
        zoomY = height / (v2.y - v1.y);
    }

// Take smaller zoom:
    if (keepAspectRatio) {
        if (zoomX < zoomY) {
            if (width != 0) {
                zoomX = zoomY = ((double) (width - 2 * zoomBorder)) /
                                (double) width * zoomX;
            }
        } else {
            if (height != 0) {
                zoomX = zoomY = ((double) (height - 2 * zoomBorder)) /
                                (double) height * zoomY;
            }
        }
    }

    zoomX = std::abs(zoomX);
    zoomY = std::abs(zoomY);

// Borders in pixel after zoom
    int pixLeft = (int) (v1.x * zoomX);
    int pixTop = (int) (v2.y * zoomY);
    int pixRight = (int) (v2.x * zoomX);
    int pixBottom = (int) (v1.y * zoomY);
    if (pixLeft == INT_MIN || pixLeft == INT_MAX ||
        pixRight == INT_MIN || pixRight == INT_MAX ||
        pixTop == INT_MIN || pixTop == INT_MAX ||
        pixBottom == INT_MIN || pixBottom == INT_MAX) {
        RS_DIALOGFACTORY->commandMessage("Requested zooming factor out of range. Zooming not changed");
        return;
    }
    saveView();

// Set new offset for zero point:
    offsetX = -pixLeft + (width - pixRight + pixLeft) / 2;
    offsetY = -pixTop + (height - pixBottom + pixTop) / 2;
    factor.x = zoomX;
    factor.y = zoomY;

    adjustOffsetControls();
    adjustZoomControls();
    invalidate();

    redraw();
}

/**
 * Centers the point v1.
 */
void RS_GraphicView::zoomPan(int dx, int dy) {
//offsetX+=(int)toGuiDX(v1.x);
//offsetY+=(int)toGuiDY(v1.y);

    offsetX += dx;
    offsetY -= dy;

    adjustOffsetControls();
//adjustZoomControls();
//    updateGrid();

    redraw();
}

/**
 * Scrolls in the given direction.
 */
void RS_GraphicView::zoomScroll(RS2::Direction direction) {
    switch (direction) {
        case RS2::Up:
            offsetY -= 50;
            break;
        case RS2::Down:
            offsetY += 50;
            break;
        case RS2::Right:
            offsetX += 50;
            break;
        case RS2::Left:
            offsetX -= 50;
            break;
    }
    adjustOffsetControls();
    adjustZoomControls();
//    updateGrid();

    redraw();
}


/**
 * Zooms to page extends.
 */
void RS_GraphicView::zoomPage() {

    RS_DEBUG->print("RS_GraphicView::zoomPage");
    if (!container) {
        return;
    }

    RS_Graphic *graphic = container->getGraphic();
    if (!graphic) {
        return;
    }

    RS_Vector s = graphic->getPrintAreaSize() / graphic->getPaperScale();

    double fx, fy;

    if (s.x > RS_TOLERANCE) {
        fx = (getWidth() - borderLeft - borderRight) / s.x;
    } else {
        fx = 1.0;
    }

    if (s.y > RS_TOLERANCE) {
        fy = (getHeight() - borderTop - borderBottom) / s.y;
    } else {
        fy = 1.0;
    }

    RS_DEBUG->print("f: %f/%f", fx, fy);

    fx = fy = std::min(fx, fy);

    RS_DEBUG->print("f: %f/%f", fx, fy);

    if (fx < RS_TOLERANCE) {
        fx = fy = 1.0;
    }

    setFactorX(fx);
    setFactorY(fy);

    RS_DEBUG->print("f: %f/%f", fx, fy);

    RS_Vector containerMin = container->getMin();
    RS_Vector containerSize = container->getSize();

    centerOffsetX(containerMin, containerSize);
    centerOffsetY(containerMin, containerSize);
    // fixme - remove debug code
//    LC_ERR << "Normal Zoom " << offsetX << " , " << offsetY << " Factor: " << fx;;
    adjustOffsetControls();
    adjustZoomControls();
//    updateGrid();

    redraw();
}

void RS_GraphicView::zoomPageEx() {

    RS_DEBUG->print("RS_GraphicView::zoomPage");
    if (!container) {
        return;
    }

    RS_Graphic *graphic = container->getGraphic();
    if (!graphic) {
        return;
    }

    RS2::Unit dest = graphic->getUnit();
    double marginsWidth = RS_Units::convert(graphic->getMarginLeft() + graphic->getMarginRight(), RS2::Millimeter, dest);
    double marginsHeight = RS_Units::convert(graphic->getMarginTop() +graphic->getMarginBottom(), RS2::Millimeter, dest);

    const RS_Vector &printAreaSize = graphic->getPrintAreaSize(true);
    double paperScale = graphic->getPaperScale();
    RS_Vector printAreaSizeInViewCoordinates = (printAreaSize + RS_Vector(marginsWidth, marginsHeight, 0)) / paperScale;

    double fx, fy;

    int widthToFit = getWidth() - borderLeft - borderRight;

    if (printAreaSizeInViewCoordinates.x > RS_TOLERANCE) {
        fx = widthToFit / printAreaSizeInViewCoordinates.x;
    } else {
        fx = 1.0;
    }

    int heightToFit = getHeight() - borderTop - borderBottom;
    if (printAreaSizeInViewCoordinates.y > RS_TOLERANCE) {
        fy = heightToFit / printAreaSizeInViewCoordinates.y;
    } else {
        fy = 1.0;
    }

    RS_DEBUG->print("f: %f/%f", fx, fy);

    fx = fy = std::min(fx, fy);

    RS_DEBUG->print("f: %f/%f", fx, fy);

    if (fx < RS_TOLERANCE) {
        fx = fy = 1.0;
    }

    setFactorX(fx);
    setFactorY(fy);

    RS_DEBUG->print("f: %f/%f", fx, fy);

    const RS_Vector &paperInsertionBase = graphic->getPaperInsertionBase();



    offsetX = (int) ((getWidth() - borderLeft - borderRight - (printAreaSizeInViewCoordinates.x) * factor.x) / 2.0 +
                     (paperInsertionBase.x * factor.x / paperScale)) + borderLeft;

    fy = factor.y;

    offsetY =
        (int) ((getHeight() - borderTop - borderBottom - (printAreaSizeInViewCoordinates.y) * fy) / 2.0 + paperInsertionBase.y * fy / paperScale) + borderBottom;

    invalidate();
    redraw();
}


/**
 * Draws the entities within the given range.
 */
void RS_GraphicView::drawWindow_DEPRECATED(RS_Vector v1, RS_Vector v2) {
    RS_DEBUG->print("RS_GraphicView::drawWindow() begin");
    if (container) {
        for (auto se: *container) {
            if (se->isInWindow(v1, v2)) {
                drawEntity(nullptr, se);
            }
        }
    }
    RS_DEBUG->print("RS_GraphicView::drawWindow() end");
}

/**
 * Draws the entities.
 * This function can only be called from within the paint event
 *
 */
void RS_GraphicView::drawLayer1(RS_Painter *painter) {
#ifdef DEBUG_RENDERING_DETAILS
    drawLayer1Timer.start();
#endif
    // drawing paper border:
    if (isPrintPreview()) {
        drawPaper(painter);
    } else {
        //increase grid point size on for DPI>96
        auto dpiX = int(qApp->screens().front()->logicalDotsPerInch());
        const bool isHiDpi = dpiX > 96;
//        DEBUG_HEADER
//        RS_DEBUG->print(RS_Debug::D_ERROR, "dpiX=%d\n",dpiX);
        const RS_Pen penSaved = painter->getPen();

        // fixme - sand - review as overall rendering pipeline. It might be that
        // it will be better to use more fine-grained painting and draw grid in own
        // pixmap, and have draft sign somewhere else?
        if (grid && !hasNoGrid) {
            if (isGridOn()) {
                grid->calculateGrid();
                grid->drawGrid(painter);

            }
            else{
                grid->calculateSnapSettings();
            }
            QString info = grid->getInfo();
            updateGridStatusWidget(info);
        }

        if (isHiDpi) {
            RS_Pen pen = penSaved;
            pen.setWidth(RS2::Width01);
            painter->setPen(pen);
        }

        if (isDraftMode())
            drawDraftSign(painter);

        if (isHiDpi)
            painter->setPen(penSaved);
    }
#ifdef DEBUG_RENDERING_DETAILS
    layer1Time += drawLayer1Timer.elapsed();
#endif
}

/*	*
 *	Function name:
 *	Description: 		Do the drawing, step 2/3.
 *	Author(s):			..., Claude Sylvain
 *	Created:				?
 *	Last modified:		23 July 2011
 *
 *	Parameters:			RS_Painter *painter:
 *								...
 *
 *	Returns:				void
 *	*/

void RS_GraphicView::drawLayer2(RS_Painter *painter) {
#ifdef DEBUG_RENDERING_DETAILS
    drawLayer2Timer.start();
#endif
    screenPDSize = determinePointScreenSize(painter, pdsize);
    double patternOffset = 0.;
    lastPaintEntityPen = RS_Pen();
    lastPaintEntityPen.setFlags(RS2::FlagInvalid);
    lastPaintedHighlighted = false;
    lastPaintedSelected = false;
    lastPaintOverlay = false;
    container->draw(painter, this, patternOffset);
#ifdef DEBUG_RENDERING_DETAILS
    layer2Time += drawLayer2Timer.elapsed();
#endif
}

void RS_GraphicView::drawLayer3(RS_Painter *painter) {
#ifdef DEBUG_RENDERING_DETAILS
    drawLayer3Timer.start();
#endif
   // drawing zero points:
    if (!isPrintPreview()) {
        drawRelativeZero(painter);
        lastPaintEntityPen = RS_Pen();
        drawOverlay(painter);
    }
#ifdef DEBUG_RENDERING_DETAILS
    layer3Time += drawLayer3Timer.elapsed();
#endif
}

void RS_GraphicView::setPenForOverlayEntity(RS_Painter *painter, RS_Entity *e, double &patternOffset) {
    // todo - potentially, for overlays (preview etc) we may have simpler processing for pens rather than for normal drawing,
    // todo - therefore, review this later
    int rtti = e->rtti();
    switch (rtti) {
        case RS2::EntityRefEllipse:
        case RS2::EntityRefPoint:
        case RS2::EntityRefLine:
        case RS2::EntityRefConstructionLine:
        case RS2::EntityRefCircle:
        case RS2::EntityRefArc: {
            // todo - if not ref point are enabled, draw as transparent? Actually, if actions are correct, we should not be there..
            RS_Pen pen = e->getPen(true);
            if (e->isHighlighted()) {
                pen.setColor(m_colorData->previewReferenceHighlightedEntitiesColor);
            } else {
                pen.setColor(m_colorData->previewReferenceEntitiesColor);
            }
            pen.setLineType(RS2::SolidLine);
            pen.setWidth(RS2::LineWidth::Width00);
            e->setPen(pen);

//            pen.setScreenWidth(0.0);
            painter->setPen(pen);
            break;
        }
        default: {
            if (draftMode){
                if (m_ignoreDraftForHighlight) {
                    setPenForEntity(painter, e, patternOffset, true);
                }
                else{
                    setPenForDraftEntity(painter, e, patternOffset, true);
                }
            }
            else{
                if (m_draftLinesMode){
                    setPenForDraftEntity(painter, e, patternOffset, true);
                }
                else {
                    setPenForEntity(painter, e, patternOffset, true);
                }
            }
        }
    }
}


/*	*
 *	Function name:
 *
 *	Description:	- Sets the pen of the painter object to the suitable pen
 *						  for the given entity.
 *
 *	Author(s):		..., Claude Sylvain
 *	Created:			?
 *	Last modified:	17 November 2011
 *
 *	Parameters:		RS_Painter *painter:
 *							...
 *
 *						RS_Entity *e:
 *							...
 *
 *	Returns:			void
 */
void RS_GraphicView::setPenForPrintingEntity(RS_Painter *painter, RS_Entity *e, double &patternOffset) {
#ifdef DEBUG_RENDERING
    setPenTimer.start();
#endif
// Getting pen from entity (or layer)

    RS_Pen pen = e->getPenResolved();
    RS_Pen originalPen = pen;
    bool highlighted = e->getFlag(RS2::FlagHighlighted);
    bool selected = e->getFlag(RS2::FlagSelected);
    if (lastPaintedHighlighted == highlighted && lastPaintedSelected == selected) {
        if (lastPaintEntityPen.isSameAs(pen, patternOffset)) {
            return;
        }
    }
    else{
        lastPaintedHighlighted = highlighted;
        lastPaintedSelected = selected;
    }
    // Avoid negative widths
    double width = pen.getWidth();
//    int w = std::max(static_cast<int>(pen.getWidth()), 0);

// - Scale pen width.
// - By default pen width is not scaled on print and print preview.
//   This is the standard (AutoCAD like) behaviour.
// bug# 3437941
// ------------------------------------------------------------

    if (pen.getAlpha() == 1.0) {
        if (width >0) {
            double wf = 1.0; // Width factor.

            if (paperScale > RS_TOLERANCE) {
                if (scaleLineWidth) {
                    wf = defaultWidthFactor;
                } else {
                    wf = 1.0 / paperScale;
                }
            }
            double screenWidth = toGuiDX(width * unitFactor100 * wf);

            /*// prevent drawing with 1-width which is slow:
            if (RS_Math::round(pen.getScreenWidth()) == 1) {
                pen.setScreenWidth(0.0);
            }*/
            if (screenWidth < 1){ // fixme - not sure about this check. However, without it, lines will stay transparent and then disappear on zooming out. Probably some other threshold value (instead 1) should be used?
                screenWidth = 0.0;
            }
            pen.setScreenWidth(screenWidth);
        }
        else{
            pen.setScreenWidth(0.0);
        }
    }
    else{
        // fixme - if we'll support transparency, add necessary processing there
        if (RS_Math::round(pen.getScreenWidth()) == 1) {
            pen.setScreenWidth(0.0);
        }
    }

    RS_Color backgroundColor;
    if (printPreview /*|| inPrintingMode*/){ //  todo - should we change color for printer mode too?
        backgroundColor = printPreviewPrintAreaColor; // same color as used for drawing print area in drawPaper
        // fixme - sand - nicer handling colors is needed. That should work fine if entity is over paper entity.
        // however, if entity is over background... it still may be the issue.
    }
    else{
        backgroundColor =  m_colorData->background;
    }

    if (pen.getColor().isEqualIgnoringFlags(backgroundColor) || (pen.getColor().toIntColor() == RS_Color::Black
                                                                         && pen.getColor().colorDistance(backgroundColor) < RS_Color::MinColorDistance)) {
        pen.setColor(this->m_colorData->foreground);
    }

    if (pen.getLineType() != RS2::SolidLine){
        pen.setDashOffset(patternOffset * defaultWidthFactor);
    }

// deleting not drawing:
    if (deleteMode || e->getFlag(RS2::FlagTransparent) ) {
        pen.setColor(backgroundColor);
    }
// LC_ERR << "PEN " << pen.getColor().name() << "Width: " << pen.getWidth() <<  " | " << pen.getScreenWidth() << " LT " << pen.getLineType();
    lastPaintEntityPen.updateBy(originalPen);
    painter->setPen(pen);
#ifdef DEBUG_RENDERING
    setPenTime += setPenTimer.nsecsElapsed();
#endif
}

void RS_GraphicView::setPenForDraftEntity(RS_Painter *painter, RS_Entity *e, double &patternOffset, bool inOverlay) {
#ifdef DEBUG_RENDERING
    setPenTimer.start();
#endif
    RS_Pen pen = e->getPenResolved();
    RS_Pen originalPen = pen;
    bool highlighted = e->getFlag(RS2::FlagHighlighted);
    bool selected = e->getFlag(RS2::FlagSelected);
    bool overlayPaint = inOverlay || inOverlayDrawing;
// try to avoid pen setup if the pen and entity flags are the same as for previous entity. This is important for performance reasons, so we'll reuse
    // painter pen set previously. This check assumed that that all previous entity drawing were performed via this function and no
    // arbitrary QPainter::setPen was called between drawing entities.
    if (lastPaintedHighlighted == highlighted && lastPaintedSelected == selected && lastPaintOverlay == overlayPaint) {
        if (lastPaintEntityPen.isSameAs(pen, patternOffset)) {
            return;
        }
    }
    else{
        lastPaintedHighlighted = highlighted;
        lastPaintedSelected = selected;
        lastPaintOverlay = overlayPaint;
    }
    pen.setScreenWidth(0.0);

    if (overlayPaint) {
        if (highlighted) {    // Glowing effects on mouse hovering: use the "selected" color
            // for glowing effects on mouse hovering, draw solid lines
            pen.setColor(m_colorData->selectedColor);
            pen.setLineType(RS2::SolidLine);
        }
        else{
            if (pen.getColor().isEqualIgnoringFlags(m_colorData->background) || (pen.getColor().toIntColor() == RS_Color::Black
                                                                                 && pen.getColor().colorDistance(m_colorData->background) < RS_Color::MinColorDistance)) {
                pen.setColor(this->m_colorData->foreground);
            }
        }
    } else {
        // this entity is selected:
        if (selected) {
            pen.setLineType(RS2::DashLineTiny);
            pen.setWidth(RS2::Width00);
            pen.setColor(m_colorData->selectedColor);
        }
        else if (highlighted) {
            pen.setColor(m_colorData->highlightedColor);
        }
        else if (deleteMode || e->getFlag(RS2::FlagTransparent)) {
            pen.setColor(m_colorData->background);
        }
        else if (pen.getColor().isEqualIgnoringFlags(m_colorData->background) || (pen.getColor().toIntColor() == RS_Color::Black
                                                                                  && pen.getColor().colorDistance(m_colorData->background) < RS_Color::MinColorDistance)) {
            pen.setColor(this->m_colorData->foreground);
        }
    }

    if (pen.getLineType() != RS2::SolidLine){
        pen.setDashOffset(patternOffset * defaultWidthFactor);
    }

// LC_ERR << "PEN " << pen.getColor().name() << "Width: " << pen.getWidth() <<  " | " << pen.getScreenWidth() << " LT " << pen.getLineType();
    lastPaintEntityPen.updateBy(originalPen);
    painter->setPen(pen);
#ifdef DEBUG_RENDERING
    setPenTime += setPenTimer.nsecsElapsed();
#endif
}


void RS_GraphicView::setPenForEntity(RS_Painter *painter, RS_Entity *e, double &patternOffset, bool inOverlay) {
#ifdef DEBUG_RENDERING
    getPenTimer.start();
#endif
    // Getting pen from entity (or layer)
    RS_Pen pen = e->getPenResolved();
#ifdef DEBUG_RENDERING
    getPenTime += getPenTimer.nsecsElapsed();
#endif
    RS_Pen originalPen = pen;
    bool highlighted = e->getFlag(RS2::FlagHighlighted);
    bool selected = e->getFlag(RS2::FlagSelected);
    bool overlayPaint = inOverlay || inOverlayDrawing;
    // try to avoid pen setup if the pen and entity flags are the same as for previous entity. This is important for performance reasons, so we'll reuse
    // painter pen set previously. This check assumed that that all previous entity drawing were performed via this function and no
    // arbitrary QPainter::setPen was called between drawing entities.
    if (lastPaintedHighlighted == highlighted && lastPaintedSelected == selected && lastPaintOverlay == overlayPaint) {
        if (lastPaintEntityPen.isSameAs(pen, patternOffset)) {
            return;
        }
    }
    else{
        lastPaintedHighlighted = highlighted;
        lastPaintedSelected = selected;
        lastPaintOverlay = overlayPaint;
    }


#ifdef DEBUG_RENDERING
    setPenTimer.start();
#endif
    // Avoid negative widths
//    int w = std::max(static_cast<int>(pen.getWidth()), 0);
    double width = pen.getWidth();
    if (pen.getAlpha() == 1.0) {
        if (width>0) {
            double screenWidth = toGuiDX(width * unitFactor100);
            // prevent drawing with 1-width which is slow:
           /* if (RS_Math::round(screenWidth) == 1) {
                screenWidth = 0.0;
            }
            else*/ if (screenWidth < 1){ // fixme - not sure about this check. However, without it, lines will stay transparent and then disappear on zooming out. Probably some other threshold value (instead 1) should be used?
                screenWidth = 0.0;
            }
            pen.setScreenWidth(screenWidth);
        }
        else{
            pen.setScreenWidth(0.0);
        }
    }
    else{
            // fixme - if we'll support transparency, add necessary processing there
        if (RS_Math::round(pen.getScreenWidth()) == 1) {
            pen.setScreenWidth(0.0);
        }
    }


    if (overlayPaint) {
        if (highlighted) {    // Glowing effects on mouse hovering: use the "selected" color
            // for glowing effects on mouse hovering, draw solid lines
            pen.setColor(m_colorData->selectedColor);
            pen.setLineType(RS2::SolidLine);
        }
        else{
            if (pen.getColor().isEqualIgnoringFlags(m_colorData->background) || (pen.getColor().toIntColor() == RS_Color::Black
                                                                                 && pen.getColor().colorDistance(m_colorData->background) < RS_Color::MinColorDistance)) {
                pen.setColor(this->m_colorData->foreground);
            }
        }
    } else {
        // this entity is selected:
        if (selected) {
            pen.setLineType(RS2::DashLineTiny);
            pen.setWidth(RS2::Width00); // fixme - move to settings?
            pen.setColor(m_colorData->selectedColor);
        }
        // this entity is highlighted:
        else if (highlighted) {
            pen.setColor(m_colorData->highlightedColor);
        }
        else  if (deleteMode || e->getFlag(RS2::FlagTransparent)) {
            pen.setColor(m_colorData->background);
        }
        else if (pen.getColor().isEqualIgnoringFlags(m_colorData->background) || (pen.getColor().toIntColor() == RS_Color::Black
                                                                                   && pen.getColor().colorDistance(m_colorData->background) < RS_Color::MinColorDistance)) {
                    pen.setColor(this->m_colorData->foreground);
        }
    }

    if (pen.getLineType() != RS2::SolidLine){
        pen.setDashOffset(patternOffset * defaultWidthFactor);
    }

    // deleting not drawing:

// LC_ERR << "PEN " << pen.getColor().name() << "Width: " << pen.getWidth() <<  " | " << pen.getScreenWidth() << " LT " << pen.getLineType();
#ifdef DEBUG_RENDERING
    setPenTime += setPenTimer.nsecsElapsed();
    painterSetPenTimer.start();
#endif
    lastPaintEntityPen.updateBy(originalPen);
    painter->setPen(pen);
#ifdef DEBUG_RENDERING
    painterSetPenTime +=painterSetPenTimer.nsecsElapsed();

#endif
}

/**
 * Draws an entity. Might be recursively called e.g. for polylines.
 * If the class wide painter is nullptr a new painter will be created
 * and destroyed afterwards.
 *
 * @param patternOffset Offset of line pattern (used for connected
 *        lines e.g. in splines).
 * @param db Double buffering on (recommended) / off
 */
void RS_GraphicView::drawEntity(RS_Entity * /*e*/, double & /*patternOffset*/) {
    RS_DEBUG->print("RS_GraphicView::drawEntity(RS_Entity*,patternOffset) not supported anymore");
// RVT_PORT this needs to be optimized
// One way to do is to send a RS2::RedrawSelected, then the draw routine will only draw all selected entities
// Dis-advantage is that we still need to iterate over all entities, but
// this might be very fast
// For now we just redraw the drawing until we are going to optimize drawing
    redraw(RS2::RedrawDrawing);
}

void RS_GraphicView::drawEntity(RS_Entity * /*e*/ /*patternOffset*/) {
    RS_DEBUG->print("RS_GraphicView::drawEntity(RS_Entity*,patternOffset) not supported anymore");
// RVT_PORT this needs to be optimized
// One way to do is to send a RS2::RedrawSelected, then the draw routine will only draw all selected entities
// Dis-advantage is that we still need to iterate over all entities, but
// this might be very fast
// For now we just redraw the drawing until we are going to optimize drawing
    redraw(RS2::RedrawDrawing);
}

void RS_GraphicView::drawEntity(RS_Painter *painter, RS_Entity *e) {
    double offset(0.);
    drawEntity(painter, e, offset);
}

void RS_GraphicView::drawEntity(RS_Painter *painter, RS_Entity *e, double &patternOffset) {
    // update is disabled:
    // given entity is nullptr:
    if (!e) {
        return;
    }

    // check for selected entity drawing
    if (/*!e->isContainer() && */(e->getFlag(RS2::FlagSelected) != painter->shouldDrawSelected())) {
        return;
    }
#ifdef DEBUG_RENDERING
    isVisibleTimer.start();
#endif
    // entity is not visible:
    bool visible = e->isVisible();
#ifdef DEBUG_RENDERING
    isVisibleTime += isVisibleTimer.nsecsElapsed();
#endif
    if (!visible) {
        return;
    }

#ifdef DEBUG_RENDERING
    isConstructionTimer.start();
#endif
    bool constructionEntity = e->isConstruction();
#ifdef DEBUG_RENDERING
    isConstructionTime += isConstructionTimer.nsecsElapsed();
#endif
    if (isPrinting()){
        // do not draw construction layer on print preview or print
        if (!e->isPrint() || constructionEntity)
            return;

        setPenForPrintingEntity(painter, e, patternOffset);
        drawEntityPlain(painter, e, patternOffset);
    }
    else {
        if (isPrintPreview()) {
            if (!e->isPrint() || constructionEntity)
                return;
        }

        // test if the entity is in the viewport
        switch (e->rtti()){
           /* case RS2::EntityGraphic:
                break;*/
            case RS2::EntityLine:{
                if (constructionEntity){
                    if (!LC_LineMath::hasIntersectionLineRect(e->getMin(), e->getMax(), view_rect.minP(), view_rect.maxP())){
                        return;
                    }
                }
                else{ // normal line
                    if (e->getMax().x < view_rect.minP().x || e->getMin().x > view_rect.maxP().x ||
                        e->getMin().y > view_rect.maxP().y || e->getMax().y < view_rect.minP().y){
                        return;
                    }
                }
                break;
            }
            default:
                if (e->getMax().x < view_rect.minP().x || e->getMin().x > view_rect.maxP().x ||
                    e->getMin().y > view_rect.maxP().y || e->getMax().y < view_rect.minP().y){
                    return;
                }
        }

        if (printPreview){
            // set pen (color):
            setPenForPrintingEntity(painter, e, patternOffset);
            drawEntityPlain(painter, e, patternOffset);
        }
        else {
            RS2::EntityType entityType = e->rtti();
            if (isDraftMode()) {
                switch (entityType) {
                    case RS2::EntityMText:
                    case RS2::EntityText:
                    case RS2::EntityImage:
                        // set pen (color):
                        setPenForDraftEntity(painter, e, patternOffset, false);
                        e->drawDraft(painter, this, patternOffset);
                        break;
                    case RS2::EntityHatch:
                        //skip hatches
                        break;
                    default:
                        setPenForDraftEntity(painter, e, patternOffset, false);
                        drawEntityPlain(painter, e, patternOffset);
                }
            } else {
                // the code below is ugly as code for normal painting is duplicated.
                // however, it's intentional and made for perfromance reasons - to avoid additional checks or method calls during painting
                if (isPanning()){
                    switch (entityType){
                        case RS2::EntityMText:
                        case RS2::EntityText:{
                            if (drawTextsAsDraftForPanning){
                                setPenForDraftEntity(painter, e, patternOffset, false);
                                e->drawDraft(painter, this, patternOffset);
                            }
                            else{
                                // normal painting
                                if (m_draftLinesMode) {
                                    setPenForDraftEntity(painter, e, patternOffset, false);
                                } else {
                                    setPenForEntity(painter, e, patternOffset, false);
                                }
                                drawEntityPlain(painter, e, patternOffset);
                            }
                            break;
                        }
                        default:
                            // normal painting
                            // set pen (color):
                            if (m_draftLinesMode) {
                                setPenForDraftEntity(painter, e, patternOffset, false);
                            } else {
                                setPenForEntity(painter, e, patternOffset, false);
                            }
                            drawEntityPlain(painter, e, patternOffset);
                            break;
                    }
                }
                else {
                    // normal painting
                    // set pen (color):
                    if (m_draftLinesMode) {
                        setPenForDraftEntity(painter, e, patternOffset, false);
                    } else {
                        setPenForEntity(painter, e, patternOffset, false);
                    }
                    drawEntityPlain(painter, e, patternOffset);
                }
            }

            // draw reference points:
            if (e->getFlag(RS2::FlagSelected) && !isPrintPreview()) {
                if (!e->isParentSelected()) {
                    drawEntityReferencePoints(painter, e);
                }
            }
        }
    }
//RS_DEBUG->print("RS_GraphicView::drawEntity() end");
}

void RS_GraphicView::drawAsChild(RS_Painter *painter, RS_Entity *e, double &patternOffset) {
    e->drawAsChild(painter, this, patternOffset);
}

void RS_GraphicView::drawEntityReferencePoints(RS_Painter *painter, const RS_Entity *e) const {
    RS_VectorSolutions const &s = e->getRefPoints();
    int sz = m_entityHandleHalfSize;
    for (size_t i = 0; i < s.getNumber(); ++i) {
        RS_Color col = m_colorData->handleColor;
        if (i == 0) {
            col = m_colorData->startHandleColor;
        } else if (i == s.getNumber() - 1) {
            col = m_colorData->endHandleColor;
        }
        if (getDeleteMode()) {
            painter->drawHandle(toGui(s.get(i)), m_colorData->background, sz);
        } else {
            painter->drawHandle(toGui(s.get(i)), col, sz);
        }
    }
}


/**
 * Draws an entity.
 * The painter must be initialized and all the attributes (pen) must be set.
 */
void RS_GraphicView::drawEntityPlain(RS_Painter *painter, RS_Entity *e, double &patternOffset) {
#ifdef DEBUG_RENDERING
    drawEntityCount++;
    drawTimer.start();
#endif
    e->draw(painter, this, patternOffset);
#ifdef DEBUG_RENDERING
    qint64 elapsed = drawTimer.nsecsElapsed();
    entityDrawTime+= elapsed;
#endif

}

void RS_GraphicView::drawEntityPlain(RS_Painter *painter, RS_Entity *e) {
    if (!e) {
        return;
    }

    if (!e->isContainer() && (e->isSelected() != painter->shouldDrawSelected())) {
        return;
    }
    double patternOffset(0.);
    e->draw(painter, this, patternOffset);
}

void RS_GraphicView::drawEntityHighlighted(RS_Entity *e, bool highlighted) {
    if (e == nullptr)
        return;
    if (e->isHighlighted() != highlighted) {
        e->setHighlighted(highlighted);
        drawEntity(e);
    }
}

/**
 * Deletes an entity with the background color.
 * Might be recursively called e.g. for polylines.
 */
void RS_GraphicView::deleteEntity(RS_Entity *e) {

// RVT_PORT When we delete a single entity, we can do this but we need to remove this then also from containerEntities
    RS_DEBUG->print("RS_GraphicView::deleteEntity will for now redraw the whole screen instead of just deleting the entity");
    setDeleteMode(true);
    drawEntity(e);
    setDeleteMode(false);
    redraw(RS2::RedrawDrawing); // fixme - sand - review redraw
}

/**
 * @return Pointer to the static pattern struct that belongs to the
 * given pattern type or nullptr.
 */
const RS_LineTypePattern *RS_GraphicView::getPattern(RS2::LineType t) {
    return RS_LineTypePattern::getPattern(t);
}

void RS_GraphicView::drawCoordinateSystemMarker(RS_Painter *painter, const RS_Vector &uiOrigin, double xAxisAngle, bool forWCS){
    int const zr = m_csZeroMarkerSize;
    RS_Vector uiXAxisEnd = uiOrigin.relative(zr, xAxisAngle);

    double yAxisAngle = xAxisAngle-M_PI_2; // as we're in UI coordinate space
    RS_Vector uiYAxisEnd = uiOrigin.relative(zr,yAxisAngle);

    RS_Pen pen_xAxis (m_colorData->xAxisExtensionColor, RS2::Width00, RS2::SolidLine);
    pen_xAxis.setScreenWidth(2);

    RS_Pen pen_yAxis (m_colorData->yAxisExtensionColor, RS2::Width00, RS2::SolidLine);
    pen_yAxis.setScreenWidth(2);

    double anchorFactor = 0.2;
    double anchorAngle = RS_Math::deg2rad(70);
    double anchorSize = zr * anchorFactor;

    double resultingAchorAngle = M_PI_2 + anchorAngle;

    painter->setFont(m_csZeroMarkerFont);


    painter->setPen(pen_xAxis);
    // axis
    painter->drawLine(uiOrigin, uiXAxisEnd);
    // anchor
    painter->drawLine(uiXAxisEnd, uiXAxisEnd.relative(anchorSize, xAxisAngle+resultingAchorAngle));
    painter->drawLine(uiXAxisEnd, uiXAxisEnd.relative(anchorSize, xAxisAngle-resultingAchorAngle));


    const char* xString = forWCS ? "wX" : "X";
    const QSize &xSize = QFontMetrics(painter->font()).size(Qt::TextSingleLine, xString);

    RS_Vector offset = RS_Vector(20,10);

    RS_Vector xTextPosition = uiXAxisEnd.relative(offset.x, yAxisAngle);

    QRect xRect = QRect(QPoint(xTextPosition.x, xTextPosition.y), xSize);
    QRect xBoundingRect;
    painter->drawText(xRect,  Qt::AlignTop | Qt::AlignRight | Qt::TextDontClip, xString, &xBoundingRect);

    painter->setPen(pen_yAxis);
    // axis
    painter->drawLine(uiOrigin, uiYAxisEnd);
    // anchor
    painter->drawLine(uiYAxisEnd, uiYAxisEnd.relative(anchorSize, yAxisAngle+resultingAchorAngle));
    painter->drawLine(uiYAxisEnd, uiYAxisEnd.relative(anchorSize, yAxisAngle-resultingAchorAngle));

    const char* yString = forWCS ? "wY" : "Y";
    const QSize &ySize = QFontMetrics(painter->font()).size(Qt::TextSingleLine, yString);

    RS_Vector yTextPosition = uiYAxisEnd.relative(offset.x, xAxisAngle);

    QRect yRect = QRect(QPoint(yTextPosition.x, yTextPosition.y), ySize);
    QRect yBoundingRect;
    painter->drawText(yRect,  Qt::AlignTop | Qt::AlignRight | Qt::TextDontClip, yString, &yBoundingRect);

    // square
    double angleFactor = 0.1;
    double angleLen = zr*angleFactor;

    RS_Vector angleX0 = uiOrigin.relative(angleLen, xAxisAngle);
    RS_Vector angleX1 = angleX0.relative(angleLen, yAxisAngle);
    RS_Vector anchorY0 = uiOrigin.relative(angleLen, yAxisAngle);

    RS_Pen anglePen (m_colorData->previewReferenceEntitiesColor, RS2::Width00, RS2::SolidLine);
    anglePen.setScreenWidth(2);

    painter->setPen(anglePen);

    painter->drawLine(anchorY0, angleX1);
    painter->drawLine(angleX0, angleX1);

    painter->drawGridPoint(uiOrigin);

}


/**
 * This virtual method can be overwritten to draw the absolute
 * zero. It's called from within drawIt(). The default implementation
 * draws a simple red cross on the zero of the sheet
 * This function can ONLY be called from within a paintEvent because it will
 * use the painter
 *
 * @see drawIt()
 */
void RS_GraphicView::drawAbsoluteZero(RS_Painter *painter){
    int const zr = m_zeroShortAxisMarkSize;

    RS_Pen pen_xAxis (m_colorData->xAxisExtensionColor, RS2::Width00, RS2::SolidLine);
    pen_xAxis.setScreenWidth(0);

    RS_Pen pen_yAxis (m_colorData->yAxisExtensionColor, RS2::Width00, RS2::SolidLine);
    pen_yAxis.setScreenWidth(0);

    auto originPoint = toGuiFromUCS(0.0, 0.0);


    int width = getWidth();
    int height = getHeight();
    if (m_extendAxisLines){

        int xAxisStartPoint;
        int xAxisEndPoint;

        switch (m_extendAxisModeX){
            case Both:
                xAxisStartPoint = 0;
                xAxisEndPoint = width;
                break;
            case Positive:
                xAxisStartPoint = originPoint.x;
                if (originPoint.x < width){
                    xAxisEndPoint = width;
                }
                else{
                    xAxisEndPoint = 0;
                }
                break;
            case Negative:
                xAxisStartPoint  = originPoint.x;
                if (originPoint.x < width){
                    xAxisEndPoint = 0;
                }
                else{
                    xAxisEndPoint = width;
                }
                break;
            case None:{ // draw short
                xAxisStartPoint  = originPoint.x - zr;
                xAxisEndPoint = originPoint.x + zr;
                break;
            }
            default:
                xAxisStartPoint = 0;
                xAxisEndPoint = 0;
                break;
        }

        painter->setPen(pen_xAxis);
        painter->drawLine(RS_Vector(xAxisStartPoint, originPoint.y), RS_Vector(xAxisEndPoint, originPoint.y));

        int yAxisStartPoint;
        int yAxisEndPoint;
        switch (m_extendAxisModeY){
            case Both:
                yAxisStartPoint  = 0;
                yAxisEndPoint  = height;
                break;
            case Positive:
                yAxisStartPoint = originPoint.y;
                if (originPoint.y < height){
                    yAxisEndPoint = 0;
                }
                else{

                    yAxisEndPoint = height;
                }
                break;
            case Negative:
                yAxisStartPoint  = originPoint.y;
                if (originPoint.y < height){
                    yAxisEndPoint = height;
                }
                else{
                    yAxisEndPoint = 0;
                }
                break;
            case None:
                yAxisStartPoint  = originPoint.y - zr;
                yAxisEndPoint = originPoint.y + zr;
                break;
            default:
                yAxisStartPoint = 0;
                yAxisEndPoint = 0;
                break;
        }

        painter->setPen(pen_yAxis);
        painter->drawLine(RS_Vector(originPoint.x, yAxisStartPoint), RS_Vector(originPoint.x, yAxisEndPoint));
    }
    else
    {
        double xAxisPoints [2];
        double yAxisPoints [2];

        if (((originPoint.x + zr) < 0) || ((originPoint.x - zr) > width)) return;
        if (((originPoint.y + zr) < 0) || ((originPoint.y - zr) > height)) return;
        xAxisPoints [0] = originPoint.x - zr;
        xAxisPoints [1] = originPoint.x + zr;

        yAxisPoints [0] = originPoint.y - zr;
        yAxisPoints [1] = originPoint.y + zr;

        painter->setPen(pen_xAxis);
        painter->drawLine(RS_Vector(xAxisPoints[0], originPoint.y), RS_Vector(xAxisPoints[1], originPoint.y));

        painter->setPen(pen_yAxis);
        painter->drawLine(RS_Vector(originPoint.x, yAxisPoints[0]), RS_Vector(originPoint.x, yAxisPoints[1]));
    }

    if (m_showUCSZeroMarker){
        drawCoordinateSystemMarker(painter, toGuiFromUCS(0,0), 0, false);
    }

    if (m_hasUcs && m_showWCSZeroMarker){
        auto uiOrigin = toGui(RS_Vector(0,0));
        drawCoordinateSystemMarker(painter, uiOrigin, -ucs.getXAxisAngle(), true);
    }
}

/**
 * This virtual method can be overwritten to draw the relative
 * zero point. It's called from within drawIt(). The default implementation
 * draws a simple red round zero point. This is the point that was last created by the user, end of a line for example
 * This function can ONLY be called from within a paintEvent because it will
 * use the painter
 *
 * @see drawIt()
 */
void RS_GraphicView::drawRelativeZero(RS_Painter *painter) {

    if (!relativeZero.valid || m_colorData->hideRelativeZero) {
        return;
    }

    RS2::LineType relativeZeroPenType = RS2::SolidLine;

//    if (m_colorData->hideRelativeZero) relativeZeroPenType = RS2::NoPen;

    RS_Pen p(m_colorData->relativeZeroColor, RS2::Width00, relativeZeroPenType);
    p.setScreenWidth(0);
    painter->setPen(p);

    int const zr = m_relativeZeroRadius * 2;
    auto vp = toGui(relativeZero);
    if (vp.x + zr < 0 || vp.x - zr > getWidth()) return;
    if (vp.y + zr < 0 || vp.y - zr > getHeight()) return;

    painter->drawLine(RS_Vector(vp.x - zr, vp.y),RS_Vector(vp.x + zr, vp.y));
    painter->drawLine(RS_Vector(vp.x, vp.y - zr),RS_Vector(vp.x, vp.y + zr));

    painter->drawCircle(vp, m_relativeZeroRadius);
}

#define DEBUG_PRINT_PREVIEW_POINTS_NO



/**
 * Draws the paper border (for print previews).
 * This function can ONLY be called from within a paintEvent because it will
 * use the painter
 *
 * @see drawIt()
 */
void RS_GraphicView::drawPaper(RS_Painter *painter) {

    if (!container) {
        return;
    }

    RS_Graphic *graphic = container->getGraphic();
    if (graphic->getPaperScale() < 1.0e-6) {
        return;
    }

// draw paper:
// RVT_PORT rewritten from     painter->setPen(Qt::gray);
    painter->setPen(QColor(Qt::gray));

    RS_Vector pinsbase = graphic->getPaperInsertionBase();
    RS_Vector printAreaSize = graphic->getPrintAreaSize();
    double scale = graphic->getPaperScale();

    RS_Vector v1 = toGui((RS_Vector(0, 0) - pinsbase) / scale);
    RS_Vector v2 = toGui((printAreaSize - pinsbase) / scale);

    int marginLeft = (int) (graphic->getMarginLeftInUnits() * factor.x / scale);
    int marginTop = (int) (graphic->getMarginTopInUnits() * factor.y / scale);
    int marginRight = (int) (graphic->getMarginRightInUnits() * factor.x / scale);
    int marginBottom = (int) (graphic->getMarginBottomInUnits() * factor.y / scale);

    int printAreaW = (int) (v2.x - v1.x);
    int printAreaH = (int) (v2.y - v1.y);

    int paperX1 = (int) v1.x;
    int paperY1 = (int) v1.y;
// Don't show margins between neighbor pages.
    int paperW = printAreaW + marginLeft + marginRight;
    int paperH = printAreaH - marginTop - marginBottom;

    int numX = graphic->getPagesNumHoriz();
    int numY = graphic->getPagesNumVert();

// gray background:
    painter->fillRect(0, 0, getWidth(), getHeight(),
                      printPreviewBackgroundColor);

// shadow:    
    painter->fillRect(paperX1 + 6, paperY1 + 6, paperW, paperH,
                      printPreviewBorderAndShadowColor);

// border:
    painter->fillRect(paperX1, paperY1, paperW, paperH,
                      printPreviewBorderAndShadowColor);

// paper:    
    painter->fillRect(paperX1 + 1, paperY1 - 1, paperW - 2, paperH + 2,
                      printPreviewPaperColor);

// print area:
    painter->fillRect(paperX1 + 1 + marginLeft, paperY1 - 1 - marginBottom,
                      printAreaW - 2, printAreaH + 2,
                      printPreviewPrintAreaColor);

// don't paint boundaries if zoom is to small
    if (qMin(std::abs(printAreaW / numX), std::abs(printAreaH / numY)) > 2) {
// boundaries between pages:
        for (int pX = 1; pX < numX; pX++) {
            double offset = ((double) printAreaW * pX) / numX;
            painter->fillRect(paperX1 + marginLeft + offset, paperY1,
                              1, paperH,
                              printPreviewBorderAndShadowColor);
        }
        for (int pY = 1; pY < numY; pY++) {
            double offset = ((double) printAreaH * pY) / numY;
            painter->fillRect(paperX1, paperY1 - marginBottom + offset,
                              paperW, 1,
                              printPreviewBorderAndShadowColor);
        }
    }


#ifdef DEBUG_PRINT_PREVIEW_POINTS
    // drawing zero
    const RS_Vector &zero = RS_Vector(0, 0);
    RS_Vector zeroGui = toGui(RS_Vector(zero) / scale);
    painter->fillRect(zeroGui.x - 5, zeroGui.y - 5, 10, 10,
                      RS_Color(255, 0, 0));

    // paper base point
    RS_Vector pinsBaseGui = toGui(-RS_Vector(pinsbase) / scale);

    painter->fillRect(pinsBaseGui.x - 5, pinsBaseGui.y - 5, 10, 10,
                      RS_Color(0, 255, 0));

    // ui point
    painter->fillRect(0, 0, 10, 10,
                      RS_Color(0, 0, 255));
#endif
}




void RS_GraphicView::drawDraftSign(RS_Painter *painter) {
    const QString draftSign = tr("Draft");
    QRect boundingRect{0, 0, 64, 64};
    for (int i = 1; i <= 4; ++i) {
        painter->drawText(boundingRect, draftSign, &boundingRect);
        QPoint position{
            (i & 1) ? getWidth() - boundingRect.width() : 0,
            (i & 2) ? getHeight() - boundingRect.height() : 0};
        boundingRect.moveTopLeft(position);
    }
}

void RS_GraphicView::drawOverlay(RS_Painter *painter) {
    double patternOffset(0.);
    // todo - using inOverlayDrawing flag is ugly, yet needed for proper drawing of containers (like dimensions or texts) that are in overlays
    // while draw for container is performed, the pen is resolved as sub-entities of containers as they are in normal drawing...
    // fixme  - review support of overlays and pens for entities for later
    inOverlayDrawing = true;
        foreach (auto ec, overlayEntities) {
                foreach (auto e, ec->getEntityList()) {
                    setPenForOverlayEntity(painter, e, patternOffset);
                    bool selected = e->isSelected();
                    // within overlays, we use temporary entities (or clones), os it's safe to modify selection state
                    e->setSelected(false);
                    e->draw(painter, this, patternOffset);
                    if (selected) {
                        drawEntityReferencePoints(painter, e);
                    }
                }
        }
    inOverlayDrawing = false;
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
RS_Vector RS_GraphicView::toGui(RS_Vector v) const {
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

/**
 * Translates a real coordinate in X to a screen coordinate X.
 * @param visible Pointer to a boolean which will contain true
 * after the call if the coordinate is within the visible range.
 */

double RS_GraphicView::toGuiX(double x) const {
    return x * factor.x + offsetX;
}

/**
 * Translates a real coordinate in Y to a screen coordinate Y.
 */
double RS_GraphicView::toGuiY(double y) const {
    return -y * factor.y + getHeight() - offsetY;
}

/**
 * Translates a real coordinate distance to a screen coordinate distance.
 */
double RS_GraphicView::toGuiDX(double d) const {
    return d * factor.x;
}

/**
 * Translates a real coordinate distance to a screen coordinate distance.
 */
double RS_GraphicView::toGuiDY(double d) const {
    return d * factor.y;
}

double RS_GraphicView::toUCSAngle(double angle) const{
    double result;
    if (m_hasUcs){
        result = ucs.toUCSAngle(angle);
    }
    else{
        result = angle;
    }
    return result;
//    return angle + ucs_Angle;
}

double RS_GraphicView::toUCSAngleDegrees(double angle) const{
    double result;
    if (m_hasUcs){
        result = ucs.toUCSAngleDegree(angle);
    }
    else{
        result = angle;
    }
    return result;
//    return angle + ucs_Angle;
}

void RS_GraphicView::toUCSDelta(const RS_Vector &worldDelta, double &ucsDX, double &ucsDY) const {
   if (m_hasUcs){
       ucs.toUCSDelta(worldDelta, ucsDX, ucsDY);
   }
   else{
       ucsDX = worldDelta.x;
       ucsDY = worldDelta.y;
   }
}

RS_Vector RS_GraphicView::toUCSDelta(const RS_Vector& worldDelta) const {
    RS_Vector result;
    if (m_hasUcs){
        double ucsDX, ucsDY;
        ucs.toUCSDelta(worldDelta, ucsDX, ucsDY);
        result = RS_Vector(ucsDX, ucsDY, 0);
    }
    else{
        result = RS_Vector(worldDelta.x, worldDelta.y, 0);
    }
    return result;
}

/**
 * Translates a vector in screen coordinates to a vector in real coordinates.
 */
RS_Vector RS_GraphicView::toGraph(const RS_Vector &v) const {
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

RS_Vector RS_GraphicView::toUCS(const RS_Vector& v) const{
    RS_Vector result;
    if (m_hasUcs){
        ucs.toUCS(v, result);
        result.valid = true;
        return result;
    }
    else{
        result = v;
    }
    return result;
}

void RS_GraphicView::toUCS(const RS_Vector& v, double& ucsX, double &ucsY) const{
    if (m_hasUcs){
        ucs.toUCS(v.x, v.y, ucsX, ucsY);
    }
    else{
        ucsX = v.x;
        ucsY = v.y;
    }
}

RS_Vector RS_GraphicView::toWorld(const RS_Vector& v) const{
    RS_Vector result;
    if (m_hasUcs){
        ucs.toWorld(v, result);
        result.valid = true;
        return result;
    }
    else{
        result = v;
    }
    return result;
}

/**
 * Translates two screen coordinates to a vector in real coordinates.
 */
RS_Vector RS_GraphicView::toGraph(const QPointF &position) const {
    return toGraph(position.x(), position.y());
}

RS_Vector RS_GraphicView::toUCS(const QPointF &position) const {
    return toUCS(RS_Vector(position.x(), position.y()));
}

/**
 * Translates two screen coordinates to a vector in real coordinates.
 */
RS_Vector RS_GraphicView::toGraph(int x, int y) const {
//    return RS_Vector(toGraphX(x), toGraphY(y));
     RS_Vector gui = RS_Vector(x, y, 0);
     return toGraph(gui);
}

/**
 * Translates a screen coordinate in X to a real coordinate X.
 */
double RS_GraphicView::toGraphX(int x) const {
    return (x - offsetX) / factor.x;
}

/**
 * Translates a screen coordinate in Y to a real coordinate Y.
 */
double RS_GraphicView::toGraphY(int y) const {
    return -(y - getHeight() + offsetY) / factor.y;
}

/**
 * Translates a screen coordinate distance to a real coordinate distance.
 */
double RS_GraphicView::toGraphDX(int d) const {
    return d / factor.x;
}

/**
 * Translates a screen coordinate distance to a real coordinate distance.
 */
double RS_GraphicView::toGraphDY(int d) const {
    return d / factor.y;
}

RS_Vector RS_GraphicView::toGraphD(int x, int y) const{
    return RS_Vector(x /factor.x, y / factor.y);
}

double RS_GraphicView::toWorldAngle(double angle) const{
    if (m_hasUcs){
       return ucs.toWorldAngle(angle);
    }
    else{
       return angle;
    }
}

double RS_GraphicView::toWorldAngleDegrees(double angle) const{
    if (m_hasUcs){
        return ucs.toWorldAngleDegrees(angle);
    }
    else{
        return angle;
    }
}


/**
 * Sets the relative zero coordinate (if not locked)
 * without deleting / drawing the point.
 */
void RS_GraphicView::setRelativeZero(const RS_Vector &pos) {
    if (relativeZeroLocked == false) {
        relativeZero = pos;
        emit relative_zero_changed(pos);
    }
}

/**
 * Sets the relative zero coordinate, deletes the old position
 * on the screen and draws the new one.
 */
void RS_GraphicView::moveRelativeZero(const RS_Vector &pos) {
    setRelativeZero(pos);
    redraw(RS2::RedrawOverlay);
}

/**
 * utility class - it's necessary for proper drawing of entities (such as RS_Point) in overlay
 * which require Graphic for their drawing.
 * todo - potentially, for usage in preview and overlay, it's better to have separate point entity that will not require variables and will not depend on settings - and so will use own drawing?
 */
class OverlayEntityContainer:public RS_EntityContainer {
public:
    OverlayEntityContainer(RS_Graphic *g):RS_EntityContainer(nullptr) {
        graphic = g;
    }

    RS_Graphic *getGraphic() const override {
        return graphic;
    }

    RS_Graphic *graphic;
};


/**
 * Gets the specified overlay container.
 */
RS_EntityContainer *RS_GraphicView::getOverlayContainer(RS2::OverlayGraphics position) {
    if (overlayEntities.contains(position)) {
        return overlayEntities[position];
    }
    if (position == RS2::OverlayGraphics::OverlayEffects) {
        overlayEntities[position] = new OverlayEntityContainer(getGraphic());
    } else {
        overlayEntities[position] = new RS_EntityContainer(nullptr);
    }
    if (position == RS2::OverlayEffects) {
        overlayEntities[position]->setOwner(true);
    }

    return overlayEntities[position];

}

RS_Grid *RS_GraphicView::getGrid() const {
    return grid.get();
}

RS_EventHandler *RS_GraphicView::getEventHandler() const {
    return eventHandler;
}

void RS_GraphicView::setBackground(const RS_Color &bg) {
    m_colorData->background = bg;

    RS_Color black(0, 0, 0);
    if (black.colorDistance(bg) >= RS_Color::MinColorDistance) {
        m_colorData->foreground = black;
    } else {
        m_colorData->foreground = RS_Color(255, 255, 255);
    }
}

RS_Color RS_GraphicView::getBackground() const {
    return m_colorData->background;
}

/**
     * @return Current foreground color.
     */
RS_Color RS_GraphicView::getForeground() const {
    return m_colorData->foreground;
}

/**
     * Sets the grid color.
     */
void RS_GraphicView::setGridColor(const RS_Color &c) {
    m_colorData->gridColor = c;
}

/**
     * Sets the meta grid color.
     */
void RS_GraphicView::setMetaGridColor(const RS_Color &c) {
    m_colorData->metaGridColor = c;
}

/**
     * Sets the selection color.
     */
void RS_GraphicView::setSelectedColor(const RS_Color &c) {
    m_colorData->selectedColor = c;
}

/**
     * Sets the highlight color.
     */
void RS_GraphicView::setHighlightedColor(const RS_Color &c) {
    m_colorData->highlightedColor = c;
}

/**
     * Sets the color for the first handle (start vertex)
     */
void RS_GraphicView::setStartHandleColor(const RS_Color &c) {
    m_colorData->startHandleColor = c;
}

/**
     * Sets the color for handles, that are neither start nor end vertices
     */
void RS_GraphicView::setHandleColor(const RS_Color &c) {
    m_colorData->handleColor = c;
}

/**
     * Sets the color for the last handle (end vertex)
     */
void RS_GraphicView::setEndHandleColor(const RS_Color &c) {
    m_colorData->endHandleColor = c;
}

const RS_Color &RS_GraphicView::getOverlayBoxLineColor() const {
    return m_colorData->overlayBoxLine;
}

void RS_GraphicView::setOverlayBoxLineColor(const RS_Color &overlayBoxLine) {
    m_colorData->overlayBoxLine = overlayBoxLine;
}

const RS_Color &RS_GraphicView::getOverlayBoxFillColor() const {
    return m_colorData->overlayBoxFill;
}

void RS_GraphicView::setOverlayBoxFillColor(const RS_Color &overlayBoxFill) {
    m_colorData->overlayBoxFill = overlayBoxFill;
}

const RS_Color &RS_GraphicView::getOverlayBoxLineInvertedColor() const {
    return m_colorData->overlayBoxLineInverted;
}

void RS_GraphicView::setOverlayBoxLineInvertedColor(const RS_Color &overlayBoxLineInverted) {
    m_colorData->overlayBoxLineInverted = overlayBoxLineInverted;
}

const RS_Color &RS_GraphicView::getOverlayBoxFillInvertedColor() const {
    return m_colorData->overlayBoxFillInverted;
}

void RS_GraphicView::setOverlayBoxFillInvertedColor(const RS_Color &overlayBoxFillInverted) {
   m_colorData->overlayBoxFillInverted = overlayBoxFillInverted;
}

void RS_GraphicView::setBorders(int left, int top, int right, int bottom) {
    borderLeft = left;
    borderTop = top;
    borderRight = right;
    borderBottom = bottom;
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

void RS_GraphicView::setFactor(double f) {
    setFactorX(f);
    setFactorY(f);
}

RS_Vector RS_GraphicView::getFactor() const {
    return factor;
}

int RS_GraphicView::getBorderLeft() const {
    return borderLeft;
}

int RS_GraphicView::getBorderTop() const {
    return borderTop;
}

int RS_GraphicView::getBorderRight() const {
    return borderRight;
}

int RS_GraphicView::getBorderBottom() const {
    return borderBottom;
}

void RS_GraphicView::freezeZoom(bool freeze) {
    zoomFrozen = freeze;
}

bool RS_GraphicView::isZoomFrozen() const {
    return zoomFrozen;
}

void RS_GraphicView::setOffsetX(int ox) {
    offsetX = ox;
    invalidate();
}

void RS_GraphicView::invalidate(){
    grid->invalidate();
}

void RS_GraphicView::setOffsetY(int oy) {
    offsetY = oy;
    invalidate();
}

int RS_GraphicView::getOffsetX() const {
    return offsetX;
}

int RS_GraphicView::getOffsetY() const {
    return offsetY;
}

void RS_GraphicView::lockRelativeZero(bool lock) {
    relativeZeroLocked = lock;
}

bool RS_GraphicView::isRelativeZeroLocked() const {
    return relativeZeroLocked;
}

RS_Vector const &RS_GraphicView::getRelativeZero() const {
    return relativeZero;
}

void RS_GraphicView::setPrintPreview(bool pv) {
    printPreview = pv;
}

bool RS_GraphicView::isPrintPreview() const {
    return printPreview;
}

void RS_GraphicView::setPrinting(bool p) {
    printing = p;
}

bool RS_GraphicView::isPrinting() const {
    return printing;
}

bool RS_GraphicView::isDraftMode() const {
    return draftMode;
}

void RS_GraphicView::setDraftMode(bool dm) {
    draftMode = dm;
}

bool RS_GraphicView::isCleanUp(void) const {
    return m_bIsCleanUp;
}

bool RS_GraphicView::isPanning() const {
    return panning;
}

void RS_GraphicView::setPanning(bool state) {
    panning = state;
}

void RS_GraphicView::setPreviewReferenceEntitiesColor(const RS_Color &c) {
    m_colorData->previewReferenceEntitiesColor = c;
}

void RS_GraphicView::setPreviewReferenceHighlightedEntitiesColor(const RS_Color &c) {
    m_colorData->previewReferenceHighlightedEntitiesColor = c;
}

void RS_GraphicView::setXAxisExtensionColor(const RS_Color &c){
    m_colorData->xAxisExtensionColor = c;
}

void RS_GraphicView::setYAxisExtensionColor(const RS_Color &c){
    m_colorData->yAxisExtensionColor = c;
}


/* Sets the color for the relative-zero marker. */
void RS_GraphicView::setRelativeZeroColor(const RS_Color &c) {
    m_colorData->relativeZeroColor = c;
}

/* Sets the hidden state for the relative-zero marker. */
void RS_GraphicView::setRelativeZeroHiddenState(bool isHidden) {
    m_colorData->hideRelativeZero = isHidden;
}

bool RS_GraphicView::isRelativeZeroHidden() {
    return m_colorData->hideRelativeZero;
}

RS2::EntityType RS_GraphicView::getTypeToSelect() const {
    return typeToSelect;
}

void RS_GraphicView::setTypeToSelect(RS2::EntityType mType) {
    typeToSelect = mType;
}

int RS_GraphicView::getMinRenderableTextHeightInPx() const {
    return minRenderableTextHeightInPx;
}

void RS_GraphicView::loadGridSettings() {
    if (grid != nullptr){
        grid->loadSettings();
    }
    update();
}

int RS_GraphicView::getPointMode() const {
    return pdmode;
}

int RS_GraphicView::getPointSize() const {
    return screenPDSize;
}

int RS_GraphicView::determinePointScreenSize(RS_Painter *painter, double pdsize) const{
    int screenPointSize;
    int deviceHeight = painter->getHeight();
    if (pdsize == 0){
        screenPointSize = deviceHeight / 20;
    }
    else if (DXF_FORMAT_PDSize_isPercent(pdsize)){
        screenPointSize = (deviceHeight * DXF_FORMAT_PDSize_Percent(pdsize)) / 100;
    }
    else {
        screenPointSize = toGuiDY(pdsize);
    }
    return screenPointSize;
}

double RS_GraphicView::getMinCircleDrawingRadius() const {
    return minCircleDrawingRadius;
}

double RS_GraphicView::getMinArcDrawingRadius() const {
    return minArcDrawingRadius;
}

double RS_GraphicView::getMinEllipseMajorRadius() const {
    return minEllipseMajorRadius;
}

double RS_GraphicView::getMinEllipseMinorRadius() const {
    return minEllipseMinorRadius;
}

double RS_GraphicView::getMinLineDrawingLen() const {
    return minLineDrawingLen;
}

void RS_GraphicView::setHasNoGrid(bool noGrid) {
    hasNoGrid = noGrid;
}

bool RS_GraphicView::isDraftLinesMode() const {
    return m_draftLinesMode;
}

void RS_GraphicView::setDraftLinesMode(bool mode) {
    m_draftLinesMode = mode;
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


bool RS_GraphicView::isDrawTextsAsDraftForPreview() const {
    return drawTextsAsDraftForPreview;
}

RS_Undoable *RS_GraphicView::getRelativeZeroUndoable() {
    RS_Undoable* result = nullptr;
    if (LC_LineMath::isMeaningfulDistance(markedRelativeZero, relativeZero)){
        result = new LC_UndoableRelZero(this, markedRelativeZero, relativeZero);
        markRelativeZero();
    }
    return result;
}


// fixme - sand - ucs - unwrapp calculations
void RS_GraphicView::UserCoordinateSystem::toUCS(const RS_Vector &worldCoordinate, RS_Vector &ucsCoordinate) const{
    // the code below is equivalent to this
    //  RS_Vector newPos = world - ucsOrigin;
    //  RS_Vector rotated = newPos.rotate(ucsOrigin, xAxisAngle);

   /* double x = worldCoordinate.x - ucsOrigin.x;
    double y = worldCoordinate.y - ucsOrigin.y;

    rotate(x,y);

    ucsCoordinate.x = x;
    ucsCoordinate.y = y;*/

   RS_Vector wcs = worldCoordinate;
   RS_Vector newPos = wcs-ucsOrigin;
   ucsCoordinate = newPos.rotate(xAxisAngle);
}
// fixme - sand - ucs - unwrapp calculations
void RS_GraphicView::UserCoordinateSystem::toUCS(double worldX, double worldY, double &ucsX, double &ucsY) const {
    // the code below is equivalent to this
    //  RS_Vector newPos = world - ucsOrigin;
    //  RS_Vector rotated = newPos.rotate(ucsOrigin, xAxisAngle);

    /*ucsX = worldX - ucsOrigin.x;
    ucsY = worldY - ucsOrigin.y;

    rotate(ucsX,ucsY);*/


    RS_Vector wcs = RS_Vector(worldX, worldY);
    RS_Vector newPos = wcs-ucsOrigin;
    newPos.rotate(xAxisAngle);
    ucsX = newPos.x;
    ucsY = newPos.y;
}
// fixme - sand - ucs - unwrapp calculations
void RS_GraphicView::UserCoordinateSystem::toUCSDelta(const RS_Vector &worldDelta, double &ucsDX, double &ucsDY) const {
    double magnitude = worldDelta.magnitude();
    double angle = worldDelta.angle();
    double ucsAngle = angle + xAxisAngle;
    ucsDX = magnitude*cos(ucsAngle);
    ucsDY = magnitude*sin(ucsAngle);
}
// fixme - sand - ucs - unwrapp calculations
void RS_GraphicView::UserCoordinateSystem::toWorld(const RS_Vector &ucsCoordinate, RS_Vector &worldCoordinate) const{
    // RS_Vector rotated = ucs.rotate(UCS_Origin, -ucs_Angle);
    // world = rotated + ucsOrigin

/*    double x = ucsCoordinate.x;
    double y = ucsCoordinate.y;

    rotateBack(x,y);

    x = x + ucsOrigin.x;
    y = y + ucsOrigin.y;

    worldCoordinate.x = x;
    worldCoordinate.y = y;*/

//    ******
    RS_Vector newPos = ucsCoordinate;
    newPos.rotate(-xAxisAngle);
    worldCoordinate  = newPos + ucsOrigin;
}
// fixme - sand - ucs - unwrapp calculations
void RS_GraphicView::UserCoordinateSystem::toWorld(double ucsX, double ucsY, double &worldX, double &worldY) const{
    // RS_Vector rotated = ucs.rotate(UCS_Origin, -ucs_Angle);
    // world = rotated + ucsOrigin

   /* double x = ucsX;
    double y = ucsY;

    rotateBack(x,y);

    x = x + ucsOrigin.x;
    y = y + ucsOrigin.y;

    worldX = x;
    worldY = y;*/

    RS_Vector ucsCoordinate = RS_Vector(ucsX, ucsY);
    ucsCoordinate.rotate(-xAxisAngle);
    RS_Vector world = ucsCoordinate + ucsOrigin;
    worldX  = world.x;
    worldY = world.y;
}


void RS_GraphicView::UserCoordinateSystem::rotate(double &x, double &y) const{
    double deltaX = x - ucsOrigin.x;
    double deltaY = y - ucsOrigin.y;

    x = ucsOrigin.x + deltaX * cosXAngle - deltaY * sinXAngle;
    y = ucsOrigin.y + deltaX * sinXAngle + deltaY * cosXAngle;
}

void RS_GraphicView::UserCoordinateSystem::rotateBack(double &x, double &y) const{
    double deltaX = x - ucsOrigin.x;
    double deltaY = y - ucsOrigin.y;

    x = ucsOrigin.x + deltaX * cosNegativeXAngle - deltaY * sinNegativeXAngle;
    y = ucsOrigin.y + deltaX * sinNegativeXAngle + deltaY * cosNegativeXAngle;
}


void RS_GraphicView::UserCoordinateSystem::setXAxisAngle(double angle){
    xAxisAngle = angle;
    xAxisAngleDegrees = RS_Math::rad2deg(angle);
    sinXAngle = sin(angle);
    cosXAngle = cos(angle);
    sinNegativeXAngle = sin(-angle);
    cosNegativeXAngle = cos(-angle);
}

void RS_GraphicView::UserCoordinateSystem::update(const RS_Vector &origin, double angle) {
    ucsOrigin = origin;
    setXAxisAngle(angle);
}

void RS_GraphicView::extractUCS(){
    if (hasUCS()){
        RS_Graphic* graphic = getGraphic();
        if (graphic != nullptr) {
            LC_UCSList *ucsList = graphic->getUCSList();
            LC_UCS *candidate = createUCSEntity(ucs.getUcsOrigin(), -ucs.getXAxisAngle(), isGridIsometric(), getIsoViewType());
            LC_UCS *createdUCS = ucsList->tryAddUCS(candidate);
            if (createdUCS != nullptr) {
                applyUCS(createdUCS);
            }
        }
    }
}

void RS_GraphicView::createUCS(const RS_Vector &origin, double angle) {
    bool customUCS = LC_LineMath::isMeaningfulAngle(angle) || LC_LineMath::isMeaningfulDistance(origin, RS_Vector(0, 0, 0));
    if (customUCS){
        RS_Graphic* graphic = getGraphic();
        if (graphic != nullptr) {
            LC_UCSList *ucsList = graphic->getUCSList();
            LC_UCS* candidate = createUCSEntity(origin, angle,isGridIsometric(), getIsoViewType());
            LC_UCS* createdUCS = ucsList->tryAddUCS(candidate);
            if (createdUCS != nullptr){
                setUCS(origin, angle, isGridIsometric(), getIsoViewType());
                graphic->setCurrentUCS(createdUCS);
                emit ucsChanged(createdUCS);
            }
        }
    }
}

LC_UCS *RS_GraphicView::createUCSEntity(const RS_Vector &origin, double angle, bool isometric, RS2::IsoGridViewType isoType) const{
    auto* result = new LC_UCS("");
    result->setOrigin(origin);

    RS_Vector xAxis = RS_Vector(1.0, 0, 0);
    RS_Vector yAxis = RS_Vector(0,1.0, 0);

    xAxis.rotate(angle);
    yAxis.rotate(angle);

    result->setXAxis(xAxis);
    result->setYAxis(yAxis);

    result->setElevation(0.0);
    result->setTemporary(true);

    result->setOrthoOrigin(RS_Vector(0,0,0));

    int orthoType = LC_UCS::NON_ORTHO;
    if (isometric){
        switch (isoType){
            case (RS2::IsoRight):{
                orthoType = LC_UCS::RIGHT;
                break;
            }
            case (RS2::IsoLeft):{
                orthoType = LC_UCS::LEFT;
                break;
            }
            case (RS2::IsoTop):{
                orthoType = LC_UCS::TOP;
                break;
            }
            default:
                orthoType = LC_UCS::NON_ORTHO;
        }
    }
    result->setName(tr("<No name>"));
    result->setOrthoType(orthoType);
    return result;
}

LC_UCS* RS_GraphicView::getCurrentUCS() const{
    LC_UCS* result = nullptr;
    if (hasUCS()){
        result = createUCSEntity(ucs.getUcsOrigin(), -ucs.getXAxisAngle(),isGridIsometric(), getIsoViewType());
    }
    return result;
}

void RS_GraphicView::applyUCS(LC_UCS *ucsToSet) {
    if (ucsToSet == nullptr){
        return;
    }
    RS_Vector originToSet = ucsToSet->getOrigin();
    double angleToSet = ucsToSet->getXAxisDirection();
    int orthoType = ucsToSet->getOrthoType();
    bool hasIso = ucsToSet->isIsometric();
    RS2::IsoGridViewType isoType = ucsToSet->getIsoGridViewType();
    setUCS(originToSet, angleToSet, hasIso, isoType);
    RS_Graphic *graphic = getGraphic();
    if (graphic != nullptr){
        graphic->setCurrentUCS(ucsToSet);
    }
    emit ucsChanged(ucsToSet);
}

void RS_GraphicView::applyUCSAfterLoad(){
    LC_UCS* ucsCurrent = getGraphic()->getCurrentUCS();
    if (ucsCurrent != nullptr) {
        RS_Vector originToSet = ucsCurrent->getOrigin();
        double angleToSet = ucsCurrent->getXAxisDirection();
        int orthoType = ucsCurrent->getOrthoType();
        bool isometric = ucsCurrent->isIsometric();
        RS2::IsoGridViewType isoType = ucsCurrent->getIsoGridViewType();
        doSetUCS(originToSet, angleToSet, isometric, isoType);
        emit ucsChanged(ucsCurrent);
        delete ucsCurrent;
    }
}

void RS_GraphicView::setUCS(const RS_Vector &origin, double angle, bool isometric, RS2::IsoGridViewType isoType) {
    RS_Vector ucsOrigin = doSetUCS(origin, angle, isometric, isoType);
    switch (m_ucsApplyingPolicy){
        case UCSApplyingPolicy::ZoomAuto: {
            zoomAuto();
            break;
        }
        case UCSApplyingPolicy::PanOriginCenter: {
            int offX = (int) ((ucsOrigin.x * factor.x)
                             + (double) (getWidth() - borderLeft - borderRight) / 2.0);
            int offY = (int) ((ucsOrigin.y * factor.y)
                             + (double) (getHeight() - borderTop - borderBottom) / 2.0);

            setOffset(offX, offY);
            redraw();
            break;
        }
        case UCSApplyingPolicy::PanOriginLowerLeft:{
            setOffset(ucsOrigin.x* factor.x+(borderLeft + borderRight)/2, ucsOrigin.y* factor.y + (borderBottom + borderRight)/2);
            redraw();
            break;
        }
        default:{
            zoomAuto();
        }
    }
    redraw();
}

RS_Vector RS_GraphicView::doSetUCS(const RS_Vector &origin, double angle, bool isometric, RS2::IsoGridViewType &isoType) {
    bool customUCS = LC_LineMath::isMeaningfulAngle(angle) || LC_LineMath::isMeaningfulDistance(origin, RS_Vector(0, 0, 0));
    RS_Vector ucsOrigin;
    if (customUCS) {
        ucs.update(origin, -angle);
        m_hasUcs = true;
        ucsOrigin = toUCS(origin);
    } else {
        m_hasUcs = false;
        ucsOrigin = RS_Vector(0, 0);
    }
    RS_Graphic *graphic = getGraphic();
    if (graphic != nullptr){
        bool oldIsometricGrid = graphic->isIsometricGrid();
        RS2::IsoGridViewType oldIsoViewType = graphic->getIsoView();
        if (oldIsometricGrid != isometric || oldIsoViewType != isoType) {
            graphic->setIsometricGrid(isometric);
            if (isometric) {
                graphic->setIsoView(isoType);
            }
            loadGridSettings();
            invalidate();
        }
    }
    return ucsOrigin;
}

double  RS_GraphicView::UserCoordinateSystem::toWorldAngle(double angle) const {return angle - xAxisAngle;};
double  RS_GraphicView::UserCoordinateSystem::toWorldAngleDegrees(double angle) const {return angle - xAxisAngleDegrees;};
double  RS_GraphicView::UserCoordinateSystem::toUCSAngle(double angle) const {return angle + xAxisAngle;}
double RS_GraphicView::UserCoordinateSystem::toUCSAngleDegree(double angle) const {
    return angle + xAxisAngleDegrees;
}

const RS_Vector &RS_GraphicView::UserCoordinateSystem::getUcsOrigin() const {
    return ucsOrigin;
}

double RS_GraphicView::UserCoordinateSystem::getXAxisAngle() const {
    return xAxisAngle;
}


RS_Vector RS_GraphicView::snapGrid(const RS_Vector &coord) const {
    if (m_hasUcs) {
        // basically, wcs coordinate still should be returned there.
        // however, it will be rotated according to the grid (which is not rotated in ucs).
        RS_Vector snap = getGrid()->snapGrid(toUCS(coord));
        snap = toWorld(snap);
        return snap;
    }
    else{
        RS_Vector snap = getGrid()->snapGrid(coord);
        return snap;
    }
}

RS_Vector RS_GraphicView::restrictHorizontal(const RS_Vector baseWCSPoint, const RS_Vector &wcsCoord) const {
    if (m_hasUcs) {
        RS_Vector ucsBase = toUCS(baseWCSPoint);
        RS_Vector ucsCoord = toUCS(wcsCoord);
        double resX, resY;
        ucs.toWorld(ucsCoord.x, ucsBase.y, resX, resY);
        return RS_Vector(resX, resY);
    }
    else{
        return RS_Vector(baseWCSPoint.x, wcsCoord.y);
    }
}

RS_Vector RS_GraphicView::restrictVertical(const RS_Vector baseWCSPoint, const RS_Vector &wcsCoord) const {
    if (m_hasUcs) {
        RS_Vector ucsBase = toUCS(baseWCSPoint);
        RS_Vector ucsCoord = toUCS(wcsCoord);
        double resX, resY;
        ucs.toWorld(ucsBase.x, ucsCoord.y, resX, resY);
        return RS_Vector(resX, resY);
    }
    else{
        return RS_Vector(wcsCoord.x, baseWCSPoint.y);
    }
}

bool RS_GraphicView::hasUCS() const {
    return m_hasUcs;
}

void RS_GraphicView::restoreView(LC_View *view) {
    if (view == nullptr){
        return;
    }

    RS_Vector center = view->getCenter();
    RS_Vector size = view->getSize();

    const RS_Vector halfSize = size / 2;
    RS_Vector v1 = center - halfSize;
    RS_Vector v2 = center + halfSize;

    RS_Vector origin = RS_Vector(0,0,0);
    double angle = 0;
    bool isometric = false;
    RS2::IsoGridViewType isoType = RS2::IsoTop;
    RS_Graphic *graphic = getGraphic();
    auto* ucs = view->getUCS();
    if (ucs != nullptr){
        origin = ucs->getOrigin();
        angle = ucs->getXAxisDirection();
        isometric = ucs->isIsometric();
        isoType = ucs->getIsoGridViewType();
    }
    else{
        ucs = &LC_WCS::instance;
    };


    if (graphic != nullptr){
        graphic->setCurrentUCS(ucs);
    }
    emit ucsChanged(ucs);

    doSetUCS(origin, angle, isometric, isoType);
    zoomWindow(v1, v2, true);
}

void RS_GraphicView::initAfterDocumentOpen() {
    applyUCSAfterLoad();
}

LC_View* RS_GraphicView::createNamedView(QString name) const{
    auto* viewToCreate = new LC_View(name);
    doUpdateViewByGraphicView(viewToCreate);
    return viewToCreate;
}

void RS_GraphicView::updateNamedView(LC_View* view) const{
    doUpdateViewByGraphicView(view);
}

void RS_GraphicView::doUpdateViewByGraphicView(LC_View *view) const {
    view->setForPaperView(isPrintPreview());

    int width = getWidth();
    int height = getHeight();

    double x = toGraphX(width);
    double y = toGraphY(height);

    double x0 = toGraphX(0);
    double y0 = toGraphY(0);

    view->setCenter({(x + x0) / 2.0, (y + y0) / 2.0, 0});
    view->setSize({(x - x0), (y - y0), 0});

    view->setTargetPoint({0, 0, 0});

    LC_UCS* viewUCS = getCurrentUCS();
    if (viewUCS != nullptr) {
        view->setUCS(viewUCS);
        RS_Graphic *graphic = getGraphic();
        if (graphic != nullptr) {
            LC_UCSList *ucsList = graphic->getUCSList();

            LC_UCS *existingListUCS = ucsList->findExisting(viewUCS);
            if (existingListUCS != nullptr) {
                QString ucsName = existingListUCS->getName();
                viewUCS->setName(ucsName);
            } else {
                viewUCS->setName(tr("<No name>"));
            }
        }
    }
    else{
        // this is WCS
        view->setUCS(new LC_WCS());
    }
}
