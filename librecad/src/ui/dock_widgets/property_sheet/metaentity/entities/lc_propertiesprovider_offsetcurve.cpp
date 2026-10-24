/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "lc_propertiesprovider_offsetcurve.h"

#include <QCoreApplication>
#include <QHash>
#include <QSet>
#include <QSignalBlocker>
#include <cmath>
#include <functional>
#include <optional>
#include <utility>
#include <vector>
#include "lc_offsetcurvepropertyqueue.h"
#include "lc_offsetcurvereadutils.h"
#include "lc_property_double_interactivepick_view.h"
#include "lc_property_editor_handler.h"
#include "lc_property_editor_utils.h"
#include "lc_property_layer.h"
#include "lc_property_lineedit_with_button.h"
#include "lc_property_linetype.h"
#include "lc_property_linewidth.h"
#include "lc_property_rscolor.h"
#include "lc_property_view_factory.h"
#include "rs_document.h"
#include "rs_graphic.h"
#include "rs_graphicview.h"
#include "rs_layer.h"
#include "rs_math.h"

namespace {
QString nativeTr(const char* text) {
    return QCoreApplication::translate("LC_OffsetCurveUI", text);
}
constexpr const char* nativeProperty = "offsetCurveProperty";
constexpr const char* unavailableProperty = "offsetCurveUnavailable";
const QByteArray numberView("offsetCurveNumber");

class NativeNumber final : public LC_PropertyDouble {
public:
    explicit NativeNumber(QObject* parent) : LC_PropertyDouble(parent, false) {}
    bool angle = false;
    QPointer<LC_OffsetCurvePropertyQueue> queue;
    std::function<void()> parseRefused;
    void requestNativeInput() const {
        const auto type = getInteractiveInputType();
        if (type == LC_ActionContext::InteractiveInputInfo::POINT_X ||
            type == LC_ActionContext::InteractiveInputInfo::POINT_Y)
            m_actionContext->interactiveInputStart(type, m_lateCompletionRequestor, getName());
        else requestInteractiveInput();
    }
};

class NativeNumberView final : public LC_PropertyDoubleInteractivePickView {
public:
    explicit NativeNumberView(LC_PropertyDouble* property)
        : LC_PropertyDoubleInteractivePickView(property) {}
protected:
    QWidget* doCreateValueEditor(QWidget*, const QRect&, const EditActivationContext*) override;
    bool doPropertyValueToStrForEdit(QString& text) const override {
        const auto* property = static_cast<const NativeNumber*>(getProperty());
        const double value = property->value();
        // Exact edit text: simply focusing a rounded display is never an edit.
        text = QString::number(property->angle ? RS_Math::rad2deg(value) : value, 'g', 17);
        return true;
    }
    bool doPropertyValueToStrForView(QString& text) override {
        if (getProperty()->property(unavailableProperty).toBool()) {
            text = nativeTr("Unavailable");
            return true;
        }
        const auto* property = static_cast<const NativeNumber*>(getProperty());
        if (property->angle) {
            text = property->getFormatter()->formatRawAngle(property->value());
            return true;
        }
        return LC_PropertyDoubleInteractivePickView::doPropertyValueToStrForView(text);
    }
};

class NativeNumberHandler final :
    public LC_PropertyEditorHandler<LC_PropertyDouble, LC_PropertyLineEditWithButton> {
public:
    NativeNumberHandler(NativeNumberView* view, LC_PropertyLineEditWithButton& editor)
        : LC_PropertyEditorHandler(view, editor) {
        auto* line = editor.getLineEdit();
        line->installEventFilter(this);
        const auto* property = static_cast<const NativeNumber*>(&getProperty());
        if (property->queue)
            connect(property->queue, &LC_OffsetCurvePropertyQueue::finishEditors, this, [this] { finish(); });
        connect(line, &QLineEdit::editingFinished, this, [this] { finish(); });
        connect(editor.getToolButton(), &QToolButton::clicked, this, [this] {
            // Capture a guarded property before stopping an editor that may be deleted.
            QPointer<NativeNumber> property = static_cast<NativeNumber*>(&getProperty());
            stopInplaceEdit();
            if (property != nullptr && property->isEditableByUser()) property->requestNativeInput();
        });
        doUpdateEditor();
    }
    void doUpdateEditor() override {
        if (getBaseProperty() == nullptr) return;
        auto* line = getEditor()->getLineEdit();
        const QSignalBlocker blocker(line);
        line->setReadOnly(!isEditableByUser());
        if (isMultiValue()) {
            line->clear();
            line->setPlaceholderText(LC_Property::getMultiValuePlaceholder());
        }
        else {
            QString text;
            static_cast<NativeNumberView*>(view())->getPropertyEditStringValue(text);
            line->setText(text);
            line->setPlaceholderText({});
            line->selectAll();
        }
        line->setModified(false);
    }
private:
    void finish() {
        if (getBaseProperty() == nullptr) return;
        auto* line = getEditor()->getLineEdit();
        if (line->isModified() && isEditableByUser() && !m_reverted) {
            auto& property = static_cast<NativeNumber&>(getProperty());
            bool ok = false;
            const QString text = line->text().trimmed();
            double value = text.isEmpty() ? 0 : RS_Math::eval(text, &ok);
            if (property.angle) value = RS_Math::deg2rad(value);
            if (ok && std::isfinite(value)) {
                property.setValue(value, changeReasonDueToEdit());
            }
            else if (property.parseRefused) {
                property.parseRefused();
            }
        }
        // Refusal, parse failure, Escape and no-op all restore from the real staged getter.
        // Clear modified before a subsequent focus-out or deferred editor destruction.
        doUpdateEditor();
        doApplyReset();
    }
};

QWidget* NativeNumberView::doCreateValueEditor(QWidget* parent, const QRect& rect,
                                               const EditActivationContext* context) {
    if (!isEditableByUser()) return nullptr;
    auto* editor = new LC_PropertyLineEditWithButton(parent);
    editor->setGeometry(rect);
    auto* button = editor->getToolButton();
    const auto icon = getButtonIconName();
    button->setVisible(!icon.isEmpty());
    button->setText({});
    button->setToolButtonStyle(Qt::ToolButtonIconOnly);
    button->setIcon(QIcon(icon));
    button->setToolTip(getButtonTooltip(typedProperty().getInteractiveInputType()));
    new NativeNumberHandler(this, *editor);
    LC_PropertyEditorUtils::initializeLineEditor(editor->getLineEdit(), context);
    return editor;
}
}

