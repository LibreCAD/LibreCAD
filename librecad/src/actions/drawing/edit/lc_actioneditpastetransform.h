/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2024 LibreCAD.org
 Copyright (C) 2024 sand1024

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

#ifndef LC_ACTIONEDITPASTETRANSFORM_H
#define LC_ACTIONEDITPASTETRANSFORM_H

#include "rs_previewactioninterface.h"

class LC_ActionEditPasteTransform :public RS_PreviewActionInterface{
Q_OBJECT
public:
    LC_ActionEditPasteTransform(RS_EntityContainer& container,
                                RS_GraphicView& graphicView);
    void mouseMoveEvent(QMouseEvent *event) override;
    void init(int status) override;
    void trigger() override;
    void setAngle(double value);
    double getFactor() const;
    void setFactor(double factor);
    bool isArrayCreated() const;
    void setArrayCreated(bool arrayCreated);
    int getArrayXCount() const;
    void setArrayXCount(int arrayXCount);
    int getArrayYCount() const;
    void setArrayYCount(int arrayYCount);
    double getArraySpacingX() const;
    void setArraySpacingX(double arraySpacing);
    double getArraySpacingY() const;
    void setArraySpacingY(double arraySpacing);
    double getArrayAngle() const;
    void setArrayAngle(double arrayAngle);
    bool isSameAngles() const {return sameAngles;}
    void setSameAngles(bool val) {sameAngles = val;}
    double getAngle() const;
protected:
    enum Status{
        SetReferencePoint
    };

    std::unique_ptr<RS_Vector> referencePoint;
    bool invokedWithControl = false;
    bool sameAngles = false;

    struct PasteData{
        double angle = 0.0;
        double factor = 1.0;
        bool arrayCreated = false;
        int arrayXCount = 1;
        int arrayYCount = 1;
        RS_Vector arraySpacing = RS_Vector(false);
        double arrayAngle = 0.0;
    };

    std::unique_ptr<PasteData> data;

    RS2::CursorType doGetMouseCursor(int status) override;
    void onMouseLeftButtonRelease(int status, QMouseEvent *e) override;
    void onMouseRightButtonRelease(int status, QMouseEvent *e) override;
    LC_ActionOptionsWidget *createOptionsWidget() override;
    void previewMultipleReferencePoints();
    void updateMouseButtonHints() override;
    void onCoordinateEvent(int status, bool isZero, const RS_Vector &pos) override;
};
#endif // LC_ACTIONEDITPASTETRANSFORM_H
