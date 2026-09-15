#ifndef PLOT_H
#define PLOT_H

#include <QObject>
#include "qc_plugininterface.h"

class plot : public QObject, QC_PluginInterface
{
    Q_OBJECT
    Q_INTERFACES(QC_PluginInterface)
    Q_PLUGIN_METADATA(IID LC_DocumentInterface_iid FILE  "plotequation.json")

public:
    explicit plot(QObject *parent = 0);

    PluginCapabilities getCapabilities() const override;
    QString name() const override;
    void init(Document_Interface *doc, QWidget *parent) override;
    void execComm(QString cmd) override;

signals:
    
public slots:
    
};

#endif
