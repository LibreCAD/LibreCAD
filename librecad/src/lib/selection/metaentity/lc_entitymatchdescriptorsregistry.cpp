/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2025 LibreCAD.org
 * Copyright (C) 2025 sand1024
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

#include "lc_entitymatchdescriptorsregistry.h"

#include "lc_matchdescriptor_arc.h"
#include "lc_matchdescriptor_circle.h"
#include "lc_matchdescriptor_dimaligned.h"
#include "lc_matchdescriptor_dimangular.h"
#include "lc_matchdescriptor_dimarc.h"
#include "lc_matchdescriptor_dimdiametric.h"
#include "lc_matchdescriptor_dimlinear.h"
#include "lc_matchdescriptor_dimordinate.h"
#include "lc_matchdescriptor_dimradial.h"
#include "lc_matchdescriptor_ellipse.h"
#include "lc_matchdescriptor_hatch.h"
#include "lc_matchdescriptor_hyperbola.h"
#include "lc_matchdescriptor_image.h"
#include "lc_matchdescriptor_insert.h"
#include "lc_matchdescriptor_leader.h"
#include "lc_matchdescriptor_line.h"
#include "lc_matchdescriptor_mtext.h"
#include "lc_matchdescriptor_multiple.h"
#include "lc_matchdescriptor_parabola.h"
#include "lc_matchdescriptor_point.h"
#include "lc_matchdescriptor_polyline.h"
#include "lc_matchdescriptor_spline.h"
#include "lc_matchdescriptor_splinepoints.h"
#include "lc_matchdescriptor_text.h"
#include "lc_matchdescriptor_tolerance.h"

LC_EntityMatchDescriptorsRegistry::~LC_EntityMatchDescriptorsRegistry() {
    qDeleteAll(m_entityMatchDescriptors);
}

LC_EntityMatchDescriptor* LC_EntityMatchDescriptorsRegistry::findEntityMatchDescriptor(const RS2::EntityType entity) {
    if (m_entityMatchDescriptors.contains(entity)) {
        return m_entityMatchDescriptors[entity];
    }
    return nullptr;
}

LC_EntityMatchDescriptorsRegistry* LC_EntityMatchDescriptorsRegistry::instance(LC_ActionContext *actionContext) {
    static LC_EntityMatchDescriptorsRegistry* uniqueInstance;
    if (uniqueInstance == nullptr) {
        uniqueInstance = new LC_EntityMatchDescriptorsRegistry();
        uniqueInstance->initEntityDescriptors(actionContext);
    }
    return uniqueInstance;
}

void LC_EntityMatchDescriptorsRegistry::initEntityDescriptors(LC_ActionContext *actionContext) {
    // NOTE: separate descriptor class per entity is used Intentionally!!
    // otherwise, if all descriptors are combined in the same class, and due to
    // amount of templates the .obj for such class becomes very large to be compiled under Windows.
    // With special compiler options such a large file will be compiled
    // Flags are - add_compile_options(/bigobj) for MSVC or -Wa,-mbig-obj for GCC

    LC_MatchDescriptorMultiple::init(m_entityMatchDescriptors);
    LC_MatchDescriptorLine::init(m_entityMatchDescriptors);
    LC_MatchDescriptorCircle::init(m_entityMatchDescriptors);
    LC_MatchDescriptorArc::init(m_entityMatchDescriptors);
    LC_MatchDescriptorEllipse::init(m_entityMatchDescriptors);
    LC_MatchDescriptorHyperbola::init(m_entityMatchDescriptors);
    LC_MatchDescriptorPolyline::init(m_entityMatchDescriptors);
    LC_MatchDescriptorPoint::init(m_entityMatchDescriptors);
    LC_MatchDescriptorSpline::init(m_entityMatchDescriptors);
    LC_MatchDescriptorSplinePoints::init(m_entityMatchDescriptors);
    LC_MatchDescriptorHatch::init(m_entityMatchDescriptors);
    LC_MatchDescriptorInsert::init(m_entityMatchDescriptors, actionContext);
    LC_MatchDescriptorText::init(m_entityMatchDescriptors);
    LC_MatchDescriptorMText::init(m_entityMatchDescriptors);
    LC_MatchDescriptorImage::init(m_entityMatchDescriptors);
    LC_MatchDescriptorDimAligned::init(m_entityMatchDescriptors, actionContext);
    LC_MatchDescriptorDimLinear::init(m_entityMatchDescriptors, actionContext);
    LC_MatchDescriptorDimRadial::init(m_entityMatchDescriptors, actionContext);
    LC_MatchDescriptorDimDiametric::init(m_entityMatchDescriptors, actionContext);
    LC_MatchDescriptorDimAngular::init(m_entityMatchDescriptors, actionContext);
    LC_MatchDescriptorDimArc::init(m_entityMatchDescriptors, actionContext);
    LC_MatchDescriptorDimOrdinate::init(m_entityMatchDescriptors, actionContext);
    LC_MatchDescriptorLeader::init(m_entityMatchDescriptors);
    LC_MatchDescriptorTolerance::init(m_entityMatchDescriptors);
    LC_MatchDescriptorParabola::init(m_entityMatchDescriptors);
}
