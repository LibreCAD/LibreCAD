/*****************************************************************************/
/*  CircleTools - plugin for LibreCAD                                        */
/*                                                                           */
/*  Copyright (C) 2026 Ivo Dörr                                              */
/*  Contact: ivo.dorr@iqkonstrukt.cz                                         */
/*                                                                           */
/*  This program is free software; you can redistribute it and/or modify     */
/*  it under the terms of the GNU General Public License as published by     */
/*  the Free Software Foundation; either version 2 of the License, or        */
/*  (at your option) any later version.                                      */
/*                                                                           */
/*  This program is distributed in the hope that it will be useful,          */
/*  but WITHOUT ANY WARRANTY; without even the implied warranty of           */
/*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           */
/*  GNU General Public License for more details.                             */
/*                                                                           */
/*  You should have received a copy of the GNU General Public License        */
/*  along with this program.  If not, see <http://www.gnu.org/licenses/>.    */
/*****************************************************************************/

#pragma once

#include <QObject>
#include <QString>

#include "qc_plugininterface.h"
#include "document_interface.h"

class CircleToolsPlugin final : public QObject, public QC_PluginInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID LC_DocumentInterface_iid FILE "circletools.json")
    Q_INTERFACES(QC_PluginInterface)

public:
    QString name() const override;
    PluginCapabilities getCapabilities() const override;
    void execComm(Document_Interface* doc, QWidget* parent, QString cmd) override;

private:
    struct RefCircle {
        double diameter = 0.0;
        QString layer;
    };

    static bool isCircle(const QHash<int, QVariant>& data);
    static double circleDiameter(const QHash<int, QVariant>& data);
    static void ensureLayerExists(Document_Interface* doc, const QString& layerName);

    bool promptReferenceCircle(Document_Interface* doc, QWidget* parent, RefCircle& outRef);
    bool promptLayerFilter(Document_Interface* doc, QWidget* parent, int& outMode, QString& outLayerName);
    bool promptTolerance(Document_Interface* doc, QWidget* parent, double& outTol);

    void opFindMoveToLayer(Document_Interface* doc, QWidget* parent);
    void opResizeSelected(Document_Interface* doc, QWidget* parent);
    void opFindResize(Document_Interface* doc, QWidget* parent);
};
