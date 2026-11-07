/* Copyright (C) 2026 LibreCAD.org. SPDX-License-Identifier: GPL-2.0-or-later */
#include "lc_matchdescriptor_offsetcurve.h"

#include <cmath>
#include <optional>
#include <utility>

#include "lc_offsetcurve.h"
#include "lc_offsetcurvereadutils.h"
#include "rs_line.h"

namespace {
using LC_OffsetCurveRead::cachedLength;

template <typename T>
using OffsetValue = std::function<std::optional<T>(const LC_OffsetCurve&)>;

// Availability is checked even when the ordinary comparator is MATCH_ALL.
template <typename T>
class OffsetMatcher final : public LC_TypedEntityMatcher<T, LC_OffsetCurve> {
    using Base = LC_TypedEntityMatcher<T, LC_OffsetCurve>;
public:
    OffsetMatcher(const LC_TypedPropertyMatchTypeDescriptor<T>* type, OffsetValue<T> get)
        : Base(type, [this](LC_OffsetCurve*) { return *m_value; }), m_get(std::move(get)) {}

    bool isMatch(RS_Entity* entity) override {
        m_value.reset();
        const auto* curve = dynamic_cast<const LC_OffsetCurve*>(entity);
        if (!curve || curve->rtti() != RS2::EntityOffsetCurve) return false;
        m_value = m_get(*curve);
        return m_value.has_value() && Base::isMatch(entity);
    }
private:
    OffsetValue<T> m_get;
    std::optional<T> m_value;
};

template <typename T>
class OffsetProperty final : public LC_PropertyMatchDescriptor {
public:
    OffsetProperty(const QString& name, const QString& label, const QString& description,
                   const LC_TypedPropertyMatchTypeDescriptor<T>& type, OffsetValue<T> get)
        : LC_PropertyMatchDescriptor(name, label, description), m_type(type), m_get(std::move(get)) {
        m_propertyType = type.getType();
    }
    bool isSupportedOperation(LC_PropertyMatchOperation operation) override {
        return m_type.supportedOperations.testFlag(operation);
    }
    LC_EntityMatcher* createMatcher() override { return new OffsetMatcher<T>(&m_type, m_get); }
private:
    const LC_TypedPropertyMatchTypeDescriptor<T>& m_type;
    OffsetValue<T> m_get;
};

class OffsetDescriptor final : public LC_TypedEntityMatchDescriptor<LC_OffsetCurve> {
public:
    explicit OffsetDescriptor(const QString& name)
        : LC_TypedEntityMatchDescriptor<LC_OffsetCurve>(name, RS2::EntityOffsetCurve) {}
    template <typename T>
    void addAvailable(const QString& name, const QString& label, const QString& description,
                      const LC_TypedPropertyMatchTypeDescriptor<T>& type, OffsetValue<T> get) {
        m_entityPropertyDescriptors.push_back(new OffsetProperty<T>(name, label, description, type, std::move(get)));
    }
};
}

void LC_MatchDescriptorOffsetCurve::init(QMap<RS2::EntityType, LC_EntityMatchDescriptor*>& map) {
    auto* entity = new OffsetDescriptor(tr("Offset Curve"));
    initCommonEntityAttributesProperties<LC_OffsetCurve>(entity);
    entity->addAvailable<bool>("defined", tr("Defined"), tr("Has an embedded offset definition"),
        LC_PropertyMatcherTypes::BOOL, [](const LC_OffsetCurve& e) -> std::optional<bool> { return e.isDefined(); });
    entity->addAvailable<bool>("consistent", tr("Consistent"), tr("The cache matches the stored definition fingerprint"),
        LC_PropertyMatcherTypes::BOOL, [](const LC_OffsetCurve& e) -> std::optional<bool> { return e.isConsistent(); });
    entity->addAvailable<bool>("editableDefinition", tr("Editable Definition"), tr("Definition is available and consistent"),
        LC_PropertyMatcherTypes::BOOL, [](const LC_OffsetCurve& e) -> std::optional<bool> { return e.hasEditableDefinition(); });
    entity->addAvailable<QString>("source", tr("Source"), tr("Embedded source geometry, not a live entity link"),
        LC_PropertyMatcherTypes::STRING, [](const LC_OffsetCurve& e) -> std::optional<QString> {
            if (!e.hasEditableDefinition()) return std::nullopt;
            return QStringLiteral("ELLIPSE");
        });
    entity->addAvailable<double>("distance", tr("Distance"), tr("Distance from the embedded source ellipse"),
        LC_PropertyMatcherTypes::LENGTH, [](const LC_OffsetCurve& e) -> std::optional<double> {
            if (!e.hasEditableDefinition()) return std::nullopt;
            return e.definition().distance;
        });
    entity->addAvailable<bool>("towardCentre", tr("Inward"), tr("True for inward, false for outward; independent of traversal"),
        LC_PropertyMatcherTypes::BOOL, [](const LC_OffsetCurve& e) -> std::optional<bool> {
            if (!e.hasEditableDefinition()) return std::nullopt;
            return e.definition().towardCentre;
        });
    entity->addAvailable<double>("length", tr("Cached Length"), tr("Length of the displayed spline cache"),
        LC_PropertyMatcherTypes::LENGTH, cachedLength);
    map.insert(RS2::EntityOffsetCurve, entity);
}
