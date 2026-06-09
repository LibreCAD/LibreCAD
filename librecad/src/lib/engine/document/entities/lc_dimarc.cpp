/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2021 Melwyn Francis Carlo <carlo.melwyn@outlook.com>
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file gpl-2.0.txt included in the
** packaging of this file.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
**
** This copyright notice MUST APPEAR in all copies of the script!
**
**********************************************************************/

#include "lc_dimarc.h"

#include <iostream>

#include "lc_rect.h"
#include "rs_arc.h"
#include "rs_debug.h"
#include "rs_graphic.h"
#include "rs_line.h"
#include "rs_solid.h"
#include "rs_units.h"

namespace {
    // trunc a floating point to multiplier of the giving unitLength
    void truncF(double& length, const double unitLength) {
        length -= std::fmod(length, unitLength);
    }

    // trunc a point to a 2D grid with spacing of the given unit length.
    void truncF(RS_Vector& point, const double unitLength) {
        truncF(point.x, unitLength);
        truncF(point.y, unitLength);
    }
}

LC_DimArcData::LC_DimArcData(const LC_DimArcData& other)
    : centre(other.centre), endAngle(other.endAngle), startAngle(other.startAngle), radius(other.radius), arcLength(other.arcLength) {
}

LC_DimArcData::LC_DimArcData(const double radius, const double arcLength, const RS_Vector& centre, const RS_Vector& endAngle,
                             const RS_Vector& startAngle)
    : centre(centre), endAngle(endAngle), startAngle(startAngle), radius(radius), arcLength(arcLength) {
}

LC_DimArc::LC_DimArc(const LC_DimArc& other) : RS_Dimension(other), m_dimArcData{other.m_dimArcData} {
}

LC_DimArc::LC_DimArc(RS_EntityContainer* parent, const RS_DimensionData& commonDimData, const LC_DimArcData& dimArcData)
    : RS_Dimension(parent, commonDimData), m_dimArcData(dimArcData) {
    LC_DimArc::update();
}

RS_Entity* LC_DimArc::clone() const {
    const auto clone = new LC_DimArc(*this);
    return clone;
}

QString LC_DimArc::getMeasuredLabel() {
    const RS_Graphic* currentGraphic = getGraphic();

    QString measuredLabel;

    if (currentGraphic != nullptr) {
        const int dimlunit{getGraphicVariableInt(QStringLiteral("$DIMLUNIT"), 2)};
        const int dimdec{getGraphicVariableInt(QStringLiteral("$DIMDEC"), 4)};

        const RS2::LinearFormat format = currentGraphic->convertLinearFormatDXF2LC(dimlunit);

        measuredLabel = RS_Units::formatLinear(m_dimArcData.arcLength, getGraphicUnit(), format, dimdec);

        if (format == RS2::Decimal) {
            const int dimzin{getGraphicVariableInt(QStringLiteral("$DIMZIN"), 1)};
            measuredLabel = stripZerosLinear(measuredLabel, dimzin);
        }

        if ((format == RS2::Decimal) || (format == RS2::ArchitecturalMetric)) {
            if (getGraphicVariableInt("$DIMDSEP", 0) == 44) {
                measuredLabel.replace(QChar('.'), QChar(','));
            }
        }
    }
    else {
        measuredLabel = QString("%1").arg(m_dimArcData.arcLength);
    }

    m_dimMeasurement = m_dimArcData.arcLength;

    return measuredLabel;
}

void LC_DimArc::arrow(const RS_Vector& point, const double angle, const double direction, const RS_Pen& pen) {
    if ((getTickSize() * getGeneralScale()) < 0.01) {
        double endAngle{0.0};

        if (m_dimArcData.radius > RS_TOLERANCE_ANGLE) {
            endAngle = getArrowSize() / m_dimArcData.radius;
        }

        const RS_Vector arrowEnd = RS_Vector::polar(m_dimArcData.radius, angle + std::copysign(endAngle, direction)) + m_dimArcData.centre;

        const double arrowAngle{arrowEnd.angleTo(point)};

        const RS_SolidData dummyVar;

        const auto arrow = new RS_Solid(this, dummyVar);
        arrow->shapeArrow(point, arrowAngle, getArrowSize() * getGeneralScale());
        arrow->setPen(pen);
        arrow->setLayer(nullptr);
        addEntity(arrow);
    }
    else {
        constexpr double deg45 = M_PI_2 / 2.0;

        const double midAngle = (m_dimArcData.startAngle.angle() + m_dimArcData.endAngle.angle()) / 2.0;

        const RS_Vector tickVector = RS_Vector::polar(getTickSize() * getGeneralScale(), midAngle - deg45);

        auto* tick = new RS_Line(this, point - tickVector, point + tickVector);
        tick->setPen(pen);
        tick->setLayer(nullptr);
        addEntity(tick);
    }
}

