/*****************************************************************************/
/*  PicFile.cpp - ascii file importer                                        */
/*                                                                           */
/*  Copyright (C) 2011 Rallaz, rallazz@gmail.com                             */
/*  Copyright (C) 2014 cgrzemba, cgrzemba@opencsw.org                        */
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
#include <math.h>

#include <QMessageBox>

#include "document_interface.h"
#include "picfile.h"

PluginCapabilities PicFile::getCapabilities() const
{
    PluginCapabilities pluginCapabilities;
    pluginCapabilities.menuEntryPoints
            << PluginMenuLocation("plugins_menu", tr("Read PIC file"));
    return pluginCapabilities;
}

QString PicFile::name() const
 {
     return (tr("import PIC file"));
 }

void PicFile::execComm(Document_Interface *doc,
                             QWidget *parent, QString cmd)
{
    Q_UNUSED(cmd);
    picPunto pdt(parent);
    int result = pdt.exec();
    if (result == QDialog::Accepted)
        pdt.processFile(doc);
}

/*****************************/

picPunto::picPunto(QWidget *parent) :  QDialog(parent)
{
    cnt = 0;
    QStringList txtformats;

    QGridLayout *mainLayout = new QGridLayout;
//readSettings();

    QPushButton *filebut = new QPushButton(tr("File..."));
    fileedit = new QLineEdit();

    QDoubleValidator *val = new QDoubleValidator(0);
    val->setBottom ( 0.0 );
    scaleedit = new QLineEdit();
    scaleedit->setValidator(val);

    QFormLayout *flo = new QFormLayout;
    flo->addRow( filebut, fileedit);
    flo->addRow( tr("Scale:"), scaleedit);
    mainLayout->addLayout(flo, 0, 0);

    QHBoxLayout *loacceptcancel = new QHBoxLayout;
    QPushButton *acceptbut = new QPushButton(tr("Accept"));
    loacceptcancel->addStretch();
    loacceptcancel->addWidget(acceptbut);

    QPushButton *cancelbut = new QPushButton(tr("Cancel"));
    loacceptcancel->addWidget(cancelbut);
    mainLayout->addLayout(loacceptcancel, 1, 0);

    setLayout(mainLayout);
    readSettings();

    connect(cancelbut, SIGNAL(clicked()), this, SLOT(reject()));
    connect(acceptbut, SIGNAL(clicked()), this, SLOT(checkAccept()));

    connect(filebut, SIGNAL(clicked()), this, SLOT(dptFile()));
}

void picPunto::checkAccept()
{

    errmsg.clear();
    if (failGUI(&errmsg)) {
        QMessageBox::critical ( this, "Pic file read plugin", errmsg );
        errmsg.clear();
        return;
    }
    writeSettings();
    accept();
}

void picPunto::dptFile()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Select file"));
    fileedit->setText(fileName);
}

bool picPunto::failGUI(QString *msg)
{
    double val = scaleedit->text().toDouble();
    if ( val == 0 ) {
        msg->insert(0, tr("Scale Factor is empty or invalid"));
        return true;
    }
    return false;
}

void picPunto::processFile(Document_Interface *doc)
{
    QString sep = " ";
    currDoc = doc;
    scale = (scaleedit->text()).toDouble();

    if (!QFile::exists(fileedit->text()) ) {
        QMessageBox::critical ( this, "picPunto", QString(tr("The file %1 not exist")).arg(fileedit->text()) );
        return;
    }
    QFile infile(fileedit->text());
    if (!infile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::critical ( this, "picPunto", QString(tr("Can't open the file %1")).arg(fileedit->text()) );
         return;
    }
    QString currlay = currDoc->getCurrentLayer();
    processFilePic(&infile);
    infile.close ();

    QMessageBox::information(this, "Info", QString(tr("%1 objects imported")).arg(cnt) );
    currDoc = NULL;
}

double picPunto::getPValue(QString p)
{
    return (p.toDouble() * scale);
}

void picPunto::drawLine()
{
    QPointF prevP, nextP;
    int i;

    for (i = 0; i < dataList.size(); ++i) {
        pointData *pd = dataList.at(i);
        if (!pd->x.isEmpty() && !pd->y.isEmpty()){
            prevP.setX(getPValue(pd->x));
            prevP.setY(getPValue(pd->y));
            i++;
            break;
        } else {
            QMessageBox::information(this, "Info", QString(tr("picPunto drawLine: first point is empty %1")).arg(i));
        }
    }
    for (; i < dataList.size(); ++i) {
        pointData *pd = dataList.at(i);
        if (!pd->x.isEmpty() && !pd->y.isEmpty()){
            nextP.setX(getPValue(pd->x));
            nextP.setY(getPValue(pd->y));
            currDoc->addLine(&prevP, &nextP);
    	    // QMessageBox::information(this, "Info", QString(tr("picPunto drawLine: addLine %1 %2,%3 %4,%5")).arg(i).arg(prevP.x()).arg(prevP.y()).arg(nextP.x()).arg(nextP.y()));
            prevP = nextP;
            cnt++;
        } else {
            QMessageBox::information(this, "Info", QString(tr("picPunto drawLine: next point is empty %1")).arg(i));
        }
    }
    while (!dataList.isEmpty())
         delete dataList.takeFirst();
}

