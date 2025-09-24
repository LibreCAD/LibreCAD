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

#include "lc_graphicvariables.h"

#include "rs_graphic.h"

LC_GraphicVariables::LC_GraphicVariables() = default;


void LC_GraphicVariables::loadFromVars(RS_Graphic* g) {
    m_gridOn = g->getVariableBool("$GRIDMODE", true);
    m_isometricGrid = g->getVariableBool("$SNAPSTYLE", false);
    m_anglesBase = g->getVariableDouble("$ANGBASE",0.0);
    m_anglesCounterClockWise = !g->getVariableBool("$ANGDIR", false);
    m_dimStyle = g->getVariableString("$DIMSTYLE", "Standard");
    m_gridViewType= (RS2::IsoGridViewType) g->getVariableInt("$SNAPISOPAIR", RS2::IsoGridViewType::IsoTop);
    m_paperInsertionBase = g->getVariableVector("$PINSBASE", RS_Vector(0.0,0.0));
    m_angleFormat = angleUnitsDXF2LC(g->getVariableInt("$AUNITS", 0));
    m_anglePrecision = g->getVariableInt("$AUPREC", 4);
    m_linearPrecision = g->getVariableInt("$LUPREC", 4);
    m_linearFormat = convertLinearFormatDXF2LC(g->getVariableInt("$LUNITS", 2));
    m_unit = static_cast<RS2::Unit>(g->getVariableInt("$INSUNITS", 0));
}

void LC_GraphicVariables::saveToVars(RS_Graphic* g) {
    g->addVariable("$GRIDMODE", m_gridOn, 70);
    g->addVariable("$SNAPSTYLE", m_isometricGrid, 70);
    g->addVariable("$ANGBASE", m_isometricGrid, 50);
    g->addVariable("$ANGDIR", !m_anglesCounterClockWise, 70);
    g->addVariable("$DIMSTYLE", m_dimStyle, 2);
    g->addVariable("$SNAPISOPAIR", m_gridViewType, 70);
    g->addVariable("$PINSBASE", m_paperInsertionBase, 10);
}

/**
 * @return true if the grid is switched on (visible).
 */
bool LC_GraphicVariables::isGridOn() const {
    return m_gridOn;
}

/**
 * Enables / disables the grid.
 */
void LC_GraphicVariables::setGridOn(bool on) {
    m_gridOn = on;
}

/**
 * @return true if the isometric grid is switched on (visible).
 */
bool LC_GraphicVariables::isIsometricGrid() const{
    return m_isometricGrid;
}

/**
 * Enables / disables isometric grid.
 */
void LC_GraphicVariables::setIsometricGrid(bool on) {
    m_isometricGrid = on;
}

double LC_GraphicVariables::getAnglesBase() const{
   return m_anglesBase;
}

void LC_GraphicVariables::setAnglesBase(double baseAngle){
    m_anglesBase = baseAngle;
}

bool LC_GraphicVariables::areAnglesCounterClockWise() const{
    return m_anglesCounterClockWise;
}

void LC_GraphicVariables::setAnglesCounterClockwise(bool on){
    m_anglesCounterClockWise = on;
}

QString LC_GraphicVariables::getDefaultDimStyleName() {
    return m_dimStyle;
}

void LC_GraphicVariables::setDefaultDimStyleName(QString name) {
    m_dimStyle = name;
}


RS2::IsoGridViewType LC_GraphicVariables::getIsoView() const{
   return m_gridViewType;
}

void LC_GraphicVariables::setIsoView(RS2::IsoGridViewType viewType){
    m_gridViewType = viewType;
}

/**
 * @return The insertion point of the drawing into the paper space.
 * This is the distance from the lower left paper edge to the zero
 * point of the drawing. DXF: $PINSBASE.
 */
RS_Vector LC_GraphicVariables::getPaperInsertionBase() {
    return m_paperInsertionBase;
}

void LC_GraphicVariables::setPaperInsertionBase(const RS_Vector& p) {
    m_paperInsertionBase = p;
}

int LC_GraphicVariables::getAnglePrecision() const {
    return m_anglePrecision;
}

/**
 * @return The linear precision for this document.
 * This is determined by the variable "$LUPREC".
 */
int LC_GraphicVariables::getLinearPrecision() const{
    return m_linearPrecision;
}

/**
 * @return The angle format type for this document.
 * This is determined by the variable "$AUNITS".
 */
RS2::AngleFormat LC_GraphicVariables::getAngleFormat() const{
    return m_angleFormat;
}

/**
 * @return The linear format type for this document.
 * This is determined by the variable "$LUNITS".
 */
RS2::LinearFormat LC_GraphicVariables::getLinearFormat() const{
    return m_linearFormat;

}

RS2::Unit LC_GraphicVariables::getUnit() const {
    return m_unit;
}

/**
 * @return The linear format type used by the variable "$LUNITS" & "$DIMLUNIT".
 */
RS2::LinearFormat LC_GraphicVariables::convertLinearFormatDXF2LC(int f){
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

RS2::AngleFormat LC_GraphicVariables::angleUnitsDXF2LC(int aunits) {
    switch (aunits) {
        case 0:
            return RS2::DegreesDecimal;
        case 1:
            return RS2::DegreesMinutesSeconds;
        case 2:
            return RS2::Gradians;
        case 3:
            return RS2::Radians;
        case 4:
            return RS2::Surveyors;
        default:
            return RS2::DegreesDecimal;
    }
}
