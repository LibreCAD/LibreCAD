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

#include "lc_dimstyletovariablesmapper.h"

#include "rs_filterdxfrw.h"
#include "rs_graphic.h"
#include "rs_units.h"
#include "lc_dimstyle.h"

LC_DimStyleToVariablesMapper::LC_DimStyleToVariablesMapper() {}


// void LC_DimStyleToVariablesMapper::updateVariables(LC_DimStyle* style, RS_VariableDict* vd) {
//     RS_VariableDict* variableDict = m_graphic->getVariableDictObjectRef();
//     toDictionary(style, variableDict);
// }

void LC_DimStyleToVariablesMapper::fromDictionary(LC_DimStyle* style, RS_VariableDict* vd, RS2::Unit unit) {
    arrowFromVars(style->arrowhead(),vd, unit);
    arcFromVars(style->arc(),vd);
    dimLineFromVars(style->dimensionLine(),vd, unit);
    extensionLineFromVars(style->extensionLine(),vd, unit);
    leaderFromVars(style->leader(),vd);
    toleranceFromVars(style->latteralTolerance(),vd, unit);
    mleaderFromVars(style->mleader(),vd);
    radialFromVars(style->radial(),vd, unit);
    roundOffFromVars(style->roundOff(),vd, unit);
    scalingFromVars(style->scaling(),vd, unit);
    textFromVars(style->text(),vd, unit);
    unitFormatFromVars(style->linearFormat(),vd);
    angularUnitFromVars(style->angularFormat(),vd);
    fractionsFromVars(style->fractions(),vd);
    zerosSuppressionFromVars(style->zerosSuppression(),vd);
}

void LC_DimStyleToVariablesMapper::toDictionary(const LC_DimStyle* style, RS_VariableDict* vd) {
    arrow2Vars(style->arrowhead(), vd);
    arc2Vars(style->arc(), vd);
    dimLine2Vars(style->dimensionLine(), vd);
    extensionLine2Vars(style->extensionLine(), vd);
    leaderStyle2Vars(style->leader(), vd);
    toleranceStyle2Vars(style->latteralTolerance(), vd);
    mleaderStyle2Vars(style->mleader(), vd);
    radial2Vars(style->radial(), vd);
    roundOff2Vars(style->roundOff(), vd);
    scaling2Vars(style->scaling(), vd);
    text2Vars(style->text(), vd);
    unitFormat2Vars(style->linearFormat(), vd);
    angularUnit2Vars(style->angularFormat(), vd);
    fractions2Vars(style->fractions(), vd);
    zerosSuppression2Vars(style->zerosSuppression(), vd);
}

void LC_DimStyleToVariablesMapper::arc2Vars(LC_DimStyle::Arc* s,RS_VariableDict* vd) {
    if (s->checkModifyState(LC_DimStyle::Arc::$DIMARCSYM)) {
        vd->add(QStringLiteral( "$DIMARCSYM"), s->arcSymbolPosition(), 90);
    }
}

void LC_DimStyleToVariablesMapper::arcFromVars(LC_DimStyle::Arc* s,RS_VariableDict* vd) {
    // not present in header, may be in dxf
    if (vd->has("$DIMARCSYM")) {
        s->setArcSymbolPositionRaw(vd->getInt("$DIMARCSYM", 0));
    }
}


void LC_DimStyleToVariablesMapper::angularUnitFromVars(LC_DimStyle::AngularFormat* s,RS_VariableDict* vd) {
    if (vd->has("$DIMAUNIT")) {
        s->setFormatRaw( vd->getInt("$DIMAUNIT", RS2::AngleFormat::DegreesDecimal));
    }
    if (vd->has("$DIMADEC")) {
        s->setDecimalPlaces(vd->getInt( "$DIMADEC", 90));
    }
}

void LC_DimStyleToVariablesMapper::angularUnit2Vars(LC_DimStyle::AngularFormat* s,RS_VariableDict* vd) {
    if (s->checkModifyState(LC_DimStyle::AngularFormat::$DIMAUNIT)) {
        vd->add(QStringLiteral( "$DIMAUNIT"), s->format(), 70);
    }
    if (s->checkModifyState(LC_DimStyle::AngularFormat::$DIMADEC)) {
        vd->add(QStringLiteral( "$DIMADEC"), s->decimalPlaces(), 70);
    }
}