void picPunto::drawCircle(QString x, QString y, QString radius)
{
    QPointF center;
    qreal rad;

    center.setX(getPValue(x));
    center.setY(getPValue(y));
    rad = getPValue(radius);
    currDoc->addCircle(&center, rad);
    cnt++;
}

void picPunto::drawText(QString x, QString y, QString txt, QString align)
{
    DPI::VAlign va = DPI::VAlignBottom;
    DPI::HAlign ha;
    QString sty = "txt";
    double height = 0.05 * scale;

    ha = (align == "ljust") ? DPI::HAlignLeft : (align == "rjust") ? DPI::HAlignRight : DPI::HAlignCenter;
    QPointF pt(getPValue(x), getPValue(y));
    currDoc->addText(txt, sty, &pt, height, 0.0, ha, va);
    cnt++;
}

void picPunto::drawBox(QString posx, QString posy, QString width, QString height)
{
    QPointF prevP, nextP;

    prevP.setX(getPValue(posx));
    prevP.setY(getPValue(posy));
    nextP.setX(getPValue(posx)+getPValue(width));
    nextP.setY(prevP.y());
    currDoc->addLine(&prevP, &nextP);
    prevP=nextP;
    nextP.setY(prevP.y()+getPValue(height));
    currDoc->addLine(&prevP, &nextP);
    prevP=nextP;
    nextP.setX(getPValue(posx));
    currDoc->addLine(&prevP, &nextP);
    prevP=nextP;
    nextP.setY(getPValue(posy));
    currDoc->addLine(&prevP, &nextP);
    cnt++;
}

void picPunto::processFilePic(QFile* file)
{
    //    QString outname, sep;
    QString sep = " ";
    QString::SplitBehavior skip = QString::KeepEmptyParts;
    QStringList data;
    QString cmd;
    pointData *pd;
    while (!file->atEnd()) {
        int i = 2;
        QString line = file->readLine();
        line.remove ( line.size()-1, 1);
        // printf("process line: %s\n",line.toStdString().c_str() );
        data = line.split(sep, skip);
        if (data.size() < 4 ) continue;
        cmd = data.at(0);
        if (cmd == "line" ) {
                if (data.at(2) == "from"){
                    i++; // dashed line
                }
                for (;i <  data.size(); i += 2) {
                    pd = new pointData;
                    pd->x = data.at(i).split(',').at(0);
                    pd->y = data.at(i).split(',').at(1);
                    dataList.append(pd);
                    if ( i < data.size()-1 and data.at(i+1) != "to") {
                        QMessageBox::critical ( this, "picPunto", QString(tr("format error in %1")).arg(line) );
                        return;
                    }
                }
                if (dataList.size() > 0 )
                    drawLine();
        } else {
            if (cmd == "circle") { // circle at 7.935,3.643 rad 0.035
                if ( data.size() != 5 ) {
                    QMessageBox::critical ( this, "picPunto", QString(tr("format error in %1")).arg(line) );
                    return;
                }
                drawCircle(data.at(2).split(',').at(0), data.at(2).split(',').at(1), data.at(4));
            } else {
                if (cmd == "box") { // box invis fill 0.000 with .sw at (7.480,6.917) width 0.079 height 0.157
                    if ( data.size() < 11 ) {
                        // QMessageBox::critical ( this, "picPunto", QString(tr("format error in %1")).arg(line) );
                        continue;
                    }
                    QString posx = data.at(7).split(',').at(0);
                    QString posy = data.at(7).split(',').at(1);
                    drawBox(posx.remove(0,1), posy.remove(posy.size()-1,1), data.at(9), data.at(11));
                } else {
                    if ( cmd.startsWith("\"\\s") and data.size() > 3 ) { // "\s5\fRAbstell fl?che\fP" at 8.132,7.456 ljust
                        QString txt = line.split("\"", skip).at(1);
                        QStringList rline = line.split("\"", skip).at(2).split(" ",skip);
                        txt.remove ( txt.size()-3, 3);
                        txt.remove (QRegExp("^.*fR"));
                        // printf("process line: %s: %s\n",rline.at(2).toStdString().c_str(),txt.toStdString().c_str() );
                        drawText(rline.at(2).split(',').at(0), rline.at(2).split(',').at(1), txt, rline.at(3));
                    }
                }
            }
        }
    }
}

picPunto::~picPunto()
{
    while (!dataList.isEmpty())
         delete dataList.takeFirst();
}

void picPunto::readSettings()
 {
    QString str;
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "LibreCAD", "picfile");
    QPoint pos = settings.value("pos", QPoint(200, 200)).toPoint();
    QSize size = settings.value("size", QSize(400,50)).toSize();
    str = settings.value("lastfile").toString();
    fileedit->setText(str);
    str = settings.value("lastscale","1.0").toString();
    scaleedit->setText(str);

    resize(size);
    move(pos);
 }

void picPunto::writeSettings()
 {
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "LibreCAD", "picfile");
    settings.setValue("pos", pos());
    settings.setValue("size", size());
    settings.setValue("lastfile", fileedit->text());
    settings.setValue("lastscale", scaleedit->text());
 }
