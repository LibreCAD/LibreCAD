#ifndef PLOT_H
#define PLOT_H

#include <QObject>
#include "qc_plugininterface.h"

class plot : public QObject, QC_PluginInterface
{
    Q_OBJECT
     Q_INTERFACES(QC_PluginInterface)
#if QT_VERSION >= 0x050000
     Q_PLUGIN_METADATA(IID "org.librecad.plotequation" FILE  "plotequation.json")
#endif

public:
    explicit plot(QObject *parent = 0);

    virtual QString name() const;
    virtual PluginCapabilities getCapabilities() const;
    virtual void execComm(Document_Interface *doc, QWidget *parent, QString cmd);

signals:
    
public slots:
    
};

#endif // PLOT_H
