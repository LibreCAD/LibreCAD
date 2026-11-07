/*******************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 * Copyright (C) 2026 LibreCAD.org
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later version.
 ******************************************************************************/

#include "lc_offsetcontactmatching.h"

#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
#include <utility>

#include <boost/multiprecision/cpp_int.hpp>

namespace LC_EllipseOffsetGeometry {
namespace {
using Rational = boost::multiprecision::cpp_rational;
using Integer = boost::multiprecision::cpp_int;
using Status = CacheContactMatchStatus;

static_assert(std::numeric_limits<double>::radix == 2 && std::numeric_limits<double>::digits == 53);

Rational exactDouble(const double value) {
    if (value == 0) { return Rational{0}; }
    int exponent = 0;
    const auto significand = static_cast<std::int64_t>(std::ldexp(std::frexp(value, &exponent), 53));
    exponent -= 53;
    Integer numerator{significand}, denominator{1};
    if (exponent >= 0) { numerator <<= exponent; }
    else { denominator <<= -exponent; }
    return Rational{numerator} / Rational{denominator};
}

struct ExactPoint { Rational x, y; };
struct ExactPosition { std::size_t segment = 0; Rational ordinal; };
struct PreparedPort {
    std::size_t node = 0, count = 0;
    Rational ordinal;
    ExactPoint point;
    std::array<ExactPosition, 2> positions;
};

ExactPoint evaluate(const CacheSpan& raw, const bool reversed, const double parameter) {
    std::array<ExactPoint, 4> net;
    for (std::size_t i = 0; i <= raw.degree; ++i) {
        const auto& point = raw.controls[reversed ? raw.degree-i : i];
        net[i] = {exactDouble(point.x), exactDouble(point.y)};
    }
    const auto t = exactDouble(parameter);
    const Rational complement = Rational{1} - t;
    // Degree <= 3 and binary64 inputs bound each reduced coordinate to < 6000
    // bits. The six de Casteljau steps are fixed work, not an iterative solve.
    for (std::size_t level = raw.degree; level != 0; --level) {
        for (std::size_t i = 0; i < level; ++i) {
            net[i].x = complement*net[i].x + t*net[i+1].x;
            net[i].y = complement*net[i].y + t*net[i+1].y;
        }
    }
    return std::move(net[0]);
}

bool finite(const LC_Interval& interval) {
    return interval.isValid() && std::isfinite(interval.lo()) && std::isfinite(interval.hi()) &&
        interval.lo() <= interval.hi();
}

bool uniqueProof(const LC_IntersectionProof proof) {
    switch (proof) {
    case LC_IntersectionProof::IntervalUniqueTransverse:
    case LC_IntersectionProof::AnalyticEndpoint:
    case LC_IntersectionProof::AnalyticCusp:
    case LC_IntersectionProof::AnalyticTangency: return true;
    default: return false;
    }
}

class Matcher {
public:
    Matcher(const CacheContactAnalysis& contacts, const std::vector<CacheContactPort>& ports, WorkBudget& budget)
        : m_contacts{contacts}, m_ports{ports}, m_budget{budget} {}

