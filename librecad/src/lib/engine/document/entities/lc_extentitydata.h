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

#include <memory>

#include <QByteArray>

#include "rs_variable.h"

class LC_ExtDataTag {
  enum TYPE {
    VAR,
    LIST,
    REF,      // entity-handle reference (DXF code 1005)
    LAYERREF, // layer-table name reference (DXF code 1003 / DWG EED type 3)
    BIN       // binary chunk (DXF code 1004 / DWG EED type 4)
  };

public:
  LC_ExtDataTag();
  LC_ExtDataTag(int code, const RS_Vector &value);
  LC_ExtDataTag(int code, int value);
  LC_ExtDataTag(int code, double value);
  LC_ExtDataTag(int code, const QString &value, bool asReference = false);
  LC_ExtDataTag(int code, const QString &value, bool asReference,
                bool asLayerRef);
  LC_ExtDataTag(int code, const QByteArray &bytes);
  explicit LC_ExtDataTag(RS_Variable *var);
  ~LC_ExtDataTag();
  void add(RS_Variable *v);
  void add(LC_ExtDataTag *tag);
  bool isAtomic() const;
  bool isRef() const;
  bool isLayerRef() const;
  bool isBinary() const;
  RS_Variable *var() const;
  const QByteArray &bytes() const { return m_bytes; }
  std::vector<LC_ExtDataTag *> *list();
private:
    void clear() const;
    RS_Variable* m_var{nullptr};
    std::vector<LC_ExtDataTag*> m_list;
    QByteArray m_bytes;
    TYPE m_type {VAR};
};

class LC_ExtDataGroup {
public:
    explicit LC_ExtDataGroup(const QString& groupName);
    ~LC_ExtDataGroup() = default;
    void add(int code, int value);
    void add(int code, double value);
    void add(int code, const QString& value);
    void addRef(int code, const QString& value);
    void addLayerRef(int code, const QString &layerName);
    void add(int code, const QByteArray &bytes);
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
    explicit LC_ExtDataAppData(const QString& applicationName);
    ~LC_ExtDataAppData();
    LC_ExtDataGroup* addGroup(const QString& groupName);
    LC_ExtDataGroup* getGroupByName(const QString& groupName) const;
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
    LC_ExtDataAppData* getAppDataByName(const QString& groupName) const;
    LC_ExtDataGroup* getGroupByName(const QString& appName, const QString& groupName) const;
    std::vector<LC_ExtDataAppData*>* getAppData();
    /// Deep-copy clone for ownership transfer (e.g. RS_Entity copy ctor).
    std::unique_ptr<LC_ExtEntityData> clone() const;

  private:
    std::vector<LC_ExtDataAppData*> m_appData;
};

#endif