void LC_DimStyleToVariablesMapper::arrowFromVars(LC_DimStyle::Arrowhead* s, RS_VariableDict* vd, RS2::Unit unit) {
    // Arrow Style
    if (vd->has("$DIMTSZ")) {
        s->setTickSize(varDouble(vd, unit, "$DIMTSZ", 0.));
    }
    if (vd->has("$DIMBLK")) {
        s->setSameBlockName(vd->getString("$DIMBLK", ""));
    }
    if (vd->has("$DIMBLK1")) {
        s->setArrowHeadBlockNameFirst(vd->getString("$DIMBLK1", ""));
    }
    // Sets the arrowhead for the second end of the dimension line when DIMSAH is on.
    if (vd->has("$DIMBLK2")) {
        s->setArrowHeadBlockNameSecond(vd->getString("$DIMBLK2", ""));
    }
    if (vd->has("$DIMSAH")) {
        s->setUseSeparateArrowHeads(vd->getBool("$DIMSAH", false));
    }
    if (vd->has("$DIMSOXD")) {
        s->setSuppressionsRaw(vd->getInt("$DIMSOXD", false));
    }
    if (vd->has("$DIMASZ")) {
        s->setSize(varDouble(vd, unit, "$DIMASZ", 2.5));
    }
}

void LC_DimStyleToVariablesMapper::arrow2Vars(LC_DimStyle::Arrowhead* s, RS_VariableDict* vd) {
    if (s->checkModifyState(LC_DimStyle::Arrowhead::$DIMTSZ)) {
        vd->add("$DIMTSZ", s->tickSize(), 40);
    }
    if (s->checkModifyState(LC_DimStyle::Arrowhead::$DIMBLK)) {
        vd->add("$DIMBLK", s->sameBlockName(), 1);
    }
    if (s->checkModifyState(LC_DimStyle::Arrowhead::$DIMBLK1)) {
        vd->add("$DIMBLK1", s->arrowHeadBlockNameFirst(), 1);
    }
    if (s->checkModifyState(LC_DimStyle::Arrowhead::$DIMBLK2)) {
        vd->add("$DIMBLK2", s->arrowHeadBlockNameSecond(), 1);
    }
    if (s->checkModifyState(LC_DimStyle::Arrowhead::$DIMSAH)) {
        vd->add("$DIMSAH", s->isUseSeparateArrowHeads(), 70);
    }
    if (s->checkModifyState(LC_DimStyle::Arrowhead::$DIMSOXD)) {
        vd->add("$DIMSOXD", s->suppression(), 70);
    }
    if (s->checkModifyState(LC_DimStyle::Arrowhead::$DIMASZ)) {
        vd->add("$DIMASZ", s->size(), 40);
    }
}

void LC_DimStyleToVariablesMapper::fractionsFromVars(LC_DimStyle::Fractions* s,RS_VariableDict* vd) {
    // not present in header, may be in dxf
    if (vd->has("$DIMFRAC")) {
        s->setStyleRaw(vd->getInt("$DIMFRAC", LC_DimStyle::Fractions::HORIZONTAL));
    }
}

void LC_DimStyleToVariablesMapper::fractions2Vars(LC_DimStyle::Fractions* s,RS_VariableDict* vd) {
    // not present in header, may be in dxf
    if (s->checkModifyState(LC_DimStyle::Fractions::$DIMFRAC)) {
        vd->add("$DIMFRAC", s->style(), 70);
    }
}

void LC_DimStyleToVariablesMapper::leaderFromVars(LC_DimStyle::Leader* s,RS_VariableDict* vd) {
    if (vd->has("$DIMLDRBLK")) {
        auto dimldrblk = vd->getString("$DIMLDRBLK", "");
        // fixme - sand - this is workaround for default logic in  DRW_Header::writeDimVars where no value for block is set to STANDARD
        // fixme - sand - probably, it's better to use more consistent approach?
        if (dimldrblk == "STANDARD") { //
            dimldrblk = "";
        }
        s->setArrowBlockName(dimldrblk);
    }
}

void LC_DimStyleToVariablesMapper::leaderStyle2Vars(LC_DimStyle::Leader* s,RS_VariableDict* vd) {
    if (s->checkModifyState(LC_DimStyle::Leader::$DIMLDRBLK)) {
        vd->add("$DIMLDRBLK", s->arrowBlockName(), 1);
    }
}

