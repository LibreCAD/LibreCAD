/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by 
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
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
class QG_GraphicView: public QWidget,
            public RS_GraphicView,
            public RS_LayerListListener,
    public RS_BlockListListener {
    Q_OBJECT

public:
    QG_GraphicView(QWidget* parent=0, const char* name=0, Qt::WFlags f=0);
    virtual ~QG_GraphicView();

    virtual int getWidth();
    virtual int getHeight();
	virtual void redraw(RS2::RedrawMethod method=RS2::RedrawAll);
    virtual void adjustOffsetControls();
    virtual void adjustZoomControls();
    virtual void setBackground(const RS_Color& bg);
    virtual void setMouseCursor(RS2::CursorType c);
        virtual void updateGridStatusWidget(const QString& text);

		virtual	QPixmap* getPixmapForView(QPixmap *pm);
		
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

protected:
    virtual void emulateMouseMoveEvent();
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
    QCursor* curCad;
    //! Delete mouse cursor
    QCursor* curDel;
    //! Select mouse cursor
    QCursor* curSelect;
    //! Magnifying glass mouse cursor
    QCursor* curMagnifier;
    //! Hand mouse cursor
    QCursor* curHand;
		
	// Used for buffering different paint layers
	QPixmap *PixmapLayer1;  // Used for grids and absolute 0
	QPixmap *PixmapLayer2;  // Used for teh actual CAD drawing
	QPixmap *PixmapLayer3;  // USed for crosshair and actionitems
	
	RS2::RedrawMethod redrawMethod;
		
};

#endif
