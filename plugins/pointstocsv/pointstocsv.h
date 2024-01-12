/*****************************************************************************/
/*  pointstocsv.h - List selected entities                                          */
/*                                                                           */
/*  Copyright (C) 2023 Joaquin, joaquinperezvalera@gmail.com                             */
/*                                                                           */
/*  This library is free software, licensed under the terms of the GNU       */
/*  General Public License as published by the Free Software Foundation,     */
/*  either version 2 of the License, or (at your option) any later version.  */
/*  You should have received a copy of the GNU General Public License        */
/*  along with this program.  If not, see <http://www.gnu.org/licenses/>.    */
/*****************************************************************************/

#ifndef EXPTOCSV_H
#define EXPTOCSV_H

#include <QDialog>
#include "qc_plugininterface.h"
#include "document_interface.h"
#include <QTextEdit>
#include <QComboBox>



class Plug_Entity;

class ExpTo_Csv : public QObject, QC_PluginInterface
{
    Q_OBJECT
    Q_INTERFACES(QC_PluginInterface)
    Q_PLUGIN_METADATA(IID LC_DocumentInterface_iid "pointstocsv.json")

    public:
        virtual PluginCapabilities getCapabilities() const Q_DECL_OVERRIDE;
        virtual QString name() const Q_DECL_OVERRIDE;
        virtual void execComm(Document_Interface *doc,
                              QWidget *parent, QString cmd) Q_DECL_OVERRIDE;

        virtual void setIsCollectingElements(bool newValue);
        virtual void setIsCollectingElementsToFalse();

    private: 
        QString getStrData(Plug_Entity *ent);
        Document_Interface *d;
        int getEntityType(Plug_Entity *ent);
        
    private: 
        QTextEdit edit;
        bool isCollectingElements = false;
        
};
class lc_Exptocsvdlg : public QDialog
{
    Q_OBJECT

    public:    
        explicit lc_Exptocsvdlg(QWidget *parent = nullptr, Document_Interface *doc = nullptr);
        ~lc_Exptocsvdlg() override;
        void setText(QString text);
        void setSelectedType(QString typeAsString);
        void selectEntities(QComboBox *comboBox, Document_Interface *doc = nullptr);
        void exportToFile();
    private:
        QList<Plug_Entity *> selectedObj;
        Document_Interface *d;
        QTextEdit edit;
        enum DPI::ETYPE selectedType = DPI::UNKNOWN;
        int selectedCount = 0;
        const QString strPoint= "Point";
        const QString strLine = "Line";
        const QString strPolyline = "Polyline";
        void setSelectedObj(QList<Plug_Entity *> *selectedObj);
        void clearSelectedObj();
        QString getFormatedText(Plug_Entity* entity);
        QString getPointFormatedText(QHash<int, QVariant> data);
        QString getLineFormatedText(QHash<int, QVariant> data);
        QString getPolylineFormatedText(Plug_Entity* entity);
};

#endif //LIST_H
