/*****************************************************************************/
/*  importshp.cpp - shape file importer                                      */
/*                                                                           */
/*  Copyright (C) 2011 Rallaz, rallazz@gmail.com                             */
/*                                                                           */
/*  This library is free software, licensed under the terms of the GNU       */
/*  General Public License as published by the Free Software Foundation,     */
/*  either version 2 of the License, or (at your option) any later version.  */
/*  You should have received a copy of the GNU General Public License        */
/*  along with this program.  If not, see <http://www.gnu.org/licenses/>.    */
/*****************************************************************************/

#include <QtPlugin>
#include <QRadioButton>
#include <QVBoxLayout>
#include <QPushButton>
#include <QFileDialog>
#include <QSettings>
#include <QMessageBox>

#include "shapefil.h"
#include "importshp.h"

PluginCapabilities ImportShp::getCapabilities() const
{
    PluginCapabilities pluginCapabilities;
    pluginCapabilities.menuEntryPoints
            << PluginMenuLocation("File/Import", tr("shape file"));
    return pluginCapabilities;
}

QString ImportShp::name() const
 {
     return (tr("shape file"));
 }

void ImportShp::execComm(Document_Interface *doc,
                             QWidget *parent, QString /*cmd*/)
{
/*    int num_ent, st;
    double min_bound[4], max_bound[4];
    SHPObject *sobject;
    QPointF pt;*/

/*    QString fileName = QFileDialog::getOpenFileName(parent, tr("Select SHP file"));
    QFileInfo fi = QFileInfo(fileName);
    QString fn = fi.canonicalFilePath ();
    if (fn.isEmpty()) return;*/
/*    SHPHandle sh = SHPOpen( fn.toLocal8Bit(), "rb" );
    SHPGetInfo( sh, &num_ent, &st, min_bound, max_bound );
    for( int i = 0; i < num_ent; i++ ) {
        sobject = SHPReadObject( sh, i );
        switch (sobject->nSHPType) {
            case SHPT_NULL:
                break;
            case SHPT_POINT:
            case SHPT_POINTZ: //3d point
                pt.setX( *(sobject->padfX));
                pt.setY(*(sobject->padfY));
                doc->addPoint(&pt);
                break;
            case SHPT_ARC:
            case SHPT_POLYGON:
            case SHPT_MULTIPOINT:
            case SHPT_ARCZ:
            case SHPT_POLYGONZ:
            case SHPT_MULTIPOINTZ:
            case SHPT_POINTM:
            case SHPT_ARCM:
            case SHPT_POLYGONM:
            case SHPT_MULTIPOINTM:
            case SHPT_MULTIPATCH:
            default:
                break;
        }
    }

    SHPClose( sh );*/

    dibSHP pdt(parent);
    int result = pdt.exec();
    if (result == QDialog::Accepted)
        pdt.procesFile(doc);
}


