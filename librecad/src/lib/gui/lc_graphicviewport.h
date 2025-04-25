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

#include "lc_coordinates_mapper.h"
#include "lc_overlaysmanager.h"

#include "rs.h"
#include "rs_vector.h"
class LC_OverlayDrawablesContainer;
class QString;
class RS_EntityContainer;
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

    void setContainer(RS_EntityContainer *c);
    RS_EntityContainer *getContainer() const {return container;}
    int getWidth() const {return m_width;}
    int getHeight() const {return m_height;}

    void setSize(int width, int height)
    {
        m_width = width;
        m_height = height;
    }
    RS_Vector getFactor() const {return factor;}

    bool isGridOn() const;
    bool isGridIsometric() const;
    void setIsoViewType(RS2::IsoGridViewType chType);
    RS2::IsoGridViewType getIsoViewType() const;
    void justSetOffsetAndFactor(int ox, int oy, double f);
    void setOffsetAndFactor(int ox, int oy, double f);
    int getOffsetX() const {return offsetX;}
    int getOffsetY() const {return offsetY;}
    void setOffsetX(int ox);
    void setOffsetY(int oy);
    void centerOffsetXandY(const RS_Vector& containerMin, const RS_Vector& constinerSize);
    void centerOffsetX(const RS_Vector& containerMin, const RS_Vector& constinerSize);
    void centerOffsetY(const RS_Vector& containerMin, const RS_Vector& containerSize);

    void setBorders(int left, int top, int right, int bottom);

    void freezeZoom(bool freeze) { zoomFrozen = freeze;}
    bool isZoomFrozen() const {return zoomFrozen;}
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
    void lockRelativeZero(bool lock) { relativeZeroLocked = lock;};
    /**
  * @return true if the position of the relative zero point is
  * locked.
  */
    bool isRelativeZeroLocked() const { return relativeZeroLocked;};
    /**
  * @return Relative zero coordinate.
  */
    RS_Vector const &getRelativeZero() const {return relativeZero;};
    void setRelativeZero(const RS_Vector &pos);
    void moveRelativeZero(const RS_Vector &pos);
    void setRelativeZeroHiddenState(bool isHidden) {hideRelativeZero = isHidden;};
    bool isRelativeZeroHidden() {return hideRelativeZero;};
    void markRelativeZero(){markedRelativeZero = relativeZero;}

    RS_Vector getMarkedRelativeZero(){return markedRelativeZero;}
    RS_Undoable* getRelativeZeroUndoable();

    RS_Vector getUCSViewLeftBottom() const;
    RS_Vector getUCSViewRightTop() const;
    void createUCS(const RS_Vector& origin, double angle);
    void extractUCS();
    RS_Vector snapGrid(const RS_Vector& coord) const;
    void applyUCS(LC_UCS* ucsToSet);
    LC_UCS* getCurrentUCS() const;
    void fillCurrentUCSInfo(RS_Vector& origin, double& xAxisDirection) const;

    double toGuiX(double ucxX) const {return ucxX * factor.x + offsetX;}
    double toGuiY(double ucsY) const {return -ucsY * factor.y + m_height - offsetY;}
    double toGuiDX(double ucsDX) const {return ucsDX * factor.x;}
    double toGuiDY(double ucsDY) const {return ucsDY * factor.y;}
    double toUcsX(int uiX) const {return (uiX - offsetX) / factor.x;}
    double toUcsY(int uiY) const {return -(uiY - m_height + offsetY) / factor.y;}
    double toUcsDX(int uiDX) const {return uiDX / factor.x;}
    double toUcsDY(int uiDy) const {return uiDy / factor.y;}
    RS_Vector toUCSFromGui(double uiX, double uiY) const { return RS_Vector(toUcsX(uiX), toUcsY(uiY));}
    RS_Vector toWorldFromUi(double uiX, double uiY) const {return toWorld(toUcsX(uiX), toUcsY(uiY));}
    void toUI(RS_Vector wcsCoordinate, double &uiX, double &uiY) const;
    double toAbsUCSAngle(double ucsBasisAngle);
    double toBasisUCSAngle(double ucsAbsAngle);


