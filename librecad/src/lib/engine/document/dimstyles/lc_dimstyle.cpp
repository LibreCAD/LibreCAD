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
#include "rs_math.h"

QString LC_DimStyle::STANDARD_DIM_STYLE = "Standard";
QString LC_DimStyle::NAME_SEPARATOR = "$";

LC_DimStyle::LC_DimStyle(const QString& name): m_name{name} {
    init();
}

LC_DimStyle::LC_DimStyle() : m_name{STANDARD_DIM_STYLE} {
    init();
}

void LC_DimStyle::init() {
    m_angularUnitFormattingStyle = std::make_unique<AngularFormat>();
    m_arrowheadStyle = std::make_unique<Arrowhead>();
    m_arcStyle = std::make_unique<Arc>();
    m_dimensionLineStyle = std::make_unique<DimensionLine>();
    m_extensionLineStyle = std::make_unique<ExtensionLine>();
    m_leaderStyle = std::make_unique<Leader>();
    m_latteralToleranceStyle = std::make_unique<LatteralTolerance>();
    m_mleaderStyle = std::make_unique<MLeader>();
    m_radialStyle = std::make_unique<Radial>();
    m_roundOffStyle = std::make_unique<LinearRoundOff>();
    m_scalingStyle = std::make_unique<Scaling>();
    m_textStyle = std::make_unique<Text>();
    m_unitFormattingStyle = std::make_unique<LinearFormat>();
    m_unitFractionsStyle = std::make_unique<Fractions>();
    m_unitZeroSuppressionStyle = std::make_unique<ZerosSuppression>();
    fillByDefaults();
}

