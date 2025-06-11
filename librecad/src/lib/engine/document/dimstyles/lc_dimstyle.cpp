/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2025 LibreCAD.org
 Copyright (C) 2025 sand1024

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

#include "lc_dimstyle.h"

#include <QRegularExpression>

#include "lc_linemath.h"
#include "rs_filterdxfrw.h"

QString LC_DimStyle::STANDARD_DIM_STYLE = "Standard";

LC_DimStyle::LC_DimStyle():
    m_angularUnitFormattingStyle{std::make_unique<AngularFormat>()},
    m_arrowheadStyle{std::make_unique<Arrowhead>()},
    m_arcStyle{std::make_unique<Arc>()},
    m_dimensionLineStyle{std::make_unique<DimensionLine>()},
    m_extensionLineStyle{std::make_unique<ExtensionLine>()},
    m_leaderStyle{std::make_unique<Leader>()},
    m_latteralToleranceStyle{std::make_unique<LatteralTolerance>()},
    m_mleaderStyle{std::make_unique<MLeader>()},
    m_radialStyle{std::make_unique<Radial>()},
    m_roundOffStyle{std::make_unique<LinearRoundOff>()},
    m_scalingStyle{std::make_unique<Scaling>()},
    m_textStyle{std::make_unique<Text>()},
    m_unitFormattingStyle{std::make_unique<LinearFormat>()},
    m_unitFractionsStyle{std::make_unique<Fractions>()},
    m_unitZeroSuppressionStyle{std::make_unique<ZerosSuppression>()} {
    fillByDefaults();
}

void LC_DimStyle::fillByDefaults(){
    name = "";
    m_angularUnitFormattingStyle->fillByDefaults();
    m_arrowheadStyle->fillByDefaults();
    m_arcStyle->fillByDefaults();
    m_dimensionLineStyle->fillByDefaults();
    m_extensionLineStyle->fillByDefaults();
    m_leaderStyle->fillByDefaults();
    m_latteralToleranceStyle->fillByDefaults();
    m_mleaderStyle->fillByDefaults();
    m_radialStyle->fillByDefaults();
    m_roundOffStyle->fillByDefaults();
    m_scalingStyle->fillByDefaults();
    m_textStyle->fillByDefaults();
    m_unitFormattingStyle->fillByDefaults();
    m_unitFractionsStyle->fillByDefaults();
    m_unitZeroSuppressionStyle->fillByDefaults();
}

void LC_DimStyle::merge(const LC_DimStyle* src){
    m_angularUnitFormattingStyle->merge(src->angularFormat());
    m_arrowheadStyle->merge(src->arrowhead());
    m_arcStyle->merge(src->arc());
    m_dimensionLineStyle->merge(src->dimensionLine());
    m_extensionLineStyle->merge(src->extensionLine());
    m_leaderStyle->merge(src->leader());
    m_latteralToleranceStyle->merge(src->latteralTolerance());
    m_mleaderStyle->merge(src->mleader());
    m_radialStyle->merge(src->radial());
    m_roundOffStyle->merge(src->roundOff());
    m_scalingStyle->merge(src->scaling());
    m_textStyle->merge(src->text());
    m_unitFormattingStyle->merge(src->linearFormat());
    m_unitFractionsStyle->merge(src->fractions());
    m_unitZeroSuppressionStyle->merge(src->zerosSuppression());
}

void LC_DimStyle::ZerosSuppression::setLinearFlag(LinearSuppressionPolicy dimzin, bool set) {
    if (set) {
        DIMZIN |= dimzin;
    }
    else {
        DIMZIN &= ~dimzin;
    }
}

void LC_DimStyle::ZerosSuppression::setAngularFlag(AngularSuppressionPolicy dimazin, bool set) {
    if (set) {
        DIMAZIN |= dimazin;
    }
    else {
        DIMAZIN &= ~dimazin;
    }
}

void LC_DimStyle::ZerosSuppression::setToleranceFlag(ToleranceSuppressionPolicy dimtzin, bool set) {
    if (set) {
        DIMTZIN |= dimtzin;
    }
    else {
        DIMTZIN &= ~dimtzin;
    }
}

void LC_DimStyle::ZerosSuppression::setAltLinearFlag(LinearSuppressionPolicy dimaltz, bool set) {
    if (set) {
        DIMALTZ |= dimaltz;
    }
    else {
        DIMALTZ &= ~dimaltz;
    }
}

void LC_DimStyle::ZerosSuppression::setAltToleranceFlag(ToleranceSuppressionPolicy dimalttz, bool set) {
    if (set) {
        DIMALTTZ |= dimalttz;
    }
    else {
        DIMALTTZ &= ~dimalttz;
    }
}

void LC_DimStyle::copyTo(LC_DimStyle* copy) {
    m_angularUnitFormattingStyle->copyTo(copy->angularFormat());
    m_arrowheadStyle->copyTo(copy->arrowhead());
    m_arcStyle->copyTo(copy->arc());
    m_dimensionLineStyle->copyTo(copy->dimensionLine());
    m_extensionLineStyle->copyTo(copy->extensionLine());
    m_leaderStyle->copyTo(copy->leader());
    m_latteralToleranceStyle->copyTo(copy->latteralTolerance());
    m_mleaderStyle->copyTo(copy->mleader());
    m_radialStyle->copyTo(copy->radial());
    m_roundOffStyle->copyTo(copy->roundOff());
    m_scalingStyle->copyTo(copy->scaling());
    m_textStyle->copyTo(copy->text());
    m_unitFormattingStyle->copyTo(copy->linearFormat());
    m_unitFractionsStyle->copyTo(copy->fractions());
    m_unitZeroSuppressionStyle->copyTo(copy->zerosSuppression());
}

LC_DimStyle* LC_DimStyle::getCopy() {
    auto copy = new LC_DimStyle();
    copyTo(copy);
    return copy;
}

const QString &LC_DimStyle::getName() const {
    return name;
}

void LC_DimStyle::Text::fillByDefaults() {
    DIMATFIT = EITHER_TEXT_OR_ARROW;
    DIMTFILL = NONE;
    DIMTFILLCLR = RS2::FlagByBlock;
    DIMTAD = ABOVE_DIM_LINE_EXCEPT_NOT_HORIZONTAL;
    DIMJUST = ABOVE_AND_CENTERED;
    DIMCLRT = RS2::FlagByBlock;
    DIMTIX = PLACE_BETWEEN_IF_SUFFICIENT_ROOM;
    DIMTIH = DRAW_HORIZONTALLY;
    DIMTOH = DRAW_HORIZONTALLY;
    DIMTVP = 0.0000;
    DIMTXSTY = "Standard";
    DIMTXT = 2.5000;
    DIMTXTDIRECTION = LEFT_TO_RIGHT;
    DIMUPT = DIM_LINE_LOCATION_ONLY;
    DIMTMOVE = DIM_LINE_WITH_TEXT;
}

void LC_DimStyle::Text::copyTo(Text* c) {
    c->DIMATFIT = DIMATFIT;
    c->DIMTFILL = DIMTFILL;
    c->DIMTFILLCLR = DIMTFILLCLR;
    c->DIMTAD = DIMTAD;
    c->DIMJUST = DIMJUST;
    c->DIMCLRT = DIMCLRT;
    c->DIMTIX = DIMTIX;
    c->DIMTIH = DIMTIH;
    c->DIMTOH = DIMTOH;
    c->DIMTVP = DIMTVP;
    c->DIMTXSTY = DIMTXSTY;
    c->DIMTXT = DIMTXT;
    c->DIMTXTDIRECTION = DIMTXTDIRECTION;
    c->DIMUPT = DIMUPT;
    c->DIMTMOVE = DIMTMOVE;
}

