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

#include "rs_entitycontainer.h"
#include "rs_snapper.h"

#include <QDateTime>
#include <QMap>
#include <tuple>
#include <memory>
#include <QAction>


class QMouseEvent;
class QKeyEvent;
class RS_ActionInterface;
class RS_EventHandler;
class RS_CommandEvent;
class RS_Grid;
struct RS_LineTypePattern;


/**
 * This class is a common GUI interface for the graphic viewer
 * widget which has to be implementet by real GUI classes such
 * as the Qt graphical view.
 *
 * Note that this is just an interface used as a slot to
 * communicate with the LibreCAD from a GUI level.
 */
class RS_GraphicView : public QWidget
{
    Q_OBJECT

public:
	RS_GraphicView(QWidget * parent = 0, Qt::WindowFlags f = 0);
	virtual ~RS_GraphicView();

    void cleanUp();

    void set_action(QAction* q_action);

	/**
	 * @return Pointer to the graphic entity if the entity container
	 * connected to this view is a graphic and valid.
	 * NULL otherwise.
	 */
	RS_Graphic* getGraphic() const;

	/**
	 * \brief setDrawingMode Sets the drawing mode.
	 */
	void setDrawingMode(RS2::DrawingMode m) {
		drawingMode = m;
	}

	/**
	 * @return Current drawing mode.
	 */
    RS2::DrawingMode getDrawingMode() const {
		return drawingMode;
	}

	/**
	 * Activates or deactivates the delete mode.
	 */
	void setDeleteMode(bool m) {
		deleteMode = m;
	}

	/**
	 * @reval true Deleting instead of drawing.
		 *        false Normal drawing mode.
	 */
    bool getDeleteMode() const{
		return deleteMode;
	}

	/** This virtual method must be overwritten to return
	  the width of the widget the graphic is shown in */
	virtual int getWidth() const= 0;
	/** This virtual method must be overwritten to return
	  the height of the widget the graphic is shown in */
	virtual int getHeight() const= 0;
	/** This virtual method must be overwritten to redraw
	  the widget. */
	virtual void redraw(RS2::RedrawMethod method=RS2::RedrawAll) = 0;
	/** This virtual method must be overwritten and is then
	  called whenever the view changed */
    virtual void adjustOffsetControls() = 0;
	/** This virtual method must be overwritten and is then
	  called whenever the view changed */
    virtual void adjustZoomControls() = 0;

	/**
	 * Sets the background color. Note that applying the background
	 * color for the widget is up to the implementing class.
	 */
	virtual void setBackground(const RS_Color& bg);

	/**
		 * @return Current background color.
		 */
	RS_Color getBackground() const{
		return background;
	}

	/**
		 * @return Current foreground color.
		 */
	RS_Color getForeground() const{
		return foreground;
	}

	/**
		 * Sets the grid color.
		 */
	void setGridColor(const RS_Color& c) {
		gridColor = c;
	}

	/**
		 * Sets the meta grid color.
		 */
	void setMetaGridColor(const RS_Color& c) {
		metaGridColor = c;
	}

	/**
		 * Sets the selection color.
		 */
	void setSelectedColor(const RS_Color& c) {
		selectedColor = c;
	}

	/**
		 * Sets the highlight color.
		 */
	void setHighlightedColor(const RS_Color& c) {
		highlightedColor = c;
	}

	/**
		 * Sets the color for the first handle (start vertex)
		 */
	void setStartHandleColor(const RS_Color& c) {
		startHandleColor = c;
	}

	/**
		 * Sets the color for handles, that are neither start nor end vertices
		 */
	void setHandleColor(const RS_Color& c) {
		handleColor = c;
	}

	/**
		 * Sets the color for the last handle (end vertex)
		 */
	void setEndHandleColor(const RS_Color& c) {
		endHandleColor = c;
	}

	/**
	 * This virtual method can be overwritten to set the mouse
	 * cursor to the given type.
	 */
    virtual void setMouseCursor(RS2::CursorType /*c*/) = 0;

