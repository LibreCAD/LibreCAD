/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
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

#include<cmath>
#include<iostream>
#include<limits>

#include<QObject>
#include<QStringList>

#include "rs_units.h"

#include "rs_debug.h"
#include "rs_math.h"
#include "rs_vector.h"

namespace {

/**
 * @brief isAbsInRange whether the absolute value is in the range of [0, unsigned_max]
 * @param double length - the value to check
 * @return bool - true, the absolute value of the input can be rounded up to a an unsigned integer
 */
bool isAbsInRange(double length)
{
    if (std::isnan(length) || !std::isfinite(length) ) {
        RS_DEBUG->print(RS_Debug::D_ERROR, "length=%lg", length);
        return false;
    }
    if (std::abs(length) > std::numeric_limits<unsigned>::max()) {
        RS_DEBUG->print(RS_Debug::D_ERROR, "length=%lg is too big in magnitude for the current dimension type", length);
        return false;
    }
    return true;
}

}

/**
 * Converts a DXF integer () to a Unit enum.
 */
RS2::Unit RS_Units::dxfint2unit(int dxfint) {
    return (RS2::Unit)dxfint;

    /*switch(dxfint) {
    default:
    case  0:
        return RS2::None;
    case  1:
        return RS2::Inch;
    case  2:
        return RS2::Foot;
    case  3:
        return RS2::Mile;
    case  4:
        return RS2::Millimeter;
    case  5:
        return RS2::Centimeter;
    case  6:
        return RS2::Meter;
    case  7:
        return RS2::Kilometer;
    case  8:
        return RS2::Microinch;
    case  9:
        return RS2::Mil;
    case 10:
        return RS2::Yard;
    case 11:
        return RS2::Angstrom;
    case 12:
        return RS2::Nanometer;
    case 13:
        return RS2::Micron;
    case 14:
        return RS2::Decimeter;
    case 15:
        return RS2::Decameter;
    case 16:
        return RS2::Hectometer;
    case 17:
        return RS2::Gigameter;
    case 18:
        return RS2::Astro;
    case 19:
        return RS2::Lightyear;
    case 20:
        return RS2::Parsec;
}*/
}


/**
 * @return a short string representing the given unit (e.g. "mm")
 */
QString RS_Units::unitToSign(RS2::Unit u) {
    QString ret = "";

    switch (u) {
    case RS2::None:
        ret = "";
        break;
    case RS2::Inch:
        ret = "\"";
        break;
    case RS2::Foot:
        ret = "'";
        break;
    case RS2::Mile:
        ret = "mi";
        break;
    case RS2::Millimeter:
        ret = "mm";
        break;
    case RS2::Centimeter:
        ret = "cm";
        break;
    case RS2::Meter:
        ret = "m";
        break;
    case RS2::Kilometer:
        ret = "km";
        break;
    case RS2::Microinch:
        ret = "µ\"";
        break;
    case RS2::Mil:
        ret = "mil";
        break;
    case RS2::Yard:
        ret = "yd";
        break;
    case RS2::Angstrom:
        ret = "A";
        break;
    case RS2::Nanometer:
        ret = "nm";
        break;
    case RS2::Micron:
        ret = "µm";
        break;
    case RS2::Decimeter:
        ret = "dm";
        break;
    case RS2::Decameter:
        ret = "dam";
        break;
    case RS2::Hectometer:
        ret = "hm";
        break;
    case RS2::Gigameter:
        ret = "Gm";
        break;
    case RS2::Astro:
        ret = "astro";
        break;
    case RS2::Lightyear:
        ret = "ly";
        break;
    case RS2::Parsec:
        ret = "pc";
        break;

    default:
        ret = "";
        break;
    }

    return ret;
}



/**
 * @return a string representing the given unit (e.g. "Millimeter").
 *      translated if @a t is true (the default).
 */
