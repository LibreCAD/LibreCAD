/*****************************************************************************/
/*  PICfile.h - pic file importer                                        */
/*                                                                           */
/*  Copyright (C) 2011 Rallaz, rallazz@gmail.com                             */
/*  Copyright (C) 2014 cgrzemba, cgrzemba@opencsw.org                         */
/*                                                                           */
/*  This library is free software, licensed under the terms of the GNU       */
/*  General Public License as published by the Free Software Foundation,     */
/*  either version 2 of the License, or (at your option) any later version.  */
/*  You should have received a copy of the GNU General Public License        */
/*  along with this program.  If not, see <http://www.gnu.org/licenses/>.    */
/*****************************************************************************/

#ifndef PICFILE_H
#define PICFILE_H

#include <QWidget>
#include <QFile>
#include <QLabel>
#include <QGroupBox>
#include <QCheckBox>
#include <QLineEdit>
#include <QComboBox>
#include <QDialog>
#include "qc_plugininterface.h"
#include "document_interface.h"

class pointBox;
class textBox;
class pointData;
class QVBoxLayout;

class PicFile : public QObject, QC_PluginInterface
{
    Q_OBJECT
    Q_INTERFACES(QC_PluginInterface)
    Q_PLUGIN_METADATA(IID LC_DocumentInterface_iid FILE  "picfile.json")

    public:
    virtual PluginCapabilities getCapabilities() const Q_DECL_OVERRIDE;
    virtual QString name() const Q_DECL_OVERRIDE;
    virtual void execComm(Document_Interface *doc,
                          QWidget *parent, QString cmd) Q_DECL_OVERRIDE;
};



class picPunto : public QDialog
{
    Q_OBJECT

    public:
        explicit picPunto(QWidget *parent = 0);
        ~picPunto();
        void SetupUI(QWidget *parent);
    
    public slots:
        void dptFile();
        void processFile(Document_Interface *doc);
        void checkAccept();
    
    private:
        void readSettings();
        void writeSettings();
        void processFilePic(QFile* file);
        void drawLine();
        void drawCircle(QString x, QString y, QString rad);
        void drawText(QString x, QString y, QString txt, QString align);
        void drawBox(QString posx, QString posy, QString width, QString height);
        bool failGUI(QString *msg);
        double getPValue(QString posxy);
    
    private:
        QString errmsg;
        QLineEdit *fileedit;
        QLineEdit *scaleedit;
        QList<pointData*> dataList;
    
        Document_Interface *currDoc;
        int cnt;
        double scale;
};

class pointData
{
public:
    QString x;
    QString y;
};

#endif // PICFILE_H
