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
#include <QComboBox>
#include <QLabel>
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
    lc_Exptocsvdlg dlg(parent, doc);
    
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
lc_Exptocsvdlg::lc_Exptocsvdlg(QWidget *parent, Document_Interface *doc) :  QDialog(parent)
{
        std::cout << "############# lc_Exptocsvdlg::lc_Exptocsvdlg\n";
        ExpTo_Csv expToCsvInstance; 
        setWindowTitle(expToCsvInstance.name());
        
        QLabel *label = new QLabel("Entity type:", this);
        label->setGeometry(10,5,100,30);
        
        QComboBox *comboBox = new QComboBox(this);
        comboBox->setParent(this);
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
        //A signal is a message sent by the object. 
        //A slot is a function that will be called when this signal is triggered.   

        //The connect function specifies which signal is linked to which slot.
        // Connect signals and slots
    connect(comboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int index) {
        // Handle the combo box selection change here
        qDebug() << "Selected Index:" << index;
        qDebug() << "Selected Value:" << comboBox->currentText();
        qDebug() << "Associated Data:" << comboBox->currentData();
        setSelectedType(comboBox->currentText());
    });

    connect(selectButton, &QPushButton::clicked, [=]() {
        selectEntities(comboBox, doc);
    });
    
}

void lc_Exptocsvdlg::setText(QString text)
{
    std::cout << "############# lc_Exptocsvdlg::setText\n";
    //edit.setText(text);
}

void lc_Exptocsvdlg::setSelectedType(QString typeAsString){
    std::cout << "Setting selected type based on: " << typeAsString.toStdString() << " \n";
    //If the selected Type is -1 then do nothing
    if(selectedType!=-1){
        //If there are selected entities compare the selected vs the new
            //If they are different then unselect all the selected entities.
            //Reset the selected count to 0
    }
    //Else, nothing to do. 
        
}

void lc_Exptocsvdlg::selectEntities(QComboBox *comboBox, Document_Interface *doc){
    if(selectedType==-1){
        setSelectedType(comboBox->currentText());
    }

    //Hide the dialog
    this->hide();
    //Call the method to select entities, and pass the selectedType
    std::cout << "############# C\n";
    QList<Plug_Entity *> obj;
    std::cout << "############# D\n";
    if(doc == nullptr){
        std::cout << "############# doc is null";    
    } else {
        std::cout << "############# doc is NOT null";    
    }
    bool yes  = doc->getSelect(&obj);
    std::cout << "############# E\n";
    //Once the selection process has ended, count the entities that are selected
    //Set the selectedCount value
    if (!yes || obj.isEmpty()){

    } else {

    }
    //Show the dialog again.
    this->show();
}


lc_Exptocsvdlg::~lc_Exptocsvdlg()
{

}