void LC_DimStyleToVariablesMapper::toleranceFromVars(LC_DimStyle::LatteralTolerance* s,RS_VariableDict* vd,RS2::Unit unit) {
    if (vd->has("$DIMALTTD")) {
        s->setDecimalPlacesAltDim(vd->getInt("$DIMALTTD", 3));
    }
    if (vd->has("$DIMLIM")) {
        s->setLimitsAreGeneratedAsDefaultText(vd->getBool("$DIMLIM", false));
    }
    if (vd->has("$DIMTDEC")) {
        s->setDecimalPlaces(vd->getInt("$DIMTDEC", 2));
    }
    if (vd->has("$DIMTFAC")) {
        s->setHeightScaleFactorToDimText(varDouble(vd, unit, "$DIMTFAC", 1.0));
    }
    if (vd->has("$DIMTM")) {
        s->setLowerToleranceLimit(varDouble(vd, unit, "$DIMTM", 0.0));
    }
    if (vd->has("$DIMTOL")) {
        s->setAppendTolerancesToDimText(vd->getBool("$DIMTOL", false));
    }
    if (vd->has("$DIMTOLJ")) {
        s->setVerticalJustificationRaw(vd->getInt("$DIMTOLJ", 0));
    }
    if (vd->has("$DIMTP")) {
        s->setUpperToleranceLimit(varDouble(vd, unit, "$DIMTP", 0.0));
    }
    if (vd->has("$DIMTALN")) {
        s->setUpperToleranceLimit(vd->getInt("$DIMTALN", 0));
    }
}

void LC_DimStyleToVariablesMapper::toleranceStyle2Vars(LC_DimStyle::LatteralTolerance* s,RS_VariableDict* vd) {
    if (s->checkModifyState(LC_DimStyle::LatteralTolerance::$DIMALTTD)) {
        vd->add("$DIMALTTD", s->decimalPlacesAltDim(), 70);
    }
    if (s->checkModifyState(LC_DimStyle::LatteralTolerance::$DIMLIM)) {
        vd->add("$DIMLIM", s->isLimitsGeneratedAsDefaultText(), 70);
    }
    if (s->checkModifyState(LC_DimStyle::LatteralTolerance::$DIMTDEC)) {
        vd->add("$DIMTDEC", s->decimalPlaces(), 70);
    }
    if (s->checkModifyState(LC_DimStyle::LatteralTolerance::$DIMTFAC)) {
        vd->add("$DIMTFAC", s->heightScaleFactorToDimText(), 40);
    }
    if (s->checkModifyState(LC_DimStyle::LatteralTolerance::$DIMTM)) {
        vd->add("$DIMTM", s->lowerToleranceLimit(), 40);
    }
    if (s->checkModifyState(LC_DimStyle::LatteralTolerance::$DIMTOL)) {
        vd->add("$DIMTOL", s->isAppendTolerancesToDimText(), 70);
    }
    if (s->checkModifyState(LC_DimStyle::LatteralTolerance::$DIMTOLJ)) {
        vd->add("$DIMTOLJ", s->verticalJustification(), 70);
    }
    if (s->checkModifyState(LC_DimStyle::LatteralTolerance::$DIMTP)) {
        vd->add("$DIMTP", s->upperToleranceLimit(), 40);
    }
    if (s->checkModifyState(LC_DimStyle::LatteralTolerance::$DIMTALN)) {
        vd->add("$DIMTALN", s->adjustment(), 70);
    }
}

void LC_DimStyleToVariablesMapper::mleaderStyle2Vars([[maybe_unused]]LC_DimStyle::MLeader* s,[[maybe_unused]]RS_VariableDict* vd) {
    // todo - sand - check why it's not stored in HEADER section of DXF
}

void LC_DimStyleToVariablesMapper::mleaderFromVars([[maybe_unused]]LC_DimStyle::MLeader* s,[[maybe_unused]]RS_VariableDict* vd) {
  // nothing is in header
}

void LC_DimStyleToVariablesMapper::radialFromVars(LC_DimStyle::Radial* s,RS_VariableDict* vd,  RS2::Unit unit) {
    if (vd->has("$DIMCEN")) {
        s->setCenterMarkOrLineSize(varDouble(vd, unit, "$DIMCEN", 2.5));
    }
    if (vd->has("$DIMJOGANG")) {
        s->setTransverseSegmentAngleInJoggedRadius(varDouble(vd, unit, "$DIMJOGANG", 45));
    }
}

void LC_DimStyleToVariablesMapper::radial2Vars(LC_DimStyle::Radial* s, RS_VariableDict* vd) {
    if (s->checkModifyState(LC_DimStyle::Radial::$DIMCEN)) {
        vd->add("$DIMCEN", s->centerCenterMarkOrLineSize(), 40);
    }
    if (s->checkModifyState(LC_DimStyle::Radial::$DIMJOGANG)) {
        vd->add("$DIMJOGANG", s->transverseSegmentAngleInJoggedRadius(), 40);
    }
}

void LC_DimStyleToVariablesMapper::roundOffFromVars(LC_DimStyle::LinearRoundOff* s, RS_VariableDict* vd,
                                                    RS2::Unit unit) {
    if (vd->has("$DIMALTRND")) {
        s->setAltRoundToValue(varDouble(vd, unit, "$DIMALTRND", 0));
    }
    if (vd->has("$DIMRND")) {
        s->setRoundToValue(varDouble(vd, unit, "$DIMRND", 0));
    }
}

