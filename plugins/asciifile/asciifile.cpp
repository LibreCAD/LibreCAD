/*****************************************************************************/
/*  Asciifile.cpp - ascii file importer                                          */
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
#include <QPicture>
#include <QPainter>
#include <QMouseEvent>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QCheckBox>
#include <QFormLayout>
#include <QPushButton>
#include <QFileDialog>
#include <QSettings>
#include <cmath>

#include <QMessageBox>

#include "document_interface.h"
#include "asciifile.h"

PluginCapabilities AsciiFile::getCapabilities() const
{
    PluginCapabilities pluginCapabilities;
    pluginCapabilities.menuEntryPoints
            << PluginMenuLocation("plugins_menu", tr("Read ascii points"));
    return pluginCapabilities;
}

QString AsciiFile::name() const
 {
     return (tr("Read ascii points"));
 }

void AsciiFile::execComm(Document_Interface *doc,
                             QWidget *parent, QString cmd)
{
    Q_UNUSED(cmd);
    dibPunto pdt(parent);
    int result = pdt.exec();
    if (result == QDialog::Accepted)
        pdt.procesFile(doc);
}

#define POINT 12

imgLabel::imgLabel(QWidget * parent, Qt::WindowFlags f) :
    QLabel(parent, f)
{
    posimage = new QPicture;
    posimage->setBoundingRect(QRect(0,0,POINT*8,POINT*8));
    currPos = DPT::N;
    drawImage();
    setPicture(*posimage);
}

void imgLabel::drawImage()
{
    int a1, a2, a3, a4;
    int b1, b2, b3, b4;
    QPainter painter;
    painter.begin(posimage);
    painter.fillRect ( 0, 0, POINT*8,POINT*8, Qt::black );
    a1 = POINT*1.75;
    a2 = POINT*3.5;
    a3 = POINT*5.25;
    a4 = POINT*6;
    painter.fillRect ( a1, a1, POINT, POINT, Qt::white );//NO
    painter.fillRect ( a2, POINT, POINT, POINT, Qt::white );//N
    painter.fillRect ( POINT, a2, POINT, POINT, Qt::white );//O
    painter.fillRect ( a3, a1, POINT, POINT, Qt::white );//NE
    painter.fillRect ( a1, a3, POINT, POINT, Qt::white );//SO
    painter.fillRect ( a3, a3, POINT, POINT, Qt::white );//SE
    painter.fillRect ( a4, a2, POINT, POINT, Qt::white );//E
    painter.fillRect ( a2, a4, POINT, POINT, Qt::white );//S
    painter.setPen ( Qt::white );
    b1 = POINT*3.2;
    b2 = POINT*3.6;
    b3 = POINT*4;
    b4 = POINT*4.4;
    painter.drawLine ( b2, b2, b4, b2 );
    painter.drawLine ( b2, b2, b2, b4 );
    painter.drawLine ( b4, b2, b4, b4 );
    painter.drawLine ( b2, b4, b4, b4 );
    b4 = POINT*4.8;
    painter.drawLine ( b1, b3, b4, b3 );
    painter.drawLine ( b3, b1, b3, b4 );

    switch (currPos) {
    case DPT::NO:
        a2 = a1 = POINT*1.75;
        break;
    case DPT::O:
        a1 = POINT;
        a2 = POINT*3.5;
        break;
    case DPT::NE:
        a1 = POINT*5.25;
        a2 = POINT*1.75;
        break;
    case DPT::SO:
        a1 = POINT*1.75;
        a2 = POINT*5.25;
        break;
    case DPT::SE:
        a2 = a1 = POINT*5.25;
        break;
    case DPT::E:
        a1 = POINT*6;
        a2 = POINT*3.5;
        break;
    case DPT::S:
        a1 = POINT*3.5;
        a2 = POINT*6;
        break;
    default: //N
        a1 = POINT*3.5;
        a2 = POINT;
    }
    painter.fillRect ( a1, a2, POINT, POINT, Qt::red );
    painter.end();
    update ();
}

