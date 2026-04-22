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

#include "lc_property_view_multiple.h"

#include "lc_guardedconnectionslist.h"
#include "lc_property_container.h"
#include "lc_property_edit_context.h"
#include "lc_property_event_context.h"
#include "lc_property_utils.h"
#include "lc_property_view_factory.h"
#include "lc_property_view_part.h"

const QByteArray LC_PropertyViewMultiple::VIEW_NAME = QByteArrayLiteral("MultiProperty");

struct LC_PropertyViewMultiple::PropertyToEdit {
    LC_PropertyMulti* owner;
    LC_PropertyAtomic* property;
    LC_GuardedConnectionsList connections;
};

LC_PropertyViewMultiple::LC_PropertyViewMultiple(LC_PropertyMulti& owner)
    : LC_PropertyViewTypedCompound(owner) {
    owner.updateMultipleState(true);
}

void LC_PropertyViewMultiple::init() {
    Q_ASSERT(m_propertiesViewsList.empty());

    auto& props = typedProperty().getProperties();
    m_propertiesViewsList.reserve(props.size());
    for (const auto prop : props) {
        auto view = getFactory()->createView(*prop);
        view->setStateProperty(&typedProperty());
        m_propertiesViewsList.emplace_back(view);
    }
}

LC_PropertyViewMultiple::~LC_PropertyViewMultiple() {
    m_subProperties.clear();
}

bool LC_PropertyViewMultiple::isSplittable() const {
    if (m_propertiesViewsList.size() > 0) {
        const auto lcPropertyView = m_propertiesViewsList.front().get();
        return lcPropertyView->isSplittable();
    }
    return true;
}

void LC_PropertyViewMultiple::onEditedPropertyDestroyed(PropertyToEdit* data) {
    Q_ASSERT(data != nullptr);
    data->owner = nullptr;
    data->property = nullptr;
    data->connections.clear();
}

void LC_PropertyViewMultiple::onEditorDestroyed(const PropertyToEdit* data) {
    const auto multiProperty = data->owner;
    if (multiProperty != nullptr) {
        multiProperty->markEdited(false);
    }
    delete data;
}

void LC_PropertyViewMultiple::doApplyAttributes(const LC_PropertyViewDescriptor& descriptor) {
    for (const auto& view : m_propertiesViewsList) {
        LC_PropertyViewDescriptor mergedAttrs;
        mergedAttrs.attributes = descriptor.attributes;
        const auto desc = view->getProperty()->getViewDescriptor();
        if (desc != nullptr) {
            mergedAttrs.viewName = desc->viewName;
            auto& attributes = desc->attributes;
            for (auto it = attributes.cbegin(); it != attributes.cend(); ++it) {
                auto rootIt = mergedAttrs.attributes.find(it.key());
                if (rootIt != mergedAttrs.attributes.end()) {
                    continue;
                }
                mergedAttrs.attributes[it.key()] = it.value();
            }
        }
        view->applyAttributes(mergedAttrs);

        for (int i = 0, count = view->getSubPropertyCount(); i < count; ++i) {
            auto property = view->getSubProperty(i);
            auto it = std::find_if(m_subProperties.begin(), m_subProperties.end(),
                                   [property](const std::unique_ptr<LC_Property>& a) -> bool {
                                       return property->propertyMetaObject() == a->propertyMetaObject() && property->getDisplayName() == a->
                                           getDisplayName();
                                   });

            const auto subContainer = property->asContainer();
            if (subContainer != nullptr) {
                LC_PropertyContainer* multiContainer = nullptr;

                if (it == m_subProperties.end()) {
                    multiContainer = new LC_PropertyContainer();
                    multiContainer->setName(subContainer->getName());
                    multiContainer->setDisplayName(subContainer->getDisplayName());
                    multiContainer->setDescription(subContainer->getDescription());
                    multiContainer->setState(subContainer->stateLocal());
                    addSubProperty(multiContainer);
                }
                else {
                    multiContainer = it->get()->asContainer();
                }

                LC_PropertyContainerUtils::gatherPropertiesToMultiSet(multiContainer, subContainer, false);
            }
            else {
                LC_PropertyMulti* multiProperty = nullptr;
                if (it == m_subProperties.end()) {
                    multiProperty = new LC_PropertyMulti(property->metaObject());
                    multiProperty->setName(property->getName());
                    multiProperty->setDisplayName(property->getDisplayName());
                    multiProperty->setDescription(property->getDescription());
                    addSubProperty(multiProperty);
                }
                else {
                    Q_ASSERT(qobject_cast<LC_PropertyMulti *>(it->get()));
                    multiProperty = static_cast<LC_PropertyMulti*>(it->get());
                }
                multiProperty->addProperty(property->asAtomic(), false);
            }
        }
    }
}

void LC_PropertyViewMultiple::doBuildViewParts(LC_PropertyPaintContext& ctx, QList<LC_PropertyViewPart>& parts) {
    Q_ASSERT(!m_propertiesViewsList.empty());
    m_propertiesViewsList.at(0)->buildViewParts(ctx, parts);
    for (auto& part : parts) {
        if (nullptr == part.funHandleEvent) {
            continue;
        }

        auto oldEventHandler = part.funHandleEvent;
        part.funHandleEvent = [oldEventHandler, this](LC_PropertyEventContext& eventCtx, const LC_PropertyViewPart& p,
                                                      LC_PropertyEditContext* editCtx) -> bool //
        {
            // if (oldEventHandler != nullptr && editCtx->isValid()) {
            if (!oldEventHandler(eventCtx, p, editCtx)) {
                return false;
            }
            // }

            if (!editCtx->isValid() || editCtx->getProperty() == getProperty() || !getProperty()->isEditableByUser()) {
                return true;
            }

            LC_PropertyEditContext oldToEdit(*editCtx);
            editCtx->setup(getProperty(), [this, oldToEdit]() -> QWidget* //
            {
                const auto thiss = this;
                auto& prop = thiss->typedProperty();
                const auto propertyToEdit = prop.getFirstProperty();
                auto data = new PropertyToEdit;
                data->owner = &prop;
                data->property = propertyToEdit;
                prop.markEdited(true);

                data->connections.emplace_back(connect(propertyToEdit, &QObject::destroyed,
                                                       std::bind(&LC_PropertyViewMultiple::onEditedPropertyDestroyed, data)));
                const auto editor = oldToEdit.createEditor();
                if (editor != nullptr) {
                    data->connections.emplace_back(connect(editor, &QObject::destroyed,
                                                           std::bind(&LC_PropertyViewMultiple::onEditorDestroyed, data)));
                }
                else {
                    onEditorDestroyed(data);
                }
                return editor;
            });
            return true;
        };
    }
}