void LC_DimStyleToVariablesMapper::roundOff2Vars(LC_DimStyle::LinearRoundOff* s, RS_VariableDict* vd) {
    if (s->checkModifyState(LC_DimStyle::LinearRoundOff::$DIMALTRND)) {
        vd->add("$DIMALTRND", s->altRoundTo(), 40);
    }
    if (s->checkModifyState(LC_DimStyle::LinearRoundOff::$DIMRND)) {
        vd->add("$DIMRND", s->roundTo(), 40);
    }
}

void LC_DimStyleToVariablesMapper::scalingFromVars(LC_DimStyle::Scaling* s, RS_VariableDict* vd, RS2::Unit unit) {
    if (vd->has("$DIMLFAC")) {
        s->setLinearFactor(varDouble(vd, unit, "$DIMLFAC", 1.0));
    }
    if (vd->has("$DIMSCALE")) {
        s->setScale(varDouble(vd, unit, "$DIMSCALE", 1.0));
    }
}

void LC_DimStyleToVariablesMapper::scaling2Vars(LC_DimStyle::Scaling* s, RS_VariableDict* vd) {
    if (s->checkModifyState(LC_DimStyle::Scaling::$DIMLFAC)) {
        vd->add("$DIMLFAC", s->linearFactor(), 40);
    }
    if (s->checkModifyState(LC_DimStyle::Scaling::$DIMSCALE)) {
        vd->add("$DIMSCALE", s->scale(), 40);
    }
}

void LC_DimStyleToVariablesMapper::unitFormatFromVars(LC_DimStyle::LinearFormat* s,RS_VariableDict* vd) {
    if (vd->has("$DIMALT")) {
        s->setAlternateUnitsRaw(vd->getInt("$DIMALT", 0));
    }
    if (vd->has("$DIMALTD")) {
        s->setAltDecimalPlaces(vd->getInt("$DIMALTD", 3));
    }
    if (vd->has("$DIMALTF")) {
        s->setAltUnitsMultiplier(vd->getDouble("$DIMALTF", 0.03937));
    }
    if (vd->has("$DIMALTU")) {
        s->setAltFormatRaw(vd->getInt("$DIMALTU", 2));
    }
    if (vd->has("$DIMAPOST")) {
        s->setAltPrefixOrSuffix(vd->getString("$DIMAPOST", ""));
    }
    if (vd->has("$DIMDEC")) {
        s->setDecimalPlaces(vd->getInt("$DIMDEC", 2));
    }
    if (vd->has("$DIMDSEP")) {
        s->setDecimalFormatSeparatorChar(vd->getInt("$DIMDSEP", ','));
    }
    if (vd->has("$DIMLUNIT")) {
        s->setFormatRaw(vd->getInt("$DIMLUNIT", 2));
    }
    if (vd->has("$DIMPOST")) {
        s->setPrefixOrSuffix(vd->getString("$DIMPOST", ""));
    }
}

void LC_DimStyleToVariablesMapper::unitFormat2Vars(LC_DimStyle::LinearFormat* s, RS_VariableDict* vd) {
    if (s->checkModifyState(LC_DimStyle::LinearFormat::$DIMALT)) {
        vd->add("$DIMALT", s->alternateUnits(), 70);
    }
    if (s->checkModifyState(LC_DimStyle::LinearFormat::$DIMALTD)) {
        vd->add("$DIMALTD", s->altDecimalPlaces(), 70);
    }
    if (s->checkModifyState(LC_DimStyle::LinearFormat::$DIMALTF)) {
        vd->add("$DIMALTF", s->altUnitsMultiplier(), 40);
    }
    if (s->checkModifyState(LC_DimStyle::LinearFormat::$DIMALTU)) {
        vd->add("$DIMALTU", s->altFormatRaw(), 70);
    }
    if (s->checkModifyState(LC_DimStyle::LinearFormat::$DIMAPOST)) {
        vd->add("$DIMAPOST", s->altPrefixOrSuffix(), 1);
    }
    if (s->checkModifyState(LC_DimStyle::LinearFormat::$DIMDEC)) {
        vd->add("$DIMDEC", s->decimalPlaces(), 70);
    }
    if (s->checkModifyState(LC_DimStyle::LinearFormat::$DIMSEP)) {
        vd->add("$DIMDSEP", s->decimalFormatSeparatorChar(), 70);
    }
    if (s->checkModifyState(LC_DimStyle::LinearFormat::$DIMLUNIT)) {
        vd->add("$DIMLUNIT", s->formatRaw(), 70);
    }
    if (s->checkModifyState(LC_DimStyle::LinearFormat::$DIMPOST)) {
        vd->add("$DIMPOST", s->prefixOrSuffix(), 1);
    }
}

