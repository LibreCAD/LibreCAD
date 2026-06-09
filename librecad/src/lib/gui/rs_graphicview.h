/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
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


#ifndef RS_GRAPHICVIEW_H
#define RS_GRAPHICVIEW_H

#include <QWidget>
#include <memory>

#include "lc_graphicviewportlistener.h"
#include "rs.h"
#include "rs_debug.h"

class LC_RelativePointInputWidget;
class RS_Document;
class LC_EventHandler;
struct LC_InfoCursorOverlayPrefs;
class QDateTime;
class QMouseEvent;
class QKeyEvent;

class RS_ActionInterface;
class RS_Entity;
class RS_EntityContainer;
class RS_EventHandler;
class RS_Color;
class RS_CommandEvent;
class RS_Graphic;
class RS_Grid;
class RS_Painter;

struct RS_LineTypePattern;
struct RS_SnapMode;
class LC_GraphicViewport;
class LC_WidgetViewPortRenderer;
class LC_VisualSnapData;

/**
 * This class is a common GUI interface for the graphic viewer
 * widget which has to be implemented by real GUI classes such
 * as the Qt graphical view.
 *
 * Note that this is just an interface used as a slot to
 * communicate with the LibreCAD from a GUI level.
 */
class RS_GraphicView:public QWidget, public LC_GraphicViewPortListener {
    Q_OBJECT
public:
    explicit RS_GraphicView(QWidget *parent = nullptr, Qt::WindowFlags f = {});
    ~RS_GraphicView() override;
    void cleanUp();
/**
 * @return Pointer to the graphic entity if the entity container
 * connected to this view is a graphic and valid.
 * NULL otherwise.
 */
    RS_Graphic *getGraphic(bool resolve = false) const;
    LC_GraphicViewport* getViewPort() const
    {
        return m_viewport.get();
    }
    void setDocument(RS_Document *c);

    virtual void loadSettings();
/** This virtual method must be overwritten to return
  the width of the widget the graphic is shown in */
    virtual int getWidth() const = 0;
/** This virtual method must be overwritten to return
  the height of the widget the graphic is shown in */
    virtual int getHeight() const = 0;
/** This virtual method must be overwritten to redraw the widget. */
    virtual void redraw(RS2::RedrawMethod method = RS2::RedrawAll, bool immediately = false) = 0;
/** This virtual method must be overwritten and is then
  called whenever the view changed */
    virtual void adjustOffsetControls() = 0;
/** This virtual method must be overwritten and is then
  called whenever the view changed */
    virtual void adjustZoomControls() = 0;

/* Sets the hidden state for the relative-zero marker. */
    void setRelativeZeroHiddenState(bool isHidden) const;
    bool isRelativeZeroHidden() const;
/**
 * This virtual method can be overwritten to set the mouse
 * cursor to the given type.
 */
    virtual void setMouseCursor(RS2::CursorType /*c*/) = 0;

    RS_Document *getDocument() const;
    void switchToDefaultAction();
    void setDefaultAction(RS_ActionInterface *action) const;
    RS_ActionInterface *getDefaultAction() const;
    void hideOptions() const;
    // fixme - sand - complete changes in plugin and remove this function from the public interface!!!
    bool setCurrentAction(const std::shared_ptr<RS_ActionInterface>& action) const;
    RS_ActionInterface *getCurrentAction() const;
    QString getCurrentActionName() const;
    QIcon getCurrentActionIcon() const;
    void killAllActions() const;
    bool killAllActionsWithResult() const;
    void back(Qt::KeyboardModifiers modifiers) const;
    void processEnterKey() const;
    void commandEvent(RS_CommandEvent *e) const;
    void keyPressEvent(QKeyEvent *event) override;
    void enableCoordinateInput() const;
    void disableCoordinateInput() const;
    void zoomAuto(bool axis=true) const;

