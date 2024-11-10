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

LC_View::LC_View(const QString &name):name(name) {}

void LC_View::setName(const QString &n) {
    name = n;
}

RS_Vector LC_View::getSize() const {
    return size;
}

void LC_View::setSize(RS_Vector s) {
    size = s;
}

void LC_View::setCenter(RS_Vector s) {
   center = s;
}

RS_Vector LC_View::getCenter() const {
    return center;
}

void LC_View::setTargetPoint(RS_Vector p) {
    targetPoint = p;
}

RS_Vector LC_View::getTargetPoint() const{
    return targetPoint;
}

void LC_View::setCameraPlottable(bool b) {
    cameraPlottable = b;
}

bool LC_View::isCameraPlottable() const{
    return cameraPlottable;
}

void LC_View::setLensLen(double d){
   lensLen = d;
}

double LC_View::getLensLen() const {
    return lensLen;
}

void LC_View::setViewDirection(RS_Vector dir) {
    viewDirection = dir;
}

const RS_Vector LC_View::getViewDirection() const {
    return viewDirection;
}

void LC_View::setFrontClippingPlaneOffset(double d) {
    frontClippingPlaneOffset = d;
}

double LC_View::getFrontClippingPlaneOffset() const {
    return frontClippingPlaneOffset;
}

void LC_View::setBackClippingPlaneOffset(double d) {
     backClippingPlaneOffset = d;
}

double LC_View::getBackClippingPlaneOffset() const {
    return backClippingPlaneOffset;
}

bool LC_View::isHasUCS() const {
    return ucs != nullptr;
}

void LC_View::setViewMode(int i) {
    viewMode = i;
}

int LC_View::getViewMode() const {
    return viewMode;
}

void LC_View::setFlags(int i) {
    flags = i;
}

int LC_View::getFlags() const {
    return flags;
}

void LC_View::setTwistAngle(double d) {
    twistAngle = d;
}

double LC_View::getTwistAngle() const{
    return twistAngle;
}

void LC_View::setUCS(LC_UCS *pUcs) {
  ucs = pUcs;
}

LC_UCS *LC_View::getUCS() const{
    return ucs;
}