QString RS_Units::unitToString(RS2::Unit u, bool t) {
    switch (u) {
    case RS2::None:
		return t ? QObject::tr("None", "unknown length unit") : "None";
    case RS2::Inch:
		return t ? QObject::tr("Inch") : "Inch";
    case RS2::Foot:
		return t ? QObject::tr("Foot") : "Foot";
    case RS2::Mile:
		return t ? QObject::tr("Mile") : "Mile";
    case RS2::Millimeter:
		return t ? QObject::tr("Millimeter") : "Millimeter";
    case RS2::Centimeter:
		return t ? QObject::tr("Centimeter") : "Centimeter";
    case RS2::Meter:
		return t ? QObject::tr("Meter") : "Meter";
    case RS2::Kilometer:
		return t ? QObject::tr("Kilometer") : "Kilometer";
    case RS2::Microinch:
		return t ? QObject::tr("Microinch") : "Microinch";
    case RS2::Mil:
		return t ? QObject::tr("Mil") : "Mil";
    case RS2::Yard:
		return t ? QObject::tr("Yard") : "Yard";
    case RS2::Angstrom:
		return t ? QObject::tr("Angstrom") : "Angstrom";
    case RS2::Nanometer:
		return t ? QObject::tr("Nanometer") : "Nanometer";
    case RS2::Micron:
		return t ? QObject::tr("Micron") : "Micron";
    case RS2::Decimeter:
		return t ? QObject::tr("Decimeter") : "Decimeter";
    case RS2::Decameter:
		return t ? QObject::tr("Decameter") : "Decameter";
    case RS2::Hectometer:
		return t ? QObject::tr("Hectometer") : "Hectometer";
    case RS2::Gigameter:
		return t ? QObject::tr("Gigameter") : "Gigameter";
    case RS2::Astro:
		return t ? QObject::tr("Astro") : "Astro";
    case RS2::Lightyear:
		return t ? QObject::tr("Lightyear") : "Lightyear";
    case RS2::Parsec:
		return t ? QObject::tr("Parsec") : "Parsec";

    default:
		return "";
    }

}



/**
 * Converts a string into a unit enum.
 */
RS2::Unit RS_Units::stringToUnit(const QString& u) {
    RS2::Unit ret = RS2::None;

    if (u=="None") {
        ret = RS2::None;
    } else if (u==QObject::tr("Inch")) {
        ret = RS2::Inch;
    } else if (u==QObject::tr("Foot")) {
        ret = RS2::Foot;
    } else if (u==QObject::tr("Mile")) {
        ret = RS2::Mile;
    } else if (u==QObject::tr("Millimeter")) {
        ret = RS2::Millimeter;
    } else if (u==QObject::tr("Centimeter")) {
        ret = RS2::Centimeter;
    } else if (u==QObject::tr("Meter")) {
        ret = RS2::Meter;
    } else if (u==QObject::tr("Kilometer")) {
        ret = RS2::Kilometer;
    } else if (u==QObject::tr("Microinch")) {
        ret = RS2::Microinch;
    } else if (u==QObject::tr("Mil")) {
        ret = RS2::Mil;
    } else if (u==QObject::tr("Yard")) {
        ret = RS2::Yard;
    } else if (u==QObject::tr("Angstrom")) {
        ret = RS2::Angstrom;
    } else if (u==QObject::tr("Nanometer")) {
        ret = RS2::Nanometer;
    } else if (u==QObject::tr("Micron")) {
        ret = RS2::Micron;
    } else if (u==QObject::tr("Decimeter")) {
        ret = RS2::Decimeter;
    } else if (u==QObject::tr("Decameter")) {
        ret = RS2::Decameter;
    } else if (u==QObject::tr("Hectometer")) {
        ret = RS2::Hectometer;
    } else if (u==QObject::tr("Gigameter")) {
        ret = RS2::Gigameter;
    } else if (u==QObject::tr("Astro")) {
        ret = RS2::Astro;
    } else if (u==QObject::tr("Lightyear")) {
        ret = RS2::Lightyear;
    } else if (u==QObject::tr("Parsec")) {
        ret = RS2::Parsec;
    }

    return ret;
}




/**
 * @return true: the unit is metric, false: the unit is imperial.
 */
bool RS_Units::isMetric(RS2::Unit u) {
	switch (u) {
	case RS2::Millimeter:
	case RS2::Centimeter:
	case RS2::Meter:
	case RS2::Kilometer:
	case RS2::Angstrom:
	case RS2::Nanometer:
	case RS2::Micron:
	case RS2::Decimeter:
	case RS2::Decameter:
	case RS2::Hectometer:
	case RS2::Gigameter:
	case RS2::Astro:
	case RS2::Lightyear:
	case RS2::Parsec:
		return true;
	default:
		return false;
	}
}

/**
 * @return factor to convert the given unit to Millimeters.
 */
double RS_Units::getFactorToMM(RS2::Unit u) {
    switch (u) {
	default:
	case RS2::None:
	case RS2::Millimeter:
		return 1.0;
    case RS2::Inch:
		return 25.4;
    case RS2::Foot:
		return 304.8;
    case RS2::Mile:
		return 1.609344e6; //international mile
    case RS2::Centimeter:
		return 10;
    case RS2::Meter:
		return 1e3;
    case RS2::Kilometer:
		return 1e6;
    case RS2::Microinch:
		return 2.54e-5;
    case RS2::Mil:
		return 0.0254;
    case RS2::Yard:
		return 914.4;
    case RS2::Angstrom:
		return 1e-7;
    case RS2::Nanometer:
		return 1e-6;
    case RS2::Micron:
		return 1e-3;
    case RS2::Decimeter:
		return 100.0;
    case RS2::Decameter:
		return 1e4;
    case RS2::Hectometer:
		return 1e5;
    case RS2::Gigameter:
		return 1e9;
    case RS2::Astro:
		return 1.495978707e14;
    case RS2::Lightyear:
		return 9.4607304725808e18;
    case RS2::Parsec:
		return 3.0856776e19;
    }

}