void imgLabel::changePos(int x, int y)
{
    if (x < POINT*3.1) {
        if (y < POINT*3.1) { setPos(DPT::NO); }
        else if (y < POINT*4.9) { setPos(DPT::O); }
        else { setPos(DPT::SO); }

    } else if (x < POINT*4.9) {
        if (y < POINT*4) { setPos(DPT::N); }
        else { setPos(DPT::S); }

    } else {
        if (y < POINT*3.1) { setPos(DPT::NE); }
        else if (y < POINT*4.9) { setPos(DPT::E); }
        else { setPos(DPT::SE); }
    }
}

void imgLabel::setPos(DPT::txtposition pos)
{
    currPos = pos;
    drawImage();
}

void imgLabel::mouseReleaseEvent(QMouseEvent *event)
 {
    if (event->button() == Qt::LeftButton) {
        changePos(event->x(), event->y());
     } else {
         QLabel::mousePressEvent(event);
     }
 }

/*****************************/
pointBox::pointBox(const QString & title, const QString & label, QWidget * parent ) :
    QGroupBox(title, parent)
{
    rb = new QCheckBox(label);
    rb->setTristate (false );
    vbox = new QVBoxLayout;
    vbox->addWidget(rb);
    QLabel *but = new QLabel(tr("Layer:"));
    layedit = new QLineEdit();
    QHBoxLayout *lolayer = new QHBoxLayout;
    lolayer->addWidget(but);
    lolayer->addWidget(layedit);
    vbox->addLayout(lolayer);
    setLayout(vbox);
}
void pointBox::setInLayout(QLayout *lo)
{
    vbox->addLayout(lo);
}
pointBox::~pointBox()
{

}
/*****************************/
textBox::textBox(const QString & title, const QString & label, QWidget * parent) :
    pointBox(title, label, parent)
{
    combostyle = new QComboBox();
    QStringList txtstyles;
     txtstyles << "txt" << "simplex" << "romans";
    combostyle->addItems(txtstyles);
    QDoubleValidator *val = new QDoubleValidator(0);
    val->setBottom ( 0.0 );
    heightedit = new QLineEdit();
    heightedit->setValidator(val);
    sepedit = new QLineEdit();
    sepedit->setValidator(val);

    QFormLayout *flo = new QFormLayout;
    flo->addRow( tr("Style:"), combostyle);
    flo->addRow( tr("Height:"), heightedit);
    flo->addRow( tr("Separation"), sepedit);
//    posimage.fill(Qt::black);
    img = new imgLabel();
    QHBoxLayout *loimage = new QHBoxLayout;
    loimage->addLayout(flo);
    loimage->addWidget(img);

    setInLayout(loimage);
}

textBox::~textBox()
{

}


/*****************************/
dibPunto::dibPunto(QWidget *parent) :  QDialog(parent)
{
//    setParent(parent);
    setWindowTitle(tr("Read ascii points"));
    QStringList txtformats;

    QGridLayout *mainLayout = new QGridLayout;
//readSettings();

    QPushButton *filebut = new QPushButton(tr("File..."));
    fileedit = new QLineEdit();
    QHBoxLayout *lofile = new QHBoxLayout;
    lofile->addWidget(filebut);
    lofile->addWidget(fileedit);
    mainLayout->addLayout(lofile, 0, 0);

    QLabel *formatlabel = new QLabel(tr("Format:"));
    formatedit = new QComboBox();
    txtformats << tr("Space Separator") << tr("Tab Separator") << tr("Comma Separator") << tr("Space in Columns") << tr("*.odb for Psion 2");
    formatedit->addItems(txtformats);
    connectPoints = new QCheckBox(tr("Connect points"));

    QHBoxLayout *loformat = new QHBoxLayout;
    loformat->addWidget(formatlabel);
    loformat->addWidget(formatedit);
    loformat->addWidget(connectPoints);
    mainLayout->addLayout(loformat, 0, 1);

    pt2d = new pointBox(tr("2D Point"),tr("Draw 2D Point"));
    pt3d = new pointBox(tr("3D Point"),tr("Draw 3D Point"));
    ptnumber = new textBox(tr("Point Number"),tr("Draw point number"));
    ptelev = new textBox(tr("Point Elevation"),tr("Draw point elevation"));
    ptcode = new textBox(tr("Point Code"),tr("Draw point code"));
    ptnumber->setPos(DPT::NO);

    QVBoxLayout *lo2d3d = new QVBoxLayout;

    lo2d3d->addWidget(pt2d);
    lo2d3d->addWidget(pt3d);
    mainLayout->addLayout(lo2d3d, 1, 0);

    mainLayout->addWidget(ptnumber, 1, 1);
    mainLayout->addWidget(ptelev, 2, 0);
    mainLayout->addWidget(ptcode, 2, 1);

    QHBoxLayout *loaccept = new QHBoxLayout;
    QPushButton *acceptbut = new QPushButton(tr("Accept"));
    loaccept->addStretch();
    loaccept->addWidget(acceptbut);
    mainLayout->addLayout(loaccept, 3, 0);

    QPushButton *cancelbut = new QPushButton(tr("Cancel"));
    QHBoxLayout *locancel = new QHBoxLayout;
    locancel->addWidget(cancelbut);
    locancel->addStretch();
    mainLayout->addLayout(locancel, 3, 1);

    setLayout(mainLayout);
    readSettings();

    connect(cancelbut, SIGNAL(clicked()), this, SLOT(reject()));
    connect(acceptbut, SIGNAL(clicked()), this, SLOT(checkAccept()));

    connect(filebut, SIGNAL(clicked()), this, SLOT(dptFile()));
}

