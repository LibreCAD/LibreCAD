/* Copyright (C) 2026 LibreCAD.org. SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef LC_OFFSETCURVEPUBLICATION_H
#define LC_OFFSETCURVEPUBLICATION_H

#include <QList>
#include <memory>

class RS_Document;
class RS_Entity;
class LC_GraphicViewport;

/** Offset creation/native replacement. Preparation owns no input entities and changes no
 * document state. The caller retains its final source/context seals and releases
 * replacement owners immediately after commit(), before notifying observers.
 */
class LC_OffsetCurvePublication final {
public:
    static std::unique_ptr<LC_OffsetCurvePublication> prepare(RS_Document& document,
        LC_GraphicViewport& viewport, const QList<RS_Entity*>& removed,
        const QList<RS_Entity*>& added, const QList<RS_Entity*>& selected);
    /** Offset creation only. sourceRoots are live top-level document members
     * containing the sources to deselect (including selected polyline children).
     * Target attributes are already applied to privately owned added entities.
     */
    static std::unique_ptr<LC_OffsetCurvePublication> prepareCreation(RS_Document&,
        LC_GraphicViewport&, const QList<RS_Entity*>& removed, const QList<RS_Entity*>& added,
        const QList<RS_Entity*>& selected, const QList<RS_Entity*>& deselected,
        const QList<RS_Entity*>& sourceRoots);
    ~LC_OffsetCurvePublication();
    bool commit() noexcept;
    void notify() noexcept;
private:
    struct Data;
    static std::unique_ptr<LC_OffsetCurvePublication> prepareImpl(RS_Document&, LC_GraphicViewport&,
        const QList<RS_Entity*>&, const QList<RS_Entity*>&, const QList<RS_Entity*>&,
        const QList<RS_Entity*>&, const QList<RS_Entity*>&, bool);
    explicit LC_OffsetCurvePublication(std::unique_ptr<Data> data);
    std::unique_ptr<Data> m_data;
};
#endif