void LC_DimStyle::Text::merge(const Text* parent) {
    if (isNotSet($DIMATFIT)) {
        setUnsufficientSpacePolicy(parent->unsufficientSpacePolicy());
    }
    if (isNotSet($DIMTFILL)) {
        setBackgroundFillMode(parent->backgroundFillMode());
    }
    if (isNotSet($DIMTFILLCLR)) {
        setExplicitBackgroundFillColor(parent->explicitBackgroundFillColor());
    }
    if (isNotSet($DIMTAD)) {
        setVerticalPositioning(parent->verticalPositioning());
    }
    if (isNotSet($DIMJUST)) {
        setHorizontalPositioning(parent->horizontalPositioning());
    }
    if (isNotSet($DIMCLRT)) {
        setColor(parent->color());
    }
    if (isNotSet($DIMTIX)) {
        setExtLinesRelativePlacement(parent->extLinesRelativePlacement());
    }
    if (isNotSet($DIMTIH)) {
        setOrientationInside(parent->orientationInside());
    }
    if (isNotSet($DIMTOH)) {
        setOrientationOutside(parent->orientationOutside());
    }
    if (isNotSet($DIMTVP)) {
        setVerticalDistanceToDimLine(parent->verticalDistanceToDimLine());
    }
    if (isNotSet($DIMTXSTY)) {
        setStyle(parent->style());
    }
    if (isNotSet($DIMTXT)) {
        setHeight(parent->height());
    }
    if (isNotSet($DIMTXTDIRECTION)) {
        setReadingDirection(parent->readingDirection());
    }
    if (isNotSet($DIMUPT)) {
        setCursorControlPolicy(parent->cursorControlPolicy());
    }
    if (isNotSet($DIMTMOVE)) {
        setPositionMovementPolicy(parent->positionMovementPolicy());
    }
}

void LC_DimStyle::DimensionLine::fillByDefaults() {
    DIMCLRD = RS2::FlagByBlock;
    DIMDLE = 0.0;
    DIMDLI = 3.7500;
    DIMGAP = 0.6250;
    DIMLTYPE = ""; // fixme - sand - code
    DIMLTYPE_LineType = RS2::LineByBlock; // fixme - sand - code
    DIMLWD = RS2::intToLineWidth(-2);
    DIMSD1 = DONT_SUPPRESS;
    DIMSD2 = DONT_SUPPRESS;
    DIMTOFL = DRAW_EVEN_IF_ARROWHEADS_ARE_OUTSIDE;
}

void LC_DimStyle::DimensionLine::copyTo(DimensionLine* c) {
    c->DIMCLRD = DIMCLRD;
    c->DIMDLE = DIMDLE;
    c->DIMDLI = DIMDLI;
    c->DIMGAP = DIMGAP;
    c->DIMLTYPE = DIMLTYPE; // fixme - sand - code
    c->DIMLTYPE_LineType = DIMLTYPE_LineType; // fixme - sand - code
    c->DIMLWD = DIMLWD;
    c->DIMSD1 = DIMSD1;
    c->DIMSD2 = DIMSD2;
    c->DIMTOFL = DIMTOFL;
}

void LC_DimStyle::DimensionLine::merge(const DimensionLine* parent) {
    if (isNotSet($DIMCLRD)) {
        setColor(parent->color());
    }
    if (isNotSet($DIMDLE)) {
        setDistanceBeyondExtLinesForObliqueStroke(parent->distanceBeyondExtLinesForObliqueStroke());
    }
    if (isNotSet($DIMDLI)) {
        setBaselineDimLinesSpacing(parent->baseLineDimLinesSpacing());
    }
    if (isNotSet($DIMGAP)) {
        setLineGap(parent->lineGap());
    }
    if (isNotSet($DIMLTYPE)) {
        setLineType(parent->lineType());
    }
    if (isNotSet($DIMLWD)) {
        setLineWidth(parent->lineWidth());
    }
    if (isNotSet($DIMSD1)) {
        setFirstLineSuppression(parent->firstLineSuppression());
    }
    if (isNotSet($DIMSD2)) {
        setSecondLineSuppression(parent->secondLineSuppression());
    }
    if (isNotSet($DIMTOFL)) {
        setDrawPolicyForOutsideText(parent->drawPolicyForOutsideText());
    }
}

void LC_DimStyle::DimensionLine::setLineGap(double dimgap) {
    if (dimgap != DIMGAP) {
        setFlag($DIMGAP);
    }
    DIMGAP = dimgap;
}
void LC_DimStyle::DimensionLine::setColor(RS_Color dimclrd) {
    if (dimclrd != DIMCLRD) {
        setFlag($DIMCLRD);
    }
    DIMCLRD = dimclrd;
}
void LC_DimStyle::DimensionLine::setLineWidth(RS2::LineWidth dimlwd) {
    if (dimlwd != DIMLWD) {
        setFlag($DIMLWD);
    }
    DIMLWD = dimlwd;
}
void LC_DimStyle::DimensionLine::setDistanceBeyondExtLinesForObliqueStroke(double dimdle) {
    if (dimdle != DIMDLE) {
        setFlag($DIMDLE);
    }
    DIMDLE = dimdle;
}
void LC_DimStyle::DimensionLine::setBaselineDimLinesSpacing(double dimdli) {
    if (dimdli != DIMDLI) {
        setFlag($DIMDLI);
    }
    DIMDLI = dimdli;
}
void LC_DimStyle::DimensionLine::setFirstLineSuppression(DimLineAndArrowSuppressionPolicy dimsd1) {
    if (dimsd1 != DIMSD1) {
        setFlag($DIMSD1);
    }
    DIMSD1 = dimsd1;
}
void LC_DimStyle::DimensionLine::setSecondLineSuppression(DimLineAndArrowSuppressionPolicy dimsd2) {
    if (dimsd2 != DIMSD2) {
        setFlag($DIMSD2);
    }
    DIMSD2 = dimsd2;
}
void LC_DimStyle::DimensionLine::setDrawPolicyForOutsideText(DrawPolicyForOutsideText dimtofl) {
    if (dimtofl != DIMTOFL) {
        setFlag($DIMTOFL);
    }
    DIMTOFL = dimtofl;
}
void LC_DimStyle::DimensionLine::setLineType(QString dimltype) {
    if (dimltype != DIMLTYPE) {
        setFlag($DIMLTYPE);
    }
    DIMLTYPE = dimltype;
    DIMLTYPE_LineType = RS_FilterDXFRW::nameToLineType(dimltype);
}

void LC_DimStyle::DimensionLine::setLineType(RS2::LineType lineType) {
    if (DIMLTYPE_LineType != lineType) {
        setFlag($DIMLTYPE);
    }
    DIMLTYPE_LineType = lineType;
    DIMLTYPE = RS_FilterDXFRW::lineTypeToName(lineType);
}

void LC_DimStyle::ExtensionLine::fillByDefaults() {
    DIMCLRE = RS2::FlagByBlock;
    DIMEXE = 1.2500;
    DIMEXO = 0.625;
    DIMFXL = 1.0;
    DIMFXLON = false;
    DIMLTEX1 = ""; // fixme - code
    DIMLTEX2 = ""; // fixme - code
    DIMLTEX1_linetype = RS2::LineByBlock;
    DIMLTEX2_linetype = RS2::LineByBlock;
    DIMLWE = RS2::intToLineWidth(-2);
    DIMSE1 = DONT_SUPPRESS;
    DIMSE2 = DONT_SUPPRESS;
}