	void setContainer(RS_EntityContainer* container);
	RS_EntityContainer* getContainer() const;
	void setFactor(double f);
	void setFactorX(double f);
	void setFactorY(double f);
	RS_Vector getFactor() const;
	/**
	 * @brief setOffset
	 * @param ox, offset X
	 * @param oy, offset Y
	 */
	virtual void setOffset(int ox, int oy);
	void setOffsetX(int ox);
	void setOffsetY(int oy);
	int getOffsetX() const;
	int getOffsetY() const;
	void centerOffsetX();
	void centerOffsetY();
	void centerX(double x);
	void centerY(double y);
	/**
	 * Sets a fixed border in pixel around the graphic. This border
	 * specifies how far the user can scroll outside the graphic
	 * area.
	 */
	void setBorders(int left, int top, int right, int bottom);

	int getBorderLeft() const;
	int getBorderTop() const;
	int getBorderRight() const;
	int getBorderBottom() const;

	void freezeZoom(bool freeze);
	bool isZoomFrozen() const;

	void setDefaultAction(RS_ActionInterface* action);
	RS_ActionInterface*  getDefaultAction();
    void setCurrentAction(RS_ActionInterface* action);
	RS_ActionInterface* getCurrentAction();

	void killSelectActions();
	void killAllActions();

	void back();
	void enter();

	void commandEvent(RS_CommandEvent* e);
	void enableCoordinateInput();
	void disableCoordinateInput();

	virtual void zoomIn(double f=1.5, const RS_Vector& center=RS_Vector(false));
	virtual void zoomInX(double f=1.5);
	virtual void zoomInY(double f=1.5);
	virtual void zoomOut(double f=1.5, const RS_Vector& center=RS_Vector(false));
	virtual void zoomOutX(double f=1.5);
	virtual void zoomOutY(double f=1.5);
	virtual void zoomAuto(bool axis=true, bool keepAspectRatio=true);
	virtual void zoomAutoY(bool axis=true);
	virtual void zoomPrevious();
	virtual void saveView();
	virtual void restoreView();
	virtual void zoomWindow(RS_Vector v1, RS_Vector v2,
							bool keepAspectRatio=true);
	//virtual void zoomPan(RS_Vector v1);
	virtual void zoomPan(int dx, int dy);
	virtual void zoomScroll(RS2::Direction direction);
	virtual void zoomPage();

	virtual void drawWindow_DEPRECATED(RS_Vector v1, RS_Vector v2);
	virtual void drawLayer1(RS_Painter *painter);
	virtual void drawLayer2(RS_Painter *painter);
	virtual void drawLayer3(RS_Painter *painter);
	virtual void deleteEntity(RS_Entity* e);
	virtual void drawEntity(RS_Painter *painter, RS_Entity* e, double& patternOffset);
	virtual void drawEntity(RS_Painter *painter, RS_Entity* e);
	virtual void drawEntity(RS_Entity* e, double& patternOffset);
	virtual void drawEntity(RS_Entity* e);
	virtual void drawEntityPlain(RS_Painter *painter, RS_Entity* e);
	virtual void drawEntityPlain(RS_Painter *painter, RS_Entity* e, double& patternOffset);
	virtual void setPenForEntity(RS_Painter *painter, RS_Entity* e );
    virtual RS_Vector getMousePosition() const = 0;

	virtual const RS_LineTypePattern* getPattern(RS2::LineType t);

	virtual void drawAbsoluteZero(RS_Painter *painter);
	virtual void drawRelativeZero(RS_Painter *painter);
	virtual void drawPaper(RS_Painter *painter);
	virtual void drawGrid(RS_Painter *painter);
	virtual void drawMetaGrid(RS_Painter *painter);
	virtual void drawOverlay(RS_Painter *painter);

	RS_Grid* getGrid() const;
    virtual void updateGridStatusWidget(const QString& /*text*/) = 0;