void LC_DimArc::doUpdateDim() {
    RS_DEBUG->print("LC_DimArc::update");

    clear();

    if (isDeleted()) {
        return;
    }
    if (!m_dimArcData.centre.valid) {
        return;
    }

    calcDimension();

    RS_Pen pen(getExtensionLineColor(), getExtensionLineWidth(), RS2::LineByBlock);

    pen.setWidth(getDimensionLineWidth());
    pen.setColor(getDimensionLineColor());

    m_extLine1->setPen(pen);
    m_extLine2->setPen(pen);

    m_extLine1->setLayer(nullptr);
    m_extLine2->setLayer(nullptr);

    addEntity(m_extLine1);
    addEntity(m_extLine2);

    auto refArc = RS_Arc(this, RS_ArcData(m_dimArcData.centre, m_dimArcData.radius, m_dimArcData.startAngle.angle(),
                                          m_dimArcData.endAngle.angle(), false));

    arrow(m_arrowStartPoint, m_dimArcData.startAngle.angle(), +1.0, pen);
    arrow(m_arrowEndPoint, m_dimArcData.endAngle.angle(), -1.0, pen);

    double textAngle{0.0};

    RS_Vector textPos{refArc.getMiddlePoint()};

    const double textAnglePreliminary{std::trunc((textPos.angleTo(m_dimArcData.centre) - M_PI) * 1.0E+10) * 1.0E-10};

    if (!this->getInsideHorizontalText()) {
        RS_Vector textPosOffset;

        constexpr double deg360{M_PI * 2.0};

        constexpr double degTolerance{1.0E-3};

        /* With regards to Quadrants #1 and #2 */
        if (((textAnglePreliminary >= -degTolerance) && (textAnglePreliminary <= (M_PI + degTolerance))) || ((textAnglePreliminary <= -(M_PI
            - degTolerance)) && (textAnglePreliminary >= -(deg360 + degTolerance)))) {
            textPosOffset.setPolar(getDimensionLineGap() * getGeneralScale(), textAnglePreliminary);
            textAngle = textAnglePreliminary + M_PI + M_PI_2;
        }
        /* With regards to Quadrants #3 and #4 */
        else {
            textPosOffset.setPolar(getDimensionLineGap() * getGeneralScale(), textAnglePreliminary + M_PI);
            textAngle = textAnglePreliminary + M_PI_2;
        }
    }

    QString dimLabel{getLabel()};

    bool ok = false;

    dimLabel.toDouble(&ok);

    if (ok) {
        dimLabel.prepend("∩ ");
    }

    auto textData{
        RS_MTextData(textPos, getTextHeight() * getGeneralScale(), 30.0, RS_MTextData::VABottom, RS_MTextData::HACenter,
                     RS_MTextData::LeftToRight, RS_MTextData::Exact, 1.0, dimLabel, QString("unicode"), textAngle)
    };

    auto text = std::make_unique<RS_MText>(this, textData);

    text->setPen(RS_Pen(getTextColor(), RS2::WidthByBlock, RS2::SolidLine));
    text->setLayer(nullptr);
    addEntity(text.get());

    double halfWidthPlusGap = text->getUsedTextWidth() / 2.0 + getDimensionLineGap() * getGeneralScale();
    double halfHeightPlusGap = getTextHeight() / 2.0 + getDimensionLineGap() * getGeneralScale();

    text->move(-RS_Vector::polar(getTextHeight() / 2.0, textAngle + M_PI_2));

    /* Text rectangle's corners : top left, top right, bottom right, bottom left. */
    RS_Vector textRectCorners[4] = {
        RS_Vector(false),
        RS_Vector(textPos + RS_Vector(+halfWidthPlusGap, +halfHeightPlusGap)),
        RS_Vector(false),
        RS_Vector(textPos + RS_Vector(-halfWidthPlusGap, -halfHeightPlusGap))
    };

    RS_Vector cornerTopRight{textRectCorners[1]};
    RS_Vector cornerBottomLeft{textRectCorners[3]};
    LC_Rect textRect{cornerBottomLeft, cornerTopRight};

    textRectCorners[0] = textRect.upperLeftCorner();
    textRectCorners[2] = textRect.lowerRightCorner();

    LC_Rect textRectRotated{};
    for (RS_Vector& corner : textRectCorners) {
        corner.rotate(textPos, text->getAngle());
        truncF(corner, 1.0E-4);
        textRectRotated = textRectRotated.merge(corner);
    }

    if (RS_DEBUG->getLevel() == RS_Debug::D_INFORMATIONAL) {
        std::cout << std::endl << " LC_DimArc::updateEntity: Text position / angle : " << textPos << " / " << text->getAngle() << std::endl;

        std::cout << std::endl << " LC_DimArc::updateEntity: Reference arc middle point : " << refArc.getMiddlePoint() << std::endl;

        std::cout << std::endl << " LC_DimArc::updateEntity: DimArc-1 start point : " << m_dimArc1->getStartpoint() << std::endl;

        std::cout << std::endl << " LC_DimArc::updateEntity: DimArc-2 start point : " << m_dimArc2->getStartpoint() << std::endl;

        std::cout << std::endl << " LC_DimArc::updateEntity: Text rectangle corners : " << textRectCorners[0] << ", " << textRectCorners[1]
            << ", " << textRectCorners[2] << ", " << textRectCorners[3] << std::endl;
    }

    text.release();

    //TODO: the current algorithm to find dimArc1/dimArc2 angles could be
    // costly.
    constexpr double deltaOffset{1.0E-2};

    while (!textRectRotated.inArea(m_dimArc1->getEndpoint()) && (m_dimArc1->getAngle2() < RS_MAXDOUBLE) && (m_dimArc1->getAngle2() >
        RS_MINDOUBLE)) {
        m_dimArc1->setAngle2(m_dimArc1->getAngle2() + deltaOffset);
    }

    while (!textRectRotated.inArea(m_dimArc2->getStartpoint()) && (m_dimArc2->getAngle1() < RS_MAXDOUBLE) && (m_dimArc2->getAngle1() >
        RS_MINDOUBLE)) {
        m_dimArc2->setAngle1(m_dimArc2->getAngle1() - deltaOffset);
    }

    m_dimArc1->setPen(pen);
    m_dimArc2->setPen(pen);

    m_dimArc1->setLayer(nullptr);
    m_dimArc2->setLayer(nullptr);

    addEntity(m_dimArc1);
    addEntity(m_dimArc2);

    calculateBorders();
}

