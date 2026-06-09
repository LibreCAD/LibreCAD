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

#include "lc_plot_settings.h"

#include "rs_graphic.h"
#include "rs_math.h"
#include "rs_settings.h"
#include "rs_units.h"
namespace {
    // default paper size A4: 210x297 mm
    const RS_Vector g_paperSizeA4{210., 297.};
}

/**
 * @return Print Area size in graphic units.
 */
RS_Vector LC_PlotSettings::getPrintAreaSize(const bool total) const {
    RS_Vector printArea = getPaperSize();
    const RS2::Unit dest = getUnit();
    printArea.x -= RS_Units::convert(m_marginLeft_mm + m_marginRight_mm, RS2::Millimeter, dest);
    printArea.y -= RS_Units::convert(m_marginTop_mm + m_marginBottom_mm, RS2::Millimeter, dest);
    if (total) {
        printArea.x *= m_pagesNumH;
        printArea.y *= m_pagesNumV;
    }
    return printArea;
}

/**
 * @return Paper format.
 * This is determined by the variables "$PLIMMIN" and "$PLIMMAX".
 *
 * @param landscape will be set to true for landscape and false for portrait if not nullptr.
 */
RS2::PaperFormat LC_PlotSettings::getPaperFormat(bool* landscape) const {
    const RS_Vector size = RS_Units::convert(getPaperSize(), getUnit(), RS2::Millimeter);
    if (landscape != nullptr) {
        *landscape = size.x > size.y;
    }
    return RS_Units::paperSizeToFormat(size);
}

/**
 * Sets the paper format to the given format.
 */
void LC_PlotSettings::setPaperFormat(const RS2::PaperFormat format, const bool landscape) {
    RS_Vector size = RS_Units::paperFormatToSize(format);

    if (landscape != (size.x > size.y)) {
        std::swap(size.x, size.y);
    }

    setPaperSize(RS_Units::convert(size, RS2::Millimeter, getUnit()));
    m_graphic->setModified(true);
}

/**
 * @return Paper space scaling (DXF: $PSVPSCALE).
 */
double LC_PlotSettings::getPaperScale() const {
    double paperScale = m_graphic->getVariableDouble("$PSVPSCALE", 1.0); // fixme - rework
    if (std::abs(paperScale) < 1e-6) {
        paperScale = 1.0;
    }
    return paperScale;
}


/**
 * Sets a new scale factor for the paper space.
 */
void LC_PlotSettings::setPaperScale(const double s) const {
    if (!m_paperScaleFixed) {
        m_graphic->addVariable("$PSVPSCALE", s, 40);
    }
    // fixme - rework, store there
    m_graphic->setModified(true);
}

void LC_PlotSettings::setMarginsInMm(const double left, const double top, const double right, const double bottom)  {
    if (left >= 0.0) {
        m_marginLeft_mm = left;
    }
    if (top >= 0.0) {
        m_marginTop_mm = top;
    }
    if (right >= 0.0) {
        m_marginRight_mm = right;
    }
    if (bottom >= 0.0) {
        m_marginBottom_mm = bottom;
    }
    m_graphic->setModified(true);
}

/**
 * Sets a new paper size.
 */
void LC_PlotSettings::setPaperSize(const RS_Vector& s) const {
    m_graphic->addVariable("$PLIMMIN", RS_Vector(0.0, 0.0), 10);
    m_graphic->addVariable("$PLIMMAX", s, 10);
    //set default paper size
    const RS_Vector def = RS_Units::convert(s, getUnit(), RS2::Millimeter);
    LC_GROUP_GUARD("Print"); // fixme - rework
    {
        LC_SET("PaperSizeX", def.x);
        LC_SET("PaperSizeY", def.y);
    }
    m_graphic->setModified(true);
}

QString LC_PlotSettings::getPaperSizeName() const {
    return m_paperSizeName;
}

void LC_PlotSettings::setPaperSizeName(const QString& paperSizeName) {
    m_paperSizeName = paperSizeName;
}

double LC_PlotSettings::getPaperWidthMm() const {
    return m_paperWidth_mm;
}

void LC_PlotSettings::setPaperWidthMm(const double paperWidthMm) {
    m_paperWidth_mm = paperWidthMm;
}

double LC_PlotSettings::getPaperHeightMm() const {
    return m_paperHeight_mm;
}

void LC_PlotSettings::setPaperHeightMm(const double paperHeightMm) {
    m_paperHeight_mm = paperHeightMm;
}

double LC_PlotSettings::getOriginOffsetXMm() const {
    return originOffsetX_mm;
}

void LC_PlotSettings::setOriginOffsetXMm(const double originOffsetXMm) {
    originOffsetX_mm = originOffsetXMm;
}

