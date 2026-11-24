/*
 * This file is part of the LibreCAD project.
 * Copyright (C) 2026 LibreCAD.org
 * Copyright (C) 2026 Dongxu Li (github.com/dxli)
 * Licensed under GPL-2.0-or-later.
 */
#ifndef LC_CURVESPLIT_H
#define LC_CURVESPLIT_H

#include <memory>

class RS_AtomicEntity;
class RS_Vector;

namespace lc::curvesplit {
bool isSupported(const RS_AtomicEntity& entity);
bool isClosed(const RS_AtomicEntity& entity);
double paramOf(const RS_AtomicEntity& entity, const RS_Vector& pointOnEntity);
RS_Vector tangentAt(const RS_AtomicEntity& entity, const RS_Vector& pointOnEntity);
std::unique_ptr<RS_AtomicEntity> subCurve(const RS_AtomicEntity& entity,
                                         double tA,
                                         const RS_Vector& pA,
                                         double tB,
                                         const RS_Vector& pB);
}

#endif