void LC_DimStyle::ExtensionLine::copyTo(ExtensionLine* c) {
    c->DIMCLRE = DIMCLRE;
    c->DIMEXE = DIMEXE;
    c->DIMEXO = DIMEXO;
    c->DIMFXL = DIMFXL;
    c->DIMFXLON = DIMFXLON;
    c->DIMLTEX1 = DIMLTEX1;
    c->DIMLTEX2 = DIMLTEX2;
    c->DIMLTEX1_linetype = DIMLTEX1_linetype;
    c->DIMLTEX2_linetype = DIMLTEX2_linetype;
    c->DIMLWE = DIMLWE;
    c->DIMSE1 = DIMSE1;
    c->DIMSE2 = DIMSE2;
}

void LC_DimStyle::ExtensionLine::merge(const ExtensionLine* parent) {
    if (isNotSet($DIMCLRE)) {
        setColor(parent->color());
    }
    if (isNotSet($DIMEXE)) {
        setDistanceBeyondDimLine(parent->distanceBeyondDimLine());
    }
    if (isNotSet($DIMEXO)) {
        setDistanceFromOriginPoint(parent->distanceFromOriginPoint());
    }
    if (isNotSet($DIMFXL)) {
        setFixedLength(parent->fixedLength());
    }
    if (isNotSet($DIMFXLON)) {
        setHasFixedLength(parent->hasFixedLength());
    }
    if (isNotSet($DIMLTEX1)) {
        setLineTypeFirst(parent->lineTypeFirst());
    }
    if (isNotSet($DIMLTEX2)) {
        setLineTypeSecond(parent->lineTypeSecond());
    }
    if (isNotSet($DIMLWE)) {
        setLineWidth(parent->lineWidth());
    }
    if (isNotSet($DIMSE1)) {
        setSuppressionFirst(parent->firstLineSuppression());
    }
    if (isNotSet($DIMSE2)) {
        setSuppressionSecondRaw(parent->secondLineSuppression());
    }
}

void LC_DimStyle::ExtensionLine::setColor(RS_Color dimclre) {
    if (dimclre != DIMCLRE) {
        setFlag($DIMCLRE);
    }
    DIMCLRE = dimclre;
}

void LC_DimStyle::ExtensionLine::setDistanceBeyondDimLine(double dimexe) {
    if (dimexe != DIMEXE) {
        setFlag($DIMEXE);
    }
    DIMEXE = dimexe;
}

void LC_DimStyle::ExtensionLine::setDistanceFromOriginPoint(double dimexo) {
    if (dimexo != DIMEXO) {
        setFlag($DIMEXO);
    }
    DIMEXO = dimexo;
}

void LC_DimStyle::ExtensionLine::setFixedLength(double dimfxl) {
    if (dimfxl != DIMFXL) {
        setFlag($DIMFXL);
    }
    DIMFXL = dimfxl;
}

void LC_DimStyle::ExtensionLine::setHasFixedLength(bool dimfxlon) {
    if (dimfxlon != DIMFXLON) {
        setFlag($DIMFXLON);
    }
    DIMFXLON = dimfxlon;
}

void LC_DimStyle::ExtensionLine::setLineWidth(RS2::LineWidth dimlwe) {
    if (dimlwe != DIMLWE) {
        setFlag($DIMLWE);
    }
    DIMLWE = dimlwe;
}

void LC_DimStyle::ExtensionLine::setLineTypeFirst(const QString& dimltex1) {
    if (dimltex1 != DIMLTEX1) {
        setFlag($DIMLTEX1);
    }
    DIMLTEX1 = dimltex1;
}

void LC_DimStyle::ExtensionLine::setLineTypeSecond(const QString& dimltex2) {
    if (dimltex2 != DIMLTEX2) {
        setFlag($DIMLTEX2);
    }
    DIMLTEX2 = dimltex2;
}

void LC_DimStyle::ExtensionLine::setLineTypeFirst(RS2::LineType lineType) {
    if (DIMLTEX1_linetype != lineType) {
        setFlag($DIMLTEX1);
    }
    DIMLTEX1_linetype = lineType;
    DIMLTEX1 = RS_FilterDXFRW::lineTypeToName(lineType);
}

void LC_DimStyle::ExtensionLine::setLineTypeSecond(RS2::LineType lineType) {
    if (DIMLTEX2_linetype != lineType) {
        setFlag($DIMLTEX2);
    }
    DIMLTEX2_linetype = lineType;
    DIMLTEX2 = RS_FilterDXFRW::lineTypeToName(lineType);
}

void LC_DimStyle::ExtensionLine::setSuppressionFirst(ExtensionLineAndArrowSuppressionPolicy dimse1) {
    if (dimse1 != DIMSE1) {
        setFlag($DIMSE1);
    }
    DIMSE1 = dimse1;
}

void LC_DimStyle::ExtensionLine::setSuppressionSecond(ExtensionLineAndArrowSuppressionPolicy dimse2) {
    if (dimse2 != DIMSE2) {
        setFlag($DIMSE2);
    }
    DIMSE2 = dimse2;
}


void LC_DimStyle::Arrowhead::fillByDefaults() {
    DIMASZ = 2.5;
    DIMBLK = "";
    DIMBLK1 = "";
    DIMBLK2 = "";
    DIMSAH = false;
    DIMSOXD = DONT_SUPPRESS;
    DIMTSZ = 0;
}

void LC_DimStyle::Arrowhead::copyTo(Arrowhead* c) {
    c->DIMASZ = DIMASZ;
    c->DIMBLK = DIMBLK;
    c->DIMBLK1 = DIMBLK1;
    c->DIMBLK2 = DIMBLK2;
    c->DIMSAH = DIMSAH;
    c->DIMSOXD = DIMSOXD;
    c->DIMTSZ = DIMTSZ;
}

void LC_DimStyle::Arrowhead::merge(const Arrowhead* parent) {
    if (isNotSet($DIMASZ)) {
        setSize(parent->size());
    }
    if (isNotSet($DIMBLK)) {
        setUseSeparateArrowHeads(parent->isUseSeparateArrowHeads());
    }
    if (isNotSet($DIMBLK1)) {
        setArrowHeadBlockNameFirst(parent->arrowHeadBlockNameFirst());
    }
    if (isNotSet($DIMBLK2)) {
        setArrowHeadBlockNameSecond(parent->arrowHeadBlockNameSecond());
    }
    if (isNotSet($DIMSAH)) {
        setUseSeparateArrowHeads(parent->isUseSeparateArrowHeads());
    }
    if (isNotSet($DIMSOXD)) {
        setSuppressions(parent->suppression());
    }
    if (isNotSet($DIMTSZ)) {
        setTickSize(parent->tickSize());
    }
}

QString LC_DimStyle::Arrowhead::obtainFirstArrowName() {
    auto result = arrowHeadBlockNameFirst();
    if (result.isEmpty()) {
        result = sameBlockName();
    }
    return result;
}

QString LC_DimStyle::Arrowhead::obtainSecondArrowName() {
    if (isUseSeparateArrowHeads()) {
        return arrowHeadBlockNameSecond();
    }
    else {
        return obtainFirstArrowName();
    }
}

void  LC_DimStyle::Arrowhead::setSuppressions(ArrowHeadSuppressionPolicy dimsoxd) {
    if (dimsoxd != DIMSOXD) {
        setFlag($DIMSOXD);
    }
    DIMSOXD = dimsoxd;
}
void  LC_DimStyle::Arrowhead::setSize(double dimasz) {
    if (dimasz != DIMASZ) {
        setFlag($DIMASZ);
    }
    DIMASZ = dimasz;
}
void  LC_DimStyle::Arrowhead::setSameBlockName(const QString &dimblk) {
    if (dimblk != DIMBLK) {
        setFlag($DIMBLK);
    }
    DIMBLK = dimblk;
}
void  LC_DimStyle::Arrowhead::setArrowHeadBlockNameFirst(const QString &dimblk1) {
    if (dimblk1 != DIMBLK1) {
        setFlag($DIMBLK1);
    }
    DIMBLK1 = dimblk1;
}
void  LC_DimStyle::Arrowhead::setArrowHeadBlockNameSecond(const QString &dimblk2) {
    if (dimblk2 != DIMBLK2) {
        setFlag($DIMBLK2);
    }
    DIMBLK2 = dimblk2;
}
void  LC_DimStyle::Arrowhead::setUseSeparateArrowHeads(bool dimsah) {
    if (dimsah != DIMSAH) {
        setFlag($DIMSAH);
    }
    DIMSAH =  dimsah;
}
void  LC_DimStyle::Arrowhead::setTickSize(double dimtsz) {
    DIMTSZ = dimtsz;
}

