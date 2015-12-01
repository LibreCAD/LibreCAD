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

#include "rs_graphicview.h"
#include "rs_layerlistlistener.h"
#include "rs_blocklistlistener.h"

class QGridLayout;
class QLabel;
class QG_ScrollBar;

/**
 * This is the Qt implementation of a widget which can view a 
 * graphic. 
 *
 * Instances of this class can be linked to layer lists using
 * addLayerListListener().
 */
class QG_GraphicView:   public RS_GraphicView,
                        public RS_LayerListListener,
                        public RS_BlockListListener
{
    Q_OBJECT

public:
    QG_GraphicView(QWidget* parent = 0, Qt::WindowFlags f = 0, RS_Document* doc = 0);
    virtual ~QG_GraphicView();

	virtual int getWidth() const;
	virtual int getHeight() const;
	virtual void redraw(RS2::RedrawMethod method=RS2::RedrawAll);
    virtual void adjustOffsetControls();
    virtual void adjustZoomControls();
    virtual void setBackground(const RS_Color& bg);
    virtual void setMouseCursor(RS2::CursorType c);
    virtual void updateGridStatusWidget(const QString& text);

	virtual	void getPixmapForView(std::unique_ptr<QPixmap>& pm);
		
    // Methods from RS_LayerListListener Interface:
    virtual void layerEdited(RS_Layer*) {
        redraw(RS2::RedrawDrawing); 
    }
    virtual void layerRemoved(RS_Layer*) {
        redraw(RS2::RedrawDrawing); 
    }
    virtual void layerToggled(RS_Layer*) {
        redraw(RS2::RedrawDrawing); 
    }
    virtual void layerActivated(RS_Layer *);
    /**
     * @brief setOffset
     * @param ox, offset X
     * @param oy, offset Y
     */
    virtual void setOffset(int ox, int oy);
    /**
     * @brief getMousePosition() mouse position in widget coordinates
     * @return the cursor position in widget coordinates
     * returns the widget center, if cursor is not on the widget
     */
    virtual RS_Vector getMousePosition() const;

    void setAntiAliasing(bool state);
    void addScrollBars();

protected:
    virtual void mousePressEvent(QMouseEvent* e);
    virtual void mouseDoubleClickEvent(QMouseEvent* e);
    virtual void mouseReleaseEvent(QMouseEvent* e);
    virtual void mouseMoveEvent(QMouseEvent* e);
    virtual void tabletEvent(QTabletEvent* e);
    virtual void leaveEvent(QEvent*);
    virtual void enterEvent(QEvent*);
    virtual void focusInEvent(QFocusEvent*);
    virtual void focusOutEvent(QFocusEvent*);
    virtual void wheelEvent(QWheelEvent* e);
    virtual void keyPressEvent(QKeyEvent* e);
    virtual void keyReleaseEvent(QKeyEvent* e);

    virtual bool event(QEvent * e);

    void paintEvent(QPaintEvent *);
    virtual void resizeEvent(QResizeEvent* e);

private slots:
    void slotHScrolled(int value);
    void slotVScrolled(int value);

protected:
    //! Horizontal scrollbar.
    QG_ScrollBar* hScrollBar;
    //! Vertical scrollbar.
    QG_ScrollBar* vScrollBar;
    //! Layout used to fit in the view and the scrollbars.
    QGridLayout* layout;
	//! Label for grid spacing.
    QLabel* gridStatus;
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
		
	// Used for buffering different paint layers
	std::unique_ptr<QPixmap> PixmapLayer1;  // Used for grids and absolute 0
	std::unique_ptr<QPixmap> PixmapLayer2;  // Used for teh actual CAD drawing
	std::unique_ptr<QPixmap> PixmapLayer3;  // USed for crosshair and actionitems
	
	RS2::RedrawMethod redrawMethod;
		
    //! Keep tracks of if we are currently doing a high-resolution scrolling
    bool isSmoothScrolling;

private:
    bool antialiasing{false};
    bool hasScrollBars{false};
    bool cursor_hiding{false};

signals:
    void xbutton1_released();
};

#endif