void LC_DimStyleToVariablesMapper::textFromVars(LC_DimStyle::Text* s, RS_VariableDict* vd, RS2::Unit unit) {
    if (vd->has("$DIMATFIT")) {
        s->setUnsufficientSpacePolicyRaw(vd->getInt(
            QStringLiteral("$DIMATFIT"),
            LC_DimStyle::Text::TextAndArrowUnsufficientSpaceArrangementPolicy::EITHER_TEXT_OR_ARROW));
    }
    if (vd->has("$DIMTAD")) {
        s->setVerticalPositioningRaw(
            vd->getInt(QStringLiteral("$DIMTAD"), LC_DimStyle::Text::ABOVE_DIM_LINE_EXCEPT_NOT_HORIZONTAL));
    }
    if (vd->has("$DIMJUST")) {
        s->setHorizontalPositioningRaw(vd->getInt(QStringLiteral("$DIMJUST"), LC_DimStyle::Text::ABOVE_AND_CENTERED));
    }
    if (vd->has("$DIMJUST")) {
        s->setHorizontalPositioningRaw(vd->getInt(QStringLiteral("$DIMJUST"), LC_DimStyle::Text::ABOVE_AND_CENTERED));
    }
    if (vd->has("$DIMCLRT")) {
        RS_Color dimclrt = RS_FilterDXFRW::numberToColor(vd->getInt("$DIMCLRT", 0));

        s->setColor(dimclrt);
    }
    if (vd->has("$DIMTIX")) {
        s->setExtLinesRelativePlacementRaw(
            vd->getInt(QStringLiteral("$DIMTIX"), LC_DimStyle::Text::PLACE_BETWEEN_IF_SUFFICIENT_ROOM));
    }
    if (vd->has("$DIMTIH")) {
        s->setOrientationInsideRaw(vd->getInt(QStringLiteral("$DIMTIH"), LC_DimStyle::Text::DRAW_HORIZONTALLY));
    }
    if (vd->has("$DIMTOH")) {
        s->setOrientationOutsideRaw(vd->getInt(QStringLiteral("$DIMTOH"), LC_DimStyle::Text::DRAW_HORIZONTALLY));
    }
    if (vd->has("$DIMTVP")) {
        s->setVerticalDistanceToDimLine(varDouble(vd, unit, QStringLiteral("$DIMTVP"), 0.0));
    }
    if (vd->has("$DIMTXSTY")) {
        s->setStyle(vd->getString("$DIMTXSTY", "standard"));
    }
    if (vd->has("$DIMTXT")) {
        s->setHeight(varDouble(vd, unit, "$DIMTXT", 2.5));
    }
    if (vd->has("$DIMUPT")) {
        s->setCursorControlPolicyRaw(vd->getInt("$DIMUPT", LC_DimStyle::Text::DIM_LINE_LOCATION_ONLY));
    }
    if (vd->has("$DIMTMOVE")) {
        s->setPositionMovementPolicyRaw(vd->getInt("$DIMTMOVE", LC_DimStyle::Text::DIM_LINE_LOCATION_ONLY));
    }
    if (vd->has("$DIMTFILL")) {
        // not present in doc, yet present in dxfs
        s->setBackgroundFillModeRaw(vd->getInt("$DIMTFILL", LC_DimStyle::Text::NONE));
    }
    if (vd->has("$DIMTFILLCLR")) {
        RS_Color fillClr = RS_FilterDXFRW::numberToColor(vd->getInt("$DIMTFILLCLR", 0));
        s->setExplicitBackgroundFillColor(fillClr);
    }
    if (vd->has("$DIMTFILL")) {
        s->setBackgroundFillModeRaw(vd->getInt("$DIMTFILL", LC_DimStyle::Text::NONE));
    }
    if (vd->has("$DIMTXTDIRECTION")) {
        s->setReadingDirectionRaw(vd->getInt("$DIMTXTDIRECTION", LC_DimStyle::Text::LEFT_TO_RIGHT));
    }
}