/*****************************/
dibSHP::dibSHP(QWidget *parent) :  QDialog(parent)
{
/*    int num_ent, st, num_field;
    double min_bound[4], max_bound[4];
    QStringList txtformats;
    char pep[12];
    SHPObject *sobject;
    QPointF pt;*/

/*    for( int i = 0; i < num_ent; i++ ) {
        sobject = SHPReadObject( sh, i );
        switch (sobject->nSHPType) {
            case SHPT_NULL:
                break;
            case SHPT_POINT:
            case SHPT_POINTZ: //3d point
                pt.setX( *(sobject->padfX));
                pt.setY(*(sobject->padfY));
                doc->addPoint(&pt);
                break;
            case SHPT_ARC:
            case SHPT_POLYGON:
            case SHPT_MULTIPOINT:
            case SHPT_ARCZ:
            case SHPT_POLYGONZ:
            case SHPT_MULTIPOINTZ:
            case SHPT_POINTM:
            case SHPT_ARCM:
            case SHPT_POLYGONM:
            case SHPT_MULTIPOINTM:
            case SHPT_MULTIPATCH:
            default:
                break;
        }
    }
    SHPClose( sh );*/

    QVBoxLayout *mainLayout = new QVBoxLayout;

    QPushButton *filebut = new QPushButton(tr("File..."));
    fileedit = new QLineEdit();
    QHBoxLayout *lofile = new QHBoxLayout;
    lofile->addWidget(filebut);
    lofile->addWidget(fileedit);
    lofile->setSizeConstraint(QLayout::SetFixedSize);//ni caso
    mainLayout->addLayout(lofile);

    QLabel *formatlabel = new QLabel(tr("File type:"));
    formattype = new QLabel(tr("Unknoun"));
    QHBoxLayout *loformat = new QHBoxLayout;
    loformat->addWidget(formatlabel);
    loformat->addWidget(formattype);
    loformat->addStretch();
    mainLayout->addLayout(loformat);

    QGroupBox *laybox = new QGroupBox(tr("Layer"));
    QRadioButton *radiolay1 = new QRadioButton(tr("Current"));
    QRadioButton *radiolay2 = new QRadioButton(tr("From data:"));
    layerdata = new QComboBox();
    radiolay1->setChecked(true);
    QHBoxLayout *laylayout = new QHBoxLayout;
    laylayout->addWidget(radiolay1);
    laylayout->addWidget(radiolay2);
    laylayout->addWidget(layerdata);
    laylayout->addStretch(0);
    laybox->setLayout(laylayout);
    mainLayout->addWidget(laybox);

    QGroupBox *colbox = new QGroupBox(tr("Color"));
    QRadioButton *radiocol1 = new QRadioButton(tr("Current"));
    QRadioButton *radiocol2 = new QRadioButton(tr("From data:"));
    colordata = new QComboBox();
    radiocol1->setChecked(true);
    QHBoxLayout *collayout = new QHBoxLayout;
    collayout->addWidget(radiocol1);
    collayout->addWidget(radiocol2);
    collayout->addWidget(colordata);
    collayout->addStretch(1);
    colbox->setLayout(collayout);
    mainLayout->addWidget(colbox);

    QGroupBox *ltypebox = new QGroupBox(tr("Line type"));
    QRadioButton *radioltype1 = new QRadioButton(tr("Current"));
    QRadioButton *radioltype2 = new QRadioButton(tr("From data:"));
    ltypedata = new QComboBox();
    radioltype1->setChecked(true);
    QHBoxLayout *ltypelayout = new QHBoxLayout;
    ltypelayout->addWidget(radioltype1);
    ltypelayout->addWidget(radioltype2);
    ltypelayout->addWidget(ltypedata);
    ltypelayout->addStretch(1);
    ltypebox->setLayout(ltypelayout);
    mainLayout->addWidget(ltypebox);

    QGroupBox *lwidthbox = new QGroupBox(tr("Width"));
    QRadioButton *radiolwidth1 = new QRadioButton(tr("Current"));
    QRadioButton *radiolwidth2 = new QRadioButton(tr("From data:"));
    lwidthdata = new QComboBox();
    radiolwidth1->setChecked(true);
    QHBoxLayout *lwidthlayout = new QHBoxLayout;
    lwidthlayout->addWidget(radiolwidth1);
    lwidthlayout->addWidget(radiolwidth2);
    lwidthlayout->addWidget(lwidthdata);
    lwidthlayout->addStretch(1);
    lwidthbox->setLayout(lwidthlayout);
    mainLayout->addWidget(lwidthbox);

    pointbox = new QGroupBox(tr("Point"));
    QRadioButton *radiopoint1 = new QRadioButton(tr("as Point"));
    QRadioButton *radiopoint2 = new QRadioButton(tr("as Label:"));
    pointdata = new QComboBox();
    radiopoint1->setChecked(true);
    QHBoxLayout *pointlayout = new QHBoxLayout;
    pointlayout->addWidget(radiopoint1);
    pointlayout->addWidget(radiopoint2);
    pointlayout->addWidget(pointdata);
    pointlayout->addStretch(1);
    pointbox->setLayout(pointlayout);
    mainLayout->addWidget(pointbox);

    QHBoxLayout *loaccept = new QHBoxLayout;
    QPushButton *acceptbut = new QPushButton(tr("Accept"));
    QPushButton *cancelbut = new QPushButton(tr("Cancel"));
    loaccept->addStretch();
    loaccept->addWidget(acceptbut);
    loaccept->addWidget(cancelbut);
    loaccept->addStretch();
    mainLayout->addLayout(loaccept);

    setLayout(mainLayout);
    readSettings();
    updateFile();

    connect(cancelbut, SIGNAL(clicked()), this, SLOT(reject()));
    connect(acceptbut, SIGNAL(clicked()), this, SLOT(checkAccept()));
    connect(filebut, SIGNAL(clicked()), this, SLOT(getFile()));
    connect(fileedit, SIGNAL(editingFinished()), this, SLOT(updateFile()));
}

void dibSHP::checkAccept()
{
    writeSettings();
    accept();
}

void dibSHP::getFile()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Select file"),
                                fileedit->text(), "Shapefiles *.shp(*.shp)");
    fileedit->setText(fileName);
    updateFile();
}

