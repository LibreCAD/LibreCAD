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

#ifndef SHORTCUT_H
#define SHORTCUT_H
#include <QObject>
#include <QWidget>
#include <QPushButton>

class LC_ShortcutInfo
{
public:
    LC_ShortcutInfo(const QString& actionName,
                    const QKeySequence& default_key)
        :name(actionName),
         defaultKey(default_key),
         key(default_key){
    }

    static const char* PROPERTY_ACTION_SHORTCUT_CONFIGURABLE;

    QString getName() const {return name;};
    bool hasNoKey() const {return key.isEmpty();};
    bool hasKey() const {return !key.isEmpty();};
    QString retrieveKey(bool useDefault)const;
    QString getKeyAsString();
    QKeySequence getKey() const {return key;};
    QList<QKeySequence> getKeysList() const;
    void resetToDefault();
    void setKey(QKeySequence newKey);
    bool isModified(){return modified;}
    void clear();
    bool hasCollision(){return collision;};
    void setCollision(bool val){collision = val;};
    static int translateModifiers(Qt::KeyboardModifiers state,const QString &text);
    bool hasTheSameKey(QKeySequence sequenceToTest);
protected:
    QString name;
    QKeySequence defaultKey;
    QKeySequence key;
    bool modified = false;
    bool collision = false;
};

#endif // SHORTCUT_H