void dibPunto::checkAccept()
{

    errmsg.clear();
    if (failGUI(&errmsg)) {
        QMessageBox::critical ( this, "Sample plugin", errmsg );
        errmsg.clear();
        return;
    }
    writeSettings();
    accept();
}

void dibPunto::dptFile()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Select file"));
    fileedit->setText(fileName);
}

bool dibPunto::failGUI(QString *msg)
{
    if (pt2d->checkOn() == true) {
        if (pt2d->getLayer().isEmpty()) {msg->insert(0, tr("Point 2D layer is empty")); return true;}
    }
    if (pt3d->checkOn() == true) {
        if (pt3d->getLayer().isEmpty()) {msg->insert(0, tr("Point 3D layer is empty")); return true;}
    }
    if (ptelev->checkOn() == true) {
        if (ptelev->getLayer().isEmpty()) {msg->insert(0, tr("Point elevation layer is empty")); return true;}
        if (ptelev->getHeightStr().isEmpty()) {msg->insert(0, tr("Point elevation height is empty")); return true;}
        if (ptelev->getSeparationStr().isEmpty()) {msg->insert(0, tr("Point elevation separation is empty")); return true;}
    }
    if (ptnumber->checkOn() == true) {
        if (ptnumber->getLayer().isEmpty()) {msg->insert(0, tr("Point number layer is empty")); return true;}
        if (ptnumber->getHeightStr().isEmpty()) {msg->insert(0, tr("Point number height is empty")); return true;}
        if (ptnumber->getSeparationStr().isEmpty()) {msg->insert(0, tr("Point number separation is empty")); return true;}
    }
    if (ptcode->checkOn() == true) {
        if (ptcode->getLayer().isEmpty()) {msg->insert(0, tr("Point code layer is empty")); return true;}
        if (ptcode->getHeightStr().isEmpty()) {msg->insert(0, tr("Point code height is empty")); return true;}
        if (ptcode->getSeparationStr().isEmpty()) {msg->insert(0, tr("Point code separation is empty")); return true;}
    }
    return false;
}

void dibPunto::procesFile(Document_Interface *doc)
{
    QString sep;
    QMessageBox::information(this, "Info", "dibpunto procesFile");
    currDoc = doc;

//Warning, can change adding or reordering "formatedit"
    QString::SplitBehavior skip = QString::KeepEmptyParts;
    switch (formatedit->currentIndex()) {
    case 0:
        sep = " ";
        break;
    case 3:
        sep = " ";
        skip = QString::SkipEmptyParts;
        break;
    case 2:
        sep = ",";
        break;
    default:
        sep = "\t";
    }
    if (!QFile::exists(fileedit->text()) ) {
        QMessageBox::critical ( this, "DibPunto", QString(tr("The file %1 not exist")).arg(fileedit->text()) );
        return;
    }
    QFile infile(fileedit->text());
    if (!infile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::critical ( this, "DibPunto", QString(tr("Can't open the file %1")).arg(fileedit->text()) );
         return;
    }

//Warning, can change adding or reordering "formatedit"
    if (formatedit->currentIndex() == 4)
        procesfileODB(&infile, sep);
    else
        procesfileNormal(&infile, sep, skip);
    infile.close ();
    QString currlay = currDoc->getCurrentLayer();

    if (pt2d->checkOn() == true)
        draw2D();
    if (pt3d->checkOn() == true)
        draw3D();
    if (ptelev->checkOn() == true)
        drawElev();
    if (ptnumber->checkOn() == true)
        drawNumber();
    if (ptcode->checkOn() == true)
        drawCode();

    currDoc->setLayer(currlay);
    /* draw lines in current layer */
    if ( connectPoints->isChecked() )
        drawLine();

    currDoc = NULL;

}