    CacheContactMatchResult run() {
        if (m_budget.status() != BudgetStatus::Ok) { return finish(); }
        if (!m_contacts.hasValidProofs()) { return invalid(); }
        const auto portCount = m_ports.size(), rootCount = m_contacts.contactCount();
        if (portCount > m_prepared.max_size() || rootCount > m_result.matches.max_size() ||
            portCount > m_budget.remainingVertices() || rootCount > m_budget.remainingVertices()-portCount) {
            m_budget.latchLimitExceeded(); return finish();
        }
        m_prepared.reserve(portCount);
        m_result.matches.reserve(rootCount);
        for (const auto& port : m_ports) {
            if (!prepare(port)) { return finish(); }
        }
        bool expected = false;
        for (std::size_t i = 0; i < portCount; ++i) {
            if (!examine()) { return finish(); }
            for (std::size_t j = 0; j < i; ++j) {
                if (!examine(2)) { return finish(); }
                if (m_prepared[i]->ordinal == m_prepared[j]->ordinal) { return invalid(); }
                expected = (m_prepared[i]->node == m_prepared[j]->node) || expected;
            }
        }
        if ((rootCount == 0 && expected) || (rootCount != 0 && !expected)) {
            m_result.status = Status::Topology; return finish();
        }
        bool unresolved = false;
        for (std::size_t root = 0; root < rootCount; ++root) {
            if (!examine(24)) { return finish(); }
            // Only the owning analysis can issue/snapshot this handle. Public
            // event copies and numeric midpoint seeds never enter the API.
            const auto snapshot = m_contacts.snapshot(m_contacts.contactHandle(root));
            if (!snapshot || !valid(*snapshot)) { m_result.status = Status::Ambiguous; return finish(); }
            CacheContactMatch match{root, std::size_t(-1), std::size_t(-1)};
            for (std::size_t i = 0; i < portCount; ++i) {
                if (!examine()) { return finish(); }
                for (std::size_t j = 0; j < i; ++j) {
                    if (!examine(2)) { return finish(); }
                    if (m_prepared[i]->point.x != m_prepared[j]->point.x ||
                        m_prepared[i]->point.y != m_prepared[j]->point.y) { continue; }
                    const auto& event = snapshot->event;
                    const bool forward = side(*m_prepared[j], event, true) && side(*m_prepared[i], event, false);
                    const bool reverse = side(*m_prepared[i], event, true) && side(*m_prepared[j], event, false);
                    if (m_budget.status() != BudgetStatus::Ok) { return finish(); }
                    if (!forward && !reverse) { continue; }
                    if (!examine(1)) { return finish(); }
                    if (m_prepared[i]->node != m_prepared[j]->node) {
                        m_result.status = Status::Topology; return finish();
                    }
                    if (match.firstPort != std::size_t(-1)) { m_result.status = Status::Ambiguous; return finish(); }
                    match.firstPort = j; match.secondPort = i;
                }
            }
            if (match.firstPort == std::size_t(-1)) { unresolved = true; continue; }
            if (!m_budget.consumeVertices(1)) { return finish(); }
            m_result.matches.push_back(match); // Pre-reserved, trivially nonthrowing record.
        }
        for (std::size_t i = 0; i < portCount; ++i) {
            if (!examine()) { return finish(); }
            for (std::size_t j = 0; j < i; ++j) {
                if (!examine(1)) { return finish(); }
                if (m_prepared[i]->node != m_prepared[j]->node) { continue; }
                bool covered = false;
                for (const auto& match : m_result.matches) {
                    if (!examine(2)) { return finish(); }
                    if (match.firstPort == j && match.secondPort == i) { covered = true; break; }
                }
                unresolved = unresolved || !covered;
            }
        }
        m_result.status = unresolved ? Status::Ambiguous : Status::Matched;
        return finish();
    }

private:
    bool examine(const std::size_t comparisons = 0) {
        return m_budget.adoptWork({0, 0, 1, 0, comparisons});
    }
    CacheContactMatchResult invalid() {
        m_budget.latchInvalidInput(); m_result.status = Status::InvalidInput; return finish();
    }
    bool prepare(const CacheContactPort& port) {
        if (!examine(3)) { return false; }
        if (port.referenceNode == std::size_t(-1) || port.positionCount == 0 || port.positionCount > 2) {
            m_budget.latchInvalidInput(); return false;
        }
        auto prepared = std::make_unique<PreparedPort>();
        prepared->node = port.referenceNode; prepared->count = port.positionCount;
        for (std::size_t i = 0; i < port.positionCount; ++i) {
            if (!examine(12)) { return false; }
            const auto& position = port.positions[i];
            if (position.segment >= m_contacts.spanCount() || !std::isfinite(position.parameter) ||
                !(position.parameter >= 0 && position.parameter <= 1)) {
                m_budget.latchInvalidInput(); return false;
            }
            const auto* raw = m_contacts.rawSpan(position.segment);
            if (!raw || (raw->degree != 1 && raw->degree != 3)) { m_result.status = Status::Ambiguous; return false; }
            Rational ordinal = Rational{position.segment} + exactDouble(position.parameter);
            prepared->positions[i].segment = position.segment;
            prepared->positions[i].ordinal = ordinal;
            if (m_contacts.closed() && position.segment == m_contacts.spanCount()-1 && position.parameter == 1) {
                ordinal = 0;
            }
            auto point = evaluate(*raw, m_contacts.reversed(), position.parameter);
            if (i == 0) { prepared->ordinal = ordinal; prepared->point = std::move(point); }
            else if (ordinal != prepared->ordinal || point.x != prepared->point.x || point.y != prepared->point.y) {
                m_budget.latchInvalidInput(); return false;
            }
        }
        if (!m_budget.consumeVertices(1)) { return false; }
        m_prepared.push_back(std::move(prepared)); // Staged pointee; pointer move cannot allocate/throw.
        return true;
    }
    bool valid(const LC_CertifiedRootSnapshot& snapshot) const {
        const auto& event = snapshot.event;
        const auto& proof = event.proofDomain;
        const auto& isolation = event.isolation;
        if (!uniqueProof(event.proof) || event.branchA != 0 || event.branchB != 0 ||
            proof.segmentA != isolation.segmentA || proof.segmentB != isolation.segmentB ||
            proof.segmentA >= m_contacts.spanCount() || proof.segmentB >= m_contacts.spanCount()) { return false; }
        const auto inside = [](const LC_Interval& bound, const LC_Interval& inner, const std::size_t segment) {
            return finite(bound) && finite(inner) && double(segment) <= bound.lo() && bound.hi() <= double(segment+1) &&
                bound.lo() <= inner.lo() && inner.hi() <= bound.hi();
        };
        return inside(proof.parameterA, isolation.parameterA, proof.segmentA) &&
            inside(proof.parameterB, isolation.parameterB, proof.segmentB);
    }
    bool side(const PreparedPort& port, const LC_ParametricIntersection& event, const bool first) {
        const auto segment = first ? event.proofDomain.segmentA : event.proofDomain.segmentB;
        const auto& proof = first ? event.proofDomain.parameterA : event.proofDomain.parameterB;
        const auto& isolation = first ? event.isolation.parameterA : event.isolation.parameterB;
        for (std::size_t i = 0; i < port.count; ++i) {
            if (!examine(5)) { return false; }
            const auto& position = port.positions[i];
            if (position.segment == segment && exactDouble(proof.lo()) <= position.ordinal &&
                position.ordinal <= exactDouble(proof.hi()) && exactDouble(isolation.lo()) <= position.ordinal &&
                position.ordinal <= exactDouble(isolation.hi())) { return true; }
        }
        return false;
    }
    CacheContactMatchResult finish() {
        if (m_budget.status() == BudgetStatus::LimitExceeded) { m_result.status = Status::LimitExceeded; }
        else if (m_budget.status() == BudgetStatus::InvalidInput) { m_result.status = Status::InvalidInput; }
        if (m_result.status != Status::Matched) { m_result.matches.clear(); }
        m_result.work = m_budget.consumed(); return std::move(m_result);
    }

    const CacheContactAnalysis& m_contacts;
    const std::vector<CacheContactPort>& m_ports;
    WorkBudget& m_budget;
    CacheContactMatchResult m_result;
    std::vector<std::unique_ptr<PreparedPort>> m_prepared;
};
} // namespace

CacheContactMatchResult matchCacheContacts(const CacheContactAnalysis& contacts,
    const std::vector<CacheContactPort>& ports, WorkBudget& budget) {
    return Matcher{contacts, ports, budget}.run();
}

} // namespace LC_EllipseOffsetGeometry