void LC_DimStyle::ZerosSuppression::fillByDefaults() {
    DIMZIN = TRAILING_IN_DECIMAL;
    DIMAZIN = DONT_SUPPRESS;
    DIMALTTZ = TOL_SUPPRESS_ZERO_FEET_AND_ZERO_INCHES;
    DIMALTZ = SUPPRESS_ZERO_FEET_AND_ZERO_INCHES;
    DIMTZIN = TRAILING_IN_DECIMAL;
}

void LC_DimStyle::ZerosSuppression::copyTo(ZerosSuppression* c) {
    c->DIMZIN = DIMZIN;
    c->DIMAZIN = DIMAZIN;
    c->DIMALTTZ = DIMALTTZ;
    c->DIMALTZ = DIMALTZ;
    c->DIMTZIN = DIMTZIN;
}

void LC_DimStyle::ZerosSuppression::merge(const ZerosSuppression* parent) {
    if (isNotSet($DIMZIN)) {
        setLinearRaw(parent->linearRaw());
    }
    if (isNotSet($DIMAZIN)) {
        setAngularRaw(parent->angularRaw());
    }
    if (isNotSet($DIMALTZ)) {
        setAltLinearRaw(parent->altLinearRaw());
    }
    if (isNotSet($DIMALTTZ)) {
        setAltToleranceRaw(parent->altToleranceRaw());
    }
    if (isNotSet($DIMTZIN)) {
        setToleranceRaw(parent->toleranceRaw());
    }
}
void LC_DimStyle::ZerosSuppression::setToleranceRaw(int dimtzin) {
    if (dimtzin != DIMTZIN) {
        setFlag($DIMTZIN);
    }
    DIMTZIN = dimtzin;
}
void LC_DimStyle::ZerosSuppression::setLinearRaw(int dimzin) {
    if (dimzin != DIMZIN) {
        setFlag($DIMZIN);
    }
    DIMZIN = dimzin;
}
void LC_DimStyle::ZerosSuppression::setAngularRaw(int dimazin) {
    if (dimazin != DIMAZIN) {
        setFlag($DIMAZIN);
    }
    DIMAZIN = dimazin;
}
void LC_DimStyle::ZerosSuppression::setAltLinearRaw(int dimaltz) {
    if (dimaltz != DIMALTZ) {
        setFlag($DIMALTZ);
    }
    DIMALTZ = dimaltz;
}
void LC_DimStyle::ZerosSuppression:: setAltToleranceRaw(int dimalttz) {
    if (dimalttz != DIMALTTZ) {
        setFlag($DIMALTTZ);
    }
    DIMALTTZ = dimalttz;
}

void LC_DimStyle::LinearRoundOff::fillByDefaults() {
    DIMALTRND = 0;
    DIMRND  = 0;
}

void LC_DimStyle::LinearRoundOff::copyTo(LinearRoundOff* c) {
    c->DIMALTRND = DIMALTRND;
    c->DIMRND  = DIMRND;
}

void LC_DimStyle::LinearRoundOff::merge(const LinearRoundOff* parent) {
    if (isNotSet($DIMALTRND)) {
        setAltRoundToValue(parent->altRoundTo());
    }
    if (isNotSet($DIMRND)) {
        setRoundToValue(parent->roundTo());
    }
}

void LC_DimStyle::LinearRoundOff::setRoundToValue(double dimrnd) {
    if (dimrnd != DIMRND) {
        setFlag($DIMRND);
    }
    DIMRND = dimrnd;
}

void LC_DimStyle::LinearRoundOff::setAltRoundToValue(double dimaltrnd) {
    if (dimaltrnd != DIMALTRND) {
        setFlag($DIMALTRND);
    }
    DIMALTRND = dimaltrnd;
}

void LC_DimStyle::Scaling::fillByDefaults() {
    DIMLFAC = 1.0;
    DIMSCALE = 1.0;
}

void LC_DimStyle::Scaling::copyTo(Scaling* c) {
    c->DIMLFAC = DIMLFAC;
    c->DIMSCALE = DIMSCALE;
}

void LC_DimStyle::Scaling::merge(const Scaling* parent) {
    if (isNotSet($DIMLFAC)) {
        setLinearFactor(parent->linearFactor());
    }
    if (isNotSet($DIMSCALE)) {
        setScale(parent->scale());
    }
}

void LC_DimStyle::Scaling::setLinearFactor(double dimlfac) {
    if (dimlfac != DIMLFAC) {
        setFlag($DIMLFAC);
    }
    DIMLFAC = dimlfac;
};
void LC_DimStyle::Scaling::setScale(double dimscale) {
    if (dimscale != DIMSCALE) {
        setFlag($DIMSCALE);
    }
    DIMSCALE = dimscale;
};

void LC_DimStyle::Fractions::fillByDefaults() {
    DIMFRAC = HORIZONTAL;
}

void LC_DimStyle::Fractions::copyTo(Fractions* c) {
    c->DIMFRAC = DIMFRAC;
}

void LC_DimStyle::Fractions::merge(const Fractions* parent) {
    if (isNotSet($DIMFRAC)) {
        setStyle(parent->style());
    }
}

void LC_DimStyle::Fractions::setStyle(FractionStylePolicy dimfrac) {
    if (dimfrac != DIMFRAC) {
        setFlag($DIMFRAC);
    }
    DIMFRAC = dimfrac;
}

LC_DimStyle::LinearFormat::~LinearFormat() {
    delete primaryPrefixSuffix;
    delete alternativePrefixSuffix;
}

void LC_DimStyle::LinearFormat::fillByDefaults() {
    DIMALT = DISABLE;
    DIMALTD = 3;
    DIMALTF = 0.0394;
    DIMALTU = RS2::Decimal;
    DIMAPOST = "";
    DIMDEC = 2;
    DIMDSEP = ',';
    DIMLUNIT = RS2::Decimal;
    DIMPOST = "";
}

void LC_DimStyle::LinearFormat::copyTo(LinearFormat* c) {
    c->DIMALT = DIMALT;
    c->DIMALTD = DIMALTD;
    c->DIMALTF = DIMALTF;
    c->DIMALTU = DIMALTU;
    c->DIMAPOST = DIMAPOST;
    c->DIMDEC = DIMDEC;
    c->DIMDSEP = DIMDSEP;
    c->DIMLUNIT = DIMLUNIT;
    c->DIMPOST = DIMPOST;
}

void LC_DimStyle::LinearFormat::merge(const LinearFormat* parent) {
    if (isNotSet($DIMALT)) {
        setAlternateUnits(parent->alternateUnits());
    }
    if (isNotSet($DIMALTD)) {
        setAlternateUnits(parent->alternateUnits());
    }
    if (isNotSet($DIMALTF)) {
        setAltDecimalPlaces(parent->altDecimalPlaces());
    }
    if (isNotSet($DIMAPOST)) {
        setAltPrefixOrSuffix(parent->altPrefixOrSuffix());
    }
    if (isNotSet($DIMDEC)) {
        setDecimalPlaces(parent->decimalPlaces());
    }
    if (isNotSet($DIMSEP)) {
        setDecimalFormatSeparatorChar(parent->decimalFormatSeparatorChar());
    }
    if (isNotSet($DIMLUNIT)) {
        setFormat(parent->format());
    }
    if (isNotSet($DIMPOST)) {
        setPrefixOrSuffix(parent->prefixOrSuffix());
    }
}