double LC_PlotSettings::getOriginOffsetYMm() const {
    return originOffsetY_mm;
}

void LC_PlotSettings::setOriginOffsetYMm(const double originOffsetYMm) {
    originOffsetY_mm = originOffsetYMm;
}

double LC_PlotSettings::getPlotWindowLowerLeftX() const {
    return m_plotWindowLowerLeftX;
}

void LC_PlotSettings::setPlotWindowLowerLeftX(const double plotWindowLowerLeftX) {
    m_plotWindowLowerLeftX = plotWindowLowerLeftX;
}

double LC_PlotSettings::getPlotWindowLowerLeftY() const {
    return m_plotWindowLowerLeftY;
}

void LC_PlotSettings::setPlotWindowLowerLeftY(const double plotWindowLowerLeftY) {
    m_plotWindowLowerLeftY = plotWindowLowerLeftY;
}

double LC_PlotSettings::getPlotWindowUpperRightX() const {
    return m_plotWindowUpperRightX;
}

void LC_PlotSettings::setPlotWindowUpperRightX(const double plotWindowUpperRightX) {
    m_plotWindowUpperRightX = plotWindowUpperRightX;
}

double LC_PlotSettings::getPlotWindowUpperRightY() const {
    return m_plotWindowUpperRightY;
}

void LC_PlotSettings::setPlotWindowUpperRightY(const double plotWindowUpperRightY) {
    m_plotWindowUpperRightY = plotWindowUpperRightY;
}

double LC_PlotSettings::getCustomPrintScalePaperUnitsNumerator() const {
    return m_customPrintScalePaperUnitsNumerator;
}

void LC_PlotSettings::setCustomPrintScalePaperUnitsNumerator(const double customPrintScalePaperUnitsNumerator) {
    m_customPrintScalePaperUnitsNumerator = customPrintScalePaperUnitsNumerator;
}

double LC_PlotSettings::getCustomPrintScaleDrawingUnitsDenominator() const {
    return m_customPrintScaleDrawingUnitsDenominator;
}

void LC_PlotSettings::setCustomPrintScaleDrawingUnitsDenominator(const double customPrintScaleDrawingUnitsDenominator) {
    m_customPrintScaleDrawingUnitsDenominator = customPrintScaleDrawingUnitsDenominator;
}

int LC_PlotSettings::getPlotLayoutFlag() const {
    return m_plotLayoutFlag;
}

void LC_PlotSettings::setPlotLayoutFlag(const int plotLayoutFlag) {
    m_plotLayoutFlag = plotLayoutFlag;
}

int LC_PlotSettings::getPlotPaperUnits() const {
    return m_plotPaperUnits;
}

void LC_PlotSettings::setPlotPaperUnits(const int plotPaperUnits) {
    m_plotPaperUnits = plotPaperUnits;
}

int LC_PlotSettings::getPlotRotation() const {
    return m_plotRotation;
}

void LC_PlotSettings::setPlotRotation(const int plotRotation) {
    m_plotRotation = plotRotation;
}

int LC_PlotSettings::getPlotType() const {
    return m_plotType;
}

void LC_PlotSettings::setPlotType(const int plotType) {
    m_plotType = plotType;
}

int LC_PlotSettings::getStandardScaleType() const {
    return m_standardScaleType;
}

void LC_PlotSettings::setStandardScaleType(const int standardScaleType) {
    m_standardScaleType = standardScaleType;
}

int LC_PlotSettings::getShadePlotMode() const {
    return m_shadePlotMode;
}

void LC_PlotSettings::setShadePlotMode(const int shadePlotMode) {
    m_shadePlotMode = shadePlotMode;
}

int LC_PlotSettings::getShadePlotResolutionMode() const {
    return m_shadePlotResolutionMode;
}

void LC_PlotSettings::setShadePlotResolutionMode(const int shadePlotResolutionMode) {
    m_shadePlotResolutionMode = shadePlotResolutionMode;
}

int LC_PlotSettings::getShadePlotCustomDpi() const {
    return m_shadePlotCustomDPI;
}

void LC_PlotSettings::setShadePlotCustomDpi(const int shadePlotCustomDpi) {
    m_shadePlotCustomDPI = shadePlotCustomDpi;
}

double LC_PlotSettings::getStandardScaleFactor() const {
    return m_standardScaleFactor;
}

void LC_PlotSettings::setStandardScaleFactor(const double standardScaleFactor) {
    m_standardScaleFactor = standardScaleFactor;
}

double LC_PlotSettings::getPaperImageOriginX() const {
    return m_paperImageOriginX;
}

void LC_PlotSettings::setPaperImageOriginX(const double paperImageOriginX) {
    m_paperImageOriginX = paperImageOriginX;
}

