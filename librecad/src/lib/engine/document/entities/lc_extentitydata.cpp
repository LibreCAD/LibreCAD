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

LC_ExtDataTag::LC_ExtDataTag() {
}

LC_ExtDataTag::LC_ExtDataTag(int code, const RS_Vector &value) {
    RS_Variable* v = new RS_Variable(value, code);
    add(v);
}

LC_ExtDataTag::LC_ExtDataTag(int code, int value) {
    RS_Variable* v = new RS_Variable(value, code);
    add(v);
}

LC_ExtDataTag::LC_ExtDataTag(int code, double value) {
    RS_Variable* v = new RS_Variable(value, code);
    add(v);
}

LC_ExtDataTag::LC_ExtDataTag(int code, const QString& value, bool asReference) {
    RS_Variable* v = new RS_Variable(value, code);
    add(v);
    if (asReference){
      type = REF;
    }
}

LC_ExtDataTag::LC_ExtDataTag(RS_Variable* var) {
    add(var);
}

LC_ExtDataTag::~LC_ExtDataTag() {
    clear();
}

void LC_ExtDataTag::clear() {
    if (type == VAR) {
        delete m_var;
    }
    else {
        m_list.clear();
    }
}

void LC_ExtDataTag::add(RS_Variable* v) {
    m_var = v;
    type = VAR;
}

void LC_ExtDataTag::add(LC_ExtDataTag* tag) {
    m_list.push_back(tag);
    type = LIST;
}

bool LC_ExtDataTag::isAtomic() const {
    return type != LIST;
}

bool LC_ExtDataTag::isRef() const {
    return type == REF;
}

RS_Variable* LC_ExtDataTag::var() const {
    return m_var;
}

std::vector<LC_ExtDataTag*>* LC_ExtDataTag::list() {
    return &m_list;
}

LC_ExtDataGroup::LC_ExtDataGroup(const QString& groupName):m_name{groupName} {
}

void LC_ExtDataGroup::add(int code, int value) {
    auto tagData = new LC_ExtDataTag(code, value);
    m_tagData.add(tagData);
}

void LC_ExtDataGroup::add(int code, double value) {
    auto tagData = new LC_ExtDataTag(code, value);
    m_tagData.add(tagData);
}

void LC_ExtDataGroup::add(int code, const QString& value) {
    auto tagData = new LC_ExtDataTag(code, value);
    m_tagData.add(tagData);
}

void LC_ExtDataGroup::addRef(int code, const QString& value) {
    auto tagData = new LC_ExtDataTag(code, value, true);
    m_tagData.add(tagData);
}

void LC_ExtDataGroup::add(int code, const RS_Vector& value) {
    auto tagData = new LC_ExtDataTag(code, value);
    m_tagData.add(tagData);
}

void LC_ExtDataGroup::add([[maybe_unused]]int code, LC_ExtDataTag* tagData) {
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

LC_ExtDataAppData::LC_ExtDataAppData(const QString& appName):m_applicationName{appName} {
}

LC_ExtDataAppData::~LC_ExtDataAppData() {
    m_groups.clear();
}

LC_ExtDataGroup* LC_ExtDataAppData::addGroup(const QString& groupName) {
    auto* group = new LC_ExtDataGroup(groupName);
    m_groups.push_back(group);
    return group;
}

LC_ExtDataGroup* LC_ExtDataAppData::getGroupByName(const QString& groupName) {
    if (groupName.isEmpty()) {
        return nullptr;
    }
    size_t count = m_groups.size();
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
    m_appData.clear();
}

LC_ExtDataAppData* LC_ExtEntityData::addAppData(const QString& appName) {
    auto* appData = getAppDataByName(appName);
    if (appData == nullptr) {
        appData = new LC_ExtDataAppData(appName);
        m_appData.push_back(appData);
    }
    return appData;

}

LC_ExtDataAppData* LC_ExtEntityData::getAppDataByName(const QString& groupName) {
    if (groupName.isEmpty()) {
        return nullptr;
    }
    size_t count = m_appData.size();
    for (size_t i = 0; i < count; i++) {
        LC_ExtDataAppData* group = m_appData[i];
        if (group->getName() == groupName) {
            return group;
        }
    }
    return nullptr;
}

LC_ExtDataGroup* LC_ExtEntityData::getGroupByName(const QString& appName, const QString& groupName) {
    auto appData = getAppDataByName(appName);
    if (appData != nullptr) {
        return appData->getGroupByName(groupName);
    }
    return nullptr;
}

std::vector<LC_ExtDataAppData*>* LC_ExtEntityData::getAppData() {
    return &m_appData;
}
