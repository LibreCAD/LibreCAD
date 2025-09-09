/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2025 LibreCAD.org
 * Copyright (C) 2025 sand1024
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

#ifndef LC_DIMORIDINAL_H
#define LC_DIMORIDINAL_H

#include "rs_dimension.h"

struct LC_DimOrdinateData {
    LC_DimOrdinateData(const RS_Vector& feature_point, const RS_Vector& leader_end_point, bool ordinateX)
        : featurePoint{feature_point},
          leaderEndPoint{leader_end_point},
          ordinateForX{ordinateX} {
    }

    LC_DimOrdinateData(const LC_DimOrdinateData& other)
      :featurePoint{other.featurePoint}, leaderEndPoint{other.leaderEndPoint}, ordinateForX{other.ordinateForX}{
    }

    ~LC_DimOrdinateData();

    RS_Vector featurePoint{false};
    RS_Vector leaderEndPoint{false};

    bool ordinateForX{false};
};

std::ostream& operator <<(std::ostream& os, const LC_DimOrdinateData& dd);

class LC_DimOrdinate : public RS_Dimension {
public:
    LC_DimOrdinate(RS_EntityContainer* parent,
                   const RS_DimensionData& d,
                   const LC_DimOrdinateData& ed);
    LC_DimOrdinate(const LC_DimOrdinate& other);

    ~LC_DimOrdinate() override = default;

    RS_Entity* clone() const override;

    RS2::EntityType rtti() const override {
        return RS2::EntityDimOrdinate;
    }

    LC_DimOrdinateData getEData() const {
        return m_dimOrdinateData;
    }

    bool isForXDirection() const {
        return m_dimOrdinateData.ordinateForX;
    }

    void setForXDirection(bool value) {
        m_dimOrdinateData.ordinateForX = value;
    }

    RS_VectorSolutions getRefPoints() const override;

    RS_Vector getFeaturePoint() const;
    RS_Vector getLeaderEndPoint() const;

    void move(const RS_Vector& offset) override;
    void rotate(const RS_Vector& center, double angle) override;
    void rotate(const RS_Vector& center, const RS_Vector& angleVector) override;
    void scale(const RS_Vector& center, const RS_Vector& factor) override;
    void mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) override;
    void moveRef(const RS_Vector& ref, const RS_Vector& offset) override;
    QString getMeasuredLabel() override;
    friend std::ostream& operator <<(std::ostream& os, const LC_DimOrdinate& d);
protected:
    void determineKneesPositions(const RS_Vector& featurePoint, const RS_Vector& leaderEndPoint, RS_Vector& kneeOne,
                                 RS_Vector& kneeTwo, RS_Vector& textOffsetV);
    void doUpdateDim() override;
    void adjustExtensionLineIfFixLength(RS_Line* extLine1, RS_Line* extLine2, bool addDimExe) const;
private:
    LC_DimOrdinateData m_dimOrdinateData;
};



#endif // LC_DIMORIDINAL_H