void LC_DimStyle::LinearFormat::setAltUnitsMultiplier(double dimaltf) {
    if (dimaltf != DIMALTF) {
        setFlag($DIMALTF);
    }
    DIMALTF = dimaltf;
}
void LC_DimStyle::LinearFormat::setAlternateUnits(AlternateUnitsPolicy dimalt) {
    if (dimalt != DIMALT) {
        setFlag($DIMALT);
    }
    DIMALT = dimalt;
}
void LC_DimStyle::LinearFormat::setAltPrefixOrSuffix(const QString &dimapost) {
    if (dimapost != DIMAPOST) {
        setFlag($DIMAPOST);
    }
    DIMAPOST = dimapost;
}
void LC_DimStyle::LinearFormat::setAltFormat( RS2::LinearFormat dimaltu) {
    if (dimaltu != DIMALTU) {
        setFlag($DIMALTU);
    }
    DIMALTU = dimaltu;
}

void LC_DimStyle::LinearFormat::setDecimalPlaces(int dimdec) {
    if (dimdec != DIMDEC) {
        setFlag($DIMDEC);
    }
    DIMDEC = dimdec;
}

void LC_DimStyle::LinearFormat::setAltDecimalPlaces(int dimaltd) {
    if (dimaltd != DIMALTD) {
        setFlag($DIMALTD);
    }
    DIMALTD = dimaltd;
}
void LC_DimStyle::LinearFormat::setDecimalFormatSeparatorChar(int dimsep) {
    if (dimsep != DIMDSEP) {
        setFlag($DIMSEP);
    }
    DIMDSEP = dimsep;
}
void LC_DimStyle::LinearFormat::setPrefixOrSuffix(const QString &dimpost) {
    if (dimpost != DIMPOST) {
        setFlag($DIMPOST);
    }
    DIMPOST = dimpost;
}
void LC_DimStyle::LinearFormat::setFormat(RS2::LinearFormat  dimlunit) {
    if (dimlunit != DIMLUNIT) {
        setFlag($DIMLUNIT);
    }
    DIMLUNIT = dimlunit;
}

LC_DimStyle::LinearFormat::TextPattern::TextPattern(bool primary, const QString& text, LinearFormat* linearFormat):separator{primary? "<>" : "[]"},
    format{linearFormat},
    forAltUnit{!primary}{
    parse(text);
}

QString LC_DimStyle::LinearFormat::TextPattern::update() {
    completeString = QString("").append(prefix).append(separator).append(suffix).append(suffixEndsWithLineFeed?  "\\X" : "");
    if (forAltUnit) {
        format->setAltPrefixOrSuffix(completeString);
    }
    else {
        format->setPrefixOrSuffix(completeString);
    }
    return completeString;
}

void LC_DimStyle::LinearFormat::TextPattern::parse(const QString& val) {
    prefix = "";
    suffix = "";
    completeString = val;
    suffixEndsWithLineFeed = false;
    if (!val.isEmpty()) {
        int pos = val.indexOf(separator);
        if (pos > 0) {
            prefix = val.left(pos);
            suffix = val.mid(pos+2);
            if (suffix.endsWith("\\X")) {
                suffix = suffix.left(suffix.length() - 1);
                suffixEndsWithLineFeed = true;
            }
        }
    }
}

QString LC_DimStyle::LinearFormat::TextPattern::getPrefix() {
    return prefix;
}

QString LC_DimStyle::LinearFormat::TextPattern::getSuffix() {
    return suffix;
}

bool LC_DimStyle::LinearFormat::TextPattern::isSuffixEndsWithLineFeed() {
    return suffixEndsWithLineFeed;
}

void LC_DimStyle::LinearFormat::TextPattern::setPrefix(const QString& p) {
    prefix = p;
    update();
}

void LC_DimStyle::LinearFormat::TextPattern::setSuffix(const QString& s) {
    suffix = s;
    update();
}

void LC_DimStyle::LinearFormat::TextPattern::setSuffixEndsWithNewLineFeed(bool set) {
    suffixEndsWithLineFeed = set;
    update();
}

LC_DimStyle::LinearFormat::TextPattern* LC_DimStyle::LinearFormat::getPrimaryPrefixOrSuffix() {
   if (primaryPrefixSuffix == nullptr) {
       primaryPrefixSuffix = new TextPattern(true, DIMPOST, this);
   }
   return primaryPrefixSuffix;
}

LC_DimStyle::LinearFormat::TextPattern* LC_DimStyle::LinearFormat::getAlternativePrefixOrSuffix() {
    if (alternativePrefixSuffix == nullptr) {
        alternativePrefixSuffix = new TextPattern(false, DIMAPOST, this);
    }
    return alternativePrefixSuffix;
}

void LC_DimStyle::AngularFormat::fillByDefaults() {
    DIMADEC = 0;
    DIMAUNIT =  RS2::DegreesDecimal;
}

void LC_DimStyle::AngularFormat::copyTo(AngularFormat* c) {
    c->DIMADEC = DIMADEC;
    c->DIMAUNIT = DIMAUNIT;
}

void LC_DimStyle::AngularFormat::merge(const AngularFormat* parent) {
    if (isNotSet($DIMADEC)) {
        setDecimalPlaces(parent->decimalPlaces());
    }
    if (isNotSet($DIMAUNIT)) {
        setFormat(parent->format());
    }
}

void  LC_DimStyle::AngularFormat::setDecimalPlaces(int dimadec) {
    if (dimadec != 0) {
        setFlag($DIMADEC);
    }
    DIMADEC = dimadec;
}
void  LC_DimStyle::AngularFormat::setFormat( RS2::AngleFormat dimaunit) {
    if (dimaunit != DIMAUNIT) {
        setFlag($DIMAUNIT);
    }
    DIMAUNIT = dimaunit;
}

void LC_DimStyle::LatteralTolerance::fillByDefaults() {
    DIMALTTD = 3;
    DIMLIM = false;
    DIMTDEC = 2;
    DIMTFAC = 1.0;
    DIMTM = 0.0;
    DIMTOL = false;
    DIMTOLJ = BOTTOM;
    DIMTP = 0.0;
}

void LC_DimStyle::LatteralTolerance::copyTo(LatteralTolerance* c) {
    c->DIMALTTD = DIMALTTD;
    c->DIMLIM = DIMLIM;
    c->DIMTDEC = DIMTDEC;
    c->DIMTFAC = DIMTFAC;
    c->DIMTM = DIMTM;
    c->DIMTOL = DIMTOL;
    c->DIMTOLJ = DIMTOLJ;
    c->DIMTP = DIMTP;
}

void LC_DimStyle::LatteralTolerance::merge(const LatteralTolerance* parent) {
    if (isNotSet($DIMALTTD)) {
        setDecimalPlacesAltDim(parent->decimalPlacesAltDim());
    }
    if (isNotSet($DIMLIM)) {
        setLimitsAreGeneratedAsDefaultText(parent->isLimitsGeneratedAsDefaultText());
    }
    if (isNotSet($DIMTDEC)) {
        setDecimalPlaces(parent->decimalPlaces());
    }
    if (isNotSet($DIMTFAC)) {
        setHeightScaleFactorToDimText(parent->heightScaleFactorToDimText());
    }
    if (isNotSet($DIMTM)) {
        setLowerToleranceLimit(parent->lowerToleranceLimit());
    }
    if (isNotSet($DIMTOL)) {
        setAppendTolerancesToDimText(parent->isAppendTolerancesToDimText());
    }
    if (isNotSet($DIMTOLJ)) {
        setVerticalJustification(parent->verticalJustification());
    }
    if (isNotSet($DIMTP)) {
        setUpperToleranceLimit(parent->upperToleranceLimit());
    }
}