	void setDefaultSnapMode(RS_SnapMode sm);
	RS_SnapMode getDefaultSnapMode() const;
	void setSnapRestriction(RS2::SnapRestriction sr);
	RS2::SnapRestriction getSnapRestriction() const;

	bool isGridOn() const;
	bool isGridIsometric() const;
	void setCrosshairType(RS2::CrosshairType chType);
	RS2::CrosshairType getCrosshairType() const;

	RS_Vector toGui(RS_Vector v) const;
	double toGuiX(double x) const;
	double toGuiY(double y) const;
	double toGuiDX(double d) const;
	double toGuiDY(double d) const;

	RS_Vector toGraph(RS_Vector v) const;
	RS_Vector toGraph(int x, int y) const;
	double toGraphX(int x) const;
	double toGraphY(int y) const;
	double toGraphDX(int d) const;
	double toGraphDY(int d) const;

	/**
		 * (Un-)Locks the position of the relative zero.
		 *
		 * @param lock true: lock, false: unlock
		 */
	void lockRelativeZero(bool lock);

	/**
		 * @return true if the position of the realtive zero point is
		 * locked.
		 */
	bool isRelativeZeroLocked() const;

	/**
		 * @return Relative zero coordinate.
		 */
	RS_Vector const& getRelativeZero() const;

	void setRelativeZero(const RS_Vector& pos);
	void moveRelativeZero(const RS_Vector& pos);

	RS_EventHandler* getEventHandler() const;

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
		 * @retval true This is a a graphic view for printing.
		 * @retval false setSnapOtherwise.
		 */
	bool isPrinting() const;

	/**
		 * @retval true Draft mode is on for this view (all lines with 1 pixel / no style scaling).
		 * @retval false Otherwise.
		 */
	bool isDraftMode() const;

	void setDraftMode(bool dm);
	bool isCleanUp(void) const;

	virtual RS_EntityContainer* getOverlayContainer(RS2::OverlayGraphics position);

protected:

    RS_EntityContainer* container{nullptr}; // Holds a pointer to all the enties
	std::unique_ptr<RS_EventHandler> eventHandler;

	/** background color (any color) */
	RS_Color background;
	/** foreground color (black or white) */
	RS_Color foreground;
	/** grid color */
	RS_Color gridColor;
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
	/** Grid */
	std::unique_ptr<RS_Grid> grid;
	/**
		 * Current default snap mode for this graphic view. Used for new
		 * actions.
		 */
	RS_SnapMode defaultSnapMode;
	/**
		 * Current default snap restriction for this graphic view. Used for new
		 * actions.
		 */
	RS2::SnapRestriction defaultSnapRes;

	RS2::DrawingMode drawingMode;

	/**
		 * Delete mode. If true, all drawing actions will delete in background color
		 * instead.
		 */
	bool deleteMode=false;

    QList<QAction*> recent_actions;

private:

	bool zoomFrozen=false;
	bool draftMode=false;

	RS_Vector factor=RS_Vector(1.,1.);
	int offsetX=0;
	int offsetY=0;

	//circular buffer for saved views
	std::vector<std::tuple<int, int, RS_Vector> > savedViews;
	unsigned short savedViewIndex=0;
	unsigned short savedViewCount=0;
	QDateTime previousViewTime;

	int borderLeft=0;
	int borderTop=0;
	int borderRight=0;
	int borderBottom=0;

	RS_Vector relativeZero{false};
	bool relativeZeroLocked=false;
	//! Print preview flag
	bool printPreview=false;
	//! Active when printing only:
	bool printing=false;

	// Map that will be used for overlaying additional items on top of the main CAD drawing
	QMap<int, RS_EntityContainer *> overlayEntities;
	/** if true, graphicView is under cleanup */
	bool m_bIsCleanUp=false;

signals:
    void relative_zero_changed(const RS_Vector&);
    void previous_zoom_state(bool);
};

#endif
