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
#include <QFileDialog>
#include <QMessageBox>
#include <cmath>
#include "iostream"
#include <string> // for string and to_string()
#include "pointstocsv.h"
#include <QDebug>

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
    //Deselecting all entities to start a fresh selection.
    d->unselectEntities();
    isCollectingElements = true;
    lc_Exptocsvdlg dlg(parent, doc);
    connect(&dlg, &lc_Exptocsvdlg::rejected, this, &ExpTo_Csv::setIsCollectingElementsToFalse);
    while(isCollectingElements){
        qDebug() << "is collectingElements";
        dlg.exec();
    }
    //dlg.setText(text);
    //dlg.exec();
    /*
    while (!obj.isEmpty())
        delete obj.takeFirst();
        */
    //dlg.close();
    qDebug() << "ExpTo_Csv execComm last line";
}

void ExpTo_Csv::setIsCollectingElements(bool newValue){
    qDebug() << "setting isCollectingElements: " << newValue;    
    isCollectingElements = newValue;
    qDebug() << "isCollectingElements: " << isCollectingElements;    
}

void ExpTo_Csv::setIsCollectingElementsToFalse(){
    qDebug() << "setting isCollectingElements as false";    
    isCollectingElements = false;
    //UsetTypeToSelectnselect all the elements 
    d->unselectEntities();
    qDebug() << "isCollectingElements: " << isCollectingElements;    
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
lc_Exptocsvdlg::lc_Exptocsvdlg(QWidget *parent, Document_Interface *doc) :  QDialog(parent), d(doc)
{
        std::cout << "############# lc_Exptocsvdlg::lc_Exptocsvdlg\n";
        d = doc;
        ExpTo_Csv expToCsvInstance; 
        setWindowTitle(expToCsvInstance.name());
        
        QLabel *label = new QLabel("Entity type:", this);
        label->setGeometry(10,5,100,30);
        
        QComboBox *comboBox = new QComboBox(this);
        comboBox->setParent(this);
        comboBox->addItem(strPoint);
        comboBox->addItem(strLine);
        comboBox->addItem(strPolyline);
        comboBox->setGeometry(120,5,150,30);
        
        QPushButton *selectButton = new QPushButton("Select objects", this);
        selectButton->setGeometry(300,5, 120, 30);
        

        QPushButton *exportButton = new QPushButton("Export", this);
        exportButton->setGeometry(300,40, 120, 30);
        
        QLabel *selectedEntitiesLabel = new QLabel("0 entities selected", this);
        selectedEntitiesLabel->setGeometry(10,40,130,30);
        
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

        connect(exportButton, &QPushButton::clicked, [=](){
            qDebug() << "Click on export button:";
            exportToFile();
        });

}

void lc_Exptocsvdlg::exportToFile()
{
    qDebug() << "############# lc_Exptocsvdlg::exportToFile";
    //edit.setText(text);
    QString fileName = QFileDialog::getSaveFileName(this, tr("Export to file"), "", tr("CSV (*.csv)"));
    qDebug() << "Destination file: " << fileName << "\n";
    if(fileName.isEmpty()){
        qDebug() << "file is empty";
        //Open the dialog again
        return;
    } else {
        //Open the file in write mode
        qDebug() << "file is NOT empty";
        QFile file(fileName);
        if(!file.open(QIODevice::WriteOnly)){
            QMessageBox::information(this, tr("Unable to open file"),
                                     file.errorString());
            return;
        }
        QTextStream out(&file);
        qDebug() << "Starting loop";
        qDebug() << "Collection size is: " << selectedObj.size();
        for (int i = 0; i < selectedObj.size(); ++i) {
            out << getFormatedText(selectedObj.at(i));
        }
        file.close();
        this->close();
    }
}

QString lc_Exptocsvdlg::getFormatedText(Plug_Entity* entity){
    qDebug() << "lc_Exptocsvdlg::getFormatedText";
    QString response = "##########\n";
    QHash<int, QVariant> data;
    entity->getData(&data);
    qDebug() << "DPI::ETYPE::POINT: " << DPI::ETYPE::POINT;
    qDebug() << "DPI::ETYPE::LINE: " << DPI::ETYPE::LINE;
    qDebug() << "DPI::ETYPE::POLYLINE: " << DPI::ETYPE::POLYLINE;
    int et = data.value(DPI::ETYPE).toInt();
    qDebug() << "EntityType: " << et;

    if(et==DPI::ETYPE::POINT){
        response = "";
        qDebug() << "Type point";
        response.append(getPointFormatedText(data));
    } else if(et==DPI::ETYPE::LINE){
        qDebug() << "Type line";
        response.append(getLineFormatedText(data));
    } else if(et==DPI::ETYPE::POLYLINE){
        qDebug() << "Type polyline";
        response.append(getPolylineFormatedText(entity));
    } else {
        qDebug() << "Unhandled case";
        response = "INVALID";
    }

    return response;
}

QString lc_Exptocsvdlg::getPointFormatedText(QHash<int, QVariant> data){
    qDebug() << "lc_Exptocsvdlg::getPointFormatedText";
    QString response = "";
    response.append(d->realToStr(data.value(DPI::STARTX).toDouble())).
            append(";").append(d->realToStr(data.value(DPI::STARTY).toDouble())).append("\n");
    return response;
}

QString lc_Exptocsvdlg::getLineFormatedText(QHash<int, QVariant> data){
    qDebug() << "lc_Exptocsvdlg::getLineFormatedText";
    QString response = "";
    QPointF ptA, ptB;
    ptA.setX( data.value(DPI::STARTX).toDouble());
    ptA.setY( data.value(DPI::STARTY).toDouble());
    ptB.setX( data.value(DPI::ENDX).toDouble());
    ptB.setY( data.value(DPI::ENDY).toDouble());
    qDebug() << "Appending: "<< d->realToStr(ptA.x()) << ";" << d->realToStr(ptA.y());
    response.append(d->realToStr(ptA.x())).append(";").
            append(d->realToStr(ptA.y())).append("\n");
    qDebug() << "Appending: "<< d->realToStr(ptB.x()) << ";" << d->realToStr(ptB.y());
    response.append(d->realToStr(ptB.x())).append(";")
            .append(d->realToStr(ptB.y())).append("\n");

    return response;
}

QString lc_Exptocsvdlg::getPolylineFormatedText(Plug_Entity* entity){
    qDebug() << "lc_Exptocsvdlg::getPolylineFormatedText";
    QString response = "";
    QList<Plug_VertexData> vl;
    entity->getPolylineData(&vl);
    int iVertices = vl.size();
    qDebug() << "Vertices: " << vl.size() << "\n";
    for (int i = 0; i < iVertices; ++i) {
        response.append( d->realToStr(vl.at(i).point.x())).append(";").append(d->realToStr(vl.at(i).point.y())).append("\n");
    }
    return response;
}

void lc_Exptocsvdlg::setText(QString text)
{
    std::cout << "############# lc_Exptocsvdlg::setText\n";
    //edit.setText(text);
}

void lc_Exptocsvdlg::setSelectedType(QString typeAsString){
    std::cout << "Setting selected type based on: " << typeAsString.toStdString() << " \n";
    //If the selected Type is -1 then do nothing
    if(selectedType==DPI::POINT || selectedType==DPI::LINE || selectedType==DPI::POLYLINE){
        //TODO
        //If there are selected entities compare the selected vs the new
            //If they are different then unselect all the selected entities.
            //Reset the selected count to 0
    } else {

    }
    if(typeAsString==strPoint){
         std::cout << "Setting selected type as point \n" ;
         selectedType=DPI::POINT;
    } else if (typeAsString==strLine){
        std::cout << "Setting selected type as line \n" ;
        selectedType=DPI::LINE;
    } else if (typeAsString==strPolyline){
        std::cout << "Setting selected type as polyline \n" ;
        selectedType=DPI::POLYLINE;
    } else {
        std::cout << "unhandled case \n" ;
    }
    //Else, nothing to do. 
        
}

void lc_Exptocsvdlg::selectEntities(QComboBox *comboBox, Document_Interface *doc){
    if(selectedType==DPI::UNKNOWN){
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
    std::cout << "############# calling getSelectByType with selectedType: " << selectedType;    
    bool yes  = doc->getSelectByType(&obj, selectedType);
    std::cout << " ############# E\n";
    //Once the selection process has ended, count the entities that are selected
    //Set the selectedCount value
    if (!yes || obj.isEmpty()){
        qDebug() << "list is empty";
        clearSelectedObj();
    } else {
        qDebug() << "list is NOT empty, size: " << obj.size();
        setSelectedObj(&obj);
    }
    //Show the dialog again.
    this->show();
}

void lc_Exptocsvdlg::setSelectedObj(QList<Plug_Entity *> *obj){
    std::cout << "lc_Exptocsvdlg::setSelectedObj \n";
    selectedObj.clear();
    for (int i = 0; i < obj->size(); ++i) {
        selectedObj.append(obj->at(i));
    }
}

void lc_Exptocsvdlg::clearSelectedObj(){
    selectedObj.clear();
}


lc_Exptocsvdlg::~lc_Exptocsvdlg()
{

}
