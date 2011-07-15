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

class pointData;

class ImportShp : public QObject, QC_PluginInterface
{
    Q_OBJECT
     Q_INTERFACES(QC_PluginInterface)

 public:
     virtual PluginCapabilities getCapabilities() const;
     virtual QString name() const;
     virtual void execComm(Document_Interface *doc,
                                        QWidget *parent, QString cmd);
};

/*namespace DPT {
    enum txtposition {N, S, E, O, NE, SE, SO, NO};
}*/

class dibSHP : public QDialog
{
    Q_OBJECT

public:
    explicit dibSHP(QWidget *parent = 0);
    ~dibSHP();
//    void SetupUI(QWidget *parent);
    void procesFile(Document_Interface *doc);

public slots:
    void getFile();
    void checkAccept();
    void updateFile();

private:
    void readSettings();
    void writeSettings();

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
//    QList<pointData*> dataList;

    Document_Interface *currDoc;

};

/***********/
class pointData
{
public:
    QString number;
    QString x;
    QString y;
    QString z;
    QString code;
};
/***********/
#endif // IMPORTSHP_H
