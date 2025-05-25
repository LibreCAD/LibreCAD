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

#ifndef LC_TOLERANCE_H
#define LC_TOLERANCE_H

#include "rs_color.h"
#include "rs_entitycontainer.h"

struct RS_TextData;

struct LC_ToleranceData {
    LC_ToleranceData(const RS_Vector& m_insertion_point, const RS_Vector& m_direction_vector,
        const QString& m_text_code, const QString& m_dim_style_name)
        : m_insertionPoint{m_insertion_point},
          m_directionVector{m_direction_vector},
          m_textCode{m_text_code},
          m_dimStyleName{m_dim_style_name} {
    }

    ~LC_ToleranceData();

    RS_Vector m_insertionPoint;
    RS_Vector m_directionVector;
    QString m_textCode;
    QString m_dimStyleName;
};

std::ostream& operator <<(std::ostream& os, const LC_ToleranceData& dd);

class LC_Tolerance:public RS_EntityContainer{
public:
    LC_Tolerance(RS_EntityContainer* parent,
                   const LC_ToleranceData& d);

    ~LC_Tolerance() override = default;

    RS_Entity* clone() const override;

    RS2::EntityType rtti() const override {
        return RS2::EntityTolerance;
    }

    void update() override;
    void doUpdateDim();

    LC_ToleranceData getData() const {return m_toleranceData;}

    QString getTextCode() const {return m_toleranceData.m_textCode;}
    RS_Vector getInsertPoint() const {return m_toleranceData.m_insertionPoint;}

    RS_VectorSolutions getRefPoints() const override;
    void move(const RS_Vector& offset) override;
    void rotate(const RS_Vector& center, double angle) override;
    void rotate(const RS_Vector& center, const RS_Vector& angleVector) override;
    void scale(const RS_Vector& center, const RS_Vector& factor) override;
    void mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) override;
    void moveRef(const RS_Vector& ref, const RS_Vector& offset) override;

    friend std::ostream& operator <<(std::ostream& os, const LC_Tolerance& d);
protected:
    void doUpdate();
    QList<QStringList> getFields() const;
    double getTextHeight();
    double getGraphicVariable(const QString& key, double defMM, int code);
    double getDimtxt(bool scale = true);
    void setDimtxt(double f);
    double getGeneralScale();
    double getDimscale();
    QString getTextStyle();
    RS_Color getTextColor();
    RS_Pen getPenForText();
    RS_Color getDimensionLineColor();
    RS_Pen getPenForLines();
    void createTextLabels(QList<QList<double>>& divisions);
    void createFrameLines(QList<QList<double>>& divisions);
    RS_Line* addDimComponentLine(RS_Vector start, RS_Vector end, const RS_Pen& pen);
    void addDimComponentEntity(RS_Entity* en, const RS_Pen& pen);
    mutable bool joinFirstField;
private:
    LC_ToleranceData m_toleranceData;
    double dimscale = 1.0; // fixme - sand
    double dimtxt;
};

#endif // LC_TOLERANCE_H
