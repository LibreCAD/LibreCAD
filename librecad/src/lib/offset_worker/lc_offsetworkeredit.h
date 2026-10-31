/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef LC_OFFSETWORKEREDIT_H
#define LC_OFFSETWORKEREDIT_H
#include <cstdint>
#include "lc_offsetworkerrequest.h"

/** Preparation for a caller-owned private clone, never live document publication.
 * Cheap edits use the existing proof. A Generate description runs no producer/C
 * stage and conveys no native adoption authority.
 */
class LC_OffsetWorkerEdit final {
public:
    struct Preparation {
        Preparation(Preparation&&) noexcept = default;
        Preparation& operator=(Preparation&&) noexcept = default;
        const LC_OffsetCurveEditResult& result() const noexcept { return m_result; }
        const std::optional<LC_OffsetWorkerRequest::Generate>& generate() const noexcept { return m_generate; }
        const std::optional<LC_OffsetWorkerRequest::CheckThrough>& membership() const noexcept { return m_membership; }
    private:
        friend class LC_OffsetWorkerEdit;
        Preparation() = default;
        LC_OffsetCurveEditResult m_result;
        std::optional<LC_OffsetWorkerRequest::Generate> m_generate;
        std::optional<LC_OffsetWorkerRequest::CheckThrough> m_membership;
        std::unique_ptr<LC_OffsetCurve> m_local;
        const LC_OffsetCurve* m_origin = nullptr;
        unsigned long long m_id = 0;
        std::uint64_t m_stamp = 0;
        LC_EllipseOffsetDefinition m_original;
    };
    static Preparation prepare(const LC_OffsetCurve& clone, const LC_EllipseOffsetDefinition&,
        const LC_OffsetRunLimits&);
    static Preparation prepareThrough(const LC_OffsetCurve& clone, const RS_Vector&, const LC_OffsetRunLimits&);
    static LC_OffsetCurveEditResult applyLocal(LC_OffsetCurve& clone, Preparation&&);
};
#endif