void dibPunto::drawLine()
{
    QPointF prevP, nextP;
    int i;

    for (i = 0; i < dataList.size(); ++i) {
        pointData *pd = dataList.at(i);
        if (!pd->x.isEmpty() && !pd->y.isEmpty()){
            prevP.setX(pd->x.toDouble());
            prevP.setY(pd->y.toDouble());
            break;
        }
    }
    for (; i < dataList.size(); ++i) {
        pointData *pd = dataList.at(i);
        if (!pd->x.isEmpty() && !pd->y.isEmpty()){
            nextP.setX(pd->x.toDouble());
            nextP.setY(pd->y.toDouble());
            currDoc->addLine(&prevP, &nextP);
            prevP = nextP;
        }
    }
}

void dibPunto::draw2D()
{
    QPointF pt;
    currDoc->setLayer(pt2d->getLayer());
    for (int i = 0; i < dataList.size(); ++i) {
        pointData *pd = dataList.at(i);
        if (!pd->x.isEmpty() && !pd->y.isEmpty()){
            pt.setX(pd->x.toDouble());
            pt.setY(pd->y.toDouble());
            currDoc->addPoint(&pt);
        }
    }
}
void dibPunto::draw3D()
{
    QPointF pt;
    currDoc->setLayer(pt3d->getLayer());
    for (int i = 0; i < dataList.size(); ++i) {
        pointData *pd = dataList.at(i);
        if (!pd->x.isEmpty() && !pd->y.isEmpty()){
            pt.setX(pd->x.toDouble());
            pt.setY(pd->y.toDouble());
/*RLZ:3d support            if (pd->z.isEmpty()) pt.setZ(0.0);
            else  pt.setZ(pd->z.toDouble());*/
            currDoc->addPoint(&pt);
        }
    }
}

void dibPunto::calcPos(DPI::VAlign *v, DPI::HAlign *h, double sep,
                 double *x, double *y, DPT::txtposition sit)
{
    double inc, incx, incy;
    DPI::VAlign va;
    DPI::HAlign ha;
    incx = incy = sep;
    inc = sqrt(incx*incx/2);
    switch (sit) {
    case DPT::NO:
        va = DPI::VAlignBottom;
        ha = DPI::HAlignRight;
        incx = -1.0*inc; incy = inc;
        break;
    case DPT::O:
        va = DPI::VAlignMiddle;
        ha = DPI::HAlignRight;
        incx = -1.0*incx; incy = 0.0;
        break;
    case DPT::NE:
        va = DPI::VAlignBottom;
        ha = DPI::HAlignLeft;
        incx = inc; incy = inc;
        break;
    case DPT::SO:
        va = DPI::VAlignTop;
        ha = DPI::HAlignRight;
        incx = -1.0*inc; incy = -1.0*inc;
        break;
    case DPT::SE:
        va = DPI::VAlignTop;
        ha = DPI::HAlignLeft;
        incx = inc; incy = -1.0*inc;
        break;
    case DPT::E:
        va = DPI::VAlignMiddle;
        ha = DPI::HAlignLeft;
        incy = 0.0;
        break;
    case DPT::S:
        va = DPI::VAlignMiddle;
        ha = DPI::HAlignCenter;
        incx = 0.0; incy = -1.0*incy;
        break;
    default: //N
        va = DPI::VAlignBottom;
        ha = DPI::HAlignCenter;
        incx = 0.0;
    }
    *x =incx;
    *y =incy;
    *v =va;
    *h =ha;
}

