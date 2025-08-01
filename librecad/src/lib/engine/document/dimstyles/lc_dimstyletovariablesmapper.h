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

#ifndef LC_DIMSTYLETOVARIABLESMAPPER_H
#define LC_DIMSTYLETOVARIABLESMAPPER_H
#include <qstring.h>

#include "lc_dimstyle.h"
#include "rs.h"

class RS_Graphic;
class LC_DimStyle;

class LC_DimStyleToVariablesMapper{
public:
    explicit LC_DimStyleToVariablesMapper();
    ~LC_DimStyleToVariablesMapper() = default;
    void fromDictionary(LC_DimStyle* style, RS_VariableDict* vd, RS2::Unit unit);
    void toDictionary(const LC_DimStyle* style, RS_VariableDict* vd);
protected:
    void angularUnit2Vars(LC_DimStyle::AngularFormat* s, RS_VariableDict* vd);
    void arrow2Vars(LC_DimStyle::Arrowhead* arrowhead_style,RS_VariableDict* vd);
    void arc2Vars(LC_DimStyle::Arc* s,RS_VariableDict* vd);
    void dimLine2Vars(LC_DimStyle::DimensionLine* dimension_line_style,RS_VariableDict* vd);
    void extensionLine2Vars(LC_DimStyle::ExtensionLine* extension_line_style,RS_VariableDict* vd);
    void leaderStyle2Vars(LC_DimStyle::Leader* leader_style,RS_VariableDict* vd);
    void toleranceStyle2Vars(LC_DimStyle::LatteralTolerance* tolerance_style,RS_VariableDict* vd);
    void mleaderStyle2Vars(LC_DimStyle::MLeader* leader_style,RS_VariableDict* vd);
    void radial2Vars(LC_DimStyle::Radial* radial_style,RS_VariableDict* vd);
    void roundOff2Vars(LC_DimStyle::LinearRoundOff* linear_round_off_style, RS_VariableDict* vd);
    void scaling2Vars(LC_DimStyle::Scaling* scaling_style,RS_VariableDict* vd);
    void text2Vars(LC_DimStyle::Text* text_style,RS_VariableDict* vd);
    void unitFormat2Vars(LC_DimStyle::LinearFormat* unit_formatting_style,RS_VariableDict* vd);
    void fractions2Vars(LC_DimStyle::Fractions* fractions_style,RS_VariableDict* vd);
    void zerosSuppression2Vars(LC_DimStyle::ZerosSuppression* unit_zeros_suppression_style,RS_VariableDict* vd);
    void scalingFromVars(LC_DimStyle::Scaling* stylev,RS_VariableDict* vd,  RS2::Unit unit);
    void unitFormatFromVars(LC_DimStyle::LinearFormat* style,RS_VariableDict* vd);
    void textFromVars(LC_DimStyle::Text* style,RS_VariableDict* vd,  RS2::Unit unit);
    void extensionLineFromVars(LC_DimStyle::ExtensionLine* style,RS_VariableDict* vd,  RS2::Unit unit);
    void arcFromVars(LC_DimStyle::Arc* s,RS_VariableDict* vd);
    void dimLineFromVars(LC_DimStyle::DimensionLine* style,RS_VariableDict* vd,  RS2::Unit unit);
    void angularUnitFromVars(LC_DimStyle::AngularFormat* style,RS_VariableDict* vd);
    void zerosSuppressionFromVars(LC_DimStyle::ZerosSuppression* style,RS_VariableDict* vd);
    void arrowFromVars(LC_DimStyle::Arrowhead* style,RS_VariableDict* vd,  RS2::Unit unit);
    void fractionsFromVars(LC_DimStyle::Fractions* fractions_style,RS_VariableDict* vd);
    void leaderFromVars(LC_DimStyle::Leader* leader_style,RS_VariableDict* vd);
    void toleranceFromVars(LC_DimStyle::LatteralTolerance* tolerance_style,RS_VariableDict* vd,RS2::Unit unit);
    void mleaderFromVars(LC_DimStyle::MLeader* leader_style,RS_VariableDict* vd);
    void radialFromVars(LC_DimStyle::Radial* radial_style,RS_VariableDict* vd,  RS2::Unit unit);
    void roundOffFromVars(LC_DimStyle::LinearRoundOff* linear_round_off_style,RS_VariableDict* vd,  RS2::Unit unit);
    double varDouble(RS_VariableDict* vd, RS2::Unit unit, const QString& key, double defMM);

    RS2::Unit getGraphicUnit() const;
};

#endif // LC_DIMSTYLETOVARIABLESMAPPER_H
