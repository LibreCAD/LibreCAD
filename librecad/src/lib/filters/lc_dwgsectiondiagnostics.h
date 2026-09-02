/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2026 LibreCAD.org
 * Copyright (C) 2026 Dongxu Li (github.com/dxli)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 * ********************************************************************************
 */

#ifndef LC_DWGSECTIONDIAGNOSTICS_H
#define LC_DWGSECTIONDIAGNOSTICS_H

#include <algorithm>
#include <cstddef>
#include <cstdint>

#include "lc_dwgadvancedmetadata.h"

struct LC_DwgSectionReferenceCounts {
    size_t unresolvedSectionSettings = 0;
    size_t unresolvedManagerSections = 0;
    size_t unresolvedSettingsSources = 0;

    size_t totalUnresolvedReferences() const {
        return unresolvedSectionSettings + unresolvedManagerSections
               + unresolvedSettingsSources;
    }
};

inline LC_DwgSectionReferenceCounts lcDwgSectionReferenceCounts(
    const LC_DwgAdvancedMetadata& metadata) {
    LC_DwgSectionReferenceCounts counts;
    const auto hasSectionObject = [&metadata](std::uint32_t handle) {
        return std::any_of(
            metadata.sectionObjects().cbegin(), metadata.sectionObjects().cend(),
            [handle](const LC_DwgAdvancedMetadata::SectionObjectRecord& record) {
                return record.handle == handle;
            });
    };
    const auto hasSectionSettings = [&metadata](std::uint32_t handle) {
        return std::any_of(
            metadata.sections().cbegin(), metadata.sections().cend(),
            [handle](const LC_DwgAdvancedMetadata::SectionRecord& record) {
                return record.kind == DRW_Section::Settings
                    && record.handle == handle;
            });
    };

    for (const auto& record : metadata.sectionObjects()) {
        if (record.sectionSettingsHandle != 0
            && !hasSectionSettings(record.sectionSettingsHandle))
            ++counts.unresolvedSectionSettings;
    }
    for (const auto& record : metadata.sections()) {
        if (record.kind == DRW_Section::Manager) {
            for (std::uint32_t handle : record.sectionHandles) {
                if (handle != 0 && !hasSectionObject(handle))
                    ++counts.unresolvedManagerSections;
            }
            continue;
        }

        if (record.types.empty()) {
            if (record.sourceHandle != 0 && !hasSectionObject(record.sourceHandle))
                ++counts.unresolvedSettingsSources;
            continue;
        }
        for (const auto& type : record.types) {
            for (std::uint32_t handle : type.m_sourceHandles) {
                if (handle != 0 && !hasSectionObject(handle))
                    ++counts.unresolvedSettingsSources;
            }
        }
    }
    return counts;
}

#endif