struct LC_OffsetCurvePropertySession {
    QPointer<LC_OffsetCurvePropertyQueue> queue;
    QPointer<RS_GraphicView> view;
    LC_ActionContext* context = nullptr;
    LC_OffsetCurvePropertyQueue::Epoch epoch;
    QList<RS_Entity*> targets;
    std::vector<std::function<void()>> refreshers;

    const LC_OffsetCurve* value(const RS_Entity* target) const {
        return queue == nullptr ? nullptr : queue->value(epoch, target);
    }
    void refresh() const {
        for (const auto& update : refreshers) update();
    }
    void apply(const LC_OffsetCurvePropertyQueue::Edit& edit) const {
        if (queue != nullptr) queue->apply(epoch, targets, edit);
        refresh();
    }
};

namespace {
/** One atomic aggregate row, not LC_PropertyMulti's post-edit first-getter broadcast. */
template<typename Property, typename Get, typename Set>
void bind(Property* property, const std::shared_ptr<LC_OffsetCurvePropertySession>& session,
          bool geometry, Get get, Set set, bool readOnly = false) {
    using Value = typename Property::ValueType;
    property->setProperty(nativeProperty, true);
    auto getter = [session, geometry, get]() -> Value {
        for (auto* target : session->targets) {
            const auto* curve = session->value(target);
            if (curve != nullptr && (!geometry || curve->hasEditableDefinition())) return get(*curve);
        }
        return Value{};
    };
    property->useOwnedDelegatedValue()->setupFull(getter,
        [session, set](Value value, LC_PropertyChangeReason) {
            // The requested payload is value-owned and identical for every target, even if the first refuses.
            session->apply([set, value](LC_OffsetCurve& curve, const LC_OffsetRunLimits& limits) {
                return set(curve, value, limits);
            });
        }, [session, geometry, get](Value value) {
            bool any = false;
            for (auto* target : session->targets) {
                const auto* curve = session->value(target);
                if (curve == nullptr || (geometry && !curve->hasEditableDefinition()) || !(get(*curve) == value)) return false;
                any = true;
            }
            return any;
        });
    const std::weak_ptr<LC_OffsetCurvePropertySession> weak = session;
    QPointer<Property> guarded(property);
    session->refreshers.emplace_back([weak, guarded, geometry, get, readOnly] {
        const auto session = weak.lock();
        if (!session || guarded == nullptr) return;
        bool available = false, editable = false, mixed = false;
        Value first{};
        for (auto* target : session->targets) {
            const auto* curve = session->value(target);
            if (curve == nullptr || (geometry && !curve->hasEditableDefinition())) continue;
            const auto value = get(*curve);
            if (!available) first = value;
            else if (!(value == first)) mixed = true;
            available = true;
            const auto* layer = target->getLayer(true);
            if (layer == nullptr || !layer->isLocked()) editable = true;
        }
        guarded->setProperty(unavailableProperty, geometry && !available);
        guarded->switchState(PropertyStateValueMulti, mixed);
        guarded->setReadOnly(readOnly || !editable);
        guarded->postUpdateEvent(PropertyChangeReasonValueLoaded);
    });
}

LC_OffsetCurveEditResult attributeResult(bool changed) {
    LC_OffsetCurveEditResult result;
    result.refusal = LC_OffsetCurveRefusal::None;
    result.changed = changed;
    return result; // attributes did not run the producer or the checker
}
}

