/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2024 LibreCAD.org
 Copyright (C) 2024 sand1024

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

#include "lc_view.h"

LC_View::LC_View() {}

LC_View::LC_View(const QString &name):m_name(name) {}

LC_View* LC_View::clone() {
    auto* clone = new LC_View(m_name);
    clone->m_cameraPlottable = m_cameraPlottable;
    clone->m_flags = m_flags;
    clone->m_viewMode = m_viewMode;
    clone->m_lensLen = m_lensLen;
    clone->m_twistAngle = m_twistAngle;
    clone->m_backClippingPlaneOffset = m_backClippingPlaneOffset;
    clone->m_frontClippingPlaneOffset = m_frontClippingPlaneOffset;
    clone->m_renderMode = m_renderMode;
    clone->m_center = m_center;
    clone->m_size = m_size;
    clone->m_targetPoint = m_targetPoint;
    clone->m_viewDirection = m_viewDirection;
    clone->m_ucs = m_ucs;
    return clone;
}

void LC_View::setName(const QString &n) {
    m_name = n;
}

RS_Vector LC_View::getSize() const {
    return m_size;
}

void LC_View::setSize(RS_Vector s) {
    m_size = s;
}

void LC_View::setCenter(RS_Vector s) {
   m_center = s;
}

RS_Vector LC_View::getCenter() const {
    return m_center;
}

void LC_View::setTargetPoint(RS_Vector p) {
    m_targetPoint = p;
}

RS_Vector LC_View::getTargetPoint() const{
    return m_targetPoint;
}

void LC_View::setCameraPlottable(bool b) {
    m_cameraPlottable = b;
}

bool LC_View::isCameraPlottable() const{
    return m_cameraPlottable;
}

void LC_View::setLensLen(double d){
   m_lensLen = d;
}

double LC_View::getLensLen() const {
    return m_lensLen;
}

void LC_View::setViewDirection(RS_Vector dir) {
    m_viewDirection = dir;
}

const RS_Vector LC_View::getViewDirection() const {
    return m_viewDirection;
}

void LC_View::setFrontClippingPlaneOffset(double d) {
    m_frontClippingPlaneOffset = d;
}

double LC_View::getFrontClippingPlaneOffset() const {
    return m_frontClippingPlaneOffset;
}

void LC_View::setBackClippingPlaneOffset(double d) {
     m_backClippingPlaneOffset = d;
}

double LC_View::getBackClippingPlaneOffset() const {
    return m_backClippingPlaneOffset;
}

bool LC_View::isHasUCS() const {
    return m_ucs != nullptr;
}

void LC_View::setViewMode(int i) {
    m_viewMode = i;
}

int LC_View::getViewMode() const {
    return m_viewMode;
}

void LC_View::setFlags(int i) {
    m_flags = i;
}

int LC_View::getFlags() const {
    return m_flags;
}

void LC_View::setTwistAngle(double d) {
    m_twistAngle = d;
}

double LC_View::getTwistAngle() const{
    return m_twistAngle;
}

void LC_View::setUCS(LC_UCS *pUcs) {
  m_ucs = pUcs;
}

LC_UCS *LC_View::getUCS() const{
    return m_ucs;
}

bool LC_View::isValidName([[maybe_unused]]QString &nameCandidate) {
    // fixme - implement Named View name validation rules there
    return true;
}