/**
 * Converts the given value 'val' from unit 'src' to unit 'dest'.
 */
double RS_Units::convert(double val, RS2::Unit src, RS2::Unit dest) {
    if (getFactorToMM(dest)>0.0) {
        return (val*getFactorToMM(src))/getFactorToMM(dest);
    } else {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "RS_Units::convert: invalid factor");
        return val;
    }
}



/**
 * Converts the given vector 'val' from unit 'src' to unit 'dest'.
 */
RS_Vector RS_Units::convert(const RS_Vector& val, RS2::Unit src, RS2::Unit dest) {
    return RS_Vector(convert(val.x, src, dest),
					 convert(val.y, src, dest),
					 convert(val.z, src, dest)
                     );
}



/**
 * Formats the given length in the given format.
 *
 * @param length The length in the current unit of the drawing.
 * @param format Format of the string.
 * @param prec Precision of the value (e.g. 0.001 or 1/128 = 0.0078125)
 & @param showUnit Append unit to the value.
 */
QString RS_Units::formatLinear(double length, RS2::Unit unit,
                                 RS2::LinearFormat format,
                                 int prec, bool showUnit) {
    QString ret;

    // unit appended to value (e.g. 'mm'):
    /*QString unitString = "";
    if (showUnit) {
        unitString = unitToSign(unit);
}*/

    // barbarian display: show as fraction:
    switch (format) {
    case RS2::Scientific:
        ret = formatScientific(length, unit, prec, showUnit);
        break;

    case RS2::Decimal:
        ret = formatDecimal(length, unit, prec, showUnit);
        break;

    case RS2::Engineering:
        ret = formatEngineering(length, unit, prec, showUnit);
        break;

    case RS2::Architectural:
        ret = formatArchitectural(length, unit, prec, showUnit);
        break;

    case RS2::Fractional:
        ret = formatFractional(length, unit, prec, showUnit);
        break;

    case RS2::ArchitecturalMetric:
        ret = formatArchitecturalMetric(length, unit, prec, showUnit);
        break;

    default:
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "RS_Units::formatLinear: Unknown format");
        ret = "";
        break;
    }

    return ret;
}



/**
 * Formats the given length in scientific format (e.g. 2.5E7).
 *
 * @param length The length in the current unit of the drawing.
 * @param prec Precision of the value (e.g. 0.001 or 1/128 = 0.0078125)
 & @param showUnit Append unit to the value.
 */
QString RS_Units::formatScientific(double length, RS2::Unit unit,
                                     int prec, bool showUnit) {

	QString const ret= QString("%1").arg(length,0,'E', prec);
	if(showUnit)
		return ret + unitToSign(unit);
	return ret;
}



/**
 * Formats the given length in decimal (normal) format (e.g. 2.5).
 *
 * @param length The length in the current unit of the drawing.
 * @param prec Precision of the value (e.g. 0.001)
 & @param showUnit Append unit to the value.
 */
QString RS_Units::formatDecimal(double length, RS2::Unit unit,
                                  int prec, bool showUnit) {
	QString const ret=RS_Math::doubleToString(length, prec);

	if(showUnit)
		return ret+unitToSign(unit);
	return ret;
}



/**
 * Formats the given length in engineering format (e.g. 5' 4.5").
 *
 * @param length The length in the current unit of the drawing.
 * @param prec Precision of the value (e.g. 0.001 or 1/128 = 0.0078125)
 & @param showUnit Append unit to the value.
 */
QString RS_Units::formatEngineering(double length, RS2::Unit unit,
                                      int prec, bool /*showUnit*/) {
    // Avoid value cannot be represented by an unsigned integer
    if (!isAbsInRange(length))
        return "<Invalid length>";

    unsigned feet = convert(std::abs(length), unit, RS2::Foot);
    double inches = convert(std::abs(length), unit, RS2::Inch) - feet * 12;

    QString sInches = RS_Math::doubleToString(inches, prec);

    if (sInches=="12") {
        feet++;
        sInches="0";
    }

    QString ret;
    if (feet > 0) {
        ret = QString(R"(%1'-%2")").arg(feet).arg(sInches);
    } else {
        ret = sInches + '"';
    }

    return std::signbit(length) ? "-" + ret : ret;
}



