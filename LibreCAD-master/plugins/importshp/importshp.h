/*****************************************************************************/
/*  importshp.h - shape file importer                                        */
/*                                                                           */
/*  Copyright (C) 2011 Rallaz, rallazz@gmail.com                             */
/*                                                                           */
/*  This library is free software, licensed under the terms of the GNU       */
/*  General Public License as published by the Free Software Foundation,     */
/*  either version 2 of the License, or (at your option) any later version.  */
/*  You should have received a copy of the GNU General Public License        */
/*  along with this program.  If not, see <http://www.gnu.org/licenses/>.    */
/*****************************************************************************/

#ifndef IMPORTSHP_H
#define IMPORTSHP_H

#include <QLabel>
#include <QGroupBox>
#include <QLineEdit>
#include <QComboBox>
#include <QDialog>
#include <QRadioButton>
#include "qc_plugininterface.h"
#include "document_interface.h"
#include "shapefil.h"

class ImportShp : public QObject, QC_PluginInterface
{
    Q_OBJECT
    Q_INTERFACES(QC_PluginInterface)
    Q_PLUGIN_METADATA(IID LC_DocumentInterface_iid FILE  "importshp.json")

 public:
    virtual PluginCapabilities getCapabilities() const Q_DECL_OVERRIDE;
    virtual QString name() const Q_DECL_OVERRIDE;
    virtual void execComm(Document_Interface *doc,
                          QWidget *parent, QString cmd) Q_DECL_OVERRIDE;
};

/*namespace DPT {
    enum txtposition {N, S, E, O, NE, SE, SO, NO};
}*/

/***********/

class AttribData
{
public:
    AttribData(){
        layer = "0";
        color = -1; //-1 == BYLAYER
        lineType = "BYLAYER";
        width = "BYLAYER";
    }
    QString layer;
    QString lineType;
    QString width;
    int color;
//    QColor color;
};

/***********/

class dibSHP : public QDialog
{
    Q_OBJECT

public:
    explicit dibSHP(QWidget *parent = 0);
    ~dibSHP();
    void procesFile(Document_Interface *doc);

public slots:
    void getFile();
    void checkAccept();
    void updateFile();

private:
    void readSettings();
    void writeSettings();

    void readPoint(DBFHandle dh, int i);
    void readPolyline(DBFHandle dh, int i);
    void readPolylineC(DBFHandle dh, int i);
    void readMultiPolyline(DBFHandle dh, int i);
//    void readText(SHPHandle sh, DBFHandle dh, int i, Plug_Entity *ent);
    void readAttributes(DBFHandle dh, int i);

private:
    QLineEdit *fileedit;
    QComboBox *layerdata;
    QComboBox *colordata;
    QComboBox *ltypedata;
    QComboBox *lwidthdata;
    QComboBox *pointdata;
    QGroupBox *pointbox;
    QRadioButton *radiolay1;
    QRadioButton *radiocol1;
    QRadioButton *radioltype1;
    QRadioButton *radiolwidth1;
    QRadioButton *radiopoint1;
    QLabel *formattype;

    int layerF, colorF, ltypeF, lwidthF, pointF;
    int layerT, colorT, ltypeT, lwidthT, pointT;
    AttribData attdata;
    SHPObject *sobject;
    QString currlayer;

    Document_Interface *currDoc;

};

#endif // IMPORTSHP_H
