/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef LC_OFFSETWORKERKERNEL_H
#define LC_OFFSETWORKERKERNEL_H

#include "lc_offsetworkerresult.h"

/** Linked only into the headless helper and its tests, never an application
 * raw-cache admission API. The helper owns all temporary entities and proofs.
 */
class LC_OffsetWorkerKernel {
public:
    struct Trace {
        bool nativePrepareEntered = false;
        bool checkEntered = false;
        bool throughEntered = false;
        bool trimEntered = false;
        bool computationCompleted = false;
        LC_EllipseOffsetGeometry::CheckWork checkWork{}, throughWork{};
        LC_OffsetCacheTrim::Work trimWork{};
    };
    /** A fatal reply carries no geometry. Trace retains any observed prefix;
     * entry flags record observed entry, not proof that other stages never ran.
     * missing work is unknown, not zero, and cannot refund a reservation.
     */
    static LC_OffsetWorkerResult::Reply run(const LC_OffsetWorkerRequest::Request&, Trace* = nullptr) noexcept;
private:
    static LC_OffsetWorkerResult::Produced produce(const LC_OffsetWorkerRequest::Produce&, Trace&);
    static LC_OffsetWorkerResult::Native generate(const LC_OffsetWorkerRequest::Generate&, Trace&);
    static LC_OffsetWorkerResult::Native validate(const LC_OffsetWorkerRequest::ValidateCache&, Trace&);
    static LC_OffsetWorkerResult::Native checkThrough(const LC_OffsetWorkerRequest::CheckThrough&, Trace&);
};
#endif
