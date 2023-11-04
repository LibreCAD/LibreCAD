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


    private: 
        QString getStrData(Plug_Entity *ent);
        Document_Interface *d;
        int getEntityType(Plug_Entity *ent);

    private: 
        QTextEdit edit;
};
class lc_Exptocsvdlg : public QDialog
{
    Q_OBJECT

    public:    
        explicit lc_Exptocsvdlg(QWidget *parent = nullptr);
        ~lc_Exptocsvdlg() override;
        void setText(QString text);

    private:
        QTextEdit edit;
};

#endif //LIST_H