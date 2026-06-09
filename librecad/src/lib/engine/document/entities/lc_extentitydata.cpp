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

#include "lc_extentitydata.h"

#include <qalgorithms.h>

LC_ExtDataTag::LC_ExtDataTag() = default;

LC_ExtDataTag::LC_ExtDataTag(const int code, const RS_Vector& value) {
    const auto v = new RS_Variable(value, code);
    add(v);
}

LC_ExtDataTag::LC_ExtDataTag(const int code, const int value) {
    const auto v = new RS_Variable(value, code);
    add(v);
}

LC_ExtDataTag::LC_ExtDataTag(const int code, const double value) {
    const auto v = new RS_Variable(value, code);
    add(v);
}

LC_ExtDataTag::LC_ExtDataTag(const int code, const QString& value, const bool asReference) {
    const auto v = new RS_Variable(value, code);
    add(v);
    if (asReference) {
        m_type = REF;
    }
}

LC_ExtDataTag::LC_ExtDataTag(int code, const QString& value, bool asReference,
                             bool asLayerRef) {
    auto* v = new RS_Variable(value, code);
    add(v);
    if (asLayerRef) {
        m_type = LAYERREF;
    }
    else if (asReference) {
        m_type = REF;
    }
}

LC_ExtDataTag::LC_ExtDataTag(int code, const QByteArray& bytes)
    : m_bytes(bytes) {
    // Keep a code-only RS_Variable so consumers that read var()->getCode()
    // see the right group code; the value field is unused for binary tags.
    auto* v = new RS_Variable(QString{}, code);
    add(v);
    m_type = BIN;
}

LC_ExtDataTag::LC_ExtDataTag(RS_Variable* var) {
    add(var);
}

LC_ExtDataTag::~LC_ExtDataTag() {
    clear();
}

void LC_ExtDataTag::clear() const {
    if (m_type == VAR) {
        delete m_var;
    }
    else {
        for (auto* tag : m_list) {
            delete tag;
        }
        qDeleteAll(m_list);
    }
}

void LC_ExtDataTag::add(RS_Variable* v) {
    m_var = v;
    m_type = VAR;
}

void LC_ExtDataTag::add(LC_ExtDataTag* tag) {
    m_list.push_back(tag);
    m_type = LIST;
}

bool LC_ExtDataTag::isAtomic() const {
    return m_type != LIST;
}

bool LC_ExtDataTag::isRef() const {
    return m_type == REF;
}

bool LC_ExtDataTag::isLayerRef() const { return m_type == LAYERREF; }

bool LC_ExtDataTag::isBinary() const { return m_type == BIN; }

RS_Variable* LC_ExtDataTag::var() const {
    return m_var;
}

std::vector<LC_ExtDataTag*>* LC_ExtDataTag::list() {
    return &m_list;
}

LC_ExtDataGroup::LC_ExtDataGroup(const QString& groupName) : m_name{groupName} {
}

void LC_ExtDataGroup::add(const int code, const int value) {
    const auto tagData = new LC_ExtDataTag(code, value);
    m_tagData.add(tagData);
}

void LC_ExtDataGroup::add(const int code, const double value) {
    const auto tagData = new LC_ExtDataTag(code, value);
    m_tagData.add(tagData);
}

void LC_ExtDataGroup::add(const int code, const QString& value) {
    const auto tagData = new LC_ExtDataTag(code, value);
    m_tagData.add(tagData);
}

void LC_ExtDataGroup::addRef(const int code, const QString& value) {
    const auto tagData = new LC_ExtDataTag(code, value, true);
    m_tagData.add(tagData);
}

void LC_ExtDataGroup::addLayerRef(int code, const QString& layerName) {
    auto tagData = new LC_ExtDataTag(code, layerName, /*asReference=*/false,
                                     /*asLayerRef=*/true);
    m_tagData.add(tagData);
}

void LC_ExtDataGroup::add(int code, const QByteArray& bytes) {
    auto tagData = new LC_ExtDataTag(code, bytes);
    m_tagData.add(tagData);
}

void LC_ExtDataGroup::add(const int code, const RS_Vector& value) {
    const auto tagData = new LC_ExtDataTag(code, value);
    m_tagData.add(tagData);
}