void  LC_DimStyle::LatteralTolerance::setDecimalPlacesAltDim(int dimalttd) {
    if (dimalttd != DIMALTTD) {
        setFlag($DIMALTTD);
    }
    DIMALTTD = dimalttd;
}
void  LC_DimStyle::LatteralTolerance::setLimitsAreGeneratedAsDefaultText(bool dimlim) {
    if (dimlim != DIMLIM) {
        setFlag($DIMLIM);
    }
    DIMLIM = dimlim;
}
void  LC_DimStyle::LatteralTolerance::setDecimalPlaces(int dimtdec) {
    if (dimtdec != DIMTDEC) {
        setFlag($DIMTDEC);
    }
    DIMTDEC = dimtdec;
}
void  LC_DimStyle::LatteralTolerance::setHeightScaleFactorToDimText(double dimtfac) {
    if (dimtfac != DIMTFAC) {
        setFlag($DIMTFAC);
    }
    DIMTFAC = dimtfac;
};
void  LC_DimStyle::LatteralTolerance::setLowerToleranceLimit(double dimtm) {
    if (dimtm != DIMTM) {
        setFlag($DIMTM);
    }
    DIMTM = dimtm;
}
void  LC_DimStyle::LatteralTolerance::setAppendTolerancesToDimText(bool dimtol) {
    if (dimtol != DIMTOL) {
        setFlag($DIMTOL);
    }
    DIMTOL = dimtol;
}
void  LC_DimStyle::LatteralTolerance::setUpperToleranceLimit(double dimtp) {
    if (dimtp != DIMTP) {
        setFlag($DIMTP);
    }
    DIMTP = dimtp;
}
void  LC_DimStyle::LatteralTolerance::setVerticalJustification(VerticalJustificationToDimText dimtolj) {
    if (dimtolj != DIMTOLJ) {
        setFlag($DIMTOLJ);
    }
    DIMTOLJ = dimtolj;
}

void LC_DimStyle::Leader::fillByDefaults() {
    DIMLDRBLK = "";
}

void LC_DimStyle::Leader::copyTo(Leader* c) {
    c->DIMLDRBLK = DIMLDRBLK;
}

void LC_DimStyle::Leader::merge(const Leader* parent) {
    if (isNotSet($DIMLDRBLK)) {
        setArrowBlockName(parent->arrowBlockName());
    }
}

void  LC_DimStyle::Leader::setArrowBlockName(const QString &dimldrblk) {
    if (dimldrblk != DIMLDRBLK) {
        setFlag($DIMLDRBLK);
    }
    DIMLDRBLK = dimldrblk;
}

void LC_DimStyle::MLeader::fillByDefaults() {
    MLEADERSCALE = 1.0;
}

void LC_DimStyle::MLeader::copyTo(MLeader* c) {
    c->MLEADERSCALE = MLEADERSCALE;
}

void LC_DimStyle::MLeader::merge(const MLeader* parent) {
    if (isNotSet($MLEADERSCALE)) {
        setScale(parent->scale());
    }
}

void LC_DimStyle::MLeader::setScale(double mleaderscale) {
    if (mleaderscale != MLEADERSCALE) {
        setFlag($MLEADERSCALE);
    }
    MLEADERSCALE = mleaderscale;
}

void LC_DimStyle::Radial::fillByDefaults() {
    DIMCEN = 2.5;
    DIMJOGANG = 45;
}

void LC_DimStyle::Radial::copyTo(Radial* c) {
    c->DIMCEN = DIMCEN;
    c->DIMJOGANG = DIMJOGANG;
}

void LC_DimStyle::Radial::merge(const Radial* parent) {
    if (isNotSet($DIMCEN)) {
        setCenterMarkOrLineSize(parent->centerCenterMarkOrLineSize());
    }
}

void LC_DimStyle::Radial::setCenterMarkOrLineSize(double dimcen) {
    if (dimcen != DIMCEN) {
        setFlag($DIMCEN);
    }
    DIMCEN = dimcen;
}

void LC_DimStyle::Arc::fillByDefaults() {
    DIMARCSYM = BEFORE;
}

void LC_DimStyle::Arc::copyTo(Arc* c) {
    c->DIMARCSYM = DIMARCSYM;
}

void LC_DimStyle::Arc::merge(const Arc* parent) {
    if (isNotSet($DIMARCSYM)) {
        setArcSymbolDisplay(parent->arcSymbolDisplay());
    }
}

void LC_DimStyle::Arc::setArcSymbolDisplay(DimArcSymbolMode dimarcsym) {
    if (dimarcsym != DIMARCSYM) {
        setFlag($DIMARCSYM);
    }
    DIMARCSYM = dimarcsym;
}

void LC_DimStyle::Arc::setArcSymbolDisplayRaw(int dimarcsym) {
    DimArcSymbolMode mode;
    switch (dimarcsym) {
        case 0: {
            mode = BEFORE;
            break;
        }
        case 1: {
            mode = ABOVE;
            break;
        }
        case 2: {
            mode = NONE;
            break;
        }
        default:
            mode = BEFORE;
    }
    setArcSymbolDisplay(mode);
}

void LC_DimStyle::DimensionLine::setLineWidthRaw(int dimlwd) {
    DIMLWD = RS2::intToLineWidth(dimlwd);
}

void LC_DimStyle::ExtensionLine::setLineWidthRaw(int dimlwe) {
    RS2::LineWidth _dimlwe = RS2::intToLineWidth(dimlwe);
    setLineWidth(_dimlwe);
}

void LC_DimStyle::ExtensionLine::setSuppressionFirstRaw(int dimse1) {
    DIMSE1 = int2SuppressionPolicy(dimse1);
}

void LC_DimStyle::ExtensionLine::setSuppressionSecondRaw(int dimse2) {
    DIMSE2 = int2SuppressionPolicy(dimse2);
}

LC_DimStyle::ExtensionLine::ExtensionLineAndArrowSuppressionPolicy LC_DimStyle::ExtensionLine:: int2SuppressionPolicy(int dimsoxd) {
    switch (dimsoxd) {
        case 0: {
            return  DONT_SUPPRESS;
        }
        case 1: {
            return  SUPPRESS;
        }
        default: {
            return  DONT_SUPPRESS;
        }
    }
}

void LC_DimStyle::Arrowhead::setSuppressionsRaw(int dimsoxd) {
    switch (dimsoxd) {
         case 0: {
             DIMSOXD =  DONT_SUPPRESS;
             break;
         }
        case 1: {
            DIMSOXD =  SUPPRESS;
            break;
        }
        default: {
            DIMSOXD =  DONT_SUPPRESS;
        }
    }
}

void LC_DimStyle::Radial::setTransverseSegmentAngleInJoggedRadius(double dimjogang) {
    if (dimjogang != DIMJOGANG) {
        setFlag($DIMJOGANG);
    }
    DIMJOGANG = dimjogang;
}

LC_DimStyle::Radial::CenterMarkDrawingMode LC_DimStyle::Radial::drawingMode() const {
    // For DIMDIAMETER and DIMRADIUS, the center mark is drawn only if you place the dimension line outside the circle or arc.
    //         0 No center marks or lines are drawn
    //         <0 Centerlines are drawn
    //         >0 Center marks are drawn

    if (LC_LineMath::isNotMeaningful(DIMCEN)) {
        return DRAW_NOTHING;
    }
    else {
        return std::signbit(DIMCEN) ? DRAW_CENTERLINES : DRAW_CENTERMARKS;
    }
}