/**
 * Formats the given length in architectural format (e.g. 5' 4 1/2").
 *
 * @param length The length in the current unit of the drawing.
 * @param prec Precision of the value (e.g. 0.001 or 1/128 = 0.0078125)
 & @param showUnit Append unit to the value.
 */
QString RS_Units::formatArchitectural(double length, RS2::Unit unit,
                                        int prec, bool showUnit) {
    // Avoid value cannot be represented by an unsigned integer
    if (!isAbsInRange(length))
        return "<Invalid length>";

    QString negativeSign = std::signbit(length)?"-":"";

    unsigned feet = convert(std::abs(length), unit, RS2::Foot);
    double inches = convert(std::abs(length), unit, RS2::Inch) - feet * 12;

    // potential rounding off
    if (inches >= 12.) {
        feet++;
        inches -= 12.;
    }

    QString sInches = formatFractional(inches, RS2::Inch, prec, showUnit);

    // due to precision
    if (sInches=="12") {
        feet++;
        sInches = "0";
    }

    if (feet != 0) {
        return QString(R"(%1%2'-%3")").arg(negativeSign).arg(feet).arg(sInches);
    } else {
        return QString(R"(%1")").arg(sInches);
    }

}



/**
 * Formats the given length in metric architectural format
 * using DIN 406 (e.g. 1.12⁵).
 *
 * @param length The length in the current unit of the drawing.
 * @param prec Precision of the value (e.g. 0.001)
 & @param showUnit Append unit to the value.
 */
QString RS_Units::formatArchitecturalMetric(double length, RS2::Unit unit,
                                            int prec, bool showUnit) {
    // Avoid value cannot be represented by an unsigned integer
    if (!isAbsInRange(length))
        return "<Invalid length>";

    QString ret = RS_Math::doubleToString(std::abs(length), prec + 1);
    unsigned iLast = QString(ret.right(1)).toUInt();

    // round on 0.005 and use superscript 5
    if ((iLast > 2) && (iLast < 8)) {
        ret = ret.replace(ret.length() - 1, 1, "\u2075");
    } else {
        ret = RS_Math::doubleToString(length, prec);
    }

    // return values < 1.00m in cm (0.42 -> 42)
    const QChar zero = '0';
    if (ret.startsWith(zero)) {
        ret = ret.split(".")[1];
        // eliminate leading zeros (0.07 -> 7)
        if (ret.startsWith(zero)) {
            ret = ret.remove(0, 1);
        }
    }
    if (showUnit) {
        ret = QString("%1 %2").arg(ret).arg(unitToSign(unit));
    }
    return std::signbit(length) ? "-" + ret : ret;
}

/**
 * Formats the given length in fractional (barbarian) format (e.g. 5' 3 1/64").
 *
 * @param length The length in the current unit of the drawing.
 * @param unit Should be inches.
 * @param prec Precision of the value (e.g. 0.001 or 1/128 = 0.0078125)
 & @param showUnit Append unit to the value.
 */
QString RS_Units::formatFractional(double length, RS2::Unit /*unit*/,
                                     int prec, bool /*showUnit*/) {
    // Avoid value cannot be represented by an unsigned integer
    if (!isAbsInRange(length))
        return "<Invalid length>";


    // number of complete inches (num' 7/128")
    unsigned num = (unsigned) std::abs(length);

    unsigned denominator = 2<<prec;
    unsigned nominator = (unsigned) RS_Math::round((length-num)*denominator);

    // fraction rounds up to 1:
    if (nominator==denominator) {
        nominator=0;
        denominator=0;
        ++num;
    }

    // Simplify the fraction
    if (nominator != 0 && denominator != 0) {
		unsigned gcd = RS_Math::findGCD(nominator, denominator);
		if (gcd) {
            nominator = nominator / gcd;
            denominator = denominator / gcd;
        } else {
            RS_DEBUG->print(RS_Debug::D_WARNING,
                                "RS_Units::formatFractional: invalid gcd");
            nominator = 0;
            denominator = 0;
        }
    }

    // sign:
    QString neg = std::signbit(length) ? "-" : "";
    QString ret;
    if (num != 0 && nominator != 0) {
        ret = QString("%1%2 %3/%4").arg(neg).arg(num).arg(nominator).arg(denominator);
    } else if(nominator != 0) {
        ret = QString("%1%2/%3").arg(neg).arg(nominator).arg(denominator);
    } else if(num != 0) {
        ret = QString("%1%2").arg(neg).arg(num);
    } else {
        ret = "0";
    }

    return ret;
}