LC_PropertiesProviderOffsetCurve::LC_PropertiesProviderOffsetCurve(
    LC_ActionContext* context, LC_PropertySheetWidget* widget)
    : LC_EntityTypePropertiesProvider(RS2::EntityOffsetCurve, context, widget) {
    LC_PropertyViewFactory::staticInstance()->registerView(&LC_PropertyDouble::staticMetaObject,
        &createViewForProperty<NativeNumberView, LC_PropertyDouble>, numberView);
}

void LC_PropertiesProviderOffsetCurve::fillGenericAttributes(
    LC_PropertyContainer* container, const QList<RS_Entity*>& list) {
    m_session = std::make_shared<LC_OffsetCurvePropertySession>();
    m_session->queue = m_widget->offsetPropertyQueue();
    m_session->epoch = m_session->queue->epoch();
    m_session->context = m_actionContext;
    m_session->view = m_actionContext->getGraphicView();
    for (auto* entity : list) {
        if (entity != nullptr && entity->rtti() == RS2::EntityOffsetCurve) m_session->targets.append(entity);
    }
    if (m_session->targets.isEmpty()) return;
    const auto section = createSection(container, {SECTION_GENERAL, tr("General"), tr("General properties")});
    const auto session = m_session;
    auto* document = getDocument();

    auto* layer = new LC_PropertyLayer(section, false);
    layer->setNames({"layer", tr("Layer"), tr("Layer of entity")});
    layer->setLayerList(document->getGraphic()->getLayerList());
    layer->setAllowByBlockValues(document->getBlock() != nullptr);
    bind(layer, session, false, [](const LC_OffsetCurve& curve) { return curve.getLayer(); },
        [](LC_OffsetCurve& curve, RS_Layer* value, const LC_OffsetRunLimits&) {
            const bool changed = curve.getLayer() != value;
            curve.setLayer(value);
            return attributeResult(changed);
        });
    section->addChildProperty(layer);

    auto* color = new LC_PropertyRSColor(section, false);
    color->setNames({"color", tr("Color"), tr("Color of entity")});
    bind(color, session, false, [](const LC_OffsetCurve& curve) { return curve.getPen(false).getColor(); },
        [](LC_OffsetCurve& curve, const RS_Color& value, const LC_OffsetRunLimits&) {
            auto pen = curve.getPen(false);
            const bool changed = !(pen.getColor() == value);
            pen.setColor(value); curve.setPen(pen);
            return attributeResult(changed);
        });
    section->addChildProperty(color);

    auto* lineType = new LC_PropertyLineType(section, false);
    lineType->setNames({"linetype", tr("Line Type"), tr("Type of entity pen line")});
    bind(lineType, session, false, [](const LC_OffsetCurve& curve) { return curve.getPen(false).getLineType(); },
        [](LC_OffsetCurve& curve, RS2::LineType value, const LC_OffsetRunLimits&) {
            auto pen = curve.getPen(false);
            const bool changed = pen.getLineType() != value;
            pen.setLineType(value); curve.setPen(pen);
            return attributeResult(changed);
        });
    section->addChildProperty(lineType);

    auto* width = new LC_PropertyLineWidth(section, false);
    width->setNames({"linewidth", tr("Line Width"), tr("Width of entity pen line")});
    bind(width, session, false, [](const LC_OffsetCurve& curve) { return curve.getPen(false).getWidth(); },
        [](LC_OffsetCurve& curve, RS2::LineWidth value, const LC_OffsetRunLimits&) {
            auto pen = curve.getPen(false);
            const bool changed = pen.getWidth() != value;
            pen.setWidth(value); curve.setPen(pen);
            return attributeResult(changed);
        });
    section->addChildProperty(width);
    session->refresh();
}

