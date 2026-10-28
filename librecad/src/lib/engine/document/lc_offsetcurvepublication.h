/* Copyright (C) 2026 LibreCAD.org. SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef LC_OFFSETCURVEPUBLICATION_H
#define LC_OFFSETCURVEPUBLICATION_H

#include <QList>
#include <memory>

class RS_Document;
class RS_Entity;
class LC_GraphicViewport;

/** Native replacement only. Preparation owns no input entities and changes no
 * document state. The caller retains its final source/context seals and releases
 * replacement owners immediately after commit(), before notifying observers.
 */
class LC_OffsetCurvePublication final {
public:
    static std::unique_ptr<LC_OffsetCurvePublication> prepare(RS_Document& document,
        LC_GraphicViewport& viewport, const QList<RS_Entity*>& removed,
        const QList<RS_Entity*>& added, const QList<RS_Entity*>& selected);
    ~LC_OffsetCurvePublication();
    bool commit() noexcept;
    void notify() noexcept;
private:
    struct Data;
    explicit LC_OffsetCurvePublication(std::unique_ptr<Data> data);
    std::unique_ptr<Data> m_data;
};
#endif
