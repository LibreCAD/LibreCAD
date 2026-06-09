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

#include "lc_creation_arc.h"

#include "lc_linemath.h"
#include "rs_circle.h"
#include "rs_debug.h"
#include "rs_information.h"
#include "rs_line.h"
#include "rs_math.h"

namespace {
    double determineArcAngleByLenAndChord(const double arcLen, const double chordLen) {
        const double k = chordLen / arcLen;
        double x = std::sqrt(6 - (6 * k));

        for (int i = 0; i < 6; i++) {
            x = x - ((std::sin(x) - k * x) / (std::cos(x) - k));
        }
        return 2 * x;
    }

}

/**
 * Creates this arc from 3 given points which define the arc line.
 *
 * @param p1 1st point.
 * @param p2 2nd point.
 * @param p3 3rd point.
 */
bool LC_CreationArc::createFrom3P(const RS_Vector& p1, const RS_Vector& p2, const RS_Vector& p3, RS_ArcData& data) {
    const RS_Vector vra = p2 - p1;
    const RS_Vector vrb = p3 - p1;
    const double ra2 = vra.squared() * 0.5;
    const double rb2 = vrb.squared() * 0.5;
    double crossp = (vra.x * vrb.y) - (vra.y * vrb.x);
    if (fabs(crossp) < RS_TOLERANCE2) {
        RS_DEBUG->print(RS_Debug::D_WARNING, "RS_Arc::createFrom3P(): " "Cannot create a arc with radius 0.0.");
        return false;
    }
    crossp = 1. / crossp;
    data.center.set((ra2 * vrb.y - rb2 * vra.y) * crossp, (rb2 * vra.x - ra2 * vrb.x) * crossp);
    data.radius = data.center.magnitude();
    data.center += p1;
    data.angle1 = data.center.angleTo(p1);
    data.angle2 = data.center.angleTo(p3);
    data.reversed = RS_Math::isAngleBetween(data.center.angleTo(p2),data.angle1, data.angle2, true);
    return true;
}

/**
 * Creates an arc from its startpoint, endpoint, start direction (angle)
 * and radius.
 *
 * @retval true Successfully created arc
 * @retval false Cannot create arc (radius to small or endpoint to far away)
 */
bool LC_CreationArc::createFrom2PDirectionRadius(const RS_Vector& startPoint, const RS_Vector& endPoint, const double direction1,
                                         const double radius, RS_ArcData& data) {
    const RS_Vector ortho = RS_Vector::polar(radius, direction1 + M_PI_2);
    const RS_Vector center1 = startPoint + ortho;
    const RS_Vector center2 = startPoint - ortho;

    if (center1.distanceTo(endPoint) < center2.distanceTo(endPoint)) {
        data.center = center1;
    }
    else {
        data.center = center2;
    }

    data.radius = radius;
    data.angle1 = data.center.angleTo(startPoint);
    data.angle2 = data.center.angleTo(endPoint);
    data.reversed = false;

    const double diff = RS_Math::correctAngle(data.getDirection1() - direction1);
    if (fabs(diff - M_PI) < 1.0e-1) {
        data.reversed = true;
    }
    // calculateBorders();

    return true;
}

/**
 * Creates an arc from its startpoint, endpoint, start direction (angle)
 * and angle length.
 *
 * @retval true Successfully created arc
 * @retval false Cannot create arc (radius to small or endpoint to far away)
 */
bool LC_CreationArc::createFrom2PDirectionAngle(const RS_Vector& startPoint, const RS_Vector& endPoint, double direction1, double angleLength, RS_ArcData& data) {
    if (angleLength <= RS_TOLERANCE_ANGLE || angleLength > (2. * M_PI) - RS_TOLERANCE_ANGLE) {
        return false;
    }
    RS_Line l0{nullptr, startPoint, startPoint - RS_Vector{direction1}};
    const double halfA = 0.5 * angleLength;
    l0.rotate(startPoint, halfA);

    double d0;
    RS_Vector vEnd0 = l0.getNearestPointOnEntity(endPoint, false, &d0);
    RS_Line l1 = l0;
    l1.rotate(startPoint, -angleLength);
    double d1;
    RS_Vector vEnd1 = l1.getNearestPointOnEntity(endPoint, false, &d1);
    if (d1 < d0) {
        vEnd0 = vEnd1;
        l0 = l1;
    }

    l0.rotate((startPoint + vEnd0) * 0.5, M_PI_2);

    l1 = RS_Line{nullptr, startPoint, startPoint + RS_Vector{direction1 + M_PI_2}};

    const auto sol = RS_Information::getIntersection(&l0, &l1, false);
    if (sol.empty()) {
        return false;
    }

    data.center = sol.at(0);

    data.radius = data.center.distanceTo(startPoint);
    data.angle1 = data.center.angleTo(startPoint);
    data.reversed = false;

    double diff = RS_Math::correctAngle(data.getDirection1() - direction1);
    if (fabs(diff - M_PI) < 1.0e-1) {
        data.angle2 = RS_Math::correctAngle(data.angle1 - angleLength);
        data.reversed = true;
    }
    else {
        data.angle2 = RS_Math::correctAngle(data.angle1 + angleLength);
    }
    // calculateBorders();
    return true;
}