void LC_DimStyleToVariablesMapper::text2Vars(LC_DimStyle::Text* s, RS_VariableDict* vd) {
    if (s->checkModifyState(LC_DimStyle::Text::$DIMATFIT)) {
        vd->add("$DIMATFIT", s->unsufficientSpacePolicy(), 70);
    }
    if (s->checkModifyState(LC_DimStyle::Text::$DIMTAD)) {
        vd->add("$DIMTAD", s->verticalPositioning(), 70);
    }
    if (s->checkModifyState(LC_DimStyle::Text::$DIMJUST)) {
        vd->add("$DIMJUST", s->horizontalPositioning(), 70);
    }

    int colRGB;
    int colNum = RS_FilterDXFRW::colorToNumber(s->color(), &colRGB);

    if (s->checkModifyState(LC_DimStyle::Text::$DIMCLRT)) {
        vd->add("$DIMCLRT", colNum, 70);
    }
    if (s->checkModifyState(LC_DimStyle::Text::$DIMTIX)) {
        vd->add("$DIMTIX", s->extLinesRelativePlacement(), 70);
    }
    if (s->checkModifyState(LC_DimStyle::Text::$DIMTIH)) {
        vd->add("$DIMTIH", s->orientationInside(), 70);
    }
    if (s->checkModifyState(LC_DimStyle::Text::$DIMTOH)) {
        vd->add("$DIMTOH", s->orientationOutside(), 70);
    }
    if (s->checkModifyState(LC_DimStyle::Text::$DIMTVP)) {
        vd->add("$DIMTVP", s->verticalDistanceToDimLine(), 40);
    }
    if (s->checkModifyState(LC_DimStyle::Text::$DIMTXSTY)) {
        vd->add("$DIMTXSTY", s->style(), 7);
    }
    if (s->checkModifyState(LC_DimStyle::Text::$DIMTXT)) {
        vd->add("$DIMTXT", s->height(), 40);
    }
    if (s->checkModifyState(LC_DimStyle::Text::$DIMUPT)) {
        vd->add("$DIMUPT", s->cursorControlPolicy(), 70);
    }
    if (s->checkModifyState(LC_DimStyle::Text::$DIMTMOVE)) {
        vd->add("$DIMTMOVE", s->positionMovementPolicy(), 70);
    }
    if (s->checkModifyState(LC_DimStyle::Text::$DIMTFILL)) {
        // not present in header's doc, present in some dxfs.
        // https://help.autodesk.com/view/ACD/2025/ENU/?guid=GUID-A85E8E67-27CD-4C59-BE61-4DC9FADBE74A
        vd->add("$DIMTFILL", s->backgroundFillMode(), 70);
    }
    if (s->checkModifyState(LC_DimStyle::Text::$DIMTFILLCLR)) {
        colNum = RS_FilterDXFRW::colorToNumber(s->explicitBackgroundFillColor(), &colRGB);

        vd->add("$DIMTFILLCLR", colNum, 70);
    }
    if (s->checkModifyState(LC_DimStyle::Text::$DIMTXTDIRECTION)) {
        vd->add("$DIMTXTDIRECTION", s->readingDirection(), 70);
    }
}

void LC_DimStyleToVariablesMapper::extensionLineFromVars(LC_DimStyle::ExtensionLine* s,RS_VariableDict* vd,  RS2::Unit unit) {
    if (vd->has("$DIMCLRE")) {
        RS_Color color = RS_FilterDXFRW::numberToColor(vd->getInt("$DIMCLRE", 0));
        s->setColor(color);
    }
    if (vd->has("$DIMEXE")) {
        s->setDistanceBeyondDimLine(varDouble(vd, unit, QStringLiteral("$DIMEXE"), 1.2500));
    }
    if (vd->has("$DIMEXO")) {
        s->setDistanceFromOriginPoint(varDouble(vd, unit, QStringLiteral("$DIMEXO"), 0.625));
    }
    if (vd->has("$DIMLWE")) {
        s->setLineWidthRaw(vd->getInt("$DIMLWE", -2));
    }
    if (vd->has("$DIMSE1")) {
        s->setSuppressFirstRaw(vd->getInt("$DIMSE1", 0));
    }
    if (vd->has("$DIMSE2")) {
        s->setSuppressFirstRaw(vd->getInt("$DIMSE2", 0));
    }
    if (vd->has("$DIMFXL")) {
        // Sets the total length of the extension lines starting from the dimension line toward the dimension origin.
        s->setFixedLength(varDouble(vd, unit, "$DIMFXL", 1.0));
    }
    if (vd->has("$DIMFXLON")) {
        s->setHasFixedLength(vd->getBool("$DIMFXLON", false));
    }
    if (vd->has("$DIMLTEX1")) {
        auto dimltex1 = vd->getString("$DIMLTEX1", "");
        if (!dimltex1.isEmpty()) {
            s->setLineTypeFirst(dimltex1);
        }
    }
    if (vd->has("$DIMLTEX2")) {
        auto dimltex2 = vd->getString("$DIMLTEX2", "");
        if (!dimltex2.isEmpty()) {
            s->setLineTypeSecond(dimltex2);
        }
    }
}