void LC_PropertiesProviderOffsetCurve::doCreateEntitySpecificProperties(
    LC_PropertyContainer* container, const QList<RS_Entity*>&) {
    const auto session = m_session;
    if (!session || session->targets.isEmpty()) return;
    const auto section = createGeometrySection(container);
    using Input = LC_ActionContext::InteractiveInputInfo;
    auto number = [this, session, section](const LC_Property::Names& names, Input::InputType input,
                                          bool angle, auto get, auto set) {
        auto* property = new NativeNumber(section);
        property->queue = session->queue;
        property->setNames(names);
        property->angle = angle;
        property->setInteractiveInputType(input);
        property->setActionContextAndLaterRequestor(m_actionContext, m_widget);
        LC_PropertyViewDescriptor descriptor;
        descriptor.viewName = numberView;
        property->setViewDescriptor(descriptor);
        property->parseRefused = [session] {
            if (session->queue != nullptr && session->queue->epoch() == session->epoch && session->context != nullptr) {
                session->context->commandMessage(nativeTr("Offset Curve: enter a finite numeric expression; the value was not changed."));
            }
            session->refresh();
        };
        bind(property, session, true, get, set);
        section->addChildProperty(property);
    };
    number({"distance", tr("Distance"), tr("Positive offset distance")}, Input::DISTANCE, false,
        [](const LC_OffsetCurve& curve) { return curve.definition().distance; },
        [](LC_OffsetCurve& curve, double value, const LC_OffsetRunLimits& limits) {
            return curve.setDistance(value, limits);
        });

    auto* side = createBoolProperty({"towardCentre", tr("Toward Centre"), tr("Physical inward side of the source ellipse")}, section);
    bind(side, session, true, [](const LC_OffsetCurve& curve) { return curve.definition().towardCentre; },
        [](LC_OffsetCurve& curve, bool value, const LC_OffsetRunLimits& limits) { return curve.setTowardCentre(value, limits); });
    section->addChildProperty(side);

    // Separate components preserve each peer's other UCS coordinate, not the first peer's.
    for (const bool x : {true, false}) {
        number({x ? "centerX" : "centerY", x ? tr("Center X") : tr("Center Y"),
                x ? tr("Source center X in UCS") : tr("Source center Y in UCS")},
            x ? Input::POINT_X : Input::POINT_Y, false,
            [view = session->view, x](const LC_OffsetCurve& curve) {
                const auto center = view->getViewPort()->toUCS(curve.definition().basis.center);
                return x ? center.x : center.y;
            }, [view = session->view, x](LC_OffsetCurve& curve, double value, const LC_OffsetRunLimits& limits) {
                auto basis = curve.definition().basis;
                auto* viewport = view->getViewPort();
                auto center = viewport->toUCS(basis.center);
                if ((x ? center.x : center.y) == value) return curve.setBasis(basis, limits);
                if (x) center.x = value; else center.y = value;
                basis.center = viewport->toWorld(center);
                return curve.setBasis(basis, limits);
            });
    }
    number({"radiusMajor", tr("Radius Major"), tr("Major radius of the source ellipse")}, Input::DISTANCE, false,
        [](const LC_OffsetCurve& curve) { return curve.definition().basis.majorP.magnitude(); },
        [](LC_OffsetCurve& curve, double value, const LC_OffsetRunLimits& limits) {
            auto basis = curve.definition().basis;
            const double major = basis.majorP.magnitude();
            const double minor = major * basis.ratio;
            // Reject rather than flip an axis or silently exchange physical radii.
            if (!(value > 0) || value < minor || !std::isfinite(value)) return LC_OffsetCurveEditResult{};
            if (value == major) return curve.setBasis(basis, limits);
            basis.majorP = RS_Vector::polar(value, basis.majorP.angle());
            basis.ratio = minor / value;
            return curve.setBasis(basis, limits);
        });
    number({"radiusMinor", tr("Radius Minor"), tr("Minor radius of the source ellipse")}, Input::DISTANCE, false,
        [](const LC_OffsetCurve& curve) { return curve.definition().basis.majorP.magnitude() * curve.definition().basis.ratio; },
        [](LC_OffsetCurve& curve, double value, const LC_OffsetRunLimits& limits) {
            auto basis = curve.definition().basis;
            const double major = basis.majorP.magnitude();
            if (!(value > 0) || value > major || !std::isfinite(value)) return LC_OffsetCurveEditResult{};
            if (value == major * basis.ratio) return curve.setBasis(basis, limits);
            basis.ratio = value / major;
            return curve.setBasis(basis, limits);
        });
    number({"angle", tr("Rotation"), tr("Source major-axis rotation in UCS")}, Input::ANGLE, true,
        [view = session->view](const LC_OffsetCurve& curve) {
            auto* viewport = view->getViewPort();
            return viewport->toBasisUCSAngle(viewport->toUCSAngle(curve.definition().basis.majorP.angle()));
        }, [view = session->view](LC_OffsetCurve& curve, double value, const LC_OffsetRunLimits& limits) {
            auto basis = curve.definition().basis;
            auto* viewport = view->getViewPort();
            const auto current = viewport->toBasisUCSAngle(viewport->toUCSAngle(basis.majorP.angle()));
            if (current == value) return curve.setBasis(basis, limits);
            const auto world = viewport->toWorldAngle(viewport->toAbsUCSAngle(value));
            basis.majorP = RS_Vector::polar(basis.majorP.magnitude(), world);
            return curve.setBasis(basis, limits);
        });
    // Raw ellipse parameters are not WCS bearings; no angle-pick button here.
    for (const bool start : {true, false}) {
        number({start ? "angle1" : "angle2", start ? tr("Start Parameter") : tr("End Parameter"),
                tr("Raw ellipse parameter in degrees")},
            Input::NOTNEEDED, true,
            [start](const LC_OffsetCurve& curve) {
                return start ? curve.definition().basis.angle1 : curve.definition().basis.angle2;
            }, [start](LC_OffsetCurve& curve, double value, const LC_OffsetRunLimits& limits) {
                auto basis = curve.definition().basis;
                if (start) basis.angle1 = value; else basis.angle2 = value;
                return curve.setBasis(basis, limits);
            });
    }

    auto* state = new LC_PropertyQString(section, false);
    state->setNames({"definitionState", tr("Definition"), tr("Native definition availability")});
    state->setProperty(nativeProperty, true);
    state->useOwnedDelegatedValue()->setupFull([session] {
        int defined = 0, undefined = 0, inconsistent = 0;
        for (auto* target : session->targets) {
            const auto* curve = session->value(target);
            if (curve == nullptr) continue;
            if (!curve->isDefined()) ++undefined;
            else if (!curve->isConsistent()) ++inconsistent;
            else ++defined;
        }
        return nativeTr("%1 defined, %2 undefined, %3 inconsistent").arg(defined).arg(undefined).arg(inconsistent);
    }, [](QString, LC_PropertyChangeReason) {});
    state->setReadOnly();
    section->addChildProperty(state);
    const QPointer<LC_PropertyQString> guarded(state);
    session->refreshers.emplace_back([guarded] {
        if (guarded != nullptr) guarded->postUpdateEvent(PropertyChangeReasonValueLoaded);
    });
    session->refresh();
}