/**
 * Creates an arc from its startpoint, endpoint and bulge.
 */
bool LC_CreationArc::createFrom2PBulge(const RS_Vector& startPoint, const RS_Vector& endPoint, const double bulge, RS_ArcData& data) {
    data.reversed = bulge < 0.0;
    const double alpha = std::atan(bulge) * 4.0;

    const RS_Vector middle = (startPoint + endPoint) / 2.0;
    const double dist = startPoint.distanceTo(endPoint) / 2.0;

    // alpha can't be 0.0 at this point
    data.radius = std::abs(dist / std::sin(alpha / 2.0));

    const double wu = std::abs((data.radius * data.radius) - (dist * dist));
    double angle = startPoint.angleTo(endPoint);
    const bool reversed = std::signbit(bulge);
    angle = reversed ? angle - M_PI_2 : angle + M_PI_2;

    const double h = (std::abs(alpha) > M_PI) ? -std::sqrt(wu) : std::sqrt(wu);

    data.center.setPolar(h, angle);
    data.center += middle;
    data.angle1 = data.center.angleTo(startPoint);
    data.angle2 = data.center.angleTo(endPoint);
    return true;
}

bool LC_CreationArc::createFrom2PAngle(const RS_Vector& firstPoint, const RS_Vector& secondPoint, double angle, const bool reversed,
                                       const bool alternate, RS_ArcData& data) {
    const double chordAngle = firstPoint.angleTo(secondPoint);
    const double chordAngleNormal = chordAngle + M_PI_2;
    const double chordAngleNormalAlt = chordAngle - M_PI_2;

    const double chordLen = secondPoint.distanceTo(firstPoint);
    const double chordLenHalf = chordLen * 0.5;

    const double angleHalf = angle*0.5;

    const double distanceFromChordCenterToCenter = chordLenHalf / tan(angleHalf);

    const RS_Vector chordLenHalfPont = (firstPoint + secondPoint) * 0.5;

    bool reverseArc = reversed;
    if (alternate){
        reverseArc = !reverseArc;
    }

    const double angleToCenter = /*reverseArc*/ reversed ? chordAngleNormalAlt : chordAngleNormal;
    const RS_Vector center = chordLenHalfPont.relative(distanceFromChordCenterToCenter, angleToCenter);

    const double radius = center.distanceTo(firstPoint);
    const double angle1 = center.angleTo(firstPoint);
    const double angle2 = center.angleTo(secondPoint);

    data.angle1 = angle1;
    data.angle2 = angle2;
    data.reversed = reverseArc;
    data.radius = radius;
    data.center = center;
    return true;
}

bool LC_CreationArc::createFrom2PHeight(const RS_Vector& firstPoint, const RS_Vector& secondPoint, double height, const bool reversed,
                                       const bool alternate, RS_ArcData& data) {

    double chordLen = firstPoint.distanceTo(secondPoint);
    double arcHeight = height;

    double radius = (arcHeight / 2) + ((chordLen * chordLen) / (8 * arcHeight));


    auto circle1 = RS_Circle(nullptr, RS_CircleData(firstPoint, radius));
    auto circle2 = RS_Circle(nullptr, RS_CircleData(secondPoint, radius));

    const RS_VectorSolutions &intersections = RS_Information::getIntersection(&circle1, &circle2);

    RS_Vector center;

    bool reverseArc = reversed;
    if (alternate){
        reverseArc = !reverseArc;
    }

    RS_Vector pointForAngle2 = secondPoint;
    if (intersections.size() == 2) {
        RS_Vector ipRight, ipLeft;
        int pointPosition = LC_LineMath::getPointPosition(firstPoint, secondPoint, intersections[0]);
        if (pointPosition == LC_LineMath::PointToLinePosition::RIGHT) {
            ipRight = intersections[0];
            ipLeft = intersections[1];
        }
        else {
            ipLeft = intersections[0];
            ipRight = intersections[1];
        }
        bool heightLargeThanHalfChord = arcHeight > (chordLen / 2);

        if (reversed) {
            center = heightLargeThanHalfChord ? ipLeft : ipRight;
        } else {
            center = heightLargeThanHalfChord ? ipRight : ipLeft;
        }
    } else {
        auto v = RS_Vector();
        v.setPolar(radius, firstPoint.angleTo(secondPoint));
        center = firstPoint + v;
        pointForAngle2 = firstPoint + v*2.0;
    }

    data.center = center;
    data.reversed = reverseArc;
    data.radius = radius;
    data.angle1 = data.center.angleTo(firstPoint);
    data.angle2 = data.center.angleTo(pointForAngle2);
    return true;
}

