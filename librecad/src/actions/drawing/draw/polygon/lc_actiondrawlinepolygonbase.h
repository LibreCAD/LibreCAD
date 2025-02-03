/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2024 LibreCAD.org
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

#ifndef LC_ACTIONDRAWLINEPOLYGONBASE_H
#define LC_ACTIONDRAWLINEPOLYGONBASE_H

#include "rs_previewactioninterface.h"

class LC_ActionDrawLinePolygonBase:public RS_PreviewActionInterface{
    Q_OBJECT

public:

    LC_ActionDrawLinePolygonBase(const char *name, RS_EntityContainer &container, RS_GraphicView &graphicView, RS2::ActionType actionType);
    ~LC_ActionDrawLinePolygonBase() override;

    QStringList getAvailableCommands() override;
    int getNumber() const{return number;}
    void setNumber(int n) {number = n;}
    bool isPolyline() const {return createPolyline;};
    void setPolyline(bool value){ createPolyline = value;};
    bool isCornersRounded() const {return roundedCorners;};
    void setCornersRounded(bool value){roundedCorners = value;};
    double getRoundingRadius(){return roundingRadius;}
    void setRoundingRadius(double val){roundingRadius = val;}
    void updateMouseButtonHints() override;
protected:
    /** Number of edges. */
    int number = 0;

    enum Status {
        SetPoint1,
        SetPoint2,
        SetNumber,
        SetRadius
    };

    struct Points {
        RS_Vector point1;
        RS_Vector point2;
    };

    struct PolygonInfo{
        RS_Vector centerPoint{false};
        double startingAngle = 0.0;
        double vertexRadius = 0.0;
        double innerRadius = 0.0;
    };

    std::unique_ptr<Points> pPoints;

/** Last status before entering text. */
    Status lastStatus = SetPoint1;

    bool createPolyline = false;
    bool roundedCorners = false;
    double roundingRadius = 0.0;

    bool completeActionOnTrigger = false;

    LC_ActionOptionsWidget* createOptionsWidget() override;
    RS2::CursorType doGetMouseCursor(int status) override;
    bool doProcessCommand(int status, const QString &command) override;
    void onMouseRightButtonRelease(int status, LC_MouseEvent *e) override;
    void onMouseLeftButtonRelease(int status, LC_MouseEvent *e) override;
    void onMouseMoveEvent(int status, LC_MouseEvent *event) override;
    void onCoordinateEvent(int status, bool isZero, const RS_Vector &pos) override;
    virtual QString getPoint1Hint() const;
    virtual QString getPoint2Hint() const = 0;
    void createPolygonPreview(const RS_Vector& mouse);
    virtual void previewAdditionalReferences([[maybe_unused]]const RS_Vector &mouse) {};
    virtual void preparePolygonInfo([[maybe_unused]]PolygonInfo &polygonInfo, [[maybe_unused]]const RS_Vector &snap){};
    RS_Polyline *createShapePolyline(PolygonInfo &polygonInfo, bool preview);
    void doTrigger() override;
};

#endif // LC_ACTIONDRAWLINEPOLYGONBASE_H