double LC_PlotSettings::getPaperImageOriginY() const {
    return m_paperImageOriginY;
}

void LC_PlotSettings::setPaperImageOriginY(const double paperImageOriginY) {
    m_paperImageOriginY = paperImageOriginY;
}

QString LC_PlotSettings::getCurrentStyleName() {
    return m_currentStyleName;
}

void LC_PlotSettings::setCurrentStyleName(const QString& name) {
    m_currentStyleName = name;
}

/**
 * @return Paper size in graphic units.
 */
RS_Vector LC_PlotSettings::getPaperSize() const {
    bool okX = false, okY = false;
    double sX = 0., sY = 0.;
    LC_GROUP_GUARD("Print"); // fixme - rework
    {
        sX = LC_GET_STR("PaperSizeX", "0.0").toDouble(&okX);
        sY = LC_GET_STR("PaperSizeY", "0.0").toDouble(&okY);
    }
    RS_Vector def;
    if (okX && okY && sX > RS_TOLERANCE && sY > RS_TOLERANCE) {
        def = RS_Units::convert(RS_Vector(sX, sY), RS2::Millimeter, getUnit());
    }
    else {
        def = RS_Units::convert(g_paperSizeA4, RS2::Millimeter, getUnit());
    }

    const RS_Vector v1 = m_graphic->getVariableVector("$PLIMMIN", RS_Vector(0.0, 0.0));
    const RS_Vector v2 =  m_graphic->getVariableVector("$PLIMMAX", def);

    return v2 - v1;
}


/**
 * Paper margins in graphic units
 */
void LC_PlotSettings::setMarginsInUnits(const double left, const double top, const double right, const double bottom) {
    const RS2::Unit unit = getUnit();
    setMarginsInMm(RS_Units::convert(left, unit, RS2::Millimeter), RS_Units::convert(top, unit, RS2::Millimeter),
               RS_Units::convert(right, unit, RS2::Millimeter), RS_Units::convert(bottom, unit, RS2::Millimeter));
}

LC_MarginsRect LC_PlotSettings::getMarginsInUnits() const {
    LC_MarginsRect res;
    const RS2::Unit unit = getUnit();
    res.left = RS_Units::convert(m_marginLeft_mm, RS2::Millimeter, unit);
    res.right = RS_Units::convert(m_marginRight_mm, RS2::Millimeter, unit);
    res.top = RS_Units::convert(m_marginTop_mm, RS2::Millimeter, unit);
    res.bottom = RS_Units::convert(m_marginBottom_mm, RS2::Millimeter, unit);
    return res;
}

double LC_PlotSettings::getMarginLeftInUnits() const {
    const RS2::Unit unit = getUnit();
    return RS_Units::convert(m_marginLeft_mm, RS2::Millimeter, unit);
}

double LC_PlotSettings::getMarginTopInUnits() const {
    const RS2::Unit unit = getUnit();
    return RS_Units::convert(m_marginTop_mm, RS2::Millimeter, unit);
}

double LC_PlotSettings::getMarginRightInUnits() const {
    const RS2::Unit unit = getUnit();
    return RS_Units::convert(m_marginRight_mm, RS2::Millimeter, unit);
}

double LC_PlotSettings::getMarginBottomInUnits() const {
    const RS2::Unit unit = getUnit();
    return RS_Units::convert(m_marginBottom_mm, RS2::Millimeter, unit);
}

void LC_PlotSettings::setPagesNum(const int horiz, const int vert) {
    if (horiz > 0) {
        m_pagesNumH = horiz;
    }
    if (vert > 0) {
        m_pagesNumV = vert;
    }
    m_graphic->setModified(true);
}

bool LC_PlotSettings::isBiggerThanPaper(const RS_Vector& size) const {
    const RS2::Unit unit = getUnit();
    const RS_Vector ps = getPrintAreaSize(unit != 0u);
    const RS_Vector s = size * getPaperScale();
    return !s.isInWindow(RS_Vector(0.0, 0.0), ps);
}

RS2::Unit LC_PlotSettings::getUnit() const {
    return m_graphic->getUnit();
}

void LC_PlotSettings::setPagesNum(const QString& horizXvert) {
    if (horizXvert.contains('x')) {
        bool ok1 = false;
        bool ok2 = false;
        const int i = horizXvert.indexOf('x');
        const int h = static_cast<int>(RS_Math::eval(horizXvert.left(i), &ok1));
        const int v = static_cast<int>(RS_Math::eval(horizXvert.mid(i + 1), &ok2));
        if (ok1 && ok2) {
            setPagesNum(h, v);
        }
    }
    m_graphic->setModified(true);
}
