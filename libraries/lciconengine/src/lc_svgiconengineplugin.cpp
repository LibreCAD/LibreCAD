/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2025 LibreCAD.org
 Copyright (C) 2025 sand1024

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
#include "lc_svgiconengine.h"

#include <QIconEnginePlugin>

class LC_SVGIconPlugin : public QIconEnginePlugin{
   Q_OBJECT
   Q_PLUGIN_METADATA(IID QIconEngineFactoryInterface_iid FILE "lc_svgicons.json")

public:
    QIconEngine *create(const QString &filename = QString()) override;
};

QIconEngine *LC_SVGIconPlugin::create(const QString &file){
    auto engine = new LC_SVGIconEngine;
    if (!file.isNull()) {
        engine->addFile(file, QSize(), QIcon::Normal, QIcon::Off);
    }
    return engine;
}

#include "lc_svgiconengineplugin.moc"
