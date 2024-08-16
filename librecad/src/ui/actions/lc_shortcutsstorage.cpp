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

#include "lc_shortcutsstorage.h"
#include "rs_debug.h"

#include <QKeySequence>
#include <QFile>
#include <QXmlStreamAttributes>
#include <QMap>
#include <QDateTime>

struct ShortcutsParsingContext {// XML parsing context with strings.
    ShortcutsParsingContext();

    const QString el_ActionMappings;
    const QString el_ActionShortcut;
    const QString attr_name;
    const QString el_Key;
    const QString attr_Value;
    const QString dtdName;
};

ShortcutsParsingContext::ShortcutsParsingContext() :
    el_ActionMappings(QLatin1String("actions-mapping")),
    el_ActionShortcut(QLatin1String("action-shortcut")),
    attr_name(QLatin1String("name")),
    el_Key(QLatin1String("key")),
    attr_Value(QLatin1String("value")),
    dtdName("LibreCADActionsMapping"){
}

int LC_ShortcutsStorage::loadShortcuts(const QString &filename, QMap<QString, QKeySequence> *result) {
   QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return ERROR_FILE;
    }

    ShortcutsParsingContext ctx;
    QXmlStreamReader r(&file);

    QString currentId;

    while (!r.atEnd()) {
        switch (r.readNext()) {
            case QXmlStreamReader::DTD: {
                const QStringView dtdName = r.dtdName();
                if (ctx.dtdName != dtdName){
                    file.close();
                    return ERROR_WRONG_DTD;
                }
                break;
            }
            case QXmlStreamReader::StartElement: {
                const QStringView name = r.name();

                if (name == ctx.el_ActionShortcut) {
                    if (!currentId.isEmpty()) {// shortcut element without key element == empty shortcut
                        result->insert(currentId, QKeySequence());
                    }
                    currentId = r.attributes().value(ctx.attr_name).toString();
                } else if (name == ctx.el_Key) {
                    const QXmlStreamAttributes attributes = r.attributes();
                    if (attributes.hasAttribute(ctx.attr_Value)) {
                        const QString keyString = attributes.value(ctx.attr_Value).toString();
                        result->insert(currentId, QKeySequence(keyString));
                    } else {
                        result->insert(currentId, QKeySequence());
                    }
                    currentId.clear();
                } // if key element
            } // case QXmlStreamReader::StartElement
            default:
                break;
        } // switch
    } // while !atEnd

    file.close();
    if (r.hasError()){
        return ERROR_PARSE;
    }

    return OK;
}

int LC_ShortcutsStorage::saveShortcuts(const QString &filename, const QList<LC_ShortcutInfo *> &items, bool resetToDefault)
{
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly|QIODevice::Text)) {
        return ERROR_FILE;
    }

    const ShortcutsParsingContext ctx;
    QXmlStreamWriter w(&file);
    w.setAutoFormatting(true);
    w.setAutoFormattingIndent(1); // Historical, used to be QDom.
    w.writeStartDocument();
    w.writeDTD(QLatin1String("<!DOCTYPE %1>").arg(ctx.dtdName));
    w.writeComment(QString::fromLatin1(" Written at %1.").
                   arg(QDateTime::currentDateTime().toString(Qt::ISODate)));
    w.writeStartElement(ctx.el_ActionMappings);
    for (const LC_ShortcutInfo *item : items) {
        const QString id = item->getName();
        if (item->hasNoKey()) {
            w.writeEmptyElement(ctx.el_ActionShortcut);
            w.writeAttribute(ctx.attr_name, id);
        } else {
            w.writeStartElement(ctx.el_ActionShortcut);
            w.writeAttribute(ctx.attr_name, id);
            w.writeEmptyElement(ctx.el_Key);
            w.writeAttribute(ctx.attr_Value, item->retrieveKey(resetToDefault));
            w.writeEndElement(); // Shortcut
        }
    }
    w.writeEndElement();
    w.writeEndDocument();

    file.close();

    if (w.hasError()){
        return ERROR_PARSE;
    }

    return OK;
}