/**
 * Formats the given angle with the given format.
 *
 * @param angle The angle (always in rad).
 * @param format Format of the string.
 * @param prec Precision of the value (e.g. 0.001 or 1/128 = 0.0078125)
 *
 * @ret String with the formatted angle.
 */
QString RS_Units::formatAngle(double angle, RS2::AngleFormat format,
                                int prec) {

    QString ret;
    double value = std::fmod(angle, 2. * M_PI);

    switch (format) {
    case RS2::Surveyors:
    case RS2::DegreesDecimal:
    case RS2::DegreesMinutesSeconds:
        value = RS_Math::rad2deg(value);
        break;
    case RS2::Gradians:
        value = RS_Math::rad2gra(value);
        break;
    case RS2::Radians:
        break;
    default:
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "RS_Units::formatAngle: Unknown Angle Unit");
        return "";
        break;
    }

    switch (format) {
    case RS2::DegreesDecimal:
    case RS2::Radians:
    case RS2::Gradians:
        ret = RS_Math::doubleToString(value, prec);
        if (format==RS2::DegreesDecimal)
            ret+=QChar(0xB0);
        if (format==RS2::Radians)
            ret+="r";
        if (format==RS2::Gradians)
            ret+="g";
        break;

    case RS2::DegreesMinutesSeconds: {
            unsigned vDegrees, vMinutes;
            double vSeconds;
            QString degrees, minutes, seconds;

            vDegrees = (unsigned)std::floor(value);
            vMinutes = (unsigned)std::floor((value - vDegrees) * 60.0);
            vSeconds = (value - vDegrees - (vMinutes/60.0)) * 3600.0;

            seconds = RS_Math::doubleToString(vSeconds, (prec>1 ? prec-2 : 0));

            if(seconds=="60") {
                seconds="0";
                ++vMinutes;
                if(vMinutes==60) {
                    vMinutes=0;
                    ++vDegrees;
                }
            }

            if (prec==0 && vMinutes>=30.0) {
                vDegrees++;
            } else if (prec==1 && vSeconds>=30.0) {
                vMinutes++;
            }

            degrees.setNum(vDegrees);
            minutes.setNum(vMinutes);

            switch (prec) {
            case 0:
                ret = degrees + QChar(0xB0);
                break;
            case 1:
                ret = degrees + QChar(0xB0) + " " + minutes + "'";
                break;
            default:
                ret = degrees + QChar(0xB0) + " " + minutes + "' "
                      + seconds + "\"";
                break;
            }
        }
        break;
    case RS2::Surveyors: {
        QString prefix,suffix;
        unsigned quadrant;
        quadrant = ((unsigned)std::floor(value)/90);
        switch(quadrant){
            case 0:
                prefix="N";
                suffix="E";
                break;
            case 1:
                prefix="S";
                suffix="E";
                value=180. - value;
                break;
            case 2:
                prefix="S";
                suffix="W";
                value=value - 180.;
                break;
            case 3:
                prefix="N";
                suffix="W";
                value=360. - value;
                break;
            }
            ret = prefix+formatAngle(RS_Math::deg2rad(value),RS2::DegreesMinutesSeconds,prec)+suffix;
            ret.replace(QChar(0xB0),"d");
            ret.replace(" ","");
        }
        break;
    default:
        break;
    }

    return ret;
}

/**
 * Converts the given number from a DXF file into an AngleFormat enum.
 *
 * @param num $DIMAUNIT from DXF (0: decimal deg, 1: deg/min/sec, 2: gradians,
 *                                3: radians, 4: surveyor's units)
 *
 * @ret Matching AngleFormat enum value.
 */
RS2::AngleFormat RS_Units::numberToAngleFormat(int num) {

    RS2::AngleFormat af;

    switch (num) {
    default:
    case 0:
        af = RS2::DegreesDecimal;
        break;
    case 1:
        af = RS2::DegreesMinutesSeconds;
        break;
    case 2:
        af = RS2::Gradians;
        break;
    case 3:
        af = RS2::Radians;
        break;
    case 4:
        af = RS2::Surveyors;
        break;
    }

    return af;
}


/**
 * @return Size of the given paper format.
 */