void dibPunto::drawNumber()
{
    double incx, incy, newx, newy;
    DPI::VAlign va;
    DPI::HAlign ha;
    calcPos(&va, &ha, ptnumber->getSeparation(),
                 &incx, &incy, ptnumber->getPosition());

    currDoc->setLayer(ptnumber->getLayer());
    QString sty = ptnumber->getStyleStr();
    for (int i = 0; i < dataList.size(); ++i) {
        pointData *pd = dataList.at(i);
        if (!pd->x.isEmpty() && !pd->y.isEmpty() && !pd->number.isEmpty()){
            newx = pd->x.toDouble() + incx;
            newy = pd->y.toDouble() + incy;
            QPointF pt(newx,newy);
            currDoc->addText(pd->number, sty, &pt, ptnumber->getHeightStr().toDouble(), 0.0, ha, va);
        }
    }
}

void dibPunto::drawElev()
{

    double incx, incy, newx, newy;
    DPI::VAlign va;
    DPI::HAlign ha;
    calcPos(&va, &ha, ptelev->getSeparation(),
                 &incx, &incy, ptelev->getPosition());

    currDoc->setLayer(ptelev->getLayer());
    QString sty = ptelev->getStyleStr();
    for (int i = 0; i < dataList.size(); ++i) {
        pointData *pd = dataList.at(i);
        if (!pd->x.isEmpty() && !pd->y.isEmpty() && !pd->z.isEmpty()){
            newx = pd->x.toDouble() + incx;
            newy = pd->y.toDouble() + incy;
            QPointF pt(newx,newy);
            currDoc->addText(pd->z, sty, &pt, ptelev->getHeightStr().toDouble(), 0.0, ha, va);
        }
    }
}
void dibPunto::drawCode()
{
    double incx, incy, newx, newy;
    DPI::VAlign va;
    DPI::HAlign ha;
    calcPos(&va, &ha, ptcode->getSeparation(),
                 &incx, &incy, ptcode->getPosition());

    currDoc->setLayer(ptcode->getLayer());
    QString sty = ptcode->getStyleStr();
    for (int i = 0; i < dataList.size(); ++i) {
        pointData *pd = dataList.at(i);
        if (!pd->x.isEmpty() && !pd->y.isEmpty() && !pd->code.isEmpty()){
            newx = pd->x.toDouble() + incx;
            newy = pd->y.toDouble() + incy;
            QPointF pt(newx,newy);
            currDoc->addText(pd->code, sty, &pt, ptcode->getHeightStr().toDouble(), 0.0, ha, va);
        }
    }
}

void dibPunto::procesfileODB(QFile* file, QString sep)
{
    QStringList data;
    pointData *pd;

    while (!file->atEnd()) {
        QString line = file->readLine();
        line.remove ( line.size()-2, 1);
        data = line.split(sep);
        pd = new pointData;
        int i = 0;
        int j = data.size();
        if (i<j && data.at(i).compare("4")==0 ){
            i = i+2;
            if (i<j) pd->x = data.at(i); else pd->x = QString();
            i++;
            if (i<j) pd->y = data.at(i); else pd->y = QString();
            i++;
            if (i<j) pd->z = data.at(i); else pd->z = QString();
            i++;
            if (i<j) pd->number = data.at(i); else pd->number = QString();
            i++;
            if (i<j) pd->code = data.at(i); else pd->code = QString();
        }
        dataList.append(pd);
    }

}
void dibPunto::procesfileNormal(QFile* file, QString sep, QString::SplitBehavior skip)
{
    //    QString outname, sep;
    QStringList data;
    pointData *pd;
    while (!file->atEnd()) {
        QString line = file->readLine();
		if(line.isEmpty()) continue;
        line.remove ( line.size()-1, 1);
        data = line.split(sep, skip);
        pd = new pointData;
        int i = 0;
        switch(data.size()){
        case 0:
        case 1:
            delete pd;
            continue;

            //allow reading in raw 2D ascii data in format:
            // x y
        case 2:
            pd->x = data.at(0);
            pd->y = data.at(1);
            break;
        default:
        case 5:
            pd->code=data.at(4);
            // fall-through
        case 4:
            pd->z = data.at(3);
            // fall-through
        case 3:
            pd->number = data.at(i);
            pd->x = data.at(1);
            pd->y = data.at(2);
            break;
        }
        dataList.append(pd);
    }
}

