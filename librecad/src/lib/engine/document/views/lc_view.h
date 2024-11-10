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

#ifndef LC_VIEW_H
#define LC_VIEW_H

#include <QString>
#include "rs_vector.h"
#include "lc_ucs.h"

class LC_View
{
public:
    LC_View();

    explicit LC_View(const QString &name);
    QString getName()const {return name;}
    void setName(const QString& name);
    void setCenter(RS_Vector c);
    RS_Vector getCenter() const ;
    void setSize(RS_Vector s);
    RS_Vector getSize() const ;
    void setTargetPoint(RS_Vector p);
    RS_Vector getTargetPoint() const ;
    void setViewDirection(RS_Vector dir);
    const RS_Vector getViewDirection() const;
    void setLensLen(double d);
    double getLensLen() const;
    void setCameraPlottable(bool b);
    bool isCameraPlottable() const ;
    bool isHasUCS() const;
    void setRenderMode(int i) {renderMode = i;};
    long getRenderMode() const {return renderMode;};
    void setBackClippingPlaneOffset(double d);
    double getBackClippingPlaneOffset() const ;
    void setFrontClippingPlaneOffset(double d);
    double getFrontClippingPlaneOffset() const ;
    void setTwistAngle(double d);
    double getTwistAngle() const ;
    void setFlags(int i);
    int getFlags() const;
    void setViewMode(int i);
    int getViewMode() const;
    void setUCS(LC_UCS *pUcs);
    LC_UCS* getUCS() const;

    bool isForPaperView() {return  flags & 1;}
    void setForPaperView(bool forPaper) {
        if (forPaper) {
            flags |= 1;
        }
        else{
            flags &= ~1;
        }
    }

protected:
    QString name;
    bool cameraPlottable = false;
    int flags = 0;
    int viewMode = 0;
    double lensLen = 0.0;
    double twistAngle = 0.0;
    double backClippingPlaneOffset = 0.0;
    double frontClippingPlaneOffset = 0.0;
    unsigned int renderMode {0};
    RS_Vector center{false};
    RS_Vector size{false};
    RS_Vector targetPoint{false};
    RS_Vector viewDirection{false};
    LC_UCS* ucs {nullptr};
};

#endif // LC_VIEW_H
