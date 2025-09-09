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

#ifndef LC_TOLERANCEUTILS_H
#define LC_TOLERANCEUTILS_H
#include <QString>

namespace LC_ToleranceUtils
{
    struct DatumInfo {
        QString value;
        QString code;
    };

    struct FeatureControlFrameInfo {
        bool composite;
        QString char_1;
        DatumInfo datumOne_1;
        DatumInfo datumTwo_1;
        DatumInfo datumThree_1;
        QString char_2;
        DatumInfo datumOne_2;
        DatumInfo datumTwo_2;
        DatumInfo datumThree_2;
    };

   FeatureControlFrameInfo stringToControlInfo();

       enum BoundaryModifierMode {
        NONE,
        DIAMETER,
        SF_DIAMETER,
        BOTH
    };

    enum Characteristic {
        STRAIGHTNESS,
        FLATNESS,
        CIRCULARITY,
        CYLINDRICITY,
        PROFILE_OF_A_LINE,
        PROFILE_OF_A_SURFACE,
        PERPENDICULARITY,
        PARALLLELISM,
        ANGULARTIRY,
        POSITION,
        CONCENTRICITY,
        SYMMETRRY,
        CIRCULAR_RUNOUT,
        TOTAL_RUNOUT
    };

    struct CharacteristicDefinition {
        CharacteristicDefinition(Characteristic t, const QString& icon,
                                 int minDatum,
                                 int maxDatum,
                                 bool tolModifiableToMaterial,
                                 bool materialModifiersForDatumRefs,
                                 BoundaryModifierMode boundaryModifier)
            : type{t},
              icon{icon},
              minDatumAllowed{minDatum},
              maxDatumAllowed{maxDatum},
              toleranceModifiersAllowed{tolModifiableToMaterial},
              materialModifiersAllowedForDatumRefs{materialModifiersForDatumRefs},
              boundaryModifierMode{boundaryModifier} {
        }

        Characteristic type;
        QString icon;
        int minDatumAllowed;
        int maxDatumAllowed;
        bool toleranceModifiersAllowed = false;
        bool materialModifiersAllowedForDatumRefs = false;
        BoundaryModifierMode boundaryModifierMode = NONE;
    };

    std::vector<CharacteristicDefinition> g_characteristics = {
        {POSITION, ":/gdt/chars/position.lci", 1, 3, true, true, BoundaryModifierMode::BOTH},
        {CONCENTRICITY, ":/gdt/chars/concentricity.lci", 1, 3, false, false, BoundaryModifierMode::BOTH},
        {SYMMETRRY, ":/gdt/chars/symmetry.lci", 1, 3, false, false, BoundaryModifierMode::NONE},
        {PARALLLELISM, ":/gdt/chars/parallelism.lci", 1, 3, true, true, BoundaryModifierMode::DIAMETER},
        {PERPENDICULARITY, ":/gdt/chars/perpendicularity.lci", 1, 3, true, true, BoundaryModifierMode::DIAMETER},
        {ANGULARTIRY, ":/gdt/chars/angularity.lci", 1, 3, true, true, BoundaryModifierMode::DIAMETER},
        {CYLINDRICITY, ":/gdt/chars/cylindricity.lci", 0, 0, false, false, BoundaryModifierMode::NONE},
        {FLATNESS, ":/gdt/chars/flatness.lci", 0, 0, true, false, BoundaryModifierMode::NONE},
        {CIRCULARITY, ":/gdt/chars/circularity.lci", 0, 0, false, false, BoundaryModifierMode::NONE},
        {STRAIGHTNESS, ":/gdt/chars/straightness.lci", 0, 1, true, false, BoundaryModifierMode::DIAMETER},
        {PROFILE_OF_A_SURFACE, ":/gdt/chars/profileofasurface.lci", 0, 3, false, true, BoundaryModifierMode::NONE},
        {PROFILE_OF_A_LINE, ":/gdt/chars/profileofaline.lci", 0, 3, false, true, BoundaryModifierMode::NONE},
        {CIRCULAR_RUNOUT, ":/gdt/chars/circular_runout.lci", 1, 2, false, false, BoundaryModifierMode::NONE},
        {TOTAL_RUNOUT, ":/gdt/chars/totalrunout.lci", 1, 2, false, false, BoundaryModifierMode::NONE}
    };

    bool LC_ToleranceUtils::isAllowedDatumName(const QString& datumName);

};

#endif // LC_TOLERANCEUTILS_H
