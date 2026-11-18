/* Copyright (C) 2026 LibreCAD.org. SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef LC_MATCHDESCRIPTOROFFSETCURVE_H
#define LC_MATCHDESCRIPTOROFFSETCURVE_H

#include "lc_matchdescriptor_base.h"

class LC_MatchDescriptorOffsetCurve : public LC_MatchDescriptorBase {
    Q_OBJECT
public:
    static void init(QMap<RS2::EntityType, LC_EntityMatchDescriptor*>& map);
};

#endif