bool LC_CreationArc::createFrom2PArcLength(const RS_Vector& firstPoint, const RS_Vector& secondPoint, double arcLen, const bool reversed,
                                       const bool alternate, RS_ArcData& data) {

    double chordLen = firstPoint.distanceTo(secondPoint);

    if (chordLen >= arcLen) {
        return false;
    }

    double angle = determineArcAngleByLenAndChord(arcLen, chordLen);
    double radius = chordLen/(2.0 * std::sin(angle/2.0));

    auto circle1 = RS_Circle(nullptr, RS_CircleData(firstPoint, radius));
    auto circle2 = RS_Circle(nullptr, RS_CircleData(secondPoint, radius));

    const RS_VectorSolutions &intersections = RS_Information::getIntersection(&circle2, &circle1, false);

    RS_Vector center;

    bool reverseArc = reversed;
    if (alternate){
        reverseArc = !reverseArc;
    }

    RS_Vector pointForAngle2 = secondPoint;
    if (intersections.size() == 2) {
        RS_Vector ipRight, ipLeft;
        int pointPosition = LC_LineMath::getPointPosition(firstPoint, secondPoint, intersections[0]);
        if (pointPosition == LC_LineMath::PointToLinePosition::RIGHT) {
            ipRight = intersections[0];
            ipLeft = intersections[1];
        }
        else {
            ipLeft = intersections[0];
            ipRight = intersections[1];
        }
        bool angleLessPI = angle < M_PI;

        if (reversed) {
            center = angleLessPI ? ipRight : ipLeft;
        } else {
            center = angleLessPI ? ipLeft : ipRight;
        }
    } else {
        double chordAngle = firstPoint.angleTo(secondPoint);
        const auto v = RS_Vector::polar(radius, chordAngle);
        center = firstPoint + v;
        pointForAngle2 = firstPoint + v*2.0;
    }

    data.center = center;
    data.reversed = reverseArc;
    data.radius = radius;
    data.angle1 = data.center.angleTo(firstPoint);
    data.angle2 = data.center.angleTo(pointForAngle2);
    return true;
}

bool LC_CreationArc::createFrom2PRadius(const RS_Vector& firstPoint, const RS_Vector& secondPoint, const double radius, const bool reversed,
                                       const bool alternate, RS_ArcData& data) {
    double chordLen = firstPoint.distanceTo(secondPoint);
    double chordHalf = chordLen * 0.5;

    const double chordAngle = firstPoint.angleTo(secondPoint);

    RS_Vector chordLenHalfPont = (firstPoint + secondPoint) * 0.5;

    RS_Vector pointForAngle2 = secondPoint;
    if (chordHalf >= radius) {
        chordLen = radius * 2;
        chordHalf = radius;
        pointForAngle2 = firstPoint.relative(chordLen, chordAngle);
        chordLenHalfPont = (firstPoint + pointForAngle2) * 0.5;
    }

    double distanceFromChordCenterToCenter = 0.0;
    if ((radius - chordHalf) > RS_TOLERANCE) {
        const double triangleLegSquared = (radius * radius) - (chordHalf * chordHalf);
        if (triangleLegSquared > 0) {
            distanceFromChordCenterToCenter = sqrt(triangleLegSquared);
        }
    }

    const double chordAngleNormal = chordAngle + M_PI_2;
    const double chordAngleNormalAlt = chordAngle - M_PI_2;

    bool reverseArc = reversed;
    if (alternate){
        reverseArc = !reverseArc;
    }

    const double angleToCenter = /*reverseArc*/ reversed ? chordAngleNormalAlt : chordAngleNormal;
    const RS_Vector center = chordLenHalfPont.relative(distanceFromChordCenterToCenter, angleToCenter);

    data.center = center;
    data.reversed = reverseArc;
    data.radius = radius;
    data.angle1 = data.center.angleTo(firstPoint);
    data.angle2 = data.center.angleTo(pointForAngle2);
    return true;
}
