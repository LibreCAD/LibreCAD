/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2024 LibreCAD.org
 Copyright (C) 2024 sand1024

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 ******************************************************************************/

#ifndef LC_ACTIONGROUP_H
#define LC_ACTIONGROUP_H

#include <QActionGroup>

class LC_ActionGroup:public QActionGroup {
    Q_OBJECT

public:
    LC_ActionGroup(QObject *parent, const QString &name, const QString &description, const char* icon);

    ~LC_ActionGroup() override;

    const QString &getName() const;

    void setName(const QString &name);

    const QString &getDescription() const;

    void setDescription(const QString &description);

    const QIcon &getIcon() const;

    void setIcon(const QIcon &icon);

    bool isActionMappingsMayBeConfigured() const;

    void setActionMappingsMayBeConfigured(bool actionMappingsMayBeConfigured);

protected:
    QString name;
    QString description;
    QIcon icon;
    bool actionMappingsMayBeConfigured = true;
};

#endif // LC_ACTIONGROUP_H