void LC_DimStyleToVariablesMapper::extensionLine2Vars(LC_DimStyle::ExtensionLine* s, RS_VariableDict* vd) {
    int colRGB;
    int colNum = RS_FilterDXFRW::colorToNumber(s->color(), &colRGB);
    if (s->checkModifyState(LC_DimStyle::ExtensionLine::$DIMCLRE)) {
        vd->add("$DIMCLRE", colNum, 70);
    }
    if (s->checkModifyState(LC_DimStyle::ExtensionLine::$DIMEXE)) {
        vd->add("$DIMEXE", s->distanceBeyondDimLine(), 40);
    }
    if (s->checkModifyState(LC_DimStyle::ExtensionLine::$DIMEXO)) {
        vd->add("$DIMEXO", s->distanceFromOriginPoint(), 40);
    }
    if (s->checkModifyState(LC_DimStyle::ExtensionLine::$DIMLWE)) {
        vd->add("$DIMLWE", RS2::lineWidth2dxfInt(s->lineWidth()), 70);
    }
    if (s->checkModifyState(LC_DimStyle::ExtensionLine::$DIMSE1)) {
        vd->add("$DIMSE1", s->suppressFirstLine(), 70);
    }
    if (s->checkModifyState(LC_DimStyle::ExtensionLine::$DIMSE2)) {
        vd->add("$DIMSE2", s->suppressSecondLine(), 70);
    }
    if (s->checkModifyState(LC_DimStyle::ExtensionLine::$DIMFXL)) {
        // not present in headers
        // https://help.autodesk.com/view/ACD/2025/ENU/?guid=GUID-A85E8E67-27CD-4C59-BE61-4DC9FADBE74A
        // yet they are present in some dxfs, so let's include them too
        vd->add("$DIMFXL", s->hasFixedLength(), 40);
    }
    if (s->checkModifyState(LC_DimStyle::ExtensionLine::$DIMFXLON)) {
        vd->add("$DIMFXLON", s->fixedLength(), 70);
    }
    if (s->checkModifyState(LC_DimStyle::ExtensionLine::$DIMLTEX1)) {
        vd->add("$DIMLTEX1", s->lineTypeFirst(), 6); // check code, it's string...
    }
    if (s->checkModifyState(LC_DimStyle::ExtensionLine::$DIMLTEX2)) {
        vd->add("$DIMLTEX2", s->lineTypeSecond(), 6); // check code, it's string...
    }
}

void LC_DimStyleToVariablesMapper::dimLineFromVars(LC_DimStyle::DimensionLine* s,RS_VariableDict* vd,  RS2::Unit unit) {
    if (vd->has("$DIMCLRD")) {
        RS_Color dimclrd = RS_FilterDXFRW::numberToColor(vd->getInt("$DIMCLRD", 0));
        s->setColor(dimclrd);
    }
    if (vd->has("$DIMDLE")) {
        s->setDistanceBeyondExtLinesForObliqueStroke(varDouble(vd, unit, "$DIMDLE", 0.0));
    }
    if (vd->has("$DIMDLI")) {
        s->setBaselineDimLinesSpacing(varDouble(vd, unit, "$DIMDLI", 3.7500));
    }
    if (vd->has("$DIMGAP")) {
        s->setLineGap(varDouble(vd, unit, "$DIMGAP", 0.625));
    }
    if (vd->has("$DIMLTYPE")) {
        s->setLineType(static_cast<RS2::LineType>(vd->getInt("$DIMLTYPE", RS2::LineType::LineByBlock)));
    }
    if (vd->has("$DIMLWD")) {
        s->setLineWidthRaw(vd->getInt("$DIMLWD", -2));
    }
    if (vd->has("$DIMSD1")) {
        s->setSuppressFirstLineRaw(vd->getInt("$DIMSD1", LC_DimStyle::DimensionLine::DONT_SUPPRESS));
    }
    if (vd->has("$DIMSD1")) {
        s->setSuppressSecondLineRaw(vd->getInt("$DIMSD1", LC_DimStyle::DimensionLine::DONT_SUPPRESS));
    }
}