void LC_DimArc::update() {
    RS_Dimension::update();
    LC_DimArc::updateDim();
}

void LC_DimArc::move(const RS_Vector& offset) {
    RS_Dimension::move(offset);

    m_dimArcData.centre.move(offset);

    update();
}

void LC_DimArc::rotate(const RS_Vector& center, const double angle) {
    rotate(center, RS_Vector(angle));

    update();
}

void LC_DimArc::rotate(const RS_Vector& center, const RS_Vector& angleVector) {
    RS_Dimension::rotate(center, angleVector);

    m_dimArcData.centre.rotate(center, angleVector);
    const double arcDeltaAngle{m_dimArcData.startAngle.angleTo(m_dimArcData.endAngle)};
    m_dimArcData.startAngle = RS_Vector(m_dimGenericData.definitionPoint.angleTo(m_dimArcData.centre) - M_PI);
    m_dimArcData.endAngle = m_dimArcData.startAngle;
    m_dimArcData.endAngle.rotate(arcDeltaAngle);
    update();
}

void LC_DimArc::scale(const RS_Vector& center, const RS_Vector& factor) {
    const double adjustedFactor = factor.x < factor.y ? factor.x : factor.y;

    const RS_Vector adjustedFactorVector(adjustedFactor, adjustedFactor);
    RS_Dimension::scale(center, adjustedFactorVector);
    m_dimArcData.centre.scale(center, adjustedFactorVector);
    m_dimArcData.radius *= adjustedFactor;
    update();
}