RS_Vector RS_Units::paperFormatToSize(RS2::PaperFormat p) {

    switch (p) {
    case RS2::Custom:
        return RS_Vector(0.0, 0.0);

    case RS2::A0:
        return RS_Vector(841.0, 1189.0);
    case RS2::A1:
        return RS_Vector(594.0, 841.0);
    case RS2::A2:
        return RS_Vector(420.0, 594.0);
    case RS2::A3:
        return RS_Vector(297.0, 420.0);
    case RS2::A4:
        return RS_Vector(210.0, 297.0);

    /* Removed ISO "B" and "C" series, C5E, Comm10E, DLE, (envelope sizes) */

    case RS2::Letter:  /* 8.5 x 11.0 in.  Sizes shown are used for 'hard' conversion to metric */
        return RS_Vector(215.9, 279.4);
    case RS2::Legal:  /* 8.5 x 14.0 in */
        return RS_Vector(215.9, 355.6);
    case RS2::Tabloid:  /* 11.0 x 17.0 */
        return RS_Vector(279.4, 431.8);

    //case RS2::Ansi_A:  /* 8.5 x 11.0 in */
    //    return RS_Vector(215.9, 279.4);
    //case RS2::Ansi_B:  /* 11.0 x 17.0 in */
    //    return RS_Vector(279.4, 431.8);
    case RS2::Ansi_C:  /* 17.0 x 22.0 in */
        return RS_Vector(431.8, 558.8);
    case RS2::Ansi_D:  /* 22.0 x 34.0 in */
        return RS_Vector(558.8, 863.6);
    case RS2::Ansi_E:  /* 34.0 x 44.0 in */
        return RS_Vector(863.6, 1117.6);

    case RS2::Arch_A:  /* 9.0 x 12.0 in */
        return RS_Vector(228.6, 304.8);
    case RS2::Arch_B:  /* 12.0 x 18.0 in */
        return RS_Vector(304.8, 457.2);
    case RS2::Arch_C:  /* 18.0 x 24.0 in */
        return RS_Vector(457.2, 609.6);
    case RS2::Arch_D:  /* 24.0 x 36.0 in */
        return RS_Vector(609.6, 914.4);
    case RS2::Arch_E:  /* 36.0 x 48.0 in */
        return RS_Vector(914.4, 1219.2);

    default:
        break;
    }

    return RS_Vector (false);
}



/**
 * Gets the paper format which matches the given size. If no
 * format matches, RS2::Custom is returned.
 */
RS2::PaperFormat RS_Units::paperSizeToFormat(const RS_Vector& s) {
    RS_Vector ts1;
    RS_Vector ts2;

    for (RS2::PaperFormat i = RS2::FirstPaperFormat; RS2::NPageFormat > i; i = static_cast<RS2::PaperFormat>(i + 1)) {
        ts1 = RS_Units::paperFormatToSize(i);
        ts2 = RS_Vector(ts1.y, ts1.x);

        if (ts1.distanceTo(s) < 1.0e-4 || ts2.distanceTo(s) < 1.0e-4) {
            return i;
        }
    }

    return RS2::Custom;
}



/**
 * Converts a paper format to a string (e.g. for a combobox).
 */
QString RS_Units::paperFormatToString(RS2::PaperFormat p) {

    switch (p) {
    case RS2::Custom: return QObject::tr( "Custom", "Paper format");

    case RS2::A0: return QObject::tr( "A0", "Paper format");
    case RS2::A1: return QObject::tr( "A1", "Paper format");
    case RS2::A2: return QObject::tr( "A2", "Paper format");
    case RS2::A3: return QObject::tr( "A3", "Paper format");
    case RS2::A4: return QObject::tr( "A4", "Paper format");

    /* Removed ISO "B" and "C" series, C5E, Comm10E, DLE, (envelope sizes) */

    case RS2::Letter: return QObject::tr( "Letter / ANSI A", "Paper format");
    case RS2::Legal:  return QObject::tr( "Legal",  "Paper format");
    case RS2::Tabloid: return QObject::tr( "Tabloid / ANSI B", "Paper format");

    //case RS2::Ansi_A: return QObject::tr( "Letter / ANSI A", "Paper format");
    //case RS2::Ansi_B: return QObject::tr( "Tabloid / ANSI B", "Paper format");
    case RS2::Ansi_C: return QObject::tr( "ANSI C", "Paper format");
    case RS2::Ansi_D: return QObject::tr( "ANSI D", "Paper format");
    case RS2::Ansi_E: return QObject::tr( "ANSI E", "Paper format");

    case RS2::Arch_A: return QObject::tr( "Arch A", "Paper format");
    case RS2::Arch_B: return QObject::tr( "Arch B", "Paper format");
    case RS2::Arch_C: return QObject::tr( "Arch C", "Paper format");
    case RS2::Arch_D: return QObject::tr( "Arch D", "Paper format");
    case RS2::Arch_E: return QObject::tr( "Arch E", "Paper format");

    default:
        break;
    }

    return QStringLiteral("");
}



/**
 * Converts a string to a paper format.
 */
