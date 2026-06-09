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

#include "lc_propertyprovider_utils.h"

#include "lc_propertiesprovider_dim_base.h"
#include "rs.h"

LC_EnumDescriptor* LC_PropertyProviderUtils::getLinearUnitsEnumDescriptor(RS2::LinearFormat format) {
     static LC_EnumDescriptor decimalPlacesScientific = {
        "decimalPlacesScientific",
        {
            {1, "1E+01"},
            {2, "1E-1"},
            {3, "1E-2"},
            {4, "1E-3"},
            {5, "1E-4"},
            {6, "1E-5"},
            {7, "1E-6"},
            {8, "1E-7"},
            {9, "1E-8"}
        }
    };

    static LC_EnumDescriptor decimalPlacesDecimal = {
        "decimalPlacesDecimal",
        {
            {1, "1.00"},
            {2, "0.1"},
            {3, "0.01"},
            {4, "0.001"},
            {5, "0.0001"},
            {6, "0.00001"},
            {7, "0.000001"},
            {8, "0.0000001"},
            {9, "0.00000001"}
        }
    };

    static LC_EnumDescriptor decimalPlacesArchitectural = {
        "decimalPlacesArchitectural",
        {
            {1, "0'-0\""},
            {2, "0'-0 1/2\""},
            {3, "0'-0 1/4\""},
            {4, "0'-0 1/8\""},
            {5, "0'-0 1/16\""},
            {6, "0'-0 1/32\""},
            {7, "0'-0 1/64\""},
            {8, "0'-0 1/128\""},
            {9, "0'-0 1/256\""}
        }
    };

    static LC_EnumDescriptor decimalPlacesEngineering = {
        "decimalPlacesEngineering",
        {
            {1, "0'-0\""},
            {2, "0'-0.0\""},
            {3, "0'-0.00\""},
            {4, "0'-0.000\""},
            {5, "0'-0.0000\""},
            {6, "0'-0.00000\""},
            {7, "0'-0.000000\""},
            {8, "0'-0.0000000\""},
            {9, "0'-0.00000000\""}
        }
    };

    static LC_EnumDescriptor decimalPlacesFractional = {
        "decimalPlacesFractional",
        {
            {1, "1"},
            {2, "0 1/2"},
            {3, "0 1/4"},
            {4, "0 1/8"},
            {5, "0 1/16"},
            {6, "0 1/32"},
            {7, "0 1/64"},
            {8, "0 1/128"},
            {9, "0 1/256"}
        }
    };

    switch (format) {
        case RS2::Scientific:
            return &decimalPlacesScientific;
        case RS2::ArchitecturalMetric:
        case RS2::Decimal:
            return &decimalPlacesDecimal;
        case RS2::Engineering:
            return &decimalPlacesEngineering;
        case RS2::Fractional:
            return &decimalPlacesFractional;
        case RS2::Architectural:
            return &decimalPlacesArchitectural;
        default:
            return &decimalPlacesDecimal;
    }

}