void LC_DimStyle::fillByDefaults() {
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

void LC_DimStyle::merge(const LC_DimStyle* src) {
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

void LC_DimStyle::mergeWith(const LC_DimStyle* src, ModificationAware::CheckFlagMode mergeMode, ModificationAware::CheckFlagMode nextMode) {
    setModifyCheckMode(mergeMode);
    merge(src);
    setModifyCheckMode(nextMode);
}

LC_DimStyle::ModificationAware::CheckFlagMode LC_DimStyle::getModifyCheckMode() {
    return m_angularUnitFormattingStyle->getModifyCheckMode();
}

void LC_DimStyle::setModifyCheckMode(ModificationAware::CheckFlagMode mode) {
    m_angularUnitFormattingStyle->setModifyCheckMode(mode);
    m_arrowheadStyle->setModifyCheckMode(mode);
    m_arcStyle->setModifyCheckMode(mode);
    m_dimensionLineStyle->setModifyCheckMode(mode);
    m_extensionLineStyle->setModifyCheckMode(mode);
    m_leaderStyle->setModifyCheckMode(mode);
    m_latteralToleranceStyle->setModifyCheckMode(mode);
    m_mleaderStyle->setModifyCheckMode(mode);
    m_radialStyle->setModifyCheckMode(mode);
    m_roundOffStyle->setModifyCheckMode(mode);
    m_scalingStyle->setModifyCheckMode(mode);
    m_textStyle->setModifyCheckMode(mode);
    m_unitFormattingStyle->setModifyCheckMode(mode);
    m_unitFractionsStyle->setModifyCheckMode(mode);
    m_unitZeroSuppressionStyle->setModifyCheckMode(mode);
}

void LC_DimStyle::ZerosSuppression::setLinearFlag(LinearSuppressionPolicy dimzin, bool set) {
    set ? DIMZIN |= dimzin : DIMZIN &= ~dimzin;
}

void LC_DimStyle::ZerosSuppression::setAngularFlag(AngularSuppressionPolicy dimazin, bool set) {
    set ? DIMAZIN |= dimazin : DIMAZIN &= ~dimazin;
}

void LC_DimStyle::ZerosSuppression::setToleranceFlag(ToleranceSuppressionPolicy dimtzin, bool set) {
    set ? DIMTZIN |= dimtzin : DIMTZIN &= ~dimtzin;
}

void LC_DimStyle::ZerosSuppression::setAltLinearFlag(LinearSuppressionPolicy dimaltz, bool set) {
    set ? DIMALTZ |= dimaltz : DIMALTZ &= ~dimaltz;
}

void LC_DimStyle::ZerosSuppression::setAltToleranceFlag(ToleranceSuppressionPolicy dimalttz, bool set) {
    set ? DIMALTTZ |= dimalttz : DIMALTTZ &= ~dimalttz;
}

void LC_DimStyle::copyTo(LC_DimStyle* copy) {
    copy->m_name = m_name;
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

void LC_DimStyle::resetFlags(bool toZero ) {
    int value = toZero ? 0 : UINT_MAX;
    m_angularUnitFormattingStyle->setFlags(value);
    m_arrowheadStyle->setFlags(value);
    m_arcStyle->setFlags(value);
    m_dimensionLineStyle->setFlags(value);
    m_extensionLineStyle->setFlags(value);
    m_leaderStyle->setFlags(value);
    m_latteralToleranceStyle->setFlags(value);
    m_mleaderStyle->setFlags(value);
    m_radialStyle->setFlags(value);
    m_roundOffStyle->setFlags(value);
    m_scalingStyle->resetFlags();
    m_textStyle->setFlags(value);
    m_unitFormattingStyle->setFlags(value);
    m_unitFractionsStyle->setFlags(value);
    m_unitZeroSuppressionStyle->setFlags(value);
}

LC_DimStyle* LC_DimStyle::getCopy() {
    auto copy = new LC_DimStyle();
    copyTo(copy);
    copy->m_name = m_name;
    copy->m_fromVars = m_fromVars;
    return copy;
}

const QString& LC_DimStyle::getName() const {
    return m_name;
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
    copyFlags(c);
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
    if (checkModifyState($DIMATFIT)) {
        DIMATFIT = parent->DIMATFIT;
    }
    if (checkModifyState($DIMTFILL)) {
        DIMTFILL = parent->DIMTFILL;
    }
    if (checkModifyState($DIMTFILLCLR)) {
        DIMTFILLCLR = parent->DIMTFILLCLR;
    }
    if (checkModifyState($DIMTAD)) {
        DIMTAD = parent->DIMTAD;
    }
    if (checkModifyState($DIMJUST)) {
        DIMJUST = parent->DIMJUST;
    }
    if (checkModifyState($DIMCLRT)) {
        DIMCLRT = parent->DIMCLRT;
    }
    if (checkModifyState($DIMTIX)) {
        DIMTIX = parent->DIMTIX;
    }
    if (checkModifyState($DIMTIH)) {
        DIMTIH = parent->DIMTIH;
    }
    if (checkModifyState($DIMTOH)) {
        DIMTOH = parent->DIMTOH;
    }
    if (checkModifyState($DIMTVP)) {
        DIMTVP = parent->DIMTVP;
    }
    if (checkModifyState($DIMTXSTY)) {
        DIMTXSTY = parent->DIMTXSTY;
    }
    if (checkModifyState($DIMTXT)) {
        DIMTXT = parent->DIMTXT;
    }
    if (checkModifyState($DIMTXTDIRECTION)) {
        DIMTXTDIRECTION = parent->DIMTXTDIRECTION;
    }
    if (checkModifyState($DIMUPT)) {
        DIMUPT = parent->DIMUPT;
    }
    if (checkModifyState($DIMTMOVE)) {
        DIMTMOVE = parent->DIMTMOVE;
    }
}

void LC_DimStyle::DimensionLine::fillByDefaults() {
    DIMCLRD = RS2::FlagByBlock;
    DIMDLE = 0.0;
    DIMDLI = 3.7500;
    DIMGAP = 0.6250;
    DIMLTYPE = "ByBlock";
    DIMLTYPE_LineType = RS2::LineByBlock;
    DIMLWD = RS2::WidthByBlock;
    DIMSD1 = DONT_SUPPRESS;
    DIMSD2 = DONT_SUPPRESS;
    DIMTOFL = DRAW_EVEN_IF_ARROWHEADS_ARE_OUTSIDE;
}

void LC_DimStyle::DimensionLine::copyTo(DimensionLine* c) {
    copyFlags(c);
    c->DIMCLRD = DIMCLRD;
    c->DIMDLE = DIMDLE;
    c->DIMDLI = DIMDLI;
    c->DIMGAP = DIMGAP;
    c->DIMLTYPE = DIMLTYPE;
    c->DIMLTYPE_LineType = DIMLTYPE_LineType;
    c->DIMLWD = DIMLWD;
    c->DIMSD1 = DIMSD1;
    c->DIMSD2 = DIMSD2;
    c->DIMTOFL = DIMTOFL;
}

void LC_DimStyle::DimensionLine::merge(const DimensionLine* parent) {
    if (checkModifyState($DIMCLRD)) {
        DIMCLRD = parent->DIMCLRD;
    }
    if (checkModifyState($DIMDLE)) {
        DIMDLE = parent->DIMDLE;
    }
    if (checkModifyState($DIMDLI)) {
        DIMDLI = parent->DIMDLI;
    }
    if (checkModifyState($DIMGAP)) {
        DIMGAP = parent->DIMGAP;
    }
    if (checkModifyState($DIMLTYPE)) {
        DIMLTYPE = parent->DIMLTYPE;
        DIMLTYPE_LineType = parent->DIMLTYPE_LineType;
    }
    if (checkModifyState($DIMLWD)) {
        DIMLWD = parent->DIMLWD;
    }
    if (checkModifyState($DIMSD1)) {
        DIMSD1 = parent->DIMSD1;
    }
    if (checkModifyState($DIMSD2)) {
        DIMSD2 = parent->DIMSD2;
    }
    if (checkModifyState($DIMTOFL)) {
        DIMTOFL = parent->DIMTOFL;
    }
}


void LC_DimStyle::DimensionLine::setLineGap(double dimgap) {
    checkModified(dimgap, DIMGAP, $DIMGAP);
    DIMGAP = dimgap;
}

void LC_DimStyle::DimensionLine::setColor(RS_Color dimclrd) {
    checkModified(dimclrd, DIMCLRD, $DIMCLRD);
    DIMCLRD = dimclrd;
}

void LC_DimStyle::DimensionLine::setLineWidth(RS2::LineWidth dimlwd) {
    checkModified(dimlwd, DIMLWD, $DIMLWD);
    DIMLWD = dimlwd;
}

void LC_DimStyle::DimensionLine::setDistanceBeyondExtLinesForObliqueStroke(double dimdle) {
    checkModified(dimdle, DIMDLE, $DIMDLE);
    DIMDLE = dimdle;
}

void LC_DimStyle::DimensionLine::setBaselineDimLinesSpacing(double dimdli) {
    checkModified(dimdli, DIMDLI, $DIMDLI);
    DIMDLI = dimdli;
}

void LC_DimStyle::DimensionLine::setSuppressFirstLine(DimLineAndArrowSuppressionPolicy dimsd1) {
    checkModified(dimsd1, DIMSD1, $DIMSD1);
    DIMSD1 = dimsd1;
}

void LC_DimStyle::DimensionLine::setSuppressSecondLine(DimLineAndArrowSuppressionPolicy dimsd2) {
    checkModified(dimsd2, DIMSD2, $DIMSD2);
    DIMSD2 = dimsd2;
}

void LC_DimStyle::DimensionLine::setDrawPolicyForOutsideText(DrawPolicyForOutsideText dimtofl) {
    checkModified(dimtofl, DIMTOFL, $DIMTOFL);
    DIMTOFL = dimtofl;
}

void LC_DimStyle::DimensionLine::setLineType(QString dimltype) {
    checkModified(dimltype, DIMLTYPE, $DIMLTYPE);
    DIMLTYPE = dimltype;
    DIMLTYPE_LineType = RS_FilterDXFRW::nameToLineType(dimltype);
}

void LC_DimStyle::DimensionLine::setLineType(RS2::LineType lineType) {
    checkModified(lineType, DIMLTYPE_LineType, $DIMLTYPE);
    DIMLTYPE_LineType = lineType;
    DIMLTYPE = RS_FilterDXFRW::lineTypeToName(lineType);
}

void LC_DimStyle::ExtensionLine::fillByDefaults() {
    DIMCLRE = RS2::FlagByBlock;
    DIMEXE = 1.2500;
    DIMEXO = 0.625;
    DIMFXL = 1.0;
    DIMFXLON = false;
    DIMLTEX1 = "ByBlock";
    DIMLTEX2 = "ByBlock";
    DIMLTEX1_linetype = RS2::LineByBlock;
    DIMLTEX2_linetype = RS2::LineByBlock;
    DIMLWE =  RS2::WidthByBlock;
    DIMSE1 = DONT_SUPPRESS;
    DIMSE2 = DONT_SUPPRESS;
}

void LC_DimStyle::ExtensionLine::copyTo(ExtensionLine* c) {
    copyFlags(c);
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
    if (checkModifyState($DIMCLRE)) {
        DIMCLRE = parent->DIMCLRE;
    }
    if (checkModifyState($DIMEXE)) {
        DIMEXE = parent->DIMEXE;
    }
    if (checkModifyState($DIMEXO)) {
        DIMEXO = parent->DIMEXO;
    }
    if (checkModifyState($DIMFXL)) {
        DIMFXL = parent->DIMFXL;
    }
    if (checkModifyState($DIMFXLON)) {
        DIMFXLON = parent->DIMFXLON;
    }
    if (checkModifyState($DIMLTEX1)) {
        DIMLTEX1 = parent->DIMLTEX1;
        DIMLTEX1_linetype = parent->DIMLTEX1_linetype;
    }
    if (checkModifyState($DIMLTEX2)) {
        DIMLTEX2 = parent->DIMLTEX2;
        DIMLTEX2_linetype = parent->DIMLTEX2_linetype;
    }
    if (checkModifyState($DIMLWE)) {
        DIMLWE = parent->DIMLWE;
    }
    if (checkModifyState($DIMSE1)) {
        DIMSE1 = parent->DIMSE1;
    }
    if (checkModifyState($DIMSE2)) {
        DIMSE2 = parent->DIMSE2;
    }
}

void LC_DimStyle::ExtensionLine::setColor(RS_Color dimclre) {
    checkModified(dimclre, DIMCLRE, $DIMCLRE);
    DIMCLRE = dimclre;
}

void LC_DimStyle::ExtensionLine::setDistanceBeyondDimLine(double dimexe) {
    checkModified(dimexe, DIMEXE, $DIMEXE);
    DIMEXE = dimexe;
}

void LC_DimStyle::ExtensionLine::setDistanceFromOriginPoint(double dimexo) {
    checkModified(dimexo, DIMEXO, $DIMEXO);
    DIMEXO = dimexo;
}

void LC_DimStyle::ExtensionLine::setFixedLength(double dimfxl) {
    checkModified(dimfxl, DIMFXL, $DIMFXL);
    DIMFXL = dimfxl;
}

void LC_DimStyle::ExtensionLine::setHasFixedLength(bool dimfxlon) {
    checkModified(dimfxlon, DIMFXLON, $DIMFXLON);
    DIMFXLON = dimfxlon;
}

void LC_DimStyle::ExtensionLine::setLineWidth(RS2::LineWidth dimlwe) {
    checkModified(dimlwe, DIMLWE, $DIMLWE);
    DIMLWE = dimlwe;
}

void LC_DimStyle::ExtensionLine::setLineTypeFirst(const QString& dimltex1) {
    checkModified(dimltex1, DIMLTEX1, $DIMLTEX1);
    DIMLTEX1_linetype = RS_FilterDXFRW::nameToLineType(dimltex1);
    DIMLTEX1 = dimltex1;
}

void LC_DimStyle::ExtensionLine::setLineTypeSecond(const QString& dimltex2) {
    checkModified(dimltex2, DIMLTEX2, $DIMLTEX2);
    DIMLTEX2_linetype = RS_FilterDXFRW::nameToLineType(dimltex2);
    DIMLTEX2 = dimltex2;
}

void LC_DimStyle::ExtensionLine::setLineTypeFirst(RS2::LineType lineType) {
    checkModified(lineType, DIMLTEX1_linetype, $DIMLTEX1);
    DIMLTEX1_linetype = lineType;
    DIMLTEX1 = RS_FilterDXFRW::lineTypeToName(lineType);
}

void LC_DimStyle::ExtensionLine::setLineTypeSecond(RS2::LineType lineType) {
    checkModified(lineType, DIMLTEX2_linetype, $DIMLTEX2);
    // if (DIMLTEX2_linetype != lineType) {
    //     setFlag($DIMLTEX2);
    // }
    DIMLTEX2_linetype = lineType;
    DIMLTEX2 = RS_FilterDXFRW::lineTypeToName(lineType);
}

void LC_DimStyle::ExtensionLine::setSuppressFirst(ExtensionLineAndArrowSuppressionPolicy dimse1) {
    checkModified(dimse1, DIMSE1, $DIMSE1);
    DIMSE1 = dimse1;
}

void LC_DimStyle::ExtensionLine::setSuppressSecond(ExtensionLineAndArrowSuppressionPolicy dimse2) {
    checkModified(dimse2, DIMSE2, $DIMSE2);
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
    copyFlags(c);
    c->DIMASZ = DIMASZ;
    c->DIMBLK = DIMBLK;
    c->DIMBLK1 = DIMBLK1;
    c->DIMBLK2 = DIMBLK2;
    c->DIMSAH = DIMSAH;
    c->DIMSOXD = DIMSOXD;
    c->DIMTSZ = DIMTSZ;
}

void LC_DimStyle::Arrowhead::merge(const Arrowhead* parent) {
    if (checkModifyState($DIMASZ)) {
        DIMASZ = parent->DIMASZ;
    }
    if (checkModifyState($DIMBLK)) {
        DIMBLK = parent->DIMBLK;
    }
    if (checkModifyState($DIMBLK1)) {
        DIMBLK1 =parent-> DIMBLK1;
    }
    if (checkModifyState($DIMBLK2)) {
        DIMBLK2 = parent->DIMBLK2;
    }
    if (checkModifyState($DIMSAH)) {
        DIMSAH = parent->DIMSAH;
    }
    if (checkModifyState($DIMSOXD)) {
        DIMSOXD = parent->DIMSOXD;
    }
    if (checkModifyState($DIMTSZ)) {
        DIMTSZ = parent->DIMTSZ;
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

void LC_DimStyle::Arrowhead::setSuppressions(ArrowHeadSuppressionPolicy dimsoxd) {
    checkModified(dimsoxd, DIMSOXD, $DIMSOXD);
    DIMSOXD = dimsoxd;
}

void LC_DimStyle::Arrowhead::setSize(double dimasz) {
    checkModified(dimasz, DIMASZ, $DIMASZ);
    DIMASZ = dimasz;
}

void LC_DimStyle::Arrowhead::setSameBlockName(const QString& dimblk) {
    checkModified(dimblk, DIMBLK, $DIMBLK);
    DIMBLK = dimblk;
}

void LC_DimStyle::Arrowhead::setArrowHeadBlockNameFirst(const QString& dimblk1) {
    checkModified(dimblk1, DIMBLK1, $DIMBLK1);
    DIMBLK1 = dimblk1;
}

void LC_DimStyle::Arrowhead::setArrowHeadBlockNameSecond(const QString& dimblk2) {
    checkModified(dimblk2, DIMBLK2, $DIMBLK2);
    DIMBLK2 = dimblk2;
}

void LC_DimStyle::Arrowhead::setUseSeparateArrowHeads(bool dimsah) {
    checkModified(dimsah, DIMSAH, $DIMSAH);
    DIMSAH = dimsah;
}

void LC_DimStyle::Arrowhead::setTickSize(double dimtsz) {
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
    copyFlags(c);
    c->DIMZIN = DIMZIN;
    c->DIMAZIN = DIMAZIN;
    c->DIMALTTZ = DIMALTTZ;
    c->DIMALTZ = DIMALTZ;
    c->DIMTZIN = DIMTZIN;
}

void LC_DimStyle::ZerosSuppression::merge(const ZerosSuppression* parent) {
    if (checkModifyState($DIMZIN)) {
        DIMZIN = parent->DIMZIN;
    }
    if (checkModifyState($DIMAZIN)) {
        DIMAZIN = parent->DIMAZIN;
    }
    if (checkModifyState($DIMALTZ)) {
        DIMALTZ = parent->DIMALTZ;
    }
    if (checkModifyState($DIMALTTZ)) {
        DIMALTTZ = parent->DIMALTTZ;
    }
    if (checkModifyState($DIMTZIN)) {
        DIMTZIN = parent->DIMTZIN;
    }
}

void LC_DimStyle::ZerosSuppression::setToleranceRaw(int dimtzin) {
    checkModified(dimtzin, DIMTZIN, $DIMTZIN);
    DIMTZIN = dimtzin;
}

void LC_DimStyle::ZerosSuppression::setLinearRaw(int dimzin) {
    checkModified(dimzin, DIMZIN, $DIMZIN);
    DIMZIN = dimzin;
}

void LC_DimStyle::ZerosSuppression::setAngularRaw(int dimazin) {
    checkModified(dimazin, DIMAZIN, $DIMAZIN);
    DIMAZIN = dimazin;
}

void LC_DimStyle::ZerosSuppression::setAltLinearRaw(int dimaltz) {
    checkModified(dimaltz, DIMALTZ, $DIMALTZ);
    DIMALTZ = dimaltz;
}

void LC_DimStyle::ZerosSuppression::setAltToleranceRaw(int dimalttz) {
    checkModified(dimalttz, DIMALTTZ, $DIMALTTZ);
    DIMALTTZ = dimalttz;
}

void LC_DimStyle::LinearRoundOff::fillByDefaults() {
    DIMALTRND = 0;
    DIMRND = 0;
}

void LC_DimStyle::LinearRoundOff::copyTo(LinearRoundOff* c) {
    copyFlags(c);
    c->DIMALTRND = DIMALTRND;
    c->DIMRND = DIMRND;
}

void LC_DimStyle::LinearRoundOff::merge(const LinearRoundOff* parent) {
    if (checkModifyState($DIMALTRND)) {
        DIMALTRND = parent->DIMALTRND;
    }
    if (checkModifyState($DIMRND)) {
        DIMRND = parent->DIMRND;
    }
}

void LC_DimStyle::LinearRoundOff::setRoundToValue(double dimrnd) {
    checkModified(dimrnd, DIMRND, $DIMRND);
    DIMRND = dimrnd;
}

void LC_DimStyle::LinearRoundOff::setAltRoundToValue(double dimaltrnd) {
    checkModified(dimaltrnd, DIMALTRND, $DIMALTRND);
    DIMALTRND = dimaltrnd;
}

void LC_DimStyle::Scaling::fillByDefaults() {
    DIMLFAC = 1.0;
    DIMSCALE = 1.0;
}

void LC_DimStyle::Scaling::copyTo(Scaling* c) {
    copyFlags(c);
    c->DIMLFAC = DIMLFAC;
    c->DIMSCALE = DIMSCALE;
}

void LC_DimStyle::Scaling::merge(const Scaling* parent) {
    if (checkModifyState($DIMLFAC)) {
        DIMLFAC = parent->DIMLFAC;
    }
    if (checkModifyState($DIMSCALE)) {
        DIMSCALE = parent->DIMSCALE;
    }
}

void LC_DimStyle::Scaling::setLinearFactor(double dimlfac) {
    checkModified(dimlfac, DIMLFAC, $DIMLFAC);
    DIMLFAC = dimlfac;
};

void LC_DimStyle::Scaling::setScale(double dimscale) {
    checkModified(dimscale, DIMSCALE, $DIMSCALE);
    DIMSCALE = dimscale;
};

void LC_DimStyle::Fractions::fillByDefaults() {
    DIMFRAC = HORIZONTAL;
}

void LC_DimStyle::Fractions::copyTo(Fractions* c) {
    copyFlags(c);
    c->DIMFRAC = DIMFRAC;
}

void LC_DimStyle::Fractions::merge(const Fractions* parent) {
    if (checkModifyState($DIMFRAC)) {
        DIMFRAC = parent->DIMFRAC;
    }
}

void LC_DimStyle::Fractions::setStyle(FractionStylePolicy dimfrac) {
    checkModified(dimfrac, DIMFRAC, $DIMFRAC);
    DIMFRAC = dimfrac;
}

LC_DimStyle::LinearFormat::~LinearFormat() {
    delete primaryPrefixSuffix;
    delete alternativePrefixSuffix;
}

void LC_DimStyle::LinearFormat::fillByDefaults() {
    DIMALT = DISABLE;
    DIMALTD = 3;
    DIMALTF = 0.03937;
    DIMALTU = RS2::Decimal;
    DIMAPOST = "";
    DIMDEC = 2;
    DIMDSEP = ',';
    DIMLUNIT = RS2::Decimal;
    DIMPOST = "";
}

void LC_DimStyle::LinearFormat::copyTo(LinearFormat* c) {
    copyFlags(c);
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
    if (checkModifyState($DIMALT)) {
        DIMALT = parent->DIMALT;
    }
    if (checkModifyState($DIMALTD)) {
        DIMALTD = parent->DIMALTD;
    }
    if (checkModifyState($DIMALTF)) {
        DIMALTF = parent->DIMALTF;
    }
    if (checkModifyState($DIMALTU)) {
        DIMALTU = parent->DIMALTU;
    }
    if (checkModifyState($DIMAPOST)) {
        DIMAPOST = parent->DIMAPOST;
    }
    if (checkModifyState($DIMDEC)) {
        DIMDEC = parent->DIMDEC;
    }
    if (checkModifyState($DIMSEP)) {
        DIMDSEP = parent->DIMDSEP;
    }
    if (checkModifyState($DIMLUNIT)) {
        DIMLUNIT = parent->DIMLUNIT;
    }
    if (checkModifyState($DIMPOST)) {
        DIMPOST = parent->DIMPOST;
    }
}

void LC_DimStyle::LinearFormat::setAltUnitsMultiplier(double dimaltf) {
    checkModified(dimaltf, DIMALTF, $DIMALTF);
    DIMALTF = dimaltf;
}

void LC_DimStyle::LinearFormat::setAlternateUnits(AlternateUnitsPolicy dimalt) {
    checkModified(dimalt, DIMALT, $DIMALT);
    DIMALT = dimalt;
}

void LC_DimStyle::LinearFormat::setAltPrefixOrSuffix(const QString& dimapost) {
    checkModified(dimapost, DIMAPOST, $DIMAPOST);
    DIMAPOST = dimapost;
}

void LC_DimStyle::LinearFormat::setAltFormat(RS2::LinearFormat dimaltu) {
    checkModified(dimaltu, DIMALTU, $DIMALTU);
    DIMALTU = dimaltu;
}

void LC_DimStyle::LinearFormat::setDecimalPlaces(int dimdec) {
    checkModified(dimdec, DIMDEC, $DIMDEC);
    DIMDEC = dimdec;
}

void LC_DimStyle::LinearFormat::setAltDecimalPlaces(int dimaltd) {
    checkModified(dimaltd, DIMALTD, $DIMALTD);
    DIMALTD = dimaltd;
}

void LC_DimStyle::LinearFormat::setDecimalFormatSeparatorChar(int dimsep) {
    checkModified(dimsep, DIMDSEP, $DIMSEP);
    DIMDSEP = dimsep;
}

void LC_DimStyle::LinearFormat::setPrefixOrSuffix(const QString& dimpost) {
    checkModified(dimpost, DIMPOST, $DIMPOST);
    DIMPOST = dimpost;
}

void LC_DimStyle::LinearFormat::setFormat(RS2::LinearFormat dimlunit) {
    checkModified(dimlunit, DIMLUNIT, $DIMLUNIT);
    DIMLUNIT = dimlunit;
}

LC_DimStyle::LinearFormat::TextPattern::TextPattern(bool primary, const QString& text, LinearFormat* linearFormat) :
    separator{primary ? "<>" : "[]"},
    format{linearFormat},
    forAltUnit{!primary} {
    parse(text);
}

QString LC_DimStyle::LinearFormat::TextPattern::update() {
    completeString = QString("").append(prefix).append(separator).append(suffix).append(
        suffixEndsWithLineFeed ? "\\X" : "");
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
            suffix = val.mid(pos + 2);
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
    DIMAUNIT = RS2::DegreesDecimal;
}

void LC_DimStyle::AngularFormat::copyTo(AngularFormat* c) {
    copyFlags(c);
    c->DIMADEC = DIMADEC;
    c->DIMAUNIT = DIMAUNIT;
}

void LC_DimStyle::AngularFormat::merge(const AngularFormat* parent) {
    if (checkModifyState($DIMADEC)) {
        DIMADEC = parent->DIMADEC;
    }
    if (checkModifyState($DIMAUNIT)) {
        DIMAUNIT = parent->DIMAUNIT;
    }
}

void LC_DimStyle::AngularFormat::setDecimalPlaces(int dimadec) {
    checkModified(dimadec, DIMADEC, $DIMADEC);
    DIMADEC = dimadec;
}

void LC_DimStyle::AngularFormat::setFormat(RS2::AngleFormat dimaunit) {
    checkModified(dimaunit, DIMAUNIT, $DIMAUNIT);
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
    DIMTALN = ALIGN_DECIMAL_SEPARATORS;
}

void LC_DimStyle::LatteralTolerance::copyTo(LatteralTolerance* c) {
    copyFlags(c);
    c->DIMALTTD = DIMALTTD;
    c->DIMLIM = DIMLIM;
    c->DIMTDEC = DIMTDEC;
    c->DIMTFAC = DIMTFAC;
    c->DIMTM = DIMTM;
    c->DIMTOL = DIMTOL;
    c->DIMTOLJ = DIMTOLJ;
    c->DIMTP = DIMTP;
    c->DIMTALN = DIMTALN;
}

void LC_DimStyle::LatteralTolerance::merge(const LatteralTolerance* parent) {
    if (checkModifyState($DIMALTTD)) {
        DIMALTTD = parent->DIMALTTD;
    }
    if (checkModifyState($DIMLIM)) {
        DIMLIM = parent->DIMLIM;
    }
    if (checkModifyState($DIMTDEC)) {
        DIMTDEC = parent->DIMTDEC;
    }
    if (checkModifyState($DIMTFAC)) {
        DIMTFAC = parent->DIMTFAC;
    }
    if (checkModifyState($DIMTM)) {
        DIMTM = parent->DIMTM;
    }
    if (checkModifyState($DIMTOL)) {
        DIMTOL = parent->DIMTOL;
    }
    if (checkModifyState($DIMTOLJ)) {
        DIMTOLJ = parent->DIMTOLJ;
    }
    if (checkModifyState($DIMTP)) {
        DIMTP = parent->DIMTP;
    }
    if (checkModifyState($DIMTALN)) {
        DIMTALN = parent->DIMTALN;
    }
}

void LC_DimStyle::LatteralTolerance::setAdjustment(AdjustmentMode dimtaln) {
    checkModified(dimtaln, DIMTALN, $DIMTALN);
    DIMTALN = dimtaln;
}

void LC_DimStyle::LatteralTolerance::setAdjustmentRaw(int dimtaln) {
    AdjustmentMode align;
    switch (dimtaln) {
        case 0:
            align = ALIGN_DECIMAL_SEPARATORS;
            break;
        case 1:
            align = ALIGN_OPERATIONAL_SYMBOLS;
            break;
        default:
            align = ALIGN_DECIMAL_SEPARATORS;
    }
    setAdjustment(align);
}

void LC_DimStyle::LatteralTolerance::setDecimalPlacesAltDim(int dimalttd) {
    checkModified(dimalttd, DIMALTTD, $DIMALTTD);
    DIMALTTD = dimalttd;
}

void LC_DimStyle::LatteralTolerance::setLimitsAreGeneratedAsDefaultText(bool dimlim) {
    checkModified(dimlim, DIMLIM, $DIMLIM);
    DIMLIM = dimlim;
}

void LC_DimStyle::LatteralTolerance::setDecimalPlaces(int dimtdec) {
    checkModified(dimtdec, DIMTDEC, $DIMTDEC);
    DIMTDEC = dimtdec;
}

void LC_DimStyle::LatteralTolerance::setHeightScaleFactorToDimText(double dimtfac) {
    checkModified(dimtfac, DIMTFAC, $DIMTFAC);
    DIMTFAC = dimtfac;
};

void LC_DimStyle::LatteralTolerance::setLowerToleranceLimit(double dimtm) {
    checkModified(dimtm, DIMTM, $DIMTM);
    DIMTM = dimtm;
}

void LC_DimStyle::LatteralTolerance::setAppendTolerancesToDimText(bool dimtol) {
    checkModified(dimtol, DIMTOL, $DIMTOL);
    DIMTOL = dimtol;
}

void LC_DimStyle::LatteralTolerance::setUpperToleranceLimit(double dimtp) {
    checkModified(dimtp, DIMTP, $DIMTP);
    DIMTP = dimtp;
}

void LC_DimStyle::LatteralTolerance::setVerticalJustification(VerticalJustificationToDimText dimtolj) {
    checkModified(dimtolj, DIMTOLJ, $DIMTOLJ);
    DIMTOLJ = dimtolj;
}

void LC_DimStyle::Leader::fillByDefaults() {
    DIMLDRBLK = "";
}

void LC_DimStyle::Leader::copyTo(Leader* c) {
    copyFlags(c);
    c->DIMLDRBLK = DIMLDRBLK;
}

void LC_DimStyle::Leader::merge(const Leader* parent) {
    if (checkModifyState($DIMLDRBLK)) {
        DIMLDRBLK = parent->DIMLDRBLK;
    }
}

void LC_DimStyle::Leader::setArrowBlockName(const QString& dimldrblk) {
    checkModified(dimldrblk, DIMLDRBLK, $DIMLDRBLK);
    DIMLDRBLK = dimldrblk;
}

void LC_DimStyle::MLeader::fillByDefaults() {
    MLEADERSCALE = 1.0;
}

void LC_DimStyle::MLeader::copyTo(MLeader* c) {
    copyFlags(c);
    c->MLEADERSCALE = MLEADERSCALE;
}

// fixme - remove to MLEADERStyle
void LC_DimStyle::MLeader::merge(const MLeader* parent) {
    if (checkModifyState($MLEADERSCALE)) {
        MLEADERSCALE = parent->MLEADERSCALE;
    }
}

void LC_DimStyle::MLeader::setScale(double mleaderscale) {
    checkModified(mleaderscale, MLEADERSCALE, $MLEADERSCALE);
    MLEADERSCALE = mleaderscale;
}

void LC_DimStyle::Radial::fillByDefaults() {
    DIMCEN = 2.5;
    DIMJOGANG = 45;
}

void LC_DimStyle::Radial::copyTo(Radial* c) {
    copyFlags(c);
    c->DIMCEN = DIMCEN;
    c->DIMJOGANG = DIMJOGANG;
}

void LC_DimStyle::Radial::merge(const Radial* parent) {
    if (checkModifyState($DIMCEN)) {
        DIMCEN = parent->DIMCEN;
    }
    if (checkModifyState($DIMJOGANG)) {
        DIMJOGANG = parent->DIMJOGANG;
    }
}

void LC_DimStyle::Radial::setCenterMarkOrLineSize(double dimcen) {
    checkModified(dimcen, DIMCEN, $DIMCEN);
    DIMCEN = dimcen;
}

void LC_DimStyle::Arc::fillByDefaults() {
    DIMARCSYM = BEFORE;
}

void LC_DimStyle::Arc::copyTo(Arc* c) {
    copyFlags(c);
    c->DIMARCSYM = DIMARCSYM;
}

void LC_DimStyle::Arc::merge(const Arc* parent) {
    if (checkModifyState($DIMARCSYM)) {
        DIMARCSYM = parent->DIMARCSYM;
    }
}

void LC_DimStyle::Arc::setArcSymbolPosition(DimArcSymbolPositionPolicy dimarcsym) {
    checkModified(dimarcsym, DIMARCSYM, $DIMARCSYM);
    DIMARCSYM = dimarcsym;
}

void LC_DimStyle::Arc::setArcSymbolPositionRaw(int dimarcsym) {
    DimArcSymbolPositionPolicy mode;
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
    setArcSymbolPosition(mode);
}

void LC_DimStyle::DimensionLine::setLineWidthRaw(int dimlwd) {
    auto lineWidth = RS2::dxfInt2lineWidth(dimlwd);
    setLineWidth(lineWidth);
}

void LC_DimStyle::ExtensionLine::setLineWidthRaw(int dimlwe) {
    RS2::LineWidth _dimlwe = RS2::dxfInt2lineWidth(dimlwe);
    setLineWidth(_dimlwe);
}

void LC_DimStyle::ExtensionLine::setSuppressFirstRaw(int dimse1) {
    auto policy = int2SuppressionPolicy(dimse1);
    setSuppressFirst(policy);
}

void LC_DimStyle::ExtensionLine::setSuppressSecondRaw(int dimse2) {
    auto policy = int2SuppressionPolicy(dimse2);
    setSuppressSecond(policy);
}

LC_DimStyle::ExtensionLine::ExtensionLineAndArrowSuppressionPolicy
    LC_DimStyle::ExtensionLine::int2SuppressionPolicy(int dimsoxd) {
    switch (dimsoxd) {
        case 0: {
            return DONT_SUPPRESS;
        }
        case 1: {
            return SUPPRESS;
        }
        default: {
            return DONT_SUPPRESS;
        }
    }
}

void LC_DimStyle::Arrowhead::setSuppressionsRaw(int dimsoxd) {
    ArrowHeadSuppressionPolicy policy;
    switch (dimsoxd) {
        case 0: {
            policy = DONT_SUPPRESS;
            break;
        }
        case 1: {
            policy = SUPPRESS;
            break;
        }
        default: {
            policy = DONT_SUPPRESS;
        }
    }
    setSuppressions(policy);
}

void LC_DimStyle::Radial::setTransverseSegmentAngleInJoggedRadius(double dimjogang) {
    checkModified(dimjogang, DIMJOGANG, $DIMJOGANG);
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

void LC_DimStyle::setName(const QString& name) {
    m_name = name;
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
            _dimupt = DIM_LINE_LOCATION_ONLY;
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
            _dimtih = ALIGN_WITH_DIM_LINE;
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

void LC_DimStyle::DimensionLine::setSuppressSecondLineRaw(int dimsd2) {
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
    setSuppressSecondLine(_dimsd2);
}

void LC_DimStyle::DimensionLine::setSuppressFirstLineRaw(int dimsd1) {
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
    setSuppressFirstLine(_dimsd1);
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
            break;
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

RS2::LinearFormat LC_DimStyle::LinearFormat::dxfInt2LinearFormat(int f) {
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

int LC_DimStyle::LinearFormat::linearFormat2dxf(RS2::LinearFormat f) {
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
            _dimalt = ENABLE;
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
        case 1: {
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
    checkModified(dimtfillclr, DIMTFILLCLR, $DIMTFILLCLR);
    DIMTFILLCLR = dimtfillclr;
}

void LC_DimStyle::Text::setUnsufficientSpacePolicy(TextAndArrowUnsufficientSpaceArrangementPolicy dimatfit) {
    checkModified(dimatfit, DIMATFIT, $DIMATFIT);
    DIMATFIT = dimatfit;
}

void LC_DimStyle::Text::setExtLinesRelativePlacement(PlacementRelatedToExtLinesPolicy dimtix) {
    checkModified(dimtix, DIMTIX, $DIMTIX);
    DIMTIX = dimtix;
}

void LC_DimStyle::Text::setBackgroundFillMode(BackgroundColorPolicy dimtfill) {
    checkModified(dimtfill, DIMTFILL, $DIMTFILL);
    DIMTFILL = dimtfill;
}

void LC_DimStyle::Text::setHorizontalPositioning(HorizontalPositionPolicy dimjust) {
    checkModified(dimjust, DIMJUST, $DIMJUST);
    DIMJUST = dimjust;
}

void LC_DimStyle::Text::setVerticalPositioning(VerticalPositionPolicy dimtad) {
    checkModified(dimtad, DIMTAD, $DIMTAD);
    DIMTAD = dimtad;
}

void LC_DimStyle::Text::setVerticalDistanceToDimLine(double dimtvp) {
    checkModified(dimtvp, DIMTVP, $DIMTVP);
    DIMTVP = dimtvp;
}

void LC_DimStyle::Text::setOrientationInside(TextOrientationPolicy dimtih) {
    checkModified(dimtih, DIMTIH, $DIMTIH);
    DIMTIH = dimtih;
}

void LC_DimStyle::Text::setOrientationOutside(TextOrientationPolicy dimtoh) {
    checkModified(dimtoh, DIMTOH, $DIMTOH);
    DIMTOH = dimtoh;
}

void LC_DimStyle::Text::setStyle(const QString& dimtxsty) {
    checkModified(dimtxsty, DIMTXSTY, $DIMTXSTY);
    DIMTXSTY = dimtxsty;
}

void LC_DimStyle::Text::setColor(const RS_Color& dimclrt) {
    checkModified(dimclrt, DIMCLRT, $DIMCLRT);
    DIMCLRT = dimclrt;
}

void LC_DimStyle::Text::setHeight(double dimtxt) {
    checkModified(dimtxt, DIMTXT, $DIMTXT);
    DIMTXT = dimtxt;
}

void LC_DimStyle::Text::setReadingDirection(TextDirection dimtxtdirection) {
    checkModified(dimtxtdirection, DIMTXTDIRECTION, $DIMTXTDIRECTION);
    DIMTXTDIRECTION = dimtxtdirection;
}

void LC_DimStyle::Text::setPositionMovementPolicy(TextMovementPolicy dimtmove) {
    checkModified(dimtmove, DIMTMOVE, $DIMTMOVE);
    DIMTMOVE = dimtmove;
}

void LC_DimStyle::Text::setCursorControlPolicy(CursorControlPolicy dimupt) {
    checkModified(dimupt, DIMUPT, $DIMUPT);
    DIMUPT = dimupt;
}

QString LC_DimStyle::getStyleNameForBaseAndType(const QString& baseName, RS2::EntityType dimType) {
   return baseName + getDimStyleNameSuffixForType(dimType);
}

QString LC_DimStyle::getDimStyleNameSuffixForType(RS2::EntityType dimType) {
    switch (dimType) {
        case RS2::EntityDimLinear:
            // fixme - sand - is there difference between 0 and 1? ACAD uses only 1 but supports both
            return "$0";
        case RS2::EntityDimAligned:
            return "$1";
        case RS2::EntityDimAngular:
            return "$2";
        case RS2::EntityDimDiametric:
            return "$3";
        case RS2::EntityDimRadial:
            return "$4";
        case RS2::EntityDimOrdinate:
            return "$5";
        case RS2::EntityTolerance: // fixme - sand - are 6 and 7 the same? Acad uses only 7
            return "$6";
        case RS2::EntityDimLeader:
            return "$7";
        default:
            return "";
    }
}

void LC_DimStyle::parseStyleName(const QString& fullName, QString& baseName, RS2::EntityType& dimensionType) {
    qsizetype pos = fullName.indexOf(NAME_SEPARATOR);
    if (pos > 0) {
        baseName = fullName.left(pos);
        QString suffix = fullName.mid(pos + 1);
        if (suffix == "0" || suffix == "1") {
            dimensionType = RS2::EntityDimLinear;
        }
        else if (suffix == "2") {
            dimensionType = RS2::EntityDimAngular;
        }
        else if (suffix == "3") {
            dimensionType = RS2::EntityDimDiametric;
        }
        else if (suffix == "4") {
            dimensionType = RS2::EntityDimRadial;
        }
        else if (suffix == "5") {
            dimensionType = RS2::EntityDimOrdinate;
        }
        else if (suffix == "6" || suffix == "7") {
            dimensionType = RS2::EntityDimLeader;
        }
    }
    else {
        baseName = fullName;
        dimensionType = RS2::EntityUnknown;
    }
}

RS2::EntityType LC_DimStyle::getDimensionType() {
    RS2::EntityType type;
    QString baseName;
    parseStyleName(m_name, baseName, type);
    return type;
}

QString LC_DimStyle::getBaseName() {
    RS2::EntityType type;
    QString baseName;
    parseStyleName(m_name, baseName, type);
    return baseName;
}

bool LC_DimStyle::isBaseStyle() {
    int dimensionType = getDimensionType();
    return dimensionType == RS2::EntityUnknown;
}

bool LC_DimStyle::ModificationAware::checkModifyState(unsigned f) {
    switch (m_checkModificationMode) {
        case ALL:
            return true;
        case SET:
            return isSet(f);
        case UNSET:
            return isNotSet(f);
        default:
            return true;
    }
}

void LC_DimStyle::ModificationAware::checkModified(const RS_Color &newValue, const RS_Color &currentValue, unsigned flag) {
    if (!newValue.isEqualIgnoringFlags(currentValue) || m_checkModificationMode == ALL) {
        setFlag(flag);
    }
}

void LC_DimStyle::ModificationAware::checkModified(double newValue, double currentValue, unsigned flag) {
    if (RS_Math::notEqual(newValue, currentValue, RS_TOLERANCE) || m_checkModificationMode == ALL) {
        setFlag(flag);
    }
}

void LC_DimStyle::ModificationAware::checkModified(int newValue, int currentValue, unsigned flag) {
    if (newValue != currentValue || m_checkModificationMode == ALL) {
        setFlag(flag);
    }
}

void LC_DimStyle::ModificationAware::checkModified(short newValue, short currentValue, unsigned flag) {
    if (newValue != currentValue || m_checkModificationMode == ALL) {
        setFlag(flag);
    }
}

void LC_DimStyle::ModificationAware::checkModified(bool newValue, bool currentValue, unsigned flag) {
    if (newValue != currentValue || m_checkModificationMode == ALL) {
        setFlag(flag);
    }
}

void LC_DimStyle::ModificationAware::checkModified(const QString& newValue, const QString& currentValue,
                                                   unsigned flag) {
    if (newValue != currentValue || m_checkModificationMode == ALL) {
        setFlag(flag);
    }
}

void    LC_DimStyle::ModificationAware::copyFlags(ModificationAware* c) {
    c->setFlags(getFlags());
}