void LC_DimArc::mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) {
    RS_Dimension::mirror(axisPoint1, axisPoint2);

    m_dimArcData.centre.mirror(axisPoint1, axisPoint2);

    /*
        // Just another way of accomplishing the operation below this comment.

        dimStartPoint.mirror (axisPoint1, axisPoint2);
        dimEndPoint.mirror   (axisPoint1, axisPoint2);

        dimArcData.startAngle = RS_Vector((dimStartPoint - dimArcData.centre).angle() - M_PI);
        dimArcData.endAngle   = RS_Vector((dimEndPoint   - dimArcData.centre).angle() - M_PI);
    */

    const RS_Vector originPoint(0.0, 0.0);

    const RS_Vector deltaAxisPoints = axisPoint2 - axisPoint1;

    /* The minus one (-1) value denotes that mirroring changes direction (and hence, sign). */
    m_dimArcData.startAngle.setPolar(-1.0, m_dimArcData.startAngle.angle());
    m_dimArcData.endAngle.setPolar(-1.0, m_dimArcData.endAngle.angle());

    m_dimArcData.startAngle.mirror(originPoint, deltaAxisPoints);
    m_dimArcData.endAngle.mirror(originPoint, deltaAxisPoints);

    update();
}

RS_Vector LC_DimArc::truncateVector(const RS_Vector& v) {
    return RS_Vector(std::trunc(v.x * 1.0E+10) * 1.0E-10, std::trunc(v.y * 1.0E+10) * 1.0E-10, 0.0);
}

void LC_DimArc::calcDimension() {
    const double endAngle = m_dimArcData.endAngle.angle();
    const double startAngle = m_dimArcData.startAngle.angle();

    m_dimArc1 = new RS_Arc(this, RS_ArcData(m_dimArcData.centre, m_dimArcData.radius, startAngle, startAngle, false));
    m_dimArc2 = new RS_Arc(this, RS_ArcData(m_dimArcData.centre, m_dimArcData.radius, endAngle, endAngle, false));

    RS_Vector entityStartPoint = truncateVector(m_dimGenericData.definitionPoint);
    const double entityRadius = m_dimArcData.centre.distanceTo(entityStartPoint);
    RS_Vector entityEndPoint = truncateVector(m_dimArcData.centre + RS_Vector(m_dimArcData.endAngle).scale(entityRadius));
    m_dimStartPoint = m_dimArcData.centre + RS_Vector(m_dimArcData.startAngle).scale(m_dimArcData.radius);
    m_dimEndPoint = m_dimArcData.centre + RS_Vector(m_dimArcData.endAngle).scale(m_dimArcData.radius);

    m_arrowStartPoint = m_dimStartPoint;
    m_arrowEndPoint = m_dimEndPoint;

    entityStartPoint += RS_Vector::polar(getExtensionLineOffset(), entityStartPoint.angleTo(m_dimStartPoint));
    entityEndPoint += RS_Vector::polar(getExtensionLineOffset(), entityEndPoint.angleTo(m_dimEndPoint));
    m_dimStartPoint += RS_Vector::polar(getExtensionLineExtension(), entityStartPoint.angleTo(m_dimStartPoint));
    m_dimEndPoint += RS_Vector::polar(getExtensionLineExtension(), entityEndPoint.angleTo(m_dimEndPoint));

    m_extLine1 = new RS_Line(this, entityStartPoint, m_dimStartPoint);
    m_extLine2 = new RS_Line(this, entityEndPoint, m_dimEndPoint);

    /* RS_DEBUG->setLevel(RS_Debug::D_INFORMATIONAL); */

    RS_DEBUG->print(RS_Debug::D_INFORMATIONAL, "\n LC_DimArc::calcDimension: Start / end angles : %lf / %lf\n", startAngle, endAngle);

    RS_DEBUG->print(RS_Debug::D_INFORMATIONAL, "\n LC_DimArc::calcDimension: Dimension / entity radii : %lf / %lf\n", m_dimArcData.radius,
                    entityRadius);

    if (RS_DEBUG->getLevel() == RS_Debug::D_INFORMATIONAL) {
        std::cout << std::endl << " LC_DimArc::calcDimension: Start Points : " << entityStartPoint << " to " << m_dimStartPoint <<
            std::endl;

        std::cout << std::endl << " LC_DimArc::calcDimension: End Points : " << entityEndPoint << " to " << m_dimEndPoint << std::endl;
    }
}

std::ostream& operator <<(std::ostream& os, const LC_DimArc& dimArc) {
    os << " DimArc Information : \n" << dimArc.getData() << std::endl << std::endl;
    return os;
}

std::ostream& operator <<(std::ostream& os, const LC_DimArcData& dimArc) {
    os << " {\n\tCentre      : " << dimArc.centre << "\n\tRadius      : " << dimArc.radius << "\n\tStart Angle : " << dimArc.startAngle <<
        "\n\tEnd   Angle : " << dimArc.endAngle << "\n}" << std::endl << std::endl;

    return os;
}
