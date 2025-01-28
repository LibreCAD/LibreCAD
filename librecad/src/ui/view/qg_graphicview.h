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

#ifndef QG_GRAPHICVIEW_H
#define QG_GRAPHICVIEW_H

#include <QWidget>

#include "rs_blocklistlistener.h"
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_layerlistlistener.h"

#include "lc_ucs_mark.h"

class QEnterEvent;
class QG_ScrollBar;
class QGridLayout;
class QLabel;
class QMenu;
class QMouseEvent;

/**
 * This is the Qt implementation of a widget which can view a 
 * graphic. 
 *
 * Instances of this class can be linked to layer lists using
 * addLayerListListener().
 */
class QG_GraphicView:   public RS_GraphicView,
                        public RS_LayerListListener,
                        public RS_BlockListListener,
                        public LC_UCSListListener
{
Q_OBJECT

public:
    explicit QG_GraphicView(QWidget *parent = nullptr,  RS_Document *doc = nullptr);
    ~QG_GraphicView() override;

    int getWidth() const override;
    int getHeight() const override;
    void redraw(RS2::RedrawMethod method=RS2::RedrawAll) override;
    void adjustOffsetControls() override;
    void adjustZoomControls() override;
    void setMouseCursor(RS2::CursorType c) override;
    void updateGridStatusWidget(QString text) override;
    void updateGridPoints();
    void loadSettings() override;

    // Methods from RS_LayerListListener Interface:
    void layerEdited(RS_Layer*) override{
        redraw(RS2::RedrawDrawing);
    }
    void layerRemoved(RS_Layer*) override{
        redraw(RS2::RedrawDrawing);
    }

    void layerToggled(RS_Layer*) override{
        const RS_EntityContainer::LC_SelectionInfo &info = container->getSelectionInfo();
        RS_DIALOGFACTORY->updateSelectionWidget(info.count, info.length);
        redraw(RS2::RedrawDrawing);
    }

    void layerActivated(RS_Layer *) override;


    /**
     * @brief setOffset
     * @param ox, offset X
     * @param oy, offset Y
     */
    void setOffset(int ox, int oy);
/*    *//**
     * @brief getMousePosition() mouse position in widget coordinates
     * @return the cursor position in widget coordinates
     * returns the widget center, if cursor is not on the widget
     *//*
    RS_Vector getMousePosition() const override;*/

    void setAntialiasing(bool state);
    bool isDraftMode() const;
    void setDraftMode(bool dm);
    void setDraftLinesMode(bool mode);

    void setCursorHiding(bool state);
    void addScrollbars();
    bool hasScrollbars();
    void setCurrentQAction(QAction* q_action);
    QString device;
    void destroyMenu(const QString& activator);
    void setMenu(const QString& activator, QMenu* menu);
    QString obtainEntityDescription(RS_Entity *entity, RS2::EntityDescriptionLevel shortDescription) override;
    virtual void initView();
protected slots:
    void slotHScrolled(int value);
    void slotVScrolled(int value);
protected:
    void mousePressEvent(QMouseEvent* e) override;
    void mouseDoubleClickEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void tabletEvent(QTabletEvent* e) override;
    void leaveEvent(QEvent*) override;
    void enterEvent(QEnterEvent*) override;
    void focusInEvent(QFocusEvent*) override;
    void focusOutEvent(QFocusEvent*) override;
    void wheelEvent(QWheelEvent* e) override;
    void keyPressEvent(QKeyEvent* e) override;
    void keyReleaseEvent(QKeyEvent* e) override;
    bool event(QEvent * e) override;
    void paintEvent(QPaintEvent *)override;
    void resizeEvent(QResizeEvent* e) override;
    QList<QAction*> recent_actions;
    void autoPanStep();

    //! Horizontal scrollbar.
    QG_ScrollBar* hScrollBar = nullptr;
    //! Vertical scrollbar.
    QG_ScrollBar* vScrollBar = nullptr;
    //! Layout used to fit in the view and the scrollbars.
    QGridLayout* layout = nullptr;
    //! CAD mouse cursor
    std::unique_ptr<QCursor> curCad;
    //! Delete mouse cursor
    std::unique_ptr<QCursor> curDel;
    //! Select mouse cursor
    std::unique_ptr<QCursor> curSelect;
    //! Magnifying glass mouse cursor
    std::unique_ptr<QCursor> curMagnifier;
    //! Hand mouse cursor
    std::unique_ptr<QCursor> curHand;

    double scrollZoomFactor = 1.137;

    //! Keep tracks of if we are currently doing a high-resolution scrolling
    bool isSmoothScrolling;

    LC_UCSMarkOptions m_ucsMarkOptions;

    QMap<QString, QMenu*> menus;
    void highlightUCSLocation(LC_UCS *ucs) override;
    void ucsHighlightStep();

    virtual void createViewRenderer();
    void addEditEntityEntry(QMouseEvent* event, QMenu& menu);

    bool scrollbars{false};
    bool cursor_hiding{false};
    bool selectCursor_hiding{false};
    bool invertZoomDirection{false};
    bool invertHorizontalScroll {false};
    bool invertVerticalScroll {false};

    // For auto panning by the cursor close to the view border
    void startAutoPanTimer(QMouseEvent *e);
    bool isAutoPan(QMouseEvent* e) const;
    struct AutoPanData;
    std::unique_ptr<AutoPanData> m_panData;
    struct UCSHighlightData;
    std::unique_ptr<UCSHighlightData> m_ucsHighlightData;
signals:
    void xbutton1_released();
    void gridStatusChanged(QString);
};

#endif
