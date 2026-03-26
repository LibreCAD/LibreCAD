/*******************************************************************************
*
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2025 LibreCAD.org
 Copyright (C) 2025 sand1024

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 ******************************************************************************/
#ifndef LC_GRAPHICVIEWPORT_H
#define LC_GRAPHICVIEWPORT_H

#include <QList>
#include <memory>

#include "dxf_format.h"
#include "lc_coordinates_mapper.h"
#include "lc_formatter.h"
#include "lc_overlaysmanager.h"
#include "rs.h"
#include "rs_entity.h"
#include "rs_vector.h"

class LC_OverlayDrawablesContainer;
class QString;
class RS_Document;
class RS_Graphic;
class LC_UCS;
class RS_Grid;
class LC_View;
class QDateTime;
class RS_Undoable;
class LC_GraphicViewPortListener;

class LC_GraphicViewport: public LC_CoordinatesMapper{
public:
    LC_GraphicViewport();
    virtual ~LC_GraphicViewport();

    void setDocument(RS_Document *c);
    RS_Document *getDocument() const {return m_document;}
    int getWidth() const {return m_width;}
    int getHeight() const {return m_height;}

    void setSize(const int width, const int height)
    {
        m_width = width;
        m_height = height;
    }
    RS_Vector getFactor() const {return m_factor;}

    bool isGridOn() const;
    bool isGridIsometric() const;
    void setIsoViewType(RS2::IsoGridViewType chType) const;
    RS2::IsoGridViewType getIsoViewType() const;
    void justSetOffsetAndFactor(int ox, int oy, double f);
    void setOffsetAndFactor(int ox, int oy, double f);
    int getOffsetX() const {return m_offsetX;}
    int getOffsetY() const {return m_offsetY;}
    void setOffsetX(int ox);
    void setOffsetY(int oy);
    void centerOffsetXandY(const RS_Vector& containerMin, const RS_Vector& containerSize);
    void centerOffsetX(const RS_Vector& containerMin, const RS_Vector& containerSize);
    void centerOffsetY(const RS_Vector& containerMin, const RS_Vector& containerSize);

    void setBorders(int left, int top, int right, int bottom);

    void freezeZoom(const bool freeze) { m_zoomFrozen = freeze;}
    bool isZoomFrozen() const {return m_zoomFrozen;}
    void zoomIn(double f = 1.5, const RS_Vector &center = RS_Vector(false));
    void zoomOut(double f = 1.5, const RS_Vector &center = RS_Vector(false));
    void zoomAuto(bool axis = true, bool keepAspectRatio = true);
    void zoomAutoEnsurePointsIncluded(const RS_Vector &wcsP1, const RS_Vector &wcsP2, const RS_Vector &wcsP3);
    void zoomPrevious();
    void zoomPan(int dx, int dy);
    void zoomScroll(RS2::Direction direction);
    void zoomPage();
    void zoomPageEx();
    void zoomWindow(RS_Vector v1, RS_Vector v2,bool keepAspectRatio = true);

    /**
* (Un-)Locks the position of the relative zero.
*
* @param lock true: lock, false: unlock
*/
    void lockRelativeZero(const bool lock) { m_relativeZeroLocked = lock;}
    /**
  * @return true if the position of the relative zero point is
  * locked.
  */
    bool isRelativeZeroLocked() const { return m_relativeZeroLocked;}
    /**
  * @return Relative zero coordinate.
  */
    const RS_Vector&getRelativeZero() const {return m_relativeZero;}
    void setRelativeZero(const RS_Vector &pos);
    void moveRelativeZero(const RS_Vector &pos);
    void setRelativeZeroHiddenState(const bool isHidden) {m_hideRelativeZero = isHidden;}
    bool isRelativeZeroHidden() const {return m_hideRelativeZero;}
    void markRelativeZero(){m_markedRelativeZero = m_relativeZero;}

    RS_Vector getMarkedRelativeZero() const {return m_markedRelativeZero;}
    RS_Undoable* getRelativeZeroUndoable();

    RS_Vector getUCSViewLeftBottom() const;
    RS_Vector getUCSViewRightTop() const;
    void createUCS(const RS_Vector& origin, double angle);
    void extractUCS();
    RS_Vector snapGrid(const RS_Vector& coord) const;
    void applyUCS(LC_UCS* ucsToSet);
    LC_UCS* getCurrentUCS() const;
    RS_Vector snapGrid(const RS_Vector& coord, RS_Entity* entity) const;
    RS_Vector snapGrid(const RS_Vector& coord, const RS_Vector& rayStart, const RS_Vector& rayEnd) const;
    void fillCurrentUCSInfo(RS_Vector& origin, double& xAxisDirection) const;

    double toGuiX(const double ucxX) const {return ucxX * m_factor.x + m_offsetX;}
    double toGuiY(const double ucsY) const {return -ucsY * m_factor.y + m_height - m_offsetY;}
    double toGuiDX(const double ucsDX) const {return ucsDX * m_factor.x;}
    double toGuiDY(const double ucsDY) const {return ucsDY * m_factor.y;}
    double toUcsX(const int uiX) const {return (uiX - m_offsetX) / m_factor.x;}
    double toUcsY(const int uiY) const {return -(uiY - m_height + m_offsetY) / m_factor.y;}
    double toUcsDX(const int uiDX) const {return uiDX / m_factor.x;}
    double toUcsDY(const int uiDy) const {return uiDy / m_factor.y;}
    RS_Vector toUCSFromGui(const double uiX, const double uiY) const { return RS_Vector(toUcsX(uiX), toUcsY(uiY));}
    RS_Vector toWorldFromUi(const double uiX, const double uiY) const {return toWorld(toUcsX(uiX), toUcsY(uiY));}
    void toUI(const RS_Vector& wcsCoordinate, double &uiX, double &uiY) const;
    double toAbsUCSAngle(double ucsBasisAngle);
    double toBasisUCSAngle(double ucsAbsAngle);

