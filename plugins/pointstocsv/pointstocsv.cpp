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

#include <cmath>

#include <QTextEdit>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>
#include <QRect>
#include <QFileDialog>
#include <QTextStream>
#include <QMessageBox>
#include <QTextStream>

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
                             [[maybe_unused]] QWidget *parent, [[maybe_unused]] QString cmd)
{
    d = doc;
    //Deselecting all entities to start a fresh selection.
    d->unselectEntities();
    isCollectingElements = true;
    lc_Exptocsvdlg dlg(parent, doc);
    connect(&dlg, &lc_Exptocsvdlg::rejected, this, &ExpTo_Csv::setIsCollectingElementsToFalse);
    while(isCollectingElements){
        dlg.exec();
    }
}

void ExpTo_Csv::setIsCollectingElements(bool newValue){
    isCollectingElements = newValue;
}

void ExpTo_Csv::setIsCollectingElementsToFalse(){    
    isCollectingElements = false;
    d->unselectEntities();
}

/*****************************/
lc_Exptocsvdlg::lc_Exptocsvdlg(QWidget *parent, Document_Interface *doc) :  QDialog(parent), d(doc)
{
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
        
        selectedEntitiesLabel = new QLabel("0 entities selected", this);

        selectedEntitiesLabel->setGeometry(10,40,150,30);
        
        this->resize ( 450, 80 );
        //A signal is a message sent by the object. 
        //A slot is a function that will be called when this signal is triggered.   

        //The connect function specifies which signal is linked to which slot.
        // Connect signals and slots
        connect(comboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [=]() {
            // Handle the combo box selection change here
            setSelectedType(comboBox->currentText());
        });

        connect(selectButton, &QPushButton::clicked, [=]() {
            selectEntities(comboBox, doc);
        });

        connect(exportButton, &QPushButton::clicked, [=](){
            exportToFile();
        });

}

void lc_Exptocsvdlg::exportToFile()
{
    //edit.setText(text);
    QString fileName = QFileDialog::getSaveFileName(this, tr("Export to file"), "", tr("CSV (*.csv)"));
    if(fileName.isEmpty()){
        //Open the dialog again
        return;
    } else {
        //Open the file in write mode
        QFile file(fileName);
        if(!file.open(QIODevice::WriteOnly)){
            QMessageBox::information(this, tr("Unable to open file"),
                                     file.errorString());
            return;
        }
        QTextStream out(&file);
        for (int i = 0; i < selectedObj.size(); ++i) {
            out << getFormatedText(selectedObj.at(i));
        }
        file.close();
        this->close();
    }
}

QString lc_Exptocsvdlg::getFormatedText(Plug_Entity* entity){
    QString response = "##########\n";
    QHash<int, QVariant> data;
    entity->getData(&data);
    int et = data.value(DPI::ETYPE).toInt();
    
    if(et==DPI::ETYPE::POINT){
        response = "";
    
        response.append(getPointFormatedText(data));
    } else if(et==DPI::ETYPE::LINE){
        response.append(getLineFormatedText(data));
    } else if(et==DPI::ETYPE::POLYLINE){
        response.append(getPolylineFormatedText(entity));
    } else {
        //Unhandled case
        response = "INVALID";
    }

    return response;
}

QString lc_Exptocsvdlg::getPointFormatedText(QHash<int, QVariant> data){
    QString response = "";
    response.append(d->realToStr(data.value(DPI::STARTX).toDouble())).
            append(";").append(d->realToStr(data.value(DPI::STARTY).toDouble())).append("\n");
    return response;
}

QString lc_Exptocsvdlg::getLineFormatedText(QHash<int, QVariant> data){
    QString response = "";
    QPointF ptA, ptB;
    ptA.setX( data.value(DPI::STARTX).toDouble());
    ptA.setY( data.value(DPI::STARTY).toDouble());
    ptB.setX( data.value(DPI::ENDX).toDouble());
    ptB.setY( data.value(DPI::ENDY).toDouble());
    response.append(d->realToStr(ptA.x())).append(";").
            append(d->realToStr(ptA.y())).append("\n");
    response.append(d->realToStr(ptB.x())).append(";")
            .append(d->realToStr(ptB.y())).append("\n");

    return response;
}

QString lc_Exptocsvdlg::getPolylineFormatedText(Plug_Entity* entity){
    QString response = "";
    QList<Plug_VertexData> vl;
    entity->getPolylineData(&vl);
    int iVertices = vl.size();
    for (int i = 0; i < iVertices; ++i) {
        response.append( d->realToStr(vl.at(i).point.x())).append(";").append(d->realToStr(vl.at(i).point.y())).append("\n");
    }
    return response;
}

void lc_Exptocsvdlg::setSelectedType(QString typeAsString){
    //If the selected Type is -1 then do nothing
    if(selectedType==DPI::POINT || selectedType==DPI::LINE || selectedType==DPI::POLYLINE){
        //TODO
        //If there are selected entities compare the selected vs the new
        if(selectedType != DPI::UNKNOWN){
            //If they are different then unselect all the selected entities.
            //Reset the selected count to 0
            if (selectedType == DPI::POINT && typeAsString!=strPoint){
                d->unselectEntities();
                clearSelectedObj();
            } else if (selectedType == DPI::LINE && typeAsString!=strLine) {
                d->unselectEntities();
                clearSelectedObj();
            } else if (selectedType == DPI::POLYLINE && typeAsString!=strPolyline){
                d->unselectEntities();
                clearSelectedObj();
            } //Otherwise the selected type should be equal to the typeAsString
        }
    }

    if(typeAsString==strPoint){
         selectedType=DPI::POINT;
    } else if (typeAsString==strLine){
        selectedType=DPI::LINE;
    } else if (typeAsString==strPolyline){
        selectedType=DPI::POLYLINE;
    } else {
        //unhandled case
    }   
}

void lc_Exptocsvdlg::selectEntities(QComboBox *comboBox, Document_Interface *doc){
    if(selectedType==DPI::UNKNOWN){
        setSelectedType(comboBox->currentText());
    }

    //Hide the dialog
    this->hide();
    //Call the method to select entities, and pass the selectedType
    QList<Plug_Entity *> obj;
    bool yes  = doc->getSelectByType(&obj, selectedType);
    //Once the selection process has ended, count the entities that are selected
    //Set the selectedCount value
    if (!yes || obj.isEmpty()){
        clearSelectedObj();
        //Call unselect entities
        doc->unselectEntities();

    } else {
        setSelectedObj(&obj);
    }
    //Show the dialog again.
    this->show();
}

void lc_Exptocsvdlg::setSelectedObj(QList<Plug_Entity *> *obj){
    selectedObj.clear();
    for (int i = 0; i < obj->size(); ++i) {
        selectedObj.append(obj->at(i));
    }

    setSelectedLabelCounterText(selectedObj.size());
    
}
void lc_Exptocsvdlg::setSelectedLabelCounterText(int count){
    if(count==1){
        selectedEntitiesLabel->setText(" 1 element selected ");
    } else if( count==0 || count >1 ){
        QString text = " %1 elements selected ";
        selectedEntitiesLabel->setText(text.arg(selectedObj.size()) );
    } else {

        selectedEntitiesLabel->setText("Invalid selection" );
    }
}
void lc_Exptocsvdlg::clearSelectedObj(){
    selectedObj.clear();
    setSelectedLabelCounterText(selectedObj.size());
}


lc_Exptocsvdlg::~lc_Exptocsvdlg()
{

}