LC_EnumDescriptor* LC_PropertyProviderUtils::getAngleUnitsEnumDescriptor(RS2::AngleFormat format) {
     static LC_EnumDescriptor decimalPlacesDecimal = {
        "decimalPlacesDecimal",
        {
            {1, "1.00"},
            {2, "0.10"},
            {3, "0.01"},
            {4, "0.001"},
            {5, "0.0001"},
            {6, "0.00001"},
            {7, "0.000001"},
            {8, "0.0000001"},
            {9, "0.00000001"}
        }
    };

    static LC_EnumDescriptor decimalPlacesDegMinSec = {
        "decimalPlacesDegMinSec",
        {
            {1, QString("0%1").arg(QChar(0xB0))},
            {2, QString("0%100'").arg(QChar(0xB0))},
            {3, QString("0%100'00\"").arg(QChar(0xB0))},
            {4, QString("0%100'00.0\"").arg(QChar(0xB0))},
            {5, QString("0%100'00.00\"").arg(QChar(0xB0))},
            {6, QString("0%100'00.000\"").arg(QChar(0xB0))},
            {7, QString("0%100'00.0000\"").arg(QChar(0xB0))},
            {8, QString("0%100'00.00000\"").arg(QChar(0xB0))},
            {9, QString("0%100'00.000000\"").arg(QChar(0xB0))}
        }
    };

    static LC_EnumDescriptor decimalPlacesGradians = {
        "decimalPlacesGradians",
        {
            {1, "1g"},
            {2, "0.1g"},
            {3, "0.01g"},
            {4, "0.001g"},
            {5, "0.0001g"},
            {6, "0.00001g"},
            {7, "0.000001g"},
            {8, "0.0000001g"},
            {9, "0.00000001g"}
        }
    };

    static LC_EnumDescriptor decimalPlacesRadians = {
        "decimalPlacesRadians",
        {
            {1, "1r"},
            {2, "0.1r"},
            {3, "0.01r"},
            {4, "0.001r"},
            {5, "0.0001r"},
            {6, "0.00001r"},
            {7, "0.000001r"},
            {8, "0.0000001r"},
            {9, "0.00000001r"}
        }
    };

    static LC_EnumDescriptor decimalPlacesSurveyors = {
        "decimalPlacesSurveyors",
        {
            {1, "N 1d E"},
            {2, "N 0d01' E"},
            {3, "N 0d00'01\" E"},
            {4, "N 0d00'00.1\" E"},
            {5, "N 0d00'00.01\" E"},
            {6, "N 0d00'00.001\" E"},
            {7, "N 0d00'00.0001\" E"},
            {8, "N 0d00'00.00001\" E"},
            {9, "N 0d00'00.000001\" E"}
        }
    };

    switch (format) {
        case RS2::DegreesDecimal:
            return &decimalPlacesDecimal;
        case RS2::DegreesMinutesSeconds:
            return &decimalPlacesDegMinSec;
        case RS2::Gradians:
            return &decimalPlacesGradians;
        case RS2::Radians:
            return &decimalPlacesRadians;
        case RS2::Surveyors:
            return &decimalPlacesSurveyors;
        default:
            return &decimalPlacesDecimal;
    }
}

LC_EnumDescriptor* LC_PropertyProviderUtils::getAngleUnitFormatEnumDescriptor() {
    static LC_EnumDescriptor angleFormatDescriptor = {
        "decimalLinearUnits",
        {
                {RS2::AngleFormat::DegreesDecimal, LC_PropertiesProviderDimBase::tr("Decimal Degrees")},
                {RS2::AngleFormat::DegreesMinutesSeconds, LC_PropertiesProviderDimBase::tr("Deg/min/sec")},
                {RS2::AngleFormat::Gradians, LC_PropertiesProviderDimBase::tr("Gradians")},
                {RS2::AngleFormat::Radians, LC_PropertiesProviderDimBase::tr("Radians")},
                {RS2::AngleFormat::Surveyors, LC_PropertiesProviderDimBase::tr("Surveyor's units")}
        }
    };
    return &angleFormatDescriptor;
}

LC_EnumDescriptor* LC_PropertyProviderUtils::getLinearUnitFormatEnumDescriptor() {
    static LC_EnumDescriptor decimalLinearUnitsDescriptor = {
        "decimalLinearUnits",
        {
                {RS2::LinearFormat::Scientific, LC_PropertiesProviderDimBase::tr("Scientific")},
                {RS2::LinearFormat::Decimal, LC_PropertiesProviderDimBase::tr("Decimal")},
                {RS2::LinearFormat::Engineering, LC_PropertiesProviderDimBase::tr("Engineering")},
                {RS2::LinearFormat::Architectural, LC_PropertiesProviderDimBase::tr("Architectural")},
                {RS2::LinearFormat::Fractional, LC_PropertiesProviderDimBase::tr("Fractional")},
                {RS2::LinearFormat::ArchitecturalMetric, LC_PropertiesProviderDimBase::tr("Architectural (metric)")}
        }
    };
    return &decimalLinearUnitsDescriptor;
}