//    RS_Vector toUCS(const RS_Vector& v) const;
//    RS_Vector toWorld(const RS_Vector& v) const;

    void restoreView(LC_View *view);
    LC_View* createNamedView(QString name) const;
    void updateNamedView(LC_View* view) const;
    void initAfterDocumentOpen();
    void loadGridSettings();
    void setPrintPreview(bool pv) {printPreview = pv;}
    bool isPrintPreview() const {return printPreview;}
    void setPrinting(bool p) {printing = p;}
    bool isPrinting() const {return printing;}

    int getBorderLeft() const {return borderLeft;}
    int getBorderTop() const {return borderTop;}
    int getBorderRight() const {return borderRight;}
    int getBorderBottom() const {return borderBottom;}
    void loadSettings();
    RS_EntityContainer* getOverlayEntitiesContainer(RS2::OverlayGraphics overlayType) {return overlaysManager.getEntitiesContainer(overlayType);}
    LC_OverlayDrawablesContainer* getOverlaysDrawablesContainer(RS2::OverlayGraphics overlayType) {return overlaysManager.getDrawablesContainer(overlayType);}
    void clearOverlayEntitiesContainer(RS2::OverlayGraphics overlayType);
    void clearOverlayDrawablesContainer(RS2::OverlayGraphics overlayType);
    RS_Grid *getGrid() const;
    LC_OverlaysManager* getOverlaysManager() { return &overlaysManager;}
    bool isPanning() const {return panning;}
    void setPanning(bool state) {  panning = state;}
    RS_Graphic* getGraphic() {return graphic;}
    void addViewportListener(LC_GraphicViewPortListener* listener);
    void removeViewportListener(LC_GraphicViewPortListener* listener);
    void notifyChanged(){ fireRedrawNeeded();}

    bool areAnglesCounterClockwise();
    double getAnglesBaseAngle();

protected:
    RS_Vector factor{1., 1.};
    int offsetX = 0;
    int offsetY = 0;

    bool m_modifyOnZoom = true;

    /** Grid */
    std::unique_ptr<RS_Grid> grid;

    //circular buffer for saved views
    std::vector<std::tuple<int, int, RS_Vector> > savedViews;
    unsigned short savedViewIndex = 0;
    unsigned short savedViewCount = 0;


    enum UCSApplyingPolicy{
        ZoomAuto,
        PanOriginCenter,
        PanOriginLowerLeft
    };

    int m_ucsApplyingPolicy = UCSApplyingPolicy::ZoomAuto;

    RS_EntityContainer *container = nullptr; // Holds a pointer to all the entities
    RS_Graphic* graphic = nullptr; // may be extracted from container, yet have separate reference for performance reasons

    bool printPreview = false;
    bool printing = false;

    std::unique_ptr<QDateTime> previousViewTime;

    RS_Vector relativeZero = RS_Vector(0, 0, 0);
    RS_Vector markedRelativeZero = RS_Vector(0,0,0);
    bool relativeZeroLocked = false;

    LC_OverlaysManager overlaysManager;

    bool panning = false;
    bool hideRelativeZero = false; // fixme - should be dispatched from action

    QList<LC_GraphicViewPortListener*> viewportListeners;

    void doUpdateViewByGraphicView(LC_View *view) const;

    RS_Vector doSetUCS(const RS_Vector &origin, double angle, bool isometric, RS2::IsoGridViewType &isoType);
    void setUCS(const RS_Vector& origin, double angle, bool isometric, RS2::IsoGridViewType type);
    LC_UCS *createUCSEntity(const RS_Vector &origin, double angle, bool isometric, RS2::IsoGridViewType isoType) const;

    void doZoomAuto(const RS_Vector& min, const RS_Vector& max, bool axis, bool keepAspectRatio);

    void applyUCSAfterLoad();

    void fireViewportChanged();
    void fireUcsChanged(LC_UCS* ucs);
    void fireRedrawNeeded();
    void firePreviousZoomChanged(bool value);
    void fireRelativeZeroChanged(const RS_Vector &pos);


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

    void invalidateGrid();
private:
    void zoomViewPortImpl();

    int m_width = 0;
    int m_height = 0;
    int borderLeft = 0;
    int borderTop = 0;
    int borderRight = 0;
    int borderBottom = 0;
    bool zoomFrozen = false;
};

#endif // LC_GRAPHICVIEWPORT_H
