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

#ifndef LC_EXTENTITYDATA_H
#define LC_EXTENTITYDATA_H

#include "rs_variable.h"

class LC_ExtDataTag {
    enum TYPE {
        VAR,
        LIST,
        REF
    };
public:
    LC_ExtDataTag();
    LC_ExtDataTag(int code, const RS_Vector& value);
    LC_ExtDataTag(int code, int value);
    LC_ExtDataTag(int code, double value);
    LC_ExtDataTag(int code, const QString& value, bool asReference = false);
    explicit LC_ExtDataTag(RS_Variable* var);
    ~LC_ExtDataTag();
    void add(RS_Variable* v);
    void add(LC_ExtDataTag* tag);
    bool isAtomic() const;
    bool isRef() const;
    RS_Variable* var() const;
    std::vector<LC_ExtDataTag*>* list();
private:
    void clear();
    RS_Variable* m_var{nullptr};
    std::vector<LC_ExtDataTag*> m_list;
    TYPE type {VAR};
};

class LC_ExtDataGroup {
public:
    explicit LC_ExtDataGroup(const QString& groupName);
    ~LC_ExtDataGroup() = default;
    void add(int code, int value);
    void add(int code, double value);
    void add(int code, const QString& value);
    void addRef(int code, const QString& value);
    void add(int code, const RS_Vector& value);
    void add(int code, LC_ExtDataTag* tagData);
    QString getName();
    LC_ExtDataTag* getData();
    std::vector<LC_ExtDataTag*>* getTagsList();
private:
    QString m_name;
    LC_ExtDataTag m_tagData;
};

class LC_ExtDataAppData {
public:
    LC_ExtDataAppData(const QString& applicationName);
    ~LC_ExtDataAppData();
    LC_ExtDataGroup* addGroup(const QString& applicationName);
    LC_ExtDataGroup* getGroupByName(const QString& applicationName);
    QString getName();
    std::vector<LC_ExtDataGroup*>* getGroups();
private:
    QString m_applicationName;
    std::vector<LC_ExtDataGroup*> m_groups;
};

class LC_ExtEntityData {
public:
    LC_ExtEntityData();
    ~LC_ExtEntityData();
    LC_ExtDataAppData* addAppData(const QString& appName);
    LC_ExtDataAppData* getAppDataByName(const QString& groupName);
    LC_ExtDataGroup* getGroupByName(const QString& appName, const QString& groupName);
    std::vector<LC_ExtDataAppData*>* getAppData();
private:
    std::vector<LC_ExtDataAppData*> m_appData;
};

#endif // LC_EXTENTITYDATA_H
