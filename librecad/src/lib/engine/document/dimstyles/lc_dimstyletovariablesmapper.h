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

#include <QString>

#include "lc_dimstyle.h"
#include "rs.h"

class RS_Graphic;
class LC_DimStyle;

class LC_DimStyleToVariablesMapper {
public:
    explicit LC_DimStyleToVariablesMapper();
    ~LC_DimStyleToVariablesMapper() = default;
    void fromDictionary(const LC_DimStyle* style, RS_VariableDict* vd, RS2::Unit unit);
    void toDictionary(const LC_DimStyle* style, RS_VariableDict* vd);

protected:
    void angularUnit2Vars(const LC_DimStyle::AngularFormat* s, RS_VariableDict* vd);
    void arrow2Vars(const LC_DimStyle::Arrowhead* s, RS_VariableDict* vd);
    void arc2Vars(const LC_DimStyle::Arc* s, RS_VariableDict* vd);
    void dimLine2Vars(const LC_DimStyle::DimensionLine* s, RS_VariableDict* vd);
    void extensionLine2Vars(const LC_DimStyle::ExtensionLine* s, RS_VariableDict* vd);
    void leaderStyle2Vars(const LC_DimStyle::Leader* s, RS_VariableDict* vd);
    void toleranceStyle2Vars(const LC_DimStyle::LatteralTolerance* s, RS_VariableDict* vd);
    void mleaderStyle2Vars(LC_DimStyle::MLeader* s, RS_VariableDict* vd);
    void radial2Vars(const LC_DimStyle::Radial* s, RS_VariableDict* vd);
    void roundOff2Vars(const LC_DimStyle::LinearRoundOff* s, RS_VariableDict* vd);
    void scaling2Vars(const LC_DimStyle::Scaling* s, RS_VariableDict* vd);
    void text2Vars(const LC_DimStyle::Text* s, RS_VariableDict* vd);
    void unitFormat2Vars(const LC_DimStyle::LinearFormat* s, RS_VariableDict* vd);
    void fractions2Vars(const LC_DimStyle::Fractions* s, RS_VariableDict* vd);
    void zerosSuppression2Vars(const LC_DimStyle::ZerosSuppression* s, RS_VariableDict* vd);
    void scalingFromVars(LC_DimStyle::Scaling* s, const RS_VariableDict* vd, RS2::Unit unit);
    void unitFormatFromVars(LC_DimStyle::LinearFormat* s, const RS_VariableDict* vd);
    void textFromVars(LC_DimStyle::Text* s, const RS_VariableDict* vd, RS2::Unit unit);
    void extensionLineFromVars(LC_DimStyle::ExtensionLine* s, const RS_VariableDict* vd, RS2::Unit unit);
    void arcFromVars(LC_DimStyle::Arc* s, const RS_VariableDict* vd);
    void dimLineFromVars(LC_DimStyle::DimensionLine* s, const RS_VariableDict* vd, RS2::Unit unit);
    void angularUnitFromVars(LC_DimStyle::AngularFormat* s, const RS_VariableDict* vd);
    void zerosSuppressionFromVars(LC_DimStyle::ZerosSuppression* s, const RS_VariableDict* vd);
    void arrowFromVars(LC_DimStyle::Arrowhead* s, const RS_VariableDict* vd, RS2::Unit unit);
    void fractionsFromVars(LC_DimStyle::Fractions* s, const RS_VariableDict* vd);
    void leaderFromVars(LC_DimStyle::Leader* s, const RS_VariableDict* vd);
    void toleranceFromVars(LC_DimStyle::LatteralTolerance* s, const RS_VariableDict* vd, RS2::Unit unit);
    void mleaderFromVars(LC_DimStyle::MLeader* s, RS_VariableDict* vd);
    void radialFromVars(LC_DimStyle::Radial* s, const RS_VariableDict* vd, RS2::Unit unit);
    void roundOffFromVars(LC_DimStyle::LinearRoundOff* s, const RS_VariableDict* vd, RS2::Unit unit);
    double varDouble(const RS_VariableDict* vd, RS2::Unit unit, const QString& key, double defMM);
};

#endif