dibPunto::~dibPunto()
{
    while (!dataList.isEmpty())
         delete dataList.takeFirst();
}

void dibPunto::readSettings()
 {
    QString str;
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "LibreCAD", "asciifile");
    QPoint pos = settings.value("pos", QPoint(200, 200)).toPoint();
    QSize size = settings.value("size", QSize(500,300)).toSize();
    str = settings.value("lastfile").toString();
    fileedit->setText(str);
    formatedit->setCurrentIndex( settings.value("format", 0).toInt() );
    connectPoints->setChecked( settings.value("connectpoints", false).toBool() );
    pt2d->setCheck( settings.value("draw2d", false).toBool() );
    str = settings.value("layer2d").toString();
    pt2d->setLayer(str);
    pt3d->setCheck( settings.value("draw3d", false).toBool() );
    str = settings.value("layer3d").toString();
    pt3d->setLayer(str);
    ptelev->setCheck( settings.value("drawelev", false).toBool() );
    str = settings.value("layerelev").toString();
    ptelev->setLayer(str);
    ptnumber->setCheck( settings.value("drawnumber", false).toBool() );
    str = settings.value("layernumber").toString();
    ptnumber->setLayer(str);
    ptcode->setCheck( settings.value("drawcode", false).toBool() );
    str = settings.value("layercode").toString();
    ptcode->setLayer(str);
    ptelev->setStyleIdx( settings.value("styleelev", 0).toInt() );
    ptnumber->setStyleIdx( settings.value("stylenumber", 0).toInt() );
    ptcode->setStyleIdx(settings.value("stylecode", 0).toInt() );
    ptelev->setHeight( settings.value("heightelev", 0.5).toDouble() );
    ptnumber->setHeight( settings.value("heightnumber", 0.5).toDouble() );
    ptcode->setHeight( settings.value("heightcode", 0.5).toDouble() );
    ptelev->setSeparation( settings.value("separationelev", 0.3).toDouble() );
    ptnumber->setSeparation( settings.value("separationnumber", 0.3).toDouble() );
    ptcode->setSeparation( settings.value("separationcode", 0.3).toDouble() );
    ptelev->setPosition( static_cast<DPT::txtposition>( settings.value("positionelev", DPT::S).toInt() ) );
    ptnumber->setPosition( static_cast<DPT::txtposition>( settings.value("positionnumber", DPT::N).toInt() ) );
    ptcode->setPosition( static_cast<DPT::txtposition>( settings.value("positioncode", DPT::E).toInt() ) );
    resize(size);
    move(pos);
 }

void dibPunto::writeSettings()
 {
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "LibreCAD", "asciifile");
    settings.setValue("pos", pos());
    settings.setValue("size", size());
    settings.setValue("lastfile", fileedit->text());
    settings.setValue("format", formatedit->currentIndex());
    settings.setValue("draw2d", pt2d->checkOn());
    settings.setValue("draw3d", pt3d->checkOn());
    settings.setValue("drawelev", ptelev->checkOn());
    settings.setValue("drawnumber", ptnumber->checkOn());
    settings.setValue("drawcode", ptcode->checkOn());
    settings.setValue("connectpoints", connectPoints->isChecked());
    settings.setValue("layer2d", pt2d->getLayer());
    settings.setValue("layer3d", pt3d->getLayer());
    settings.setValue("layerelev", ptelev->getLayer());
    settings.setValue("layernumber", ptnumber->getLayer());
    settings.setValue("layercode", ptcode->getLayer());
    settings.setValue("styleelev", ptelev->getStyleIdx());
    settings.setValue("stylenumber", ptnumber->getStyleIdx());
    settings.setValue("stylecode", ptcode->getStyleIdx());
    settings.setValue("heightelev", ptelev->getHeightStr());
    settings.setValue("heightnumber", ptnumber->getHeightStr());
    settings.setValue("heightcode", ptcode->getHeightStr());
    settings.setValue("separationelev", ptelev->getSeparationStr());
    settings.setValue("separationnumber", ptnumber->getSeparationStr());
    settings.setValue("separationcode", ptcode->getSeparationStr());
    settings.setValue("positionelev", ptelev->getPosition());
    settings.setValue("positionnumber", ptnumber->getPosition());
    settings.setValue("positioncode", ptcode->getPosition());
 }
