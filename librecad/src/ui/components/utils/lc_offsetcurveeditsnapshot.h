/* Copyright (C) 2026 LibreCAD.org. SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef LC_OFFSETCURVEEDITSNAPSHOT_H
#define LC_OFFSETCURVEEDITSNAPSHOT_H

#include <cstddef>
#include <memory>

class LC_OffsetCurve;

/** Bounded source-value baseline, shared by native sheet and dialog edits.
 * Capture before cloning. Callers must prove live membership/owner/context
 * before capture or matches; this value object does not own the source entity.
 */
class LC_OffsetCurveEditSnapshot final {
public:
    static constexpr std::size_t MaximumSnapshotItems = 4096;
    static constexpr std::size_t MaximumSnapshotBytes = 1024*1024;
    static std::unique_ptr<LC_OffsetCurveEditSnapshot> capture(const LC_OffsetCurve& curve);
    ~LC_OffsetCurveEditSnapshot();
    bool matches(const LC_OffsetCurve& curve) const;
    /** No virtual display reads. Use after every callbackful batch read and
     * renewed live membership checks, before handing off prepared replacements.
     * Bounded value copies may still throw; this does not own the source.
     */
    bool matchesStoredState(const LC_OffsetCurve& curve) const;
    LC_OffsetCurveEditSnapshot(const LC_OffsetCurveEditSnapshot&) = delete;
    LC_OffsetCurveEditSnapshot& operator=(const LC_OffsetCurveEditSnapshot&) = delete;
private:
    struct Data;
    explicit LC_OffsetCurveEditSnapshot(std::unique_ptr<Data> data) noexcept;
    std::unique_ptr<Data> m_data;
};
#endif