void LC_DimStyle::setName(const QString &name) {
    LC_DimStyle::name = name;
}

void LC_DimStyle::Text::setUnsufficientSpacePolicyRaw(int dimatfit) {
    TextAndArrowUnsufficientSpaceArrangementPolicy _dimatfit;
    switch (dimatfit) {
        case 0: {
            _dimatfit = OUTSIDE_EXT_LINES;
            break;
        }
        case 1: {
            _dimatfit = ARROW_FIRST_THEN_TEXT;
            break;
        }
        case 2: {
            _dimatfit = TEXT_FIRST_THEN_ARROW;
            break;
        }
        case 3: {
            _dimatfit = EITHER_TEXT_OR_ARROW;
            break;
        }
        default:
            _dimatfit = EITHER_TEXT_OR_ARROW;
            break;
    }
    setUnsufficientSpacePolicy(_dimatfit);
}

void LC_DimStyle::Text::setPositionMovementPolicyRaw(int dimtmove) {
    TextMovementPolicy _dimtmove;
    switch (dimtmove) {
        case 0: {
            _dimtmove = DIM_LINE_WITH_TEXT;
            break;
        }
        case 1: {
            _dimtmove = ADDS_LEADER;
            break;
        }
        case 2: {
            _dimtmove = ALLOW_FREE_POSITIONING;
            break;
        }
        default:
            _dimtmove = DIM_LINE_WITH_TEXT;
            break;
    }
    setPositionMovementPolicy(_dimtmove);
}

void LC_DimStyle::Text::setCursorControlPolicyRaw(int dimupt) {
    CursorControlPolicy _dimupt;
    switch (dimupt) {
        case 0: {
            _dimupt =  DIM_LINE_LOCATION_ONLY;
            break;
        }
        case 1: {
            _dimupt = TEXT_AND_DIM_LINE_LOCATION;
            break;
        }
        default:
            _dimupt = DIM_LINE_LOCATION_ONLY;
            break;
    }
    setCursorControlPolicy(_dimupt);
}

void LC_DimStyle::Text::setBackgroundFillModeRaw(int dimtfill) {
    BackgroundColorPolicy _dimtfill;
    switch (dimtfill) {
        case 0: {
            _dimtfill = NONE;
            break;
        }
        case 1: {
            _dimtfill = DRAWING;
            break;
        }
        case 2: {
            _dimtfill = EXPLICIT;
            break;
        }
        default:
            _dimtfill = NONE;
            break;
    }

    setBackgroundFillMode(_dimtfill);
}

void LC_DimStyle::Text::setExtLinesRelativePlacementRaw(int dimtix) {
    PlacementRelatedToExtLinesPolicy _dimtix;
    switch (dimtix) {
        case 0: {
            _dimtix = PLACE_BETWEEN_IF_SUFFICIENT_ROOM;
            break;
        }
        case 1: {
            _dimtix = PLACE_ALWAYS_INSIDE;
            break;
        }
        default:
            _dimtix = PLACE_BETWEEN_IF_SUFFICIENT_ROOM;
            break;
    }
    setExtLinesRelativePlacement(_dimtix);
}

void LC_DimStyle::Text::setVerticalPositioningRaw(int dimtad) {
    VerticalPositionPolicy _dimtad;
    switch (dimtad) {
        case 0: {
            _dimtad = CENTER_BETWEEN_EXT_LINES;
            break;
        }
        case 1: {
            _dimtad = ABOVE_DIM_LINE_EXCEPT_NOT_HORIZONTAL;
            break;
        }
        case 2: {
            _dimtad = FAREST_SIDE_FROM_DEF_POINTS;
            break;
        }
        case 3: {
            _dimtad = JIS_POSTION;
            break;
        }
        case 4: {
            _dimtad = BELOW_DIMENSION_LINE;
            break;
        }
        default:
            _dimtad = ABOVE_DIM_LINE_EXCEPT_NOT_HORIZONTAL;
            break;
    }
    setVerticalPositioning(_dimtad);
}

void LC_DimStyle::Text::setHorizontalPositioningRaw(int dimjust) {
    HorizontalPositionPolicy _dimjust;
    switch (dimjust) {
        case 0: {
            _dimjust = ABOVE_AND_CENTERED;
            break;
        }
        case 1: {
            _dimjust = NEXT_TO_EXT_ONE;
            break;
        }
        case 2: {
            _dimjust = NEXT_TO_EXT_TWO;
            break;
        }
        case 3: {
            _dimjust = ABOVE_ALIGN_EXT_ONE;
            break;
        }
        case 4: {
            _dimjust = ABOVE_ALIGN_EXT_TWO;
            break;
        }
        default:
            _dimjust = ABOVE_AND_CENTERED;
            break;
    }
    setHorizontalPositioning(_dimjust);
}

void LC_DimStyle::Text::setOrientationInsideRaw(int dimtih) {
    TextOrientationPolicy _dimtih;
    switch (dimtih) {
        case 0: {
            _dimtih =  ALIGN_WITH_DIM_LINE;
            break;
        }
        case 1: {
            _dimtih = DRAW_HORIZONTALLY;
            break;
        }
        default:
            _dimtih = DRAW_HORIZONTALLY;
            break;
    }
    setOrientationInside(_dimtih);
}

void LC_DimStyle::Text::setOrientationOutsideRaw(int dimtoh) {
    TextOrientationPolicy _dimtoh;
    switch (dimtoh) {
        case 0: {
            _dimtoh = ALIGN_WITH_DIM_LINE;
            break;
        }
        case 1: {
            _dimtoh = DRAW_HORIZONTALLY;
            break;
        }
        default:
            _dimtoh = DRAW_HORIZONTALLY;
            break;
    }
    setOrientationOutside(_dimtoh);
}

void LC_DimStyle::DimensionLine::setDrawPolicyForOutsideTextRaw(int dimtofl) {
    DrawPolicyForOutsideText _dimtofl;
    switch (dimtofl) {
        case 0: {
            _dimtofl = DONT_DRAW_IF_ARROWHEADS_ARE_OUTSIDE;
            break;
        }
        case 1: {
            _dimtofl = DRAW_EVEN_IF_ARROWHEADS_ARE_OUTSIDE;
            break;
        }
        default:
            _dimtofl = DRAW_EVEN_IF_ARROWHEADS_ARE_OUTSIDE;
            break;
    }
    setDrawPolicyForOutsideText(_dimtofl);
}

void LC_DimStyle::DimensionLine::setSecondLineSuppressionRaw(int dimsd2) {
    DimLineAndArrowSuppressionPolicy _dimsd2;
    switch (dimsd2) {
        case 0: {
            _dimsd2 = DONT_SUPPRESS;
            break;
        }
        case 1: {
            _dimsd2 = SUPPRESS;
            break;
        }
        default:
            _dimsd2 = DONT_SUPPRESS;
            break;
    }
    setSecondLineSuppression(_dimsd2);
}

void LC_DimStyle::DimensionLine::setFirstLineSuppressionRaw(int dimsd1) {
    DimLineAndArrowSuppressionPolicy _dimsd1;
    switch (dimsd1) {
        case 0: {
            _dimsd1 = DONT_SUPPRESS;
            break;
        }
        case 1: {
            _dimsd1 = SUPPRESS;
            break;
        }
        default:
            _dimsd1 = DONT_SUPPRESS;
            break;
    }
    setFirstLineSuppression(_dimsd1);
}

