/*
 * This file is part of the LibreCAD project.
 * Copyright (C) 2026 LibreCAD.org
 * Licensed under GPL-2.0-or-later.
 */
#ifndef LC_TRIMBATCH_H
#define LC_TRIMBATCH_H

#include <memory>
#include <vector>

#include <QList>

#include "rs_vector.h"

class RS_AtomicEntity;
class RS_Entity;
struct LC_DocumentModificationBatch;

bool isReplaceableByTrim(const RS_Entity& entity);

struct LC_TrimCut {
    double t = 0.0;
    RS_Vector point;
    int component = 0;
    int sideBefore = 0;
    int sideAfter = 0;
};

class LC_TrimLimitSet {
public:
    LC_TrimLimitSet();
    ~LC_TrimLimitSet();
    LC_TrimLimitSet(LC_TrimLimitSet&&) noexcept;
    LC_TrimLimitSet& operator=(LC_TrimLimitSet&&) noexcept;
    LC_TrimLimitSet(const LC_TrimLimitSet&) = delete;
    LC_TrimLimitSet& operator=(const LC_TrimLimitSet&) = delete;

    void add(const RS_Entity& limit);
    void build();
    int componentCount() const;
    bool isClosed(int component) const;
    int sideOfPoint(int component, const RS_Vector& point) const;
    int sideOfDirection(int component, const RS_Vector& point, const RS_Vector& direction) const;
    bool containsSourceOf(const RS_Entity* entity) const;

private:
    struct Data;
    std::unique_ptr<Data> m_data;
    friend class LC_TrimBatch;
};

class LC_TrimBatch {
public:
    enum class Verdict { Kept, Removed, Undetermined };
    struct Stats {
        int trimmed = 0;
        int removed = 0;
        int undetermined = 0;
        int untouched = 0;
    };

    LC_TrimBatch(const QList<RS_Entity*>& targets, const LC_TrimLimitSet& limits);
    ~LC_TrimBatch();
    LC_TrimBatch(LC_TrimBatch&&) noexcept;
    LC_TrimBatch& operator=(LC_TrimBatch&&) noexcept;
    LC_TrimBatch(const LC_TrimBatch&) = delete;
    LC_TrimBatch& operator=(const LC_TrimBatch&) = delete;

    bool anyCut() const;
    int targetCount() const;
    int pieceCount(int target) const;
    std::vector<RS_Vector> cutPoints() const;
    Verdict verdict(int target, int piece, const std::vector<int>& cursorSides) const;
    std::vector<int> cursorSides(const RS_Vector& cursor) const;
    std::vector<std::unique_ptr<RS_AtomicEntity>> removedPieces(const std::vector<int>& cursorSides) const;
    bool pieceAt(const RS_Vector& point, double catchDistance, int& target, int& piece) const;
    void toggle(int target, int piece);
    bool undoToggle();
    Stats apply(const std::vector<int>& cursorSides, bool divideOnly, LC_DocumentModificationBatch& context) const;

private:
    struct Data;
    std::unique_ptr<Data> m_data;
};

#endif
