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
#include <QVBoxLayout>
#include <QPushButton>
#include <QFileDialog>
#include <QSettings>
#include <QMessageBox>

#include "importshp.h"

/*TODO
    complete readAttributes
    void readPolyline(DBFHandle dh, int i);
    void readPolylineC(DBFHandle dh, int i);
    void readMultiPolyline(DBFHandle dh, int i);
    SHPT_MULTIPOINT
*/

PluginCapabilities ImportShp::getCapabilities() const
{
    PluginCapabilities pluginCapabilities;
    pluginCapabilities.menuEntryPoints
            << PluginMenuLocation("plugins_menu", "ESRI Shapefile");
    return pluginCapabilities;
}

QString ImportShp::name() const
{
    return (tr("Import ESRI Shapefile"));
}

void ImportShp::execComm(Document_Interface *doc,
                             QWidget *parent, QString /*cmd*/)
{
    dibSHP pdt(parent);
    int result = pdt.exec();
    if (result == QDialog::Accepted)
        pdt.procesFile(doc);
}


/*****************************/
dibSHP::dibSHP(QWidget *parent) :  QDialog(parent)
{
    setWindowTitle(tr("Import ESRI Shapefile"));
    QVBoxLayout *mainLayout = new QVBoxLayout;

    QPushButton *filebut = new QPushButton(tr("File..."));
    fileedit = new QLineEdit();
    QHBoxLayout *lofile = new QHBoxLayout;
    lofile->addWidget(filebut);
    lofile->addWidget(fileedit);
    lofile->setSizeConstraint(QLayout::SetFixedSize);//ni caso
    mainLayout->addLayout(lofile);

    QLabel *formatlabel = new QLabel(tr("File type:"));
    formattype = new QLabel(tr("Unknown"));
    QHBoxLayout *loformat = new QHBoxLayout;
    loformat->addWidget(formatlabel);
    loformat->addWidget(formattype);
    loformat->addStretch();
    mainLayout->addLayout(loformat);

    QGroupBox *laybox = new QGroupBox(tr("Layer"));
    radiolay1 = new QRadioButton(tr("Current"));
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
    radiocol1 = new QRadioButton(tr("Current"));
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
    radioltype1 = new QRadioButton(tr("Current"));
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
    radiolwidth1 = new QRadioButton(tr("Current"));
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
    radiopoint1 = new QRadioButton(tr("as Point"));
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
                                fileedit->text(), "ESRI Shapefiles (*.shp)");
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
    if (fi.suffix().toLower() != "shp") return;
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
        formattype->setText(tr("Point"));
        pointbox->setDisabled(false);
        break;
    case SHPT_POINTM:
        formattype->setText(tr("Point+Measure"));
        pointbox->setDisabled(false);
        break;
    case SHPT_POINTZ: //3d point
        formattype->setText(tr("3D Point"));
        pointbox->setDisabled(false);
        break;
    case SHPT_MULTIPOINT:
        formattype->setText(tr("Multi Point"));
        pointbox->setDisabled(false);
        break;
    case SHPT_MULTIPOINTM:
        formattype->setText(tr("Multi Point+Measure"));
        pointbox->setDisabled(false);
        break;
    case SHPT_MULTIPOINTZ:
        formattype->setText(tr("3D Multi Point"));
        pointbox->setDisabled(false);
        break;
    case SHPT_ARC:
        formattype->setText(tr("Arc"));
        pointbox->setDisabled(true);
        break;
    case SHPT_ARCM:
        formattype->setText(tr("Arc+Measure"));
        pointbox->setDisabled(true);
        break;
    case SHPT_ARCZ:
        formattype->setText(tr("3D Arc"));
        pointbox->setDisabled(true);
        break;
    case SHPT_POLYGON:
        formattype->setText(tr("Polygon"));
        pointbox->setDisabled(true);
        break;
    case SHPT_POLYGONM:
        formattype->setText(tr("Polygon+Measure"));
        pointbox->setDisabled(true);
        break;
    case SHPT_POLYGONZ:
        formattype->setText(tr("3D Polygon"));
        pointbox->setDisabled(true);
        break;
    case SHPT_MULTIPATCH:
        formattype->setText(tr("Multipatch"));
        pointbox->setDisabled(true);
        break;
    case SHPT_NULL:
    default:
        formattype->setText(tr("Unknown"));
        pointbox->setDisabled(true);
        break;
    }
}

