/****************************************************************************
**
* Action performs pasting of copied entities with transformation of the them

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
**********************************************************************/
#ifndef LC_ACTIONEDITPASTETRANSFORM_H
#define LC_ACTIONEDITPASTETRANSFORM_H

#include "rs_previewactioninterface.h"

class LC_ActionEditPasteTransform :public RS_PreviewActionInterface{
Q_OBJECT
   enum Status{
       SetReferencePoint
};
public:
    LC_ActionEditPasteTransform(RS_EntityContainer& container,
                                RS_GraphicView& graphicView);
    void mouseMoveEvent(QMouseEvent *event) override;
    void updateMouseButtonHints() override;
    void trigger() override;

protected:
    std::unique_ptr<RS_Vector> referencePoint;
    bool invokedWithControl = false;

    struct PasteData{
        double angle = 0.0;
        double factor = 1.0;
        bool arrayCreated = false;
        int arrayXCount = 1;
        int arrayYCount = 1;
        RS_Vector arraySpacing = RS_Vector(false);
        bool  rotateArray = false;
        double arrayAngle = 0.0;
    };

    std::unique_ptr<PasteData> data;

    RS2::CursorType doGetMouseCursor(int status) override;
    void mouseLeftButtonReleaseEvent(int status, QMouseEvent *e) override;
    void mouseRightButtonReleaseEvent(int status, QMouseEvent *e) override;
    LC_ActionOptionsWidget *createOptionsWidget() override;

public:
    void coordinateEvent(RS_CoordinateEvent *event) override;
    double getAngle() const;
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
    bool isRotateArray() const;
    void setRotateArray(bool rotateArray);
    double getArrayAngle() const;
    void setArrayAngle(double arrayAngle);

    void previewMultipleReferencePoints();

    void init(int status) override;
};

#endif // LC_ACTIONEDITPASTETRANSFORM_H