    LC_Formatter* getFormatter() const {return m_formatter.get();}


//    RS_Vector toUCS(const RS_Vector& v) const;
//    RS_Vector toWorld(const RS_Vector& v) const;

    void restoreView(const LC_View *view);
    LC_View* createNamedView(const QString& name) const;
    void updateNamedView(LC_View* view) const;
    void initAfterDocumentOpen();
    void loadGridSettings() const;
    void setPrintPreview(const bool pv) {m_printPreview = pv;}
    bool isPrintPreview() const {return m_printPreview;}
    void setPrinting(const bool p) {m_printing = p;}
    bool isPrinting() const {return m_printing;}

    int getBorderLeft() const {return m_borderLeft;}
    int getBorderTop() const {return m_borderTop;}
    int getBorderRight() const {return m_borderRight;}
    int getBorderBottom() const {return m_borderBottom;}
    void loadSettings();
    RS_EntityContainer* getOverlayEntitiesContainer(const RS2::OverlayGraphics overlayType) {return m_overlaysManager.getEntitiesContainer(overlayType);}
    LC_OverlayDrawablesContainer* getOverlaysDrawablesContainer(const RS2::OverlayGraphics overlayType) {return m_overlaysManager.getDrawablesContainer(overlayType);}
    void clearOverlayEntitiesContainer(RS2::OverlayGraphics overlayType) const;
    void clearOverlayDrawablesContainer(RS2::OverlayGraphics overlayType) const;
    RS_Grid *getGrid() const;
    LC_OverlaysManager* getOverlaysManager() { return &m_overlaysManager;}
    bool isPanning() const {return m_panning;}
    void setPanning(const bool state) {  m_panning = state;}
    RS_Graphic* getGraphic() const {return m_graphic;}
    void addViewportListener(LC_GraphicViewPortListener* listener);
    void removeViewportListener(LC_GraphicViewPortListener* listener);
    void notifyChanged(const RS2::RedrawMethod method = RS2::RedrawDrawing) const { fireRedrawNeeded(method);}

    bool areAnglesCounterClockwise() const;
    double getAnglesBaseAngle() const;

    void highlightLocation(const RS_Vector& vector);
    void clearLocationsHighlight();

protected:
    RS_Vector m_factor{1., 1.};
    int m_offsetX = 0;
    int m_offsetY = 0;

    bool m_modifyOnZoom = true;

    double m_refPointSize = 2.0;
    int m_refPointMode = DXF_FORMAT_PDMode_EncloseSquare(DXF_FORMAT_PDMode_CentreDot);

    /** Grid */
    std::unique_ptr<RS_Grid> m_grid;

    //circular buffer for saved views
    std::vector<std::tuple<int, int, RS_Vector> > m_savedViews;
    unsigned short m_savedViewIndex = 0;
    unsigned short m_savedViewCount = 0;


    enum UCSApplyingPolicy{
        ZoomAuto,
        PanOriginCenter,
        PanOriginLowerLeft
    };

    int m_ucsApplyingPolicy = UCSApplyingPolicy::ZoomAuto;

    RS_Document *m_document = nullptr; // Holds a pointer to all the entities
    RS_Graphic* m_graphic = nullptr; // may be extracted from document, yet have separate reference for performance reasons

    bool m_printPreview = false;
    bool m_printing = false;

    std::unique_ptr<QDateTime> m_previousViewTime;

    RS_Vector m_relativeZero = RS_Vector(0, 0, 0);
    RS_Vector m_markedRelativeZero = RS_Vector(0,0,0);
    bool m_relativeZeroLocked = false;

    LC_OverlaysManager m_overlaysManager;

    bool m_panning = false;
    bool m_hideRelativeZero = false; // fixme - should be dispatched from action

    QList<LC_GraphicViewPortListener*> m_viewportListeners;

    std::unique_ptr<LC_Formatter> m_formatter;

    void doUpdateViewByGraphicView(LC_View *view) const;

    RS_Vector doSetUCS(const RS_Vector &origin, double angle, bool isometric, RS2::IsoGridViewType isoType);
    void setUCS(const RS_Vector& origin, double angle, bool isometric, RS2::IsoGridViewType isoType);
    LC_UCS *createUCSEntity(const RS_Vector &origin, double angle, bool isometric, RS2::IsoGridViewType isoType) const;

    void doZoomAuto(const RS_Vector& min, const RS_Vector& max, bool axis, bool keepAspectRatio);

    void applyUCSAfterLoad();

    void fireViewportChanged() const;
    void fireUcsChanged(LC_UCS* ucs) const;
    void fireRedrawNeeded(RS2::RedrawMethod method) const;
    void firePreviousZoomChanged(bool value);
    void fireRelativeZeroChanged(const RS_Vector &pos) const;

    void setOffset(int ox, int oy);
    void setFactor(double f);

    void saveView();
    void restoreView(); // fixme - check where it's called

    // tmp - method to review and remove if they are not needed
    void centerX(double x);
    void centerY(double y);


    void zoomInX(double f = 1.5);
    void zoomInY(double f = 1.5);
    void zoomOutX(double f = 1.5);
    void zoomOutY(double f = 1.5);
    void zoomAutoY(bool axis = true);
    void setFactorX(double f);
    void setFactorY(double f);

    void setGraphic(RS_Graphic* g);

    void invalidateGrid() const;
private:
    int m_width = 0;
    int m_height = 0;
    int m_borderLeft = 0;
    int m_borderTop = 0;
    int m_borderRight = 0;
    int m_borderBottom = 0;
    bool m_zoomFrozen = false;
};

#endif