RS2::PaperFormat RS_Units::stringToPaperFormat(const QString& p) {
    QString ls {p.toLower()};

    // use toLower() on localized paper format strings, don't trust that translators keep lower case
    if (ls == QStringLiteral("custom") || ls == QObject::tr("custom", "Paper format").toLower())
        return RS2::Custom;

    if (ls == QStringLiteral("a0") || ls == QObject::tr("a0", "Paper format").toLower())
        return RS2::A0;
    if (ls == QStringLiteral("a1") || ls == QObject::tr("a1", "Paper format").toLower())
        return RS2::A1;
    if (ls == QStringLiteral("a2") || ls == QObject::tr("a2", "Paper format").toLower())
        return RS2::A2;
    if (ls == QStringLiteral("a3") || ls == QObject::tr("a3", "Paper format").toLower())
        return RS2::A3;
    if (ls == QStringLiteral("a4") || ls == QObject::tr("a4", "Paper format").toLower())
        return RS2::A4;

    /* Removed ISO "B" and "C" series, C5E, Comm10E, DLE, (envelope sizes) */

    if (ls == QStringLiteral("letter") || ls == QObject::tr("letter", "Paper format").toLower())
        return RS2::Letter;
    if (ls == QStringLiteral("legal")  || ls == QObject::tr("legal",  "Paper format").toLower())
        return RS2::Legal;
    if (ls == QStringLiteral("tabloid") || ls == QObject::tr("tabloid", "Paper format").toLower())
        return RS2::Tabloid;

    //if (ls == QStringLiteral("ansi a") || ls == QObject::tr("ansi a", "Paper format").toLower())
    //    return RS2::Ansi_A;
    //if (ls == QStringLiteral("ansi b") || ls == QObject::tr("ansi b", "Paper format").toLower())
    //    return RS2::Ansi_B;
    if (ls == QStringLiteral("ansi c") || ls == QObject::tr("ansi c", "Paper format").toLower())
        return RS2::Ansi_C;
    if (ls == QStringLiteral("ansi d") || ls == QObject::tr("ansi d", "Paper format").toLower())
        return RS2::Ansi_D;
    if (ls == QStringLiteral("ansi e") || ls == QObject::tr("ansi e", "Paper format").toLower())
        return RS2::Ansi_E;

    if (ls == QStringLiteral("arch a") || ls == QObject::tr("arch a", "Paper format").toLower())
        return RS2::Arch_A;
    if (ls == QStringLiteral("arch b") || ls == QObject::tr("arch b", "Paper format").toLower())
        return RS2::Arch_B;
    if (ls == QStringLiteral("arch c") || ls == QObject::tr("arch c", "Paper format").toLower())
        return RS2::Arch_C;
    if (ls == QStringLiteral("arch d") || ls == QObject::tr("arch d", "Paper format").toLower())
        return RS2::Arch_D;
    if (ls == QStringLiteral("arch e") || ls == QObject::tr("arch e", "Paper format").toLower())
        return RS2::Arch_E;

    return RS2::Custom;
}

/**
  * Calculates a scaling factor from given dpi and units.
  */
double RS_Units::dpiToScale(double dpi, RS2::Unit unit) {
    double scale = RS_Units::convert(1.0, RS2::Inch, unit) / dpi;
    return scale;
}

/**
  * Calculates a dpi value from given scaling factor and units.
  */
double RS_Units::scaleToDpi(double scale, RS2::Unit unit) {
    double dpi = RS_Units::convert(1.0, RS2::Inch, unit) / scale;
    return dpi;
}

/**
 * Performs some testing for the math class.
 */