void LC_PropertiesProviderOffsetCurve::fillComputedProperites(
    LC_PropertyContainer* container, const QList<RS_Entity*>&) {
    const auto session = m_session;
    if (!session || session->targets.isEmpty()) return;
    auto* section = createCalculatedInfoSection(container);
    auto* length = new NativeNumber(section);
    length->setNames({"length", tr("Length"), tr("Length of the retained offset cache")});
    length->setActionContextAndLaterRequestor(m_actionContext, nullptr);
    LC_PropertyViewDescriptor descriptor;
    descriptor.viewName = numberView;
    length->setViewDescriptor(descriptor);
    const std::weak_ptr<LC_OffsetCurvePropertySession> weak = session;
    auto read = [weak] {
        std::pair<std::optional<double>, bool> result;
        const auto session = weak.lock();
        if (!session) return result;
        for (auto* target : session->targets) {
            const auto* curve = session->value(target);
            const auto value = curve ? LC_OffsetCurveRead::cachedLength(*curve) : std::nullopt;
            if (!value) return std::pair<std::optional<double>, bool>{std::nullopt, false};
            if (!result.first) result.first = value;
            else if (*result.first != *value) result.second = true;
        }
        return result;
    };
    length->setProperty(nativeProperty, true);
    length->setReadOnly();
    length->useOwnedDelegatedValue()->setupFull([read] { return read().first.value_or(0.); },
        [](double, LC_PropertyChangeReason) {});
    const QPointer<NativeNumber> guarded(length);
    session->refreshers.emplace_back([guarded, read] {
        if (!guarded) return;
        const auto value = read();
        guarded->setProperty(unavailableProperty, !value.first.has_value());
        guarded->switchState(PropertyStateValueMulti, value.second);
        guarded->postUpdateEvent(PropertyChangeReasonValueLoaded);
    });
    section->addChildProperty(length);
    session->refresh();
}