void dibSHP::updateFile()
{
    QString fileName = fileedit->text();
    int num_ent, st, num_field;
    double min_bound[4], max_bound[4];
    QStringList txtformats;
    char field_name[12];

    QFileInfo fi = QFileInfo(fileName);
    if (fi.suffix() != "shp") return;
    QString file = fi.canonicalFilePath ();
    if (file.isEmpty()) return;

    SHPHandle sh = SHPOpen( file.toLocal8Bit(), "rb" );
    SHPGetInfo( sh, &num_ent, &st, min_bound, max_bound );
    SHPClose( sh );
    DBFHandle dh = DBFOpen( file.toLocal8Bit(), "rb" );
    num_field = DBFGetFieldCount( dh );

    for( int i = 0; i < num_field; i++ ) {
        DBFGetFieldInfo( dh, i, field_name,NULL, NULL);

        txtformats << field_name;
    }
    DBFClose( dh );

    txtformats.sort();
    layerdata->clear();
    layerdata->addItems(txtformats);
    colordata->clear();
    colordata->addItems(txtformats);
    ltypedata->clear();
    ltypedata->addItems(txtformats);
    lwidthdata->clear();
    lwidthdata->addItems(txtformats);
    pointdata->clear();
    pointdata->addItems(txtformats);

    switch (st) {
    case SHPT_POINT:
    case SHPT_POINTM:
    case SHPT_POINTZ: //3d point
    case SHPT_MULTIPOINT:
    case SHPT_MULTIPOINTM:
    case SHPT_MULTIPOINTZ:
        formattype->setText(tr("Point"));
        pointbox->setDisabled(false);
        break;
    case SHPT_ARC:
    case SHPT_ARCM:
    case SHPT_ARCZ:
        formattype->setText(tr("Arc"));
        pointbox->setDisabled(true);
        break;
    case SHPT_POLYGON:
    case SHPT_POLYGONM:
    case SHPT_POLYGONZ:
        formattype->setText(tr("Poligon"));
        pointbox->setDisabled(true);
        break;
    case SHPT_MULTIPATCH:
    case SHPT_NULL:
    default:
        formattype->setText(tr("Unknoun"));
        pointbox->setDisabled(true);
        break;
    }
}

void dibSHP::procesFile(Document_Interface *doc)
{
    int num_ent, st;
    double min_bound[4], max_bound[4];
    SHPObject *sobject;
    QPointF pt;

    currDoc = doc;

    QFileInfo fi = QFileInfo(fileedit->text());
    if (fi.suffix() != "shp") {
        QMessageBox::critical ( this, "Shapefile", QString(tr("The file %1 not have extension .shp")).arg(fileedit->text()) );
        return;
    }

    if (!fi.exists() ) {
        QMessageBox::critical ( this, "Shapefile", QString(tr("The file %1 not exist")).arg(fileedit->text()) );
        return;
    }

    QString file = fi.canonicalFilePath ();

    SHPHandle sh = SHPOpen( file.toLocal8Bit(), "rb" );
    SHPGetInfo( sh, &num_ent, &st, min_bound, max_bound );
    DBFHandle dh = DBFOpen( file.toLocal8Bit(), "rb" );

    for( int i = 0; i < num_ent; i++ ) {
        sobject = SHPReadObject( sh, i );
        switch (sobject->nSHPType) {
            case SHPT_NULL:
                break;
            case SHPT_POINT:
            case SHPT_POINTZ: //3d point
                pt.setX( *(sobject->padfX));
                pt.setY(*(sobject->padfY));
                currDoc->addPoint(&pt);
                break;
            case SHPT_ARC:
            case SHPT_POLYGON:
            case SHPT_MULTIPOINT:
            case SHPT_ARCZ:
            case SHPT_POLYGONZ:
            case SHPT_MULTIPOINTZ:
            case SHPT_POINTM:
            case SHPT_ARCM:
            case SHPT_POLYGONM:
            case SHPT_MULTIPOINTM:
            case SHPT_MULTIPATCH:
            default:
                break;
        }
    }

    SHPClose( sh );
    DBFClose( dh );

}

dibSHP::~dibSHP()
{
/*    while (!dataList.isEmpty())
         delete dataList.takeFirst();*/
}

void dibSHP::readSettings()
 {
    QString str;
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "LibreCAD", "importshp");
    QPoint pos = settings.value("pos", QPoint(200, 200)).toPoint();
    QSize size = settings.value("size", QSize(325,425)).toSize();
    str = settings.value("lastfile").toString();
    fileedit->setText(str);
    resize(size);
    move(pos);
 }

void dibSHP::writeSettings()
 {
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "LibreCAD", "importshp");
    settings.setValue("pos", pos());
    settings.setValue("size", size());
    settings.setValue("lastfile", fileedit->text());
 }

Q_EXPORT_PLUGIN2(importshp, ImportShp);