void RS_Units::test() {
    QString s;
    double v;

    /*
       std::cout << "RS_Units::test: formatLinear (decimal):\n";
       v = 0.1;
       s = RS_Units::formatLinear(v, RS2::Millimeter, RS2::Decimal,
                                 3, false);
       std::cout << "s: " << s << "\n";
       assert(s=="0.1");
       v = 0.01;
       s = RS_Units::formatLinear(v, RS2::Millimeter, RS2::Decimal,
                                 3, false);
       std::cout << "s: " << s << "\n";
       assert(s=="0.01");
       v = 0.001;
       s = RS_Units::formatLinear(v, RS2::Millimeter, RS2::Decimal,
                                 3, false);
       std::cout << "s: " << s << "\n";
       assert(s=="0.001");
       v = 0.009;
       s = RS_Units::formatLinear(v, RS2::Millimeter, RS2::Decimal,
                                 2, false);
       std::cout << "s: " << s << "\n";
       assert(s=="0.01");
       v = 0.005;
       s = RS_Units::formatLinear(v, RS2::Millimeter, RS2::Decimal,
                                 2, false);
       std::cout << "s: " << s << "\n";
       assert(s=="0.01");
       v = 0.0049999;
       s = RS_Units::formatLinear(v, RS2::Millimeter, RS2::Decimal,
                                 2, false);
       std::cout << "s: " << s << "\n";
       assert(s=="0");

       v = 0.1;
       s = RS_Units::formatLinear(v, RS2::Millimeter, RS2::Decimal,
                                 4, true);
       std::cout << "s: " << s << "\n";
       assert(s=="0.1mm");


       std::cout << "RS_Units::test: formatLinear (fractional):\n";
       v = 1.2;
       s = RS_Units::formatLinear(v, RS2::Inch, RS2::Fractional,
                                 6, false);
       std::cout << "s: " << s << "\n";
       assert(s=="1 13/64");

       v = 1.2;
       s = RS_Units::formatLinear(v, RS2::Inch, RS2::Fractional,
                                 8, false);
       std::cout << "s: " << s << "\n";
       assert(s=="1 51/256");

       v = 0.2;
       s = RS_Units::formatLinear(v, RS2::Inch, RS2::Fractional,
                                 8, false);
       std::cout << "s: " << s << "\n";
       assert(s=="51/256");

       v = 4.5;
       s = RS_Units::formatLinear(v, RS2::Inch, RS2::Fractional,
                                 6, true);
       std::cout << "s: " << s << "\n";
       assert(s=="4 1/2");

       v = 0.001;
       s = RS_Units::formatLinear(v, RS2::Inch, RS2::Fractional,
                                 0, false);
       std::cout << "s: " << s << "\n";
       assert(s=="0");

       v = 0.01;
       s = RS_Units::formatLinear(v, RS2::Inch, RS2::Fractional,
                                 8, false);
       std::cout << "s: " << s << "\n";
       assert(s=="3/256");

       v = 0.0078125;
       s = RS_Units::formatLinear(v, RS2::Inch, RS2::Fractional,
                                 8, false);
       std::cout << "s: " << s << "\n";
       assert(s=="1/128");

       v = 0.001;
       s = RS_Units::formatLinear(v, RS2::Inch, RS2::Fractional,
                                 8, false);
       std::cout << "s: " << s << "\n";
       assert(s=="0");

       v = 9.9999;
       s = RS_Units::formatLinear(v, RS2::Inch, RS2::Fractional,
                                 6, false);
       std::cout << "s: " << s << "\n";
       assert(s=="10");
    */

    for (v=11.9999; v<12.0001; v+=0.0000001) {
        for (int prec=0; prec<=6; ++prec) {
            s = RS_Units::formatLinear(v, RS2::Inch, RS2::Architectural,
                                       prec, true);
                        // RVT_PORT changed  << s to s.ascii()
            std::cout << "prec: " << prec << " v: " << v << " s: " << s.toLatin1().data() << "\n";
        }
    }

    /*for (v=0.0; v<10.0; v+=0.001) {
        s = RS_Units::formatLinear(v, RS2::Foot, RS2::Fractional,
                                  1.0/128.0, true);
        std::cout << "v: " << v << " s: " << s << "\n";
}*/

    /*
    std::cout << "RS_Units::test: formatLinear (scientific):\n";
    v = 0.001;
    s = RS_Units::formatLinear(v, RS2::Millimeter, RS2::Scientific,
                              0.0001, false);
    std::cout << "s: " << s << "\n";
    assert(s=="1.0e-3");
    */


    /*
       std::cout << "RS_Units::test: formatAngle (deg / decimal):\n";
       v = 0.0261799;
       s = RS_Units::formatAngle(v, RS2::DegreesDecimal, 2);
       std::cout << "s: " << s << "\n";
       assert(s=="1.5∞");

       v = 0;
       s = RS_Units::formatAngle(v, RS2::DegreesDecimal, 2);
       std::cout << "s: " << s << "\n";
       assert(s=="0∞");

       v = 1.5707963;
       s = RS_Units::formatAngle(v, RS2::DegreesDecimal, 2);
       std::cout << "s: " << s << "\n";
       assert(s=="90∞");

       std::cout << "RS_Units::test: formatAngle (deg / d/m/s):\n";

       v = 0.0260926;
       s = RS_Units::formatAngle(v, RS2::DegreesMinutesSeconds, 1);
       std::cout << "s: " << s << "\n";
       assert(s=="1∞ 29' 42\"");

       v = 0.0261799;
       s = RS_Units::formatAngle(v, RS2::DegreesMinutesSeconds, 1);
       std::cout << "s: " << s << "\n";
       assert(s=="1∞ 30' 0\"");
    */
}
