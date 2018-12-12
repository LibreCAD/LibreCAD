#include <QMessageBox>
#include <QDialog>
#include "document_interface.h"
#include "lc_myplugin.h"


QString LC_myPlugin::name() const
{
    return (tr("My first plugin"));
}


PluginCapabilities LC_myPlugin::getCapabilities() const
{
    PluginCapabilities pluginCapabilities;
    pluginCapabilities.menuEntryPoints << PluginMenuLocation("plugins_menu", tr("My first plugin"));
    return pluginCapabilities;
}

void LC_myPlugin::execComm(Document_Interface *doc, QWidget *parent,QString cmd)
{
    //Q_UNUSED(doc);
    Q_UNUSED(cmd);
    //QMessageBox::information ( parent, "LibreCAD rules", "This is my first plugin");

    MyPluginWnd wnd(parent);

    //QObject::connect(&wnd,SIGNAL(getData(std::vector<QPointF>)),this,SLOT(setData(std::vector<QPointF>)));
    int res = wnd.exec();

    if(res == QDialog::Accepted)
    {
        vector<QPointF> oriPoints = wnd.getData();
        //oriPoints.emplace_back(oriPoints.front());

        vector<QPointF> offPoints = wnd.getOffsetData();
        //offPoints.emplace_back(offPoints.front());

        if(!oriPoints.empty()) doc->addLines(oriPoints,true);
        if(!offPoints.empty()) doc->addLines(offPoints,true);
    }
    else
    {
        return;
    }

    //wnd.accept();
}

void LC_myPlugin::setData(std::vector<QPointF> data)
{
    points = data;
}

//Q_EXPORT_PLUGIN2(myplugin, LC_myPlugin);