void LC_ExtDataGroup::add([[maybe_unused]] int code, LC_ExtDataTag* tagData) {
    m_tagData.add(tagData);
}

QString LC_ExtDataGroup::getName() {
    return m_name;
}

LC_ExtDataTag* LC_ExtDataGroup::getData() {
    return &m_tagData;
}

std::vector<LC_ExtDataTag*>* LC_ExtDataGroup::getTagsList() {
    return m_tagData.list();
}

LC_ExtDataAppData::LC_ExtDataAppData(const QString& applicationName) : m_applicationName{applicationName} {
}

LC_ExtDataAppData::~LC_ExtDataAppData() {
    for (auto* group : m_groups) {
        delete group;
    }
}

LC_ExtDataGroup* LC_ExtDataAppData::addGroup(const QString& groupName) {
    auto* group = new LC_ExtDataGroup(groupName);
    m_groups.push_back(group);
    return group;
}

LC_ExtDataGroup* LC_ExtDataAppData::getGroupByName(const QString& groupName) const {
    if (groupName.isEmpty()) {
        return nullptr;
    }
    const size_t count = m_groups.size();
    for (size_t i = 0; i < count; i++) {
        LC_ExtDataGroup* group = m_groups[i];
        if (group->getName() == groupName) {
            return group;
        }
    }
    return nullptr;
}

std::vector<LC_ExtDataGroup*>* LC_ExtDataAppData::getGroups() {
    return &m_groups;
}

QString LC_ExtDataAppData::getName() {
    return m_applicationName;
}

LC_ExtEntityData::LC_ExtEntityData() = default;

LC_ExtEntityData::~LC_ExtEntityData() {
    for (auto* app : m_appData) {
        delete app;
    }
}

LC_ExtDataAppData* LC_ExtEntityData::addAppData(const QString& appName) {
    auto* appData = getAppDataByName(appName);
    if (appData == nullptr) {
        appData = new LC_ExtDataAppData(appName);
        m_appData.push_back(appData);
    }
    return appData;
}

LC_ExtDataAppData* LC_ExtEntityData::getAppDataByName(const QString& groupName) const {
    if (groupName.isEmpty()) {
        return nullptr;
    }
    const size_t count = m_appData.size();
    for (size_t i = 0; i < count; i++) {
        LC_ExtDataAppData* group = m_appData[i];
        if (group->getName() == groupName) {
            return group;
        }
    }
    return nullptr;
}

LC_ExtDataGroup* LC_ExtEntityData::getGroupByName(const QString& appName, const QString& groupName) const {
    const auto appData = getAppDataByName(appName);
    if (appData != nullptr) {
        return appData->getGroupByName(groupName);
    }
    return nullptr;
}

std::vector<LC_ExtDataAppData*>* LC_ExtEntityData::getAppData() {
    return &m_appData;
}

std::unique_ptr<LC_ExtEntityData> LC_ExtEntityData::clone() const {
    auto out = std::make_unique<LC_ExtEntityData>();
    for (auto* app : m_appData) {
        if (app == nullptr)
            continue;
        auto* dstApp = out->addAppData(app->getName());
        for (auto* group : *app->getGroups()) {
            if (group == nullptr)
                continue;
            auto* dstGroup = dstApp->addGroup(group->getName());
            for (auto* tag : *group->getTagsList()) {
                if (tag == nullptr || !tag->isAtomic())
                    continue;
                auto* var = tag->var();
                if (var == nullptr)
                    continue;
                int code = var->getCode();
                if (tag->isBinary()) {
                    dstGroup->add(code, tag->bytes());
                    continue;
                }
                if (tag->isLayerRef()) {
                    dstGroup->addLayerRef(code, var->getString());
                    continue;
                }
                if (tag->isRef()) {
                    dstGroup->addRef(code, var->getString());
                    continue;
                }
                switch (var->getType()) {
                    case RS2::VariableInt:
                        dstGroup->add(code, var->getInt());
                        break;
                    case RS2::VariableDouble:
                        dstGroup->add(code, var->getDouble());
                        break;
                    case RS2::VariableString:
                        dstGroup->add(code, var->getString());
                        break;
                    case RS2::VariableVector:
                        dstGroup->add(code, var->getVector());
                        break;
                    case RS2::VariableVoid:
                        break;
                }
            }
        }
    }
    return out;
}
