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

#include "rs_vector.h"
#include "lc_ucs.h"

class LC_View{
public:
    LC_View();
    explicit LC_View(const QString &name);

    LC_View* clone();
    QString getName()const {return m_name;}
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
    void setRenderMode(int i) {m_renderMode = i;};
    long getRenderMode() const {return m_renderMode;};
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
    bool isForPaperView() {return  m_flags & 1;}
    void setForPaperView(bool forPaper) {
        if (forPaper) {
            m_flags |= 1;
        }
        else{
            m_flags &= ~1;
        }
    }

    static bool isValidName(QString &nameCandidate);

protected:
    QString m_name;
    bool m_cameraPlottable = false;
    int m_flags = 0;
    int m_viewMode = 0;
    double m_lensLen = 0.0;
    double m_twistAngle = 0.0;
    double m_backClippingPlaneOffset = 0.0;
    double m_frontClippingPlaneOffset = 0.0;
    unsigned int m_renderMode {0};
    RS_Vector m_center{false};
    RS_Vector m_size{false};
    RS_Vector m_targetPoint{false};
    RS_Vector m_viewDirection{false};
    LC_UCS* m_ucs {nullptr};
};

#endif // LC_VIEW_H