void LC_DimStyle::Fractions::setStyleRaw(int dimfrac) {
    FractionStylePolicy _dimfrac;
    switch (dimfrac) {
        case (0): {
            _dimfrac = HORIZONTAL;
            break;
        }
        case (1): {
            _dimfrac = DIAGONAL_STACKING;
            break;
        }
        case (2): {
            _dimfrac = NOT_STACKED;
        }
        default:
            _dimfrac = HORIZONTAL;
    }
    setStyle(_dimfrac);
}

void LC_DimStyle::LinearFormat::setAltFormatRaw(int dimaltu) {
     RS2::LinearFormat _dimaltu = dxfInt2LinearFormat(dimaltu);
    setAltFormat(_dimaltu);
}

RS2::LinearFormat LC_DimStyle::LinearFormat::dxfInt2LinearFormat(int f){
    switch (f) {
        case 1:
            return RS2::Scientific;
        case 2:
            return RS2::Decimal;
        case 3:
            return RS2::Engineering;
        case 4:
            return RS2::Architectural;
        case 5:
            return RS2::Fractional;
        case 6:
            return RS2::ArchitecturalMetric;
        default:
            return RS2::Decimal;
    }
}

int LC_DimStyle::LinearFormat::linearFormat2dxf(RS2::LinearFormat f){
    switch (f) {
        case RS2::Scientific:
            return 1;
        case RS2::Decimal:
            return 2;
        case RS2::Engineering:
            return 3;
        case RS2::Architectural:
            return 4;
        case RS2::Fractional:
            return 5;
        case RS2::ArchitecturalMetric:
            return 6;
        default:
            return 2;
    }
}


void LC_DimStyle::LinearFormat::setAlternateUnitsRaw(int dimalt) {
    AlternateUnitsPolicy _dimalt;
    switch (dimalt) {
        case 0: {
            _dimalt = DISABLE;
            break;
        }
        case 1: {
            _dimalt =  ENABLE;
            break;
        }
        default:
            _dimalt = DISABLE;
    };
    setAlternateUnits(_dimalt);
}

void LC_DimStyle::LinearFormat::setFormatRaw(int dimlunit) {
    RS2::LinearFormat _dimlunit = dxfInt2LinearFormat(dimlunit);
    setFormat(_dimlunit);
}

int LC_DimStyle::LinearFormat::formatRaw() {
    return linearFormat2dxf(DIMLUNIT);
}

int LC_DimStyle::LinearFormat::altFormatRaw() {
    return linearFormat2dxf(DIMALTU);
}

void LC_DimStyle::AngularFormat::setFormatRaw(int dimaunit) {
    RS2::AngleFormat _dimaunit;
    switch (dimaunit) {
        case 0:
            _dimaunit = RS2::DegreesDecimal;
            break;
        case 1:
            _dimaunit = RS2::DegreesMinutesSeconds;
            break;
        case 2:
            _dimaunit = RS2::Gradians;
            break;
        case 3:
            _dimaunit = RS2::Radians;
            break;
        case 4:
            _dimaunit = RS2::Surveyors;
            break;
        default:
            _dimaunit = RS2::DegreesDecimal;
    }
    setFormat(_dimaunit);
}

void LC_DimStyle::LatteralTolerance::setVerticalJustificationRaw(int dimtolj) {
    VerticalJustificationToDimText _dimtolj{BOTTOM};
    switch (dimtolj) {
        case 0:
            _dimtolj = BOTTOM;
            break;
        case 1:
            _dimtolj = MIDDLE;
            break;
        case 2:
            _dimtolj = TOP;
            break;
        default:
            _dimtolj = BOTTOM;
    }
    setVerticalJustification(_dimtolj);
}

void LC_DimStyle::Text::setReadingDirectionRaw(int dimtxtdirection) {
    TextDirection _dimtxtdirection{LEFT_TO_RIGHT};
    switch (dimtxtdirection) {
        case 0: {
            _dimtxtdirection = LEFT_TO_RIGHT;
            break;
        }
        case 1:{
            _dimtxtdirection = RIGHT_TO_LEFT;
            break;
        }
        default: {
            _dimtxtdirection = LEFT_TO_RIGHT;
        }
    }
    setReadingDirection(_dimtxtdirection);
}



void LC_DimStyle::Text::setExplicitBackgroundFillColor(const RS_Color& dimtfillclr) {
    if (dimtfillclr != DIMTFILLCLR) {
        setFlag($DIMTFILLCLR);
    }
    DIMTFILLCLR = dimtfillclr;
}

void LC_DimStyle::Text::setUnsufficientSpacePolicy(TextAndArrowUnsufficientSpaceArrangementPolicy dimatfit) {
    if (dimatfit != DIMATFIT) {
        setFlag($DIMATFIT);
    }
    DIMATFIT = dimatfit;
}

void LC_DimStyle::Text::setExtLinesRelativePlacement(PlacementRelatedToExtLinesPolicy dimtix) {
    if (DIMTIX != dimtix) {
        setFlag($DIMTIX);
    }
    DIMTIX = dimtix;
}

void LC_DimStyle::Text::setBackgroundFillMode(BackgroundColorPolicy dimtfill) {
    if (dimtfill != DIMTFILL) {
        setFlag($DIMTFILL);
    }
    DIMTFILL = dimtfill;
}

void LC_DimStyle::Text::setHorizontalPositioning(HorizontalPositionPolicy dimjust) {
    if (dimjust != DIMJUST) {
        setFlag($DIMJUST);
    }
    DIMJUST = dimjust;
}

void LC_DimStyle::Text::setVerticalPositioning(VerticalPositionPolicy dimtad) {
    if (dimtad != DIMTAD) {
        setFlag($DIMTAD);
    }
    DIMTAD = dimtad;
}

void LC_DimStyle::Text::setVerticalDistanceToDimLine(double dimtvp) {
    if (dimtvp != DIMTVP) {
        setFlag($DIMTVP);
    }
    DIMTVP = dimtvp;
}

void LC_DimStyle::Text::setOrientationInside(TextOrientationPolicy dimtih) {
    if (dimtih != DIMTIH) {
        setFlag($DIMTIH);
    }
    DIMTIH = dimtih;
}

void LC_DimStyle::Text::setOrientationOutside(TextOrientationPolicy dimtoh) {
    if (dimtoh != DIMTOH) {
        setFlag($DIMTOH);
    }
    DIMTOH = dimtoh;
}

void LC_DimStyle::Text::setStyle(const QString& dimtxsty) {
    if (dimtxsty != DIMTXSTY) {
        setFlag($DIMTXSTY);
    }
    DIMTXSTY = dimtxsty;
}

void LC_DimStyle::Text::setColor(const RS_Color& dimclrt) {
    if (dimclrt != DIMCLRT) {
        setFlag($DIMCLRT);
    }
    DIMCLRT = dimclrt;
}

void LC_DimStyle::Text::setHeight(double dimtxt) {
    if (dimtxt != DIMTXT) {
        setFlag($DIMTXT);
    }
    DIMTXT = dimtxt;
}

void LC_DimStyle::Text::setReadingDirection(TextDirection dimtxtdirection) {
    if (dimtxtdirection != DIMTXTDIRECTION) {
        setFlag($DIMTXTDIRECTION);
    }
    DIMTXTDIRECTION = dimtxtdirection;
}

void LC_DimStyle::Text::setPositionMovementPolicy(TextMovementPolicy dimtmove) {
    if (dimtmove != DIMTMOVE) {
        setFlag($DIMTMOVE);
    }
    DIMTMOVE = dimtmove;
}

void LC_DimStyle::Text::setCursorControlPolicy(CursorControlPolicy dimupt) {
    if (dimupt != DIMUPT) {
        setFlag($DIMUPT);
    }
    DIMUPT = dimupt;
}