void LC_PropertiesProviderOffsetCurve::createSourceActionLinks(
    LC_PropertyContainer* container, LC_PropertySheetWidget* widget, LC_ActionContext* context,
    const QList<RS_Entity*>& entities, const bool single) {
    if (!container || !widget || !context) return;
    const QPointer<LC_PropertySheetWidget> sheet(widget);
    const QPointer<RS_GraphicView> view(context->getGraphicView());
    const QPointer<LC_OffsetCurvePropertyQueue> queue(widget->offsetPropertyQueue());
    auto* document = context->getDocument();
    if (!document || !view || view->isClosing() || !queue || view->getDocument() != document) return;
    const auto epoch = queue->epoch();
    // Supplied addresses may be stale. Walk live members once instead of doing
    // a linear membership search for every selected (including ordinary) entity.
    const QSet<RS_Entity*> requested(entities.cbegin(), entities.cend());
    QHash<RS_Entity*, unsigned long long> targets;
    for (auto* entity : std::as_const(*document)) {
        if (!entity || !requested.contains(entity) || entity->getParent() != document ||
            entity->isDeleted() || entity->rtti() != RS2::EntityOffsetCurve ||
            !dynamic_cast<LC_OffsetCurve*>(entity)) continue;
        targets.insert(entity, entity->getId());
    }
    if (targets.empty() || (single && targets.size() != 1)) return;
    // The existing link utility defers activation by 10 ms. Capture identities,
    // not a provider or an entity to dereference before checking current ownership.
    LC_PropertyProviderUtils::createSingleEntityCommand<RS_Entity>(
        container, single ? "offsetSourceSingle" : "offsetSourceSelection",
        nativeTr("Update from Source"), nativeTr("Update selected offset curves from a picked ellipse"),
        nativeTr("Restore Original"), nativeTr("Restore embedded source ellipses"),
        nullptr,
        [sheet, view, queue, context, document, epoch, targets, single](RS_Entity*, const int index) {
            if (!sheet || !view || view->isClosing() || !queue || queue->epoch() != epoch ||
                context->getDocument() != document || context->getGraphicView() != view.data() ||
                view->getDocument() != document || (index != 0 && index != 1)) return;
            if (queue->pendingCount() != 0) {
                context->commandMessage(nativeTr("Offset Curve: finish pending property edits before starting a source action."));
                return;
            }
            RS_Entity* contextEntity = nullptr;
            for (auto* entity : std::as_const(*document)) {
                const auto target = targets.constFind(entity);
                if (target == targets.cend()) continue;
                if (entity->getParent() != document || entity->isDeleted() || entity->getId() != target.value() ||
                    entity->rtti() != RS2::EntityOffsetCurve || !dynamic_cast<LC_OffsetCurve*>(entity)) continue;
                if (single || entity->isSelected()) { contextEntity = entity; break; }
            }
            if (!contextEntity) return;
            // The old popup entity is unrelated to these guarded targets and
            // may already be dead. Discard its identity without deselecting it.
            context->saveContextMenuActionContext(nullptr, RS_Vector(false), false);
            if (single) context->saveContextMenuActionContext(contextEntity, {0, 0}, false);
            context->setCurrentAction(index == 0 ? RS2::ActionOffsetCurveUpdateSource :
                RS2::ActionOffsetCurveRestoreOriginal, nullptr);
        }, nativeTr("Offset source operations"));
}

void LC_PropertiesProviderOffsetCurve::fillSingleEntityCommands(
    LC_PropertyContainer* container, const QList<RS_Entity*>& list) {
    auto* section = createSingleEntityActionsSection(container);
    createSourceActionLinks(section, m_widget, m_actionContext, list, true);
}

void LC_PropertiesProviderOffsetCurve::doCreateSelectedSetCommands(
    LC_PropertyContainer* container, const QList<RS_Entity*>& list) {
    LC_EntityTypePropertiesProvider::doCreateSelectedSetCommands(container, list);
    createSourceActionLinks(container, m_widget, m_actionContext, list, false);
}

void LC_PropertiesProviderOffsetCurve::fillSelectedSetCommands(
    LC_PropertyContainer* container, const QList<RS_Entity*>& list) {
    if (m_session && !m_session->targets.isEmpty()) {
        LC_EntityTypePropertiesProvider::fillSelectedSetCommands(container, list);
    }
}
