#ifndef LC_OFFSET_H
#define LC_OFFSET_H

#include <QObject>
#include <vector>
#include <QMessageBox>
#include "qc_plugininterface.h"
#include "document_interface.h"
#include "clipper.hpp"
#include "lc_offsetdlg.h"

using namespace std;

class LC_Offset : public QObject,QC_PluginInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID LC_DocumentInterface_iid FILE  "offset.json")
    Q_INTERFACES(QC_PluginInterface)


public:
    virtual QString name() const Q_DECL_OVERRIDE;
    virtual PluginCapabilities getCapabilities() const Q_DECL_OVERRIDE;
    virtual void execComm(Document_Interface *doc, QWidget *parent, QString cmd) Q_DECL_OVERRIDE;
    vector<QPointF> getOffset(const vector<QPointF>& vec,double delta);

private:
    //vector<QPointF> vec1;

};


#endif // LC_OFFSET_H