void LC_DimStyleToVariablesMapper::dimLine2Vars(LC_DimStyle::DimensionLine* s, RS_VariableDict* vd) {
    if (s->checkModifyState(LC_DimStyle::DimensionLine::$DIMCLRD)) {
        int colRGB;
        int colNum = RS_FilterDXFRW::colorToNumber(s->color(), &colRGB);
        vd->add("$DIMCLRD", colNum, 70);
    }
    if (s->checkModifyState(LC_DimStyle::DimensionLine::$DIMDLE)) {
        vd->add("$DIMDLE", s->distanceBeyondExtLinesForObliqueStroke(), 40);
    }
    if (s->checkModifyState(LC_DimStyle::DimensionLine::$DIMDLI)) {
        vd->add("$DIMDLI", s->baseLineDimLinesSpacing(), 40);
    }
    if (s->checkModifyState(LC_DimStyle::DimensionLine::$DIMGAP)) {
        vd->add("$DIMGAP", s->lineGap(), 40);
    }
    if (s->checkModifyState(LC_DimStyle::DimensionLine::$DIMLTYPE)) {
        vd->add("$DIMLTYPE", s->lineType(), 70);
    }
    if (s->checkModifyState(LC_DimStyle::DimensionLine::$DIMLWD)) {
        vd->add("$DIMLWD", RS2::lineWidth2dxfInt(s->lineWidth()), 70);
    }
    if (s->checkModifyState(LC_DimStyle::DimensionLine::$DIMSD1)) {
        vd->add("$DIMSD1", s->suppressFirstLine(), 70);
    }
    if (s->checkModifyState(LC_DimStyle::DimensionLine::$DIMSD2)) {
        vd->add("$DIMSD2", s->suppressSecondLine(), 70);
    }
}

void LC_DimStyleToVariablesMapper::zerosSuppressionFromVars(LC_DimStyle::ZerosSuppression* s, RS_VariableDict* vd) {
    if (vd->has("$DIMZIN")) {
        s->setLinearRaw(vd->getInt("$DIMZIN", LC_DimStyle::ZerosSuppression::TRAILING_IN_DECIMAL));
    }
    if (vd->has("$DIMAZIN")) {
        s->setAngularRaw(vd->getInt("$DIMAZIN", LC_DimStyle::ZerosSuppression::DONT_SUPPRESS));
    }
    if (vd->has("$DIMALTTZ")) {
        s->setAltToleranceRaw(vd->getInt(
            "$DIMALTTZ", LC_DimStyle::ZerosSuppression::TOL_SUPPRESS_ZERO_FEET_AND_ZERO_INCHES));
    }
    if (vd->has("$DIMALTZ")) {
        s->setAltLinearRaw(
            vd->getInt("$DIMALTZ", LC_DimStyle::ZerosSuppression::SUPPRESS_ZERO_FEET_AND_ZERO_INCHES));
    }
    if (vd->has("$DIMTZIN")) {
        s->setToleranceRaw(vd->getInt("$DIMTZIN", LC_DimStyle::ZerosSuppression::TRAILING_IN_DECIMAL));
    }
}

void LC_DimStyleToVariablesMapper::zerosSuppression2Vars(LC_DimStyle::ZerosSuppression* s, RS_VariableDict* vd) {
    if (s->checkModifyState(LC_DimStyle::ZerosSuppression::$DIMZIN)) {
        vd->add("$DIMZIN", s->linearRaw(), 70);
    }
    if (s->checkModifyState(LC_DimStyle::ZerosSuppression::$DIMAZIN)) {
        vd->add("$DIMAZIN", s->angularRaw(), 70);
    }
    if (s->checkModifyState(LC_DimStyle::ZerosSuppression::$DIMALTTZ)) {
        vd->add("$DIMALTTZ", s->altToleranceRaw(), 70);
    }
    if (s->checkModifyState(LC_DimStyle::ZerosSuppression::$DIMALTZ)) {
        vd->add("$DIMALTZ", s->altLinearRaw(), 70);
    }
    if (s->checkModifyState(LC_DimStyle::ZerosSuppression::$DIMTZIN)) {
        vd->add("$DIMTZIN", s->toleranceRaw(), 70);
    }
}

/**
 * @return the given graphic variable or the default value given in mm
 * converted to the graphic unit.
 * If the variable is not found it is added with the given default
 * value converted to the local unit.
 */
double LC_DimStyleToVariablesMapper::varDouble(RS_VariableDict* vd, RS2::Unit graphicUnit, const QString& key, double defMM) {
    double v = vd->getDouble(key, RS_MINDOUBLE);
    if (v <= RS_MINDOUBLE) {
        double defaultValue  =  RS_Units::convert(defMM, RS2::Millimeter, graphicUnit);
        v = defaultValue;
    }
    return v;
}


/**
 * @return The unit the parent graphic works on or None if there's no
 * parent graphic.
 */
// RS2::Unit LC_DimStyleToVariablesMapper::getGraphicUnit() const{
    // RS_Graphic* graphic = getGraphic();
    // RS2::Unit ret = RS2::None;
    // if (graphic) {
        // ret = graphic->getUnit();
    // }
    // return ret;
// }