    virtual void updateGridStatusWidget(QString) = 0;
    void setDefaultSnapMode(RS_SnapMode sm) const;
    RS_SnapMode getDefaultSnapMode() const;
    void setSnapRestriction(RS2::SnapRestriction sr);
    RS2::SnapRestriction getSnapRestriction() const;
    bool isCurrentActionRunning(const RS_ActionInterface* action) const;
    /**
  * Enables or disables print preview.
  */
    void setPrintPreview(bool pv);
    /**
  * @retval true This is a print preview graphic view.
  * @retval false Otherwise.
  */
    bool isPrintPreview() const;

    bool isCleanUp() const;

    void setLineWidthScaling(bool state) const;
    bool getLineWidthScaling() const;

    RS2::EntityType getTypeToSelect() const;
    void setTypeToSelect(RS2::EntityType mType);
    virtual QString obtainEntityDescription(const RS_Entity* entity, RS2::EntityDescriptionLevel descriptionLevel);
    LC_InfoCursorOverlayPrefs* getInfoCursorOverlayPreferences() const;

    bool getPanOnZoom() const;
    bool getSkipFirstZoom() const;

    void setShowEntityDescriptionOnHover(bool show);
    bool isShowEntityDescriptionOnHover() const {
        return m_showEntityDescriptionOnHover;
    }
    virtual void highlightUCSLocation([[maybe_unused]]LC_UCS *ucs) {}
    void onViewportChanged() override;
    void onRelativeZeroChanged(const RS_Vector &pos) override;
    void onUCSChanged(LC_UCS* ucs) override;
    void notifyCurrentActionChanged(RS2::ActionType actionType);
    bool hasAction() const;
    void notifyLastActionFinished() const;
    void onSwitchToDefaultAction(bool actionIsDefault, RS2::ActionType actionRtti, RS2::ActionType prevActionRtti);
    void showRelativeInputWidget(const RS_Vector& wcsPos, const RS_Vector& basePoint, bool baseIsRelativePoint, RS2::RelativePointParam param) const;
    void hideRelativeInputWidget() const;
    void restoreRelativeInputWidget() const;
    bool isInRelativePointInput() const;
    void onViewportRedrawNeeded(RS2::RedrawMethod method, bool redrawImmediately) override;
    LC_VisualSnapData* getVisualSnapData() const {return m_visualSnapData;}
signals:
    void ucsChanged(LC_UCS* ucs);
    void relativeZeroChanged(const RS_Vector &);
    void previousZoomAvailable(bool available);
    void currentActionChanged(RS2::ActionType actionType);
    void defaultActionActivated(bool value,RS2::ActionType actionRtti, RS2::ActionType prevActionRtti);
protected:
    void setRenderer(std::unique_ptr<LC_WidgetViewPortRenderer> renderer);
    LC_WidgetViewPortRenderer* getRenderer() const;
    void resizeEvent(QResizeEvent *event) override;
    LC_EventHandler *getEventHandler() const;

    LC_RelativePointInputWidget* m_relativePointWidgetHolder = nullptr;
private:
    std::unique_ptr<LC_EventHandler> m_eventHandler;
    RS_Document *m_document = nullptr;
    std::unique_ptr<LC_GraphicViewport> m_viewport;
    std::unique_ptr<LC_WidgetViewPortRenderer> m_renderer;
    LC_VisualSnapData* m_visualSnapData;

    /**
     * Current default snap mode for this graphic view. Used for new
     * actions.
     */
    std::unique_ptr<RS_SnapMode> m_defaultSnapMode;
 /**
  * Current default snap restriction for this graphic view. Used for new
  * actions.
  */
    RS2::SnapRestriction m_defaultSnapRes{};
    std::unique_ptr<LC_InfoCursorOverlayPrefs> m_infoCursorOverlayPreferences;

    /** if true, graphicView is under cleanup */
    bool m_bIsCleanUp = false;
    bool m_printPreview = false;

    RS2::EntityType m_typeToSelect = RS2::EntityType::EntityUnknown;

    bool m_showEntityDescriptionOnHover = false;
    bool m_panOnZoom = false;
    bool m_skipFirstZoom = false;

    const RS_LineTypePattern *getPattern(RS2::LineType t);
    bool setEventHandlerAction(const std::shared_ptr<RS_ActionInterface>&) const;

};
#endif
