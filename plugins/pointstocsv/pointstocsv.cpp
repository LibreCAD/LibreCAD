/*****************************************************************************/
/*  pointstocsv.cpp - Exports coordinates of points of lines, polylines to   */
/*  csv file, and as well the coordinates of selected points                 */
/*                                                                           */
/*                                                                           */
/*  Copyright (C) 2023 Joaquin, joaquinperezvalera@gmail.com                 */
/*                                                                           */
/*  This library is free software, licensed under the terms of the GNU       */
/*  General Public License as published by the Free Software Foundation,     */
/*  either version 2 of the License, or (at your option) any later version.  */
/*  You should have received a copy of the GNU General Public License        */
/*  along with this program.  If not, see <http://www.gnu.org/licenses/>.    */
/*****************************************************************************/


#include <QTextEdit>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QRect>
#include <cmath>
#include "iostream"
#include <string> // for string and to_string()
#include "pointstocsv.h"

QString ExpTo_Csv::name() const
{
    return (tr("Export points to csv"));
}

PluginCapabilities ExpTo_Csv::getCapabilities() const
{
    PluginCapabilities pluginCapabilities;
    pluginCapabilities.menuEntryPoints << PluginMenuLocation("plugins_menu",
                                        name());
    return pluginCapabilities;
}

void ExpTo_Csv::execComm(Document_Interface *doc,
                             QWidget *parent, QString cmd)
{
    std::cout << "############# ExpTo_Csv::execComm\n";
    Q_UNUSED(parent);
    std::cout << "############# A\n";
    Q_UNUSED(cmd);
    std::cout << "############# B\n";
    d = doc;
    /*std::cout << "############# C\n";
    QList<Plug_Entity *> obj;
    std::cout << "############# D\n";
    bool yes  = doc->getSelect(&obj);
    std::cout << "############# E\n";
    if (!yes || obj.isEmpty()) return;
    std::cout << "############# F\n";
    QString text;
    std::cout << "############# G\n";
    //RS_Entity is the basic drawing entity
    //Plug_Entity is a wrapper for access entities from plugins
    /*
    for (int i = 0; i < obj.size(); ++i) {
        Plug_Entity *mEnt = obj.at(i);
        int entityType = mEnt->getEntityType();
        //int entityType = mEnt->getEntityType();
        std::cout << "############# entityType " << entityType << " \n";
        text.append( QString("%1 %2: ").arg(tr("n")).arg(i+1));
        text.append( getStrData(obj.at(i)));
        text.append( "\n");
    }
    std::cout << "############# calling lc_Exptocsvdlg\n";*/
    lc_Exptocsvdlg dlg(parent);
    
    //std::cout << std::string("############# Setting text\n") + text.toStdString() + std::string(" \n");
    //dlg.setText(text);
    dlg.exec();
    /*
    while (!obj.isEmpty())
        delete obj.takeFirst();
        */
}

QString ExpTo_Csv::getStrData(Plug_Entity *ent)
{
    std::cout << "############# ExpTo_Csv::getStrData\n";
    QString strData("Hello world!");
    QHash<int, QVariant> data;
    ent->getData(&data);
    //specific entity data
    int et = data.value(DPI::ETYPE).toInt();
    return strData;

}

/*****************************/
    lc_Exptocsvdlg::lc_Exptocsvdlg(QWidget *parent) :  QDialog(parent)
    {
        std::cout << "############# lc_Exptocsvdlg::lc_Exptocsvdlg\n";
        ExpTo_Csv expToCsvInstance; 
        setWindowTitle(expToCsvInstance.name());
        
        QLabel *label = new QLabel("Entity type:", this);
        label->setGeometry(10,5,100,30);
        
        QComboBox *comboBox = new QComboBox(this);
        comboBox->addItem("Point");
        comboBox->addItem("Line");
        comboBox->addItem("PolyLine");
        comboBox->setGeometry(120,5,150,30);
        
        QPushButton *selectButton = new QPushButton("Select objects", this);
        selectButton->setGeometry(300,5, 120, 30);
        

        QPushButton *exportButton = new QPushButton("Export", this);
        exportButton->setGeometry(300,40, 120, 30);
        
        QLabel *selectedEntitiesCount = new QLabel("0 entities selected", this);
        selectedEntitiesCount->setGeometry(10,40,130,30);
        
        this->resize ( 450, 80 );

        
    }

void lc_Exptocsvdlg::setText(QString text)
{
    std::cout << "############# lc_Exptocsvdlg::setText\n";
    //edit.setText(text);
}
lc_Exptocsvdlg::~lc_Exptocsvdlg()
{

}
