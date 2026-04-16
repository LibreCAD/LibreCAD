
/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2026 LibreCAD.org
 * Copyright (C) 2026 sand1024
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 * ********************************************************************************
 */

#ifndef LC_VISUALSNAPSOLUTIONVISUALIZER_H
#define LC_VISUALSNAPSOLUTIONVISUALIZER_H

#include "lc_highlight.h"
#include "lc_ref_snap_construction_line.h"
#include "lc_visual_snap_solution.h"
#include "rs_preview.h"

struct LC_VisualSnapOptions;
class RS_Snapper;

class LC_VisualSnapSolutionVisualizer {
public:
    LC_VisualSnapSolutionVisualizer(RS_Snapper* snapper, LC_VisualSnapOptions* options)
        : m_snapper(snapper), m_options(options) {
    }

    void visualizeSolution(RS_Preview* preview, LC_Highlight* highlight, LC_VisualSnapSolution& solution) const;
    void visualizeOrdinaryRestrictions(RS_Preview* preview, LC_Highlight* highlight) const;
    void setViewport(LC_GraphicViewport* viewport) {
        m_viewport = viewport;
    }
protected:
    void adjustGuidingEntitiesLabelsPositions(std::vector<LC_RefSnapConstructionLine*> lines) const;
    void assignGuidingEntitiesLabels(RS_EntityContainer* preview, std::list<RS_Entity*> clones) const;

    LC_GraphicViewport* m_viewport = nullptr;
    RS_Snapper* m_snapper{nullptr};
    LC_VisualSnapOptions* m_options {nullptr};
};

#endif
