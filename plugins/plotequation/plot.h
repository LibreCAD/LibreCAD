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

    virtual PluginCapabilities getCapabilities() const Q_DECL_OVERRIDE;
    virtual QString name() const Q_DECL_OVERRIDE;
    virtual void execComm(Document_Interface *doc,
                          QWidget *parent, QString cmd) Q_DECL_OVERRIDE;

signals:
    
public slots:
    
};

#endif // PLOT_H
