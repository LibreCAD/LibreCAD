/*
 * This file is part of the LibreCAD project, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include <QMetaObject>
#include <QObject>
#include <QWidget>

#include <utility>

#include "document_interface.h"
#include "qc_plugininterface.h"

class OffsetCurveImportPlugin final : public QObject, public QC_PluginInterface {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID LC_DocumentInterface_iid)
    Q_INTERFACES(QC_PluginInterface)

public:
    QString name() const override { return QStringLiteral("OffsetCurveExternalImportTest"); }
    PluginCapabilities getCapabilities() const override { return {}; }

    void execComm(Document_Interface* document, QWidget*, QString fileName) override {
        setProperty("finished", false);
        setProperty("blockName", QString{});
        setProperty("error", QString{});
        if (document == nullptr) {
            setProperty("error", QStringLiteral("Document interface is null"));
            setProperty("finished", true);
            return;
        }

        document->addBlockFromDiskAsync(std::move(fileName), [this](QString blockName, QString error) {
            QMetaObject::invokeMethod(this, [this, blockName = std::move(blockName), error = std::move(error)] {
                setProperty("blockName", blockName);
                setProperty("error", error);
                setProperty("finished", true);
            }, Qt::QueuedConnection);
        });
    }
};

#include "offsetcurve_import_plugin.moc"
