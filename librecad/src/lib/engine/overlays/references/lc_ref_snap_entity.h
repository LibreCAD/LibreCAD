/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2026 LibreCAD.org
 * Copyright (C) 2026 sand1024
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 * ********************************************************************************
 */

#ifndef LC_REFSNAPENTITY_H
#define LC_REFSNAPENTITY_H

#include "rs.h"
#include "rs_math.h"
#include "rs_painter.h"
#include "rs_vector.h"

struct RefSnapInfo {
    void update(const RefSnapInfo& other) {
        guideType = other.guideType;
        strict = other.strict;
        active = other.active;
        nearestPoint = other.nearestPoint;
        refPoint = other.refPoint;
        angle = other.angle;
        wcsBaseAngle = other.wcsBaseAngle;
        labelOffset = other.labelOffset;
    }

    void setupForPoint(const RS_Vector& point, double wcsAngle, RS2::VisualSnapGuideEntityType type) {
        nearestPoint = point;
        angle = wcsAngle;
        guideType = type;
    }

    void setAngle(double a) {
        angle = a;
        wcsBaseAngle = RS_Math::correctAngle0ToPi(a);
    }

    RS2::VisualSnapGuideEntityType guideType = RS2::VisualSnapGuideEntityType::VSNAP_NONE;
    RS_Vector nearestPoint{false};

    RS_Vector refPoint{false};
    double angle{0.0};
    double wcsBaseAngle{0.0};
    double labelOffset{0.5};
    bool strict{false};
    bool active{false};
};

class LC_RefSnapEntity {
public:
    bool isStrict() const {
        return m_snapInfo.strict;
    }

    void setStrict(bool v) {
        m_snapInfo.strict = v;
    }

    bool isActive() const {
        return m_snapInfo.active;
    }

    void setActive(bool v) {
        m_snapInfo.active = v;
    }

    RefSnapInfo& getRefSnapInfo() {
        return m_snapInfo;
    }

    RS2::VisualSnapGuideEntityType getGuideType() const{
         return m_snapInfo.guideType;
    }

    void updateSnapInfo(const RefSnapInfo& other) {
        m_snapInfo.update(other);
    }

    void setLabel(QString label) {
        m_labelString = label;
    }

    void setFont(QFont font) {
        m_font = font;
    }

    void setBaseLabelOffset(int offset) {
        m_baseLabelOffset = offset;
    }

    void drawMarker(RS_Painter* painter, const QFont& font, const RS_Vector& basePoint, int uiOffset, double offsetLevel,
                    const QString& markerLetter);

    void setupForPoint(const RS_Vector& nearestPoint, RS2::VisualSnapGuideEntityType guideType, bool isActive, double wcsAngle = RS_MINDOUBLE) {
        m_snapInfo.setupForPoint(nearestPoint, wcsAngle, guideType);
        m_snapInfo.active = isActive;
    }

    void setOriginalId(int id) {
        m_oridinalId = id;
    }

    int getOriginalId() const{
        return m_oridinalId;
    }

protected:
    RefSnapInfo m_snapInfo;
    QString m_labelString;
    QFont m_font;
    int m_baseLabelOffset = 50;
    int m_oridinalId = -1;
};

#endif
