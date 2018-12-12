#ifndef LC_MYPLUGIN_H
#define LC_MYPLUGIN_H

#include "qc_plugininterface.h"
#include "mypluginwnd.h"
//#include "clipper.hpp"

class LC_myPlugin : public QObject, QC_PluginInterface {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID LC_DocumentInterface_iid FILE  "myplugin.json")
    Q_INTERFACES(QC_PluginInterface)


public:
    virtual QString name() const Q_DECL_OVERRIDE;
    virtual PluginCapabilities getCapabilities() const Q_DECL_OVERRIDE;
    virtual void execComm(Document_Interface *doc, QWidget *parent, QString cmd) Q_DECL_OVERRIDE;

private:
    std::vector<QPointF> points;

private slots:
    void setData(std::vector<QPointF> );
};

#endif // LC_MYPLUGIN_H
