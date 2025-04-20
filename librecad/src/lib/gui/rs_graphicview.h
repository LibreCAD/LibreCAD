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

#include <memory>

#include <QWidget>
#include "lc_graphicviewportlistener.h"
#include "rs.h"

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

/**
 * This class is a common GUI interface for the graphic viewer
 * widget which has to be implemented by real GUI classes such
 * as the Qt graphical view.
 *
 * Note that this is just an interface used as a slot to
 * communicate with the LibreCAD from a GUI level.
 */
class RS_GraphicView:public QWidget, LC_GraphicViewPortListener {
    Q_OBJECT
public:
    RS_GraphicView(QWidget *parent = nullptr, Qt::WindowFlags f = {});
    virtual ~RS_GraphicView();
    void cleanUp();
/**
 * @return Pointer to the graphic entity if the entity container
 * connected to this view is a graphic and valid.
 * NULL otherwise.
 */
    RS_Graphic *getGraphic() const;
    LC_GraphicViewport* getViewPort() const
    {
        return m_viewport.get();
    }
    void setContainer(RS_EntityContainer *c);

    virtual void loadSettings();
/** This virtual method must be overwritten to return
  the width of the widget the graphic is shown in */
    virtual int getWidth() const = 0;
/** This virtual method must be overwritten to return
  the height of the widget the graphic is shown in */
    virtual int getHeight() const = 0;
/** This virtual method must be overwritten to redraw
  the widget. */
    virtual void redraw(RS2::RedrawMethod method = RS2::RedrawAll) = 0;
/** This virtual method must be overwritten and is then
  called whenever the view changed */
    virtual void adjustOffsetControls() = 0;
/** This virtual method must be overwritten and is then
  called whenever the view changed */
    virtual void adjustZoomControls() = 0;

/* Sets the hidden state for the relative-zero marker. */
    void setRelativeZeroHiddenState(bool isHidden);
    bool isRelativeZeroHidden();
/**
 * This virtual method can be overwritten to set the mouse
 * cursor to the given type.
 */
    virtual void setMouseCursor(RS2::CursorType /*c*/) = 0;

    RS_EntityContainer *getContainer() const;
    void switchToDefaultAction();
    void setDefaultAction(RS_ActionInterface *action) const;
    RS_ActionInterface *getDefaultAction() const;
    void hideOptions() const;
    // fixme - sand - complete changes in plugin and remove this function from the public interface!!!
    bool setCurrentAction(std::shared_ptr<RS_ActionInterface> action);
    RS_ActionInterface *getCurrentAction() const;
    QString getCurrentActionName() const;
    QIcon getCurrentActionIcon() const;
    void killSelectActions() const;
    void killAllActions() const;
    void back() const;
    void processEnterKey();
    void commandEvent(RS_CommandEvent *e);
    void keyPressEvent(QKeyEvent *event) override;
    void enableCoordinateInput();
    void disableCoordinateInput();
    void zoomAuto(bool axis=true);
    void zoomPage();
    void zoomPageEx();

    virtual void updateGridStatusWidget(QString) = 0;
    void setDefaultSnapMode(RS_SnapMode sm);
    RS_SnapMode getDefaultSnapMode() const;
    void setSnapRestriction(RS2::SnapRestriction sr);
    RS2::SnapRestriction getSnapRestriction() const;
    RS_EventHandler *getEventHandler() const;
    /**
  * Enables or disables print preview.
  */
    void setPrintPreview(bool pv);
    /**
  * @retval true This is a print preview graphic view.
  * @retval false Otherwise.
  */
    bool isPrintPreview() const;
    /**
  * Enables or disables printing.
  */
    void setPrinting(bool p);
/**
  * @retval true This is a graphic view for printing.
  * @retval false setSnapOtherwise.
  */
    bool isPrinting() const;

    bool isCleanUp(void) const;

    void setLineWidthScaling(bool state);
    bool getLineWidthScaling() const;

    RS2::EntityType getTypeToSelect() const;
    void setTypeToSelect(RS2::EntityType mType);

    void setForcedActionKillAllowed(bool forcedActionKillAllowed);
    virtual QString obtainEntityDescription(RS_Entity *entity, RS2::EntityDescriptionLevel shortDescription);

    LC_InfoCursorOverlayPrefs* getInfoCursorOverlayPreferences();

    bool getPanOnZoom() const;
    bool getSkipFirstZoom() const;

    void setShowEntityDescriptionOnHover(bool show);
    bool isShowEntityDescriptionOnHover(){
        return showEntityDescriptionOnHover;
    }
    virtual void highlightUCSLocation([[maybe_unused]]LC_UCS *ucs) {}
    void onViewportChanged() override;
    void onRelativeZeroChanged(const RS_Vector &pos) override;
    void onUCSChanged(LC_UCS* ucs) override;
    void notifyNoActiveAction();
signals:
    void ucsChanged(LC_UCS* ucs);
    void relativeZeroChanged(const RS_Vector &);
    void previous_zoom_state(bool);

    void currentActionChanged(RS_ActionInterface* action);
protected:
    void setRenderer(std::unique_ptr<LC_WidgetViewPortRenderer> renderer);
    LC_WidgetViewPortRenderer* getRenderer() const;
    void resizeEvent(QResizeEvent *event) override;
    void onViewportRedrawNeeded() override;
private:
    std::unique_ptr<RS_EventHandler> m_eventHandler;
    RS_EntityContainer *container = nullptr;
    std::unique_ptr<LC_GraphicViewport> m_viewport;
    std::unique_ptr<LC_WidgetViewPortRenderer> m_renderer;

    /**
     * Current default snap mode for this graphic view. Used for new
     * actions.
     */
    std::unique_ptr<RS_SnapMode> defaultSnapMode;
 /**
  * Current default snap restriction for this graphic view. Used for new
  * actions.
  */
    RS2::SnapRestriction defaultSnapRes{};
    std::unique_ptr<LC_InfoCursorOverlayPrefs> infoCursorOverlayPreferences;

    /** if true, graphicView is under cleanup */
    bool m_bIsCleanUp = false;
    bool printPreview = false;

    RS2::EntityType typeToSelect = RS2::EntityType::EntityUnknown;

    bool forcedActionKillAllowed = true;
    bool showEntityDescriptionOnHover = false;
    bool m_panOnZoom = false;
    bool m_skipFirstZoom = false;
    const RS_LineTypePattern *getPattern(RS2::LineType t);

    bool setEventHandlerAction(std::shared_ptr<RS_ActionInterface>);
};
#endif