void dibSHP::procesFile(Document_Interface *doc)
{
    int num_ent, st;
    double min_bound[4], max_bound[4];

    currDoc = doc;

    QFileInfo fi = QFileInfo(fileedit->text());
    if (fi.suffix().toLower() != "shp") {
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

    if (radiolay1->isChecked()) {
        layerF = -1;
        attdata.layer = currDoc->getCurrentLayer();
    } else {
        layerF = DBFGetFieldIndex( dh, (layerdata->currentText()).toLatin1().data() );
        layerT = DBFGetFieldInfo( dh, layerF, NULL, NULL, NULL );
    }
    if (radiocol1->isChecked())
        colorF = -1;
    else {
        colorF = DBFGetFieldIndex( dh, (colordata->currentText()).toLatin1().data() );
        colorT = DBFGetFieldInfo( dh, colorF, NULL, NULL, NULL );
    }
    if (radioltype1->isChecked())
        ltypeF = -1;
    else {
        ltypeF = DBFGetFieldIndex( dh, (ltypedata->currentText()).toLatin1().data() );
        ltypeT = DBFGetFieldInfo( dh, ltypeF, NULL, NULL, NULL );
    }
    if (radiolwidth1->isChecked())
        lwidthF = -1;
    else {
        lwidthF = DBFGetFieldIndex( dh, (lwidthdata->currentText()).toLatin1().data() );
        lwidthT = DBFGetFieldInfo( dh, lwidthF, NULL, NULL, NULL );
    }
    if (radiopoint1->isChecked())
        pointF = -1;
    else {
        pointF = DBFGetFieldIndex( dh, (pointdata->currentText()).toLatin1().data() );
        pointT = DBFGetFieldInfo( dh, pointF, NULL, NULL, NULL );
    }

    currlayer =currDoc->getCurrentLayer();
    for( int i = 0; i < num_ent; i++ ) {
        sobject= NULL;
        sobject = SHPReadObject( sh, i );
        if (sobject) {
            switch (sobject->nSHPType) {
            case SHPT_NULL:
                break;
            case SHPT_POINT:
            case SHPT_POINTM: //2d point with measure
            case SHPT_POINTZ: //3d point
                readPoint(dh, i);
                break;
            case SHPT_MULTIPOINT:
            case SHPT_MULTIPOINTM:
            case SHPT_MULTIPOINTZ:
                break;
            case SHPT_ARC:
            case SHPT_ARCM:
            case SHPT_ARCZ:
            case SHPT_POLYGON:
                readPolyline(dh, i);
                break;
            case SHPT_POLYGONM:
            case SHPT_POLYGONZ:
                readPolylineC(dh, i);
                // fall-through
            case SHPT_MULTIPATCH:
                readMultiPolyline(dh, i);
            default:
                break;
            }
            SHPDestroyObject(sobject);
        }
    }

    SHPClose( sh );
    DBFClose( dh );
    currDoc->setLayer(currlayer);
}

void dibSHP::readPoint(DBFHandle dh, int i){
    Plug_Entity *ent =NULL;
    QHash<int, QVariant> data;
    if (pointF < 0) {
        ent = currDoc->newEntity(DPI::POINT);
        ent->getData(&data);
    } else {
        ent = currDoc->newEntity(DPI::MTEXT);
        ent->getData(&data);
        data.insert(DPI::TEXTCONTENT, DBFReadStringAttribute( dh, i, pointF ) );
    }
    data.insert(DPI::STARTX, *(sobject->padfX));
    data.insert(DPI::STARTY, *(sobject->padfY));
    readAttributes(dh, i);
    data.insert(DPI::LAYER, attdata.layer);
    ent->updateData(&data);
    currDoc->addEntity(ent);
}

void dibSHP::readPolyline(DBFHandle dh, int i){
    int maxPoints;
    Plug_Entity *ent =NULL;
    QHash<int, QVariant> data;
    QList<Plug_VertexData> vl;

    readAttributes(dh, i);
    data.insert(DPI::LAYER, attdata.layer);
    for( int i = 0; i < sobject->nParts; i++ ) {
        if ( (i+1) < sobject->nParts) maxPoints = sobject->panPartStart[i+1];
        else maxPoints = sobject->nVertices;
        vl.clear();
        for( int j = sobject->panPartStart[i]; j < maxPoints; j++ ) {
            vl.append( Plug_VertexData( QPointF(sobject->padfX[j],sobject->padfY[j]), 0.0) );
        }
        if (vl.size() > 2 ) {
            ent = currDoc->newEntity(DPI::POLYLINE);
            ent->updateData(&data);
            currDoc->addEntity(ent);
            ent->updatePolylineData(&vl);
        }
    }
}

void dibSHP::readPolylineC(DBFHandle /*dh*/, int /*i*/){
}

void dibSHP::readMultiPolyline(DBFHandle /*dh*/, int /*i*/){
}

void dibSHP::readAttributes(DBFHandle dh, int i){
    if (layerF >= 0) {
        attdata.layer = DBFReadStringAttribute( dh, i, layerF );
        currDoc->setLayer(attdata.layer);
    }
/*    if (colorF >= 0)
        readColor(dh, i);
    if (ltypeF >= 0)
        readLType(dh, i);
    if (lwidthF >= 0)
        readWidth(dh, i);*/
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
