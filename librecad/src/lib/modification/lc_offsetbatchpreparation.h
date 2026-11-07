/* Copyright (C) 2026 LibreCAD.org. SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef LC_OFFSETBATCHPREPARATION_H
#define LC_OFFSETBATCHPREPARATION_H

#include "rs_modification.h"
namespace LC_OffsetWorkerRequest { struct Token; }

/** Ordered numeric staging, never document/publication authority. The caller
 * owns live source/context seals across every asynchronous turn. Ordinary work
 * runs once; eligible ellipse/native work waits for a joined helper completion.
 */
class LC_OffsetBatchPreparation final {
public:
    static constexpr qsizetype maximumSources = 4096;
    LC_OffsetBatchPreparation(const RS_OffsetData&, const QList<RS_Entity*>&,
        bool preview, const LC_OffsetBatchLimits&);
    ~LC_OffsetBatchPreparation();
    bool advance(LC_DocumentModificationBatch&, LC_OffsetPreviewCache* = nullptr);
    const RS_Entity* pendingSource() const noexcept;
    std::unique_ptr<LC_OffsetWorkerRequest::Request> request(const LC_OffsetWorkerRequest::Token&,
        LC_OffsetPreviewCache* = nullptr);
    bool accept(const LC_OffsetWorkerCompletion&);
    const LC_OffsetBatchOutcome& outcome() const noexcept;
    /** Preview ownership transfer, in final dispatch order. Allocation precedes
     * every move; a refused/throwing transfer retains the complete owned batch.
     */
    std::vector<std::unique_ptr<RS_Entity>> take();
    /** After allocation-free publication, relinquish only the final added set. */
    void release() noexcept;
private:
    friend class RS_Modification;
    struct Data;
    std::unique_ptr<Data> m_data;
};

#endif
